/* server.c (A) — เว็บเซิร์ฟเวอร์เล็กๆ: อ่าน event จาก FIFO แล้วส่งให้ browser ผ่าน SSE */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define FIFO_PATH   "/tmp/seesh.fifo"
#define PORT        8080
#define WEB_DIR     "viewer/web"
#define MAX_CLIENTS 16
#define MAX_EVENTS  5000

static char *events[MAX_EVENTS];   /* event ทั้งหมดที่ได้รับ (ส่งให้ browser ที่เปิดทีหลัง) */
static int   nevents = 0;
static int   sse[MAX_CLIENTS];     /* fd ของ browser ที่เปิด /events ค้างไว้ (-1 = ว่าง) */

/* เขียนให้ครบทุก byte */
static int send_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t w = write(fd, buf, len);
        if (w < 0) { if (errno == EINTR) continue; return -1; }
        buf += w;
        len -= w;
    }
    return 0;
}

/* รูปแบบ SSE: "data: <json>\n\n" */
static int send_event(int fd, const char *json) {
    char buf[2048];
    int n = snprintf(buf, sizeof(buf), "data: %s\n\n", json);
    if (n <= 0 || n >= (int)sizeof(buf)) return 0;
    return send_all(fd, buf, n);
}

static void broadcast(const char *json) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (sse[i] >= 0 && send_event(sse[i], json) < 0) {   /* browser ปิดไปแล้ว */
            close(sse[i]);
            sse[i] = -1;
        }
}

static void on_event_line(const char *line) {
    /* คำสั่งที่ 1 ของ shell = shell เพิ่งเริ่มใหม่: ล้างของเก่า */
    if (strncmp(line, "{\"cmd\":1,\"seq\":1,", 17) == 0 && strstr(line, "\"cmd_start\"")) {
        for (int i = 0; i < nevents; i++) free(events[i]);
        nevents = 0;
        broadcast("{\"type\":\"reset\"}");
    }
    if (nevents < MAX_EVENTS) events[nevents++] = strdup(line);
    broadcast(line);
}

/* อ่าน FIFO แล้วตัดเป็นบรรทัด (บรรทัดอาจมาไม่ครบในการอ่านครั้งเดียว) */
static void read_fifo(int fd) {
    static char carry[4096];
    static size_t carry_len = 0;
    char buf[4096];
    ssize_t n = read(fd, buf, sizeof(buf));
    for (ssize_t i = 0; i < n; i++) {
        if (buf[i] == '\n') {
            carry[carry_len] = '\0';
            if (carry_len > 0) on_event_line(carry);
            carry_len = 0;
        } else if (carry_len < sizeof(carry) - 1) {
            carry[carry_len++] = buf[i];
        }
    }
}

static const char *content_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (ext && strcmp(ext, ".html") == 0) return "text/html; charset=utf-8";
    if (ext && strcmp(ext, ".css") == 0)  return "text/css; charset=utf-8";
    if (ext && strcmp(ext, ".js") == 0)   return "application/javascript; charset=utf-8";
    return "text/plain; charset=utf-8";
}

static void send_status(int fd, const char *status) {
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "HTTP/1.1 %s\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n"
        "Connection: close\r\n\r\n%s", status, strlen(status), status);
    send_all(fd, buf, n);
}

static void serve_file(int fd, const char *path) {
    if (strcmp(path, "/") == 0) path = "/index.html";
    if (strstr(path, "..")) { send_status(fd, "403 Forbidden"); return; }  /* กันอ่านไฟล์นอกโฟลเดอร์ */

    char full[512];
    snprintf(full, sizeof(full), "%s%s", WEB_DIR, path);
    int f = open(full, O_RDONLY);
    if (f < 0) { send_status(fd, "404 Not Found"); return; }

    struct stat st;
    fstat(f, &st);
    char hdr[256];
    int n = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\n\r\n",
        content_type(full), (long long)st.st_size);
    send_all(fd, hdr, n);

    char buf[8192];
    ssize_t r;
    while ((r = read(f, buf, sizeof(buf))) > 0)
        if (send_all(fd, buf, r) < 0) break;
    close(f);
}

static void handle_client(int fd) {
    char req[4096], method[8], path[256];
    ssize_t n = read(fd, req, sizeof(req) - 1);
    if (n <= 0) { close(fd); return; }
    req[n] = '\0';

    if (sscanf(req, "%7s %255s", method, path) != 2 || strcmp(method, "GET") != 0) {
        send_status(fd, "400 Bad Request");
        close(fd);
        return;
    }
    char *q = strchr(path, '?');
    if (q) *q = '\0';

    if (strcmp(path, "/events") == 0) {          /* browser ขอรับ event แบบต่อเนื่อง */
        const char *hdr = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\n"
                          "Cache-Control: no-cache\r\nConnection: keep-alive\r\n\r\n";
        if (send_all(fd, hdr, strlen(hdr)) < 0) { close(fd); return; }
        for (int i = 0; i < nevents; i++)        /* ส่ง event เก่าทั้งหมดก่อน */
            if (send_event(fd, events[i]) < 0) { close(fd); return; }
        for (int i = 0; i < MAX_CLIENTS; i++)    /* เก็บไว้ส่ง event ใหม่ทีหลัง */
            if (sse[i] < 0) { sse[i] = fd; return; }
        close(fd);                               /* เต็ม */
        return;
    }

    serve_file(fd, path);
    close(fd);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);                    /* browser ปิดกลางคัน: ได้ error แทนการตาย */
    for (int i = 0; i < MAX_CLIENTS; i++) sse[i] = -1;

    if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) { perror("mkfifo"); return 1; }
    int fifo = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fifo < 0) { perror("open fifo"); return 1; }
    int keep = open(FIFO_PATH, O_WRONLY);        /* เปิดฝั่งเขียนค้างไว้: กัน EOF ตอน shell ปิด */
    if (keep < 0) { perror("open fifo (keep)"); return 1; }

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));   /* รันซ้ำได้ทันทีหลังปิด */

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);                   /* รับเฉพาะเครื่องตัวเอง */
    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(srv, 16) < 0) { perror("listen"); return 1; }

    printf("SeeSh Viewer: http://localhost:%d  (กด Ctrl+C เพื่อปิด)\n", PORT);

    while (1) {
        struct pollfd pfd[2 + MAX_CLIENTS];
        int who[2 + MAX_CLIENTS];                /* pfd ช่องนี้เป็นของ sse[ช่องไหน] */
        int np = 0;
        pfd[np++] = (struct pollfd){ .fd = srv,  .events = POLLIN };
        pfd[np++] = (struct pollfd){ .fd = fifo, .events = POLLIN };
        for (int i = 0; i < MAX_CLIENTS; i++)
            if (sse[i] >= 0) { who[np] = i; pfd[np++] = (struct pollfd){ .fd = sse[i], .events = POLLIN }; }

        if (poll(pfd, np, -1) < 0) {             /* รอจนกว่าช่องไหนมีอะไรเกิดขึ้น */
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        if (pfd[1].revents & POLLIN) read_fifo(fifo);

        for (int k = 2; k < np; k++) {           /* browser ปิดหน้าเว็บ */
            if (pfd[k].revents & (POLLIN | POLLHUP | POLLERR)) {
                char tmp[256];
                if (read(pfd[k].fd, tmp, sizeof(tmp)) <= 0) {
                    close(pfd[k].fd);
                    sse[who[k]] = -1;
                }
            }
        }

        if (pfd[0].revents & POLLIN) {           /* มีคนเชื่อมต่อเข้ามาใหม่ */
            int c = accept(srv, NULL, NULL);
            if (c >= 0) handle_client(c);
        }
    }
    return 0;
}
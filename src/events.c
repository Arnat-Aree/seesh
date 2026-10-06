/* events.c (A) — เขียน event เป็น JSON Lines ลง FIFO แบบไม่บล็อก */
#include "events.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

static int ev_fd = -1;    /* fd ของ FIFO (-1 = ยังไม่มี viewer) */
static int cur_cmd = 0;
static int seq = 0;

/* ใส่ \ หน้า " และ \ เพื่อไม่ให้ JSON พัง */
static void escape(const char *in, char *out, size_t n) {
    size_t j = 0;
    for (; *in && j + 3 < n; in++) {
        if (*in == '"' || *in == '\\') out[j++] = '\\';
        out[j++] = *in;
    }
    out[j] = '\0';
}

static void try_open(void) {
    if (ev_fd >= 0) return;
    ev_fd = open(SEESH_FIFO, O_WRONLY | O_NONBLOCK);    /* ยังไม่มีผู้อ่าน = ได้ -1 (ENXIO) */
    if (ev_fd >= 0) fcntl(ev_fd, F_SETFD, FD_CLOEXEC);  /* ปิดเองตอน exec: โปรแกรมอื่นไม่ได้ fd นี้ */
}

void ev_init(void) {
    signal(SIGPIPE, SIG_IGN);                   /* viewer ปิดไป = เขียนได้ error แทนการฆ่า shell */
    if (mkfifo(SEESH_FIFO, 0666) < 0 && errno != EEXIST)
        perror("mkfifo");
}

int ev_begin(const char *raw) {
    char esc[1024];
    try_open();                                 /* ลองเชื่อมต่อใหม่ทุกคำสั่ง */
    cur_cmd++;
    seq = 0;
    escape(raw, esc, sizeof(esc));
    ev_emit("cmd_start", "A", "", "\"raw\":\"%s\"", esc);
    return cur_cmd;
}

int ev_cmd(void) { return cur_cmd; }

void ev_parse(const command_t *cmd) {
    char names[512] = "";
    size_t len = 0;
    for (int k = 0; k < cmd->ncmds && len < sizeof(names); k++) {
        int w = snprintf(names + len, sizeof(names) - len, "%s\"%s\"",
                         k ? "," : "", cmd->cmds[k].argv[0]);
        if (w < 0) break;
        len += w;
    }
    ev_emit("parse", "A", "parse", "\"ncmds\":%d,\"bg\":%s,\"names\":[%s]",
            cmd->ncmds, cmd->background ? "true" : "false", names);
}

void ev_end(int status) {
    ev_emit("cmd_end", "A", "", "\"status\":%d", status);
}

void ev_emit_cmd(int cmd, const char *type, const char *owner,
                 const char *syscall, const char *fmt, ...) {
    if (ev_fd < 0) return;                      /* ไม่มี viewer: ข้าม */

    char data[1024], line[1536];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(data, sizeof(data), fmt, ap);
    va_end(ap);

    int n = snprintf(line, sizeof(line),
        "{\"cmd\":%d,\"seq\":%d,\"type\":\"%s\",\"owner\":\"%s\",\"pid\":%d,"
        "\"syscall\":\"%s\",\"data\":{%s}}\n",
        cmd, ++seq, type, owner, (int)getpid(), syscall, data);
    if (n <= 0 || n >= (int)sizeof(line)) return;

    if (write(ev_fd, line, n) < 0 && errno == EPIPE) {  /* viewer ปิดไปแล้ว */
        close(ev_fd);
        ev_fd = -1;
    }
}
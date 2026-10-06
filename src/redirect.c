/* redirect.c (B) — <, >, >> ด้วย open + dup2 */
#include "redirect.h"
#include "events.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* เปิดไฟล์ไม่สำเร็จ: แจ้งผู้ใช้ และส่ง event พร้อมเหตุผล (errno) */
static void open_failed(const char *file, const char *mode) {
    int e = errno;                                       /* เก็บไว้ก่อน เพราะ perror อาจเปลี่ยนค่า */
    fprintf(stderr, "seesh: %s: %s\n", file, strerror(e));
    ev_emit("open_fail", "B", "open()", "\"file\":\"%s\",\"mode\":\"%s\",\"errno\":%d,\"error\":\"%s\"",
            file, mode, e, strerror(e));
}

int apply_redirect(simple_cmd_t *sc) {
    if (sc->in_file) {                                   /* < file */
        int fd = open(sc->in_file, O_RDONLY);
        if (fd < 0) { open_failed(sc->in_file, "read"); return -1; }
        ev_emit("open", "B", "open()", "\"file\":\"%s\",\"mode\":\"read\"", sc->in_file);
        dup2(fd, STDIN_FILENO);                          /* stdin (fd 0) → ไฟล์ */
        ev_emit("dup2", "B", "dup2()", "\"fd\":\"stdin\",\"target\":\"file:%s\"", sc->in_file);
        close(fd);
    }
    if (sc->out_file) {                                  /* > file หรือ >> file */
        const char *mode = sc->append ? "append" : "write";
        int flags = O_WRONLY | O_CREAT | (sc->append ? O_APPEND : O_TRUNC);
        int fd = open(sc->out_file, flags, 0644);        /* 0644 = rw-r--r-- */
        if (fd < 0) { open_failed(sc->out_file, mode); return -1; }
        ev_emit("open", "B", "open()", "\"file\":\"%s\",\"mode\":\"%s\"", sc->out_file, mode);
        dup2(fd, STDOUT_FILENO);                         /* stdout (fd 1) → ไฟล์ */
        ev_emit("dup2", "B", "dup2()", "\"fd\":\"stdout\",\"target\":\"file:%s\"", sc->out_file);
        close(fd);
    }
    return 0;
}
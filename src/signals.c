/* signals.c (C) — ติดตั้ง handler ด้วย sigaction */
#include "signals.h"
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t g_sigchld = 0;

/* Ctrl+C: shell แค่ขึ้นบรรทัดใหม่ ไม่ปิดตัว */
static void on_sigint(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);   /* ใน handler ใช้ได้เฉพาะ write ไม่ใช่ printf */
}

/* ลูกจบ: แค่ตั้ง flag แล้วไปเก็บกวาดใน main loop */
static void on_sigchld(int sig) {
    (void)sig;
    g_sigchld = 1;
}

void signals_init(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = on_sigint;
    sa.sa_flags = SA_RESTART;                  /* ให้ fgets/waitpid ทำงานต่อหลังโดนขัด */
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = on_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;   /* สนใจเฉพาะลูกที่จบ ไม่สนลูกที่ถูกหยุดชั่วคราว */
    sigaction(SIGCHLD, &sa, NULL);
}
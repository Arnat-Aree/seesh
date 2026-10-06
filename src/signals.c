/* signals.c (C) — ติดตั้ง handler ด้วย sigaction */
#include "signals.h"
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t g_sigchld = 0;
volatile sig_atomic_t g_at_prompt = 0;

static char   prompt_buf[4200];   /* prompt ล่าสุด เตรียมไว้ให้ handler ใช้ */
static size_t prompt_len = 0;

void signals_set_prompt(const char *p) {
    size_t n = strlen(p);
    if (n >= sizeof(prompt_buf)) n = sizeof(prompt_buf) - 1;
    memcpy(prompt_buf, p, n);
    prompt_len = n;
}

/* Ctrl+C: shell ไม่ปิด ถ้ากำลังรอพิมพ์คำสั่ง ให้พิมพ์ prompt ใหม่ */
static void on_sigint(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);          /* ใน handler ใช้ได้เฉพาะ write ไม่ใช่ printf */
    if (g_at_prompt)
        write(STDOUT_FILENO, prompt_buf, prompt_len);
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
/* signals.h (C) — จัดการ Ctrl+C และ SIGCHLD */
#ifndef SIGNALS_H
#define SIGNALS_H
#include <signal.h>

extern volatile sig_atomic_t g_sigchld;     /* 1 = มีลูกจบแล้วตั้งแต่ครั้งก่อน */
extern volatile sig_atomic_t g_at_prompt;   /* 1 = กำลังรอผู้ใช้พิมพ์คำสั่ง */

void signals_init(void);                    /* เรียกครั้งเดียวตอนเริ่ม shell */
void signals_set_prompt(const char *p);     /* จำ prompt ไว้ให้ handler พิมพ์ซ้ำ */

#endif
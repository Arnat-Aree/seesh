/* signals.h (C) — จัดการ Ctrl+C และ SIGCHLD */
#ifndef SIGNALS_H
#define SIGNALS_H
#include <signal.h>

extern volatile sig_atomic_t g_sigchld;   /* 1 = มีลูกจบแล้วตั้งแต่ครั้งก่อน */

void signals_init(void);                  /* เรียกครั้งเดียวตอนเริ่ม shell */

#endif
/* events.h (A) — ส่ง event การทำงานของ shell ไปให้ viewer ผ่าน FIFO */
#ifndef EVENTS_H
#define EVENTS_H
#include "parser.h"

#define SEESH_FIFO "/tmp/seesh.fifo"

void ev_init(void);                    /* เรียกครั้งเดียวตอนเริ่ม shell */
int  ev_begin(const char *raw);        /* เริ่มคำสั่งใหม่ คืนเลขคำสั่ง */
int  ev_cmd(void);                     /* เลขคำสั่งปัจจุบัน */
void ev_parse(const command_t *cmd);   /* ส่ง event parse */
void ev_end(int status);               /* ส่ง event cmd_end */

/* ส่ง event 1 บรรทัด: fmt คือเนื้อหาข้างใน {...} ของ data แบบ printf */
void ev_emit_cmd(int cmd, const char *type, const char *owner,
                 const char *syscall, const char *fmt, ...)
                 __attribute__((format(printf, 5, 6)));

#define ev_emit(type, owner, syscall, ...) \
        ev_emit_cmd(ev_cmd(), type, owner, syscall, __VA_ARGS__)

#endif
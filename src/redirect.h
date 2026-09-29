/* redirect.h (B) — ส่งต่อ input/output ไปยังไฟล์ */
#ifndef REDIRECT_H
#define REDIRECT_H
#include "parser.h"

/* เรียกในลูกหลัง fork ก่อน exec — คืน 0 = สำเร็จ, -1 = เปิดไฟล์ไม่ได้ */
int apply_redirect(simple_cmd_t *sc);

#endif
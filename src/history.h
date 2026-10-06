/* history.h (C) — เก็บคำสั่งที่เคยพิมพ์ */
#ifndef HISTORY_H
#define HISTORY_H

void history_add(const char *line);   /* เพิ่มคำสั่ง 1 บรรทัด */
void history_print(void);             /* คำสั่ง history */

#endif
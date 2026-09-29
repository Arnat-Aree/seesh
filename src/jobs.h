/* jobs.h (C) — รายการ background job */
#ifndef JOBS_H
#define JOBS_H
#include <sys/types.h>

int  jobs_add(pid_t pid, const char *raw);   /* คืนหมายเลข job */
void jobs_reap(void);                        /* เก็บกวาด job ที่จบแล้ว (ป้องกัน zombie) */
void jobs_print(void);                       /* คำสั่ง jobs */

#endif
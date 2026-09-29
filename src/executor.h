/* executor.h (A) — จุดเริ่มรันคำสั่งทั้งหมด */
#ifndef EXECUTOR_H
#define EXECUTOR_H
#include "parser.h"

int  execute(command_t *cmd);      /* คืน exit status */
void exec_simple(simple_cmd_t *sc); /* ใช้ในลูกเท่านั้น: redirect แล้ว exec (ไม่คืนค่า) */

#endif
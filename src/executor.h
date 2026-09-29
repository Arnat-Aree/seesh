/* executor.h (A) — จุดเริ่มรันคำสั่งทั้งหมด */
#ifndef EXECUTOR_H
#define EXECUTOR_H
#include "parser.h"

int execute(command_t *cmd);   /* คืน exit status */

#endif
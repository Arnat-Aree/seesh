/* pipeline.h (B) — รันคำสั่งที่ต่อกันด้วย | */
#ifndef PIPELINE_H
#define PIPELINE_H
#include "parser.h"

int run_pipeline(command_t *cmd);   /* ใช้เมื่อ ncmds >= 2 */

#endif
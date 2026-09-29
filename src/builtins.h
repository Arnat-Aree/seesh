/* builtins.h (A) — คำสั่งที่ shell ต้องทำเอง */
#ifndef BUILTINS_H
#define BUILTINS_H
#include "parser.h"

int is_builtin(const char *name);
int run_builtin(simple_cmd_t *sc);

#endif
/* main.c (A) — วนรับคำสั่งจากผู้ใช้ (REPL) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "parser.h"
#include "executor.h"
#include "signals.h"
#include "jobs.h"
#include "events.h"
#include "history.h"

#define LINE_MAX_LEN 1024

/* แสดง prompt พร้อมโฟลเดอร์ปัจจุบัน (ย่อ home เป็น ~) */
static void print_prompt(void) {
    char cwd[PATH_MAX], prompt[PATH_MAX + 32];
    const char *home = getenv("HOME");

    if (getcwd(cwd, sizeof(cwd)) == NULL) {             /* system call: ถามโฟลเดอร์ปัจจุบัน */
        snprintf(prompt, sizeof(prompt), "seesh$ ");
    } else if (home && *home && strncmp(cwd, home, strlen(home)) == 0 &&
               (cwd[strlen(home)] == '/' || cwd[strlen(home)] == '\0')) {
        snprintf(prompt, sizeof(prompt), "seesh:~%s$ ", cwd + strlen(home));
    } else {
        snprintf(prompt, sizeof(prompt), "seesh:%s$ ", cwd);
    }

    signals_set_prompt(prompt);   /* ให้ handler ของ Ctrl+C พิมพ์ซ้ำได้ */
    fputs(prompt, stdout);
    fflush(stdout);               /* บังคับให้ prompt ขึ้นจอทันที */
}

int main(void) {
    char line[LINE_MAX_LEN];

    signals_init();
    ev_init();
    printf("Welcome to SeeSh v0.1 — type 'exit' to quit\n");

    while (1) {
        jobs_reap();                           /* เก็บกวาด background job ที่จบแล้ว */
        print_prompt();

        g_at_prompt = 1;                       /* กำลังรอพิมพ์: Ctrl+C ให้พิมพ์ prompt ใหม่ */
        char *got = fgets(line, sizeof(line), stdin);
        g_at_prompt = 0;

        if (got == NULL) {                     /* Ctrl+D = จบ */
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';      /* ตัด Enter ท้ายบรรทัดออก */

        if (line[0] == '\0') continue;         /* บรรทัดว่าง: ขึ้น prompt ใหม่ */
        history_add(line);                     /* บันทึกทุกคำสั่งที่ไม่ว่าง */

        command_t cmd;
        if (parse_line(line, &cmd) < 0) {
            fprintf(stderr, "seesh: syntax error\n");
            continue;
        }

        if (strcmp(cmd.cmds[0].argv[0], "exit") == 0) {
            free_command(&cmd);
            break;
        }

        ev_begin(cmd.raw);
        ev_parse(&cmd);
        int status = execute(&cmd);
        ev_end(status);
        free_command(&cmd);
    }

    printf("Bye!\n");
    return 0;
}
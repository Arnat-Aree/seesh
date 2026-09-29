/* main.c (A) — วนรับคำสั่งจากผู้ใช้ (REPL) */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "parser.h"
#include "executor.h"

#define LINE_MAX_LEN 1024

/* แสดง prompt พร้อมโฟลเดอร์ปัจจุบัน */
static void print_prompt(void)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) /* system call: ถามโฟลเดอร์ปัจจุบัน */
        printf("seesh:%s$ ", cwd);
    else
        printf("seesh$ ");
    fflush(stdout); /* บังคับให้ prompt ขึ้นจอทันที */
}

int main(void)
{
    char line[LINE_MAX_LEN];

    printf("Welcome to SeeSh v0.1 — type 'exit' to quit\n");

    while (1)
    {
        print_prompt();

        if (fgets(line, sizeof(line), stdin) == NULL)
        { /* Ctrl+D = จบ */
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0'; /* ตัด Enter ท้ายบรรทัดออก */

        if (line[0] == '\0')
            continue; /* บรรทัดว่าง: ขึ้น prompt ใหม่ */
        command_t cmd;
        if (parse_line(line, &cmd) < 0)
        {
            fprintf(stderr, "seesh: syntax error\n");
            continue;
        }

        if (strcmp(cmd.cmds[0].argv[0], "exit") == 0)
        {
            free_command(&cmd);
            break;
        }

        execute(&cmd);
        free_command(&cmd);
    }

    printf("Bye!\n");
    return 0;
}
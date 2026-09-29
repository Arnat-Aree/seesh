/* main.c (A) — วนรับคำสั่งจากผู้ใช้ (REPL) */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define LINE_MAX_LEN 1024

/* แสดง prompt พร้อมโฟลเดอร์ปัจจุบัน */
static void print_prompt(void) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)   /* system call: ถามโฟลเดอร์ปัจจุบัน */
        printf("seesh:%s$ ", cwd);
    else
        printf("seesh$ ");
    fflush(stdout);                          /* บังคับให้ prompt ขึ้นจอทันที */
}

int main(void) {
    char line[LINE_MAX_LEN];

    printf("Welcome to SeeSh v0.1 — type 'exit' to quit\n");

    while (1) {
        print_prompt();

        if (fgets(line, sizeof(line), stdin) == NULL) {  /* Ctrl+D = จบ */
            printf("\n");
            break;
        }
        line[strcspn(line, "\n")] = '\0';   /* ตัด Enter ท้ายบรรทัดออก */

        if (line[0] == '\0') continue;       /* บรรทัดว่าง: ขึ้น prompt ใหม่ */
        if (strcmp(line, "exit") == 0) break;

        printf("(ยังรันไม่ได้) คุณพิมพ์: %s\n", line);
    }

    printf("Bye!\n");
    return 0;
}
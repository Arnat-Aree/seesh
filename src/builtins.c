/* builtins.c (A) — cd, pwd, help, jobs, history */
#include "builtins.h"
#include "jobs.h"
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

int is_builtin(const char *name) {
    return strcmp(name, "cd") == 0 || strcmp(name, "pwd") == 0 ||
           strcmp(name, "help") == 0 || strcmp(name, "jobs") == 0 ||
           strcmp(name, "history") == 0;
}

int run_builtin(simple_cmd_t *sc) {
    const char *name = sc->argv[0];

    if (strcmp(name, "cd") == 0) {
        const char *dir = sc->argc > 1 ? sc->argv[1] : getenv("HOME");
        if (dir == NULL) dir = "/";
        if (chdir(dir) < 0) {                 /* system call: เปลี่ยนโฟลเดอร์ของ shell เอง */
            perror("cd");
            return 1;
        }
        return 0;
    }

    if (strcmp(name, "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) { perror("pwd"); return 1; }
        printf("%s\n", cwd);
        return 0;
    }

    if (strcmp(name, "jobs") == 0) {
        jobs_print();
        return 0;
    }

    if (strcmp(name, "history") == 0) {
        history_print();
        return 0;
    }

    /* help */
    printf("SeeSh built-in commands:\n");
    printf("  cd [dir]   เปลี่ยนโฟลเดอร์\n");
    printf("  pwd        แสดงโฟลเดอร์ปัจจุบัน\n");
    printf("  jobs       แสดง background job\n");
    printf("  history    แสดงคำสั่งที่เคยพิมพ์\n");
    printf("  help       แสดงรายการนี้\n");
    printf("  exit       ออกจาก shell\n");
    return 0;
}
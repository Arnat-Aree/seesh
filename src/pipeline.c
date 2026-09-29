/* pipeline.c (B) — pipe หลายขั้นด้วย pipe + fork + dup2 */
#include "pipeline.h"
#include "executor.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int run_pipeline(command_t *cmd) {
    int n = cmd->ncmds;
    pid_t pids[MAX_CMDS];
    int started = 0;
    int prev_read = -1;                      /* ปลายอ่านของท่อก่อนหน้า */

    for (int i = 0; i < n; i++) {
        int fd[2] = { -1, -1 };
        int last = (i == n - 1);

        if (!last && pipe(fd) < 0) { perror("pipe"); break; }   /* fd[0]=อ่าน, fd[1]=เขียน */

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (!last) { close(fd[0]); close(fd[1]); }
            break;
        }

        if (pid == 0) {                                      /* ---- ลูกตัวที่ i ---- */
            if (prev_read != -1) {                           /* รับข้อมูลจากท่อก่อนหน้า */
                dup2(prev_read, STDIN_FILENO);
                close(prev_read);
            }
            if (!last) {                                     /* ส่งข้อมูลเข้าท่อถัดไป */
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            exec_simple(&cmd->cmds[i]);                      /* ไม่คืนค่า */
        }

        /* ---- แม่: ปิดปลายที่ไม่ใช้ ---- */
        pids[started++] = pid;
        if (prev_read != -1) close(prev_read);
        if (!last) { close(fd[1]); prev_read = fd[0]; }
        else prev_read = -1;
    }
    if (prev_read != -1) close(prev_read);

    int status = 0, result = 1;
    for (int i = 0; i < started; i++) {                      /* รอลูกทุกตัว */
        if (waitpid(pids[i], &status, 0) < 0) perror("waitpid");
        else if (i == n - 1) result = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    }
    return result;
}
/* executor.c (A) — รันคำสั่งด้วย fork / execvp / waitpid */
#include "executor.h"
#include "builtins.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int execute(command_t *cmd) {
    simple_cmd_t *sc = &cmd->cmds[0];

    /* built-in ทำใน shell เอง ไม่ต้อง fork */
    if (cmd->ncmds == 1 && is_builtin(sc->argv[0]))
        return run_builtin(sc);

    if (cmd->ncmds > 1) {                       /* จะทำในขั้นที่ 5 */
        fprintf(stderr, "seesh: pipe ยังไม่รองรับ\n");
        return 1;
    }

    pid_t pid = fork();                         /* สร้าง process ลูก */
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {                             /* ---- ฝั่งลูก ---- */
        execvp(sc->argv[0], sc->argv);          /* เปลี่ยนตัวเองเป็นโปรแกรมจริง */
        fprintf(stderr, "seesh: %s: command not found\n", sc->argv[0]);
        _exit(127);                             /* มาถึงบรรทัดนี้ได้แปลว่า exec ล้มเหลว */
    }

    int status;                                 /* ---- ฝั่งแม่ ---- */
    if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return 1; }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
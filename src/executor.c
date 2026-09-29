/* executor.c (A) — รันคำสั่งด้วย fork / execvp / waitpid */
#include "executor.h"
#include "builtins.h"
#include "redirect.h"
#include "pipeline.h"
#include "jobs.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

/* ใช้ในลูกหลัง fork: ต่อ redirect แล้วเปลี่ยนตัวเองเป็นโปรแกรมจริง */
void exec_simple(simple_cmd_t *sc) {
    if (apply_redirect(sc) < 0) _exit(1);
    execvp(sc->argv[0], sc->argv);
    fprintf(stderr, "seesh: %s: command not found\n", sc->argv[0]);
    _exit(127);                                 /* มาถึงบรรทัดนี้ได้แปลว่า exec ล้มเหลว */
}

int execute(command_t *cmd) {
    simple_cmd_t *sc = &cmd->cmds[0];

    if (cmd->background && cmd->ncmds > 1) {    /* จะทำในเฟส 2 */
        fprintf(stderr, "seesh: background pipeline ยังไม่รองรับ\n");
        return 1;
    }

    if (cmd->ncmds > 1)                         /* มี | → ให้ pipeline จัดการ */
        return run_pipeline(cmd);

    if (is_builtin(sc->argv[0]))                /* built-in ทำใน shell เอง */
        return run_builtin(sc);

    pid_t pid = fork();                         /* สร้าง process ลูก */
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {                             /* ---- ฝั่งลูก ---- */
        if (cmd->background) setpgid(0, 0);     /* แยกกลุ่ม: Ctrl+C จะไม่โดน job เบื้องหลัง */
        exec_simple(sc);
    }

    if (cmd->background) {                      /* ---- แม่: background ไม่รอ ---- */
        setpgid(pid, pid);
        jobs_add(pid, cmd->raw);
        return 0;
    }

    int status;                                 /* ---- แม่: foreground รอ ---- */
    if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return 1; }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
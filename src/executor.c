/* executor.c (A) — รันคำสั่งด้วย fork / execvp / waitpid */
#include "executor.h"
#include "builtins.h"
#include "redirect.h"
#include "pipeline.h"
#include "jobs.h"
#include "events.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

/* ใช้ในลูกหลัง fork: ต่อ redirect แล้วเปลี่ยนตัวเองเป็นโปรแกรมจริง */
void exec_simple(simple_cmd_t *sc) {
    if (apply_redirect(sc) < 0) _exit(1);
    ev_emit("exec", "A", "execvp()", "\"name\":\"%s\"", sc->argv[0]);
    signal(SIGPIPE, SIG_DFL);                   /* คืนค่าปกติให้โปรแกรมที่จะรัน */
    execvp(sc->argv[0], sc->argv);

    /* มาถึงบรรทัดนี้ได้แปลว่า exec ล้มเหลว */
    int e = errno;
    signal(SIGPIPE, SIG_IGN);                   /* กันลูกตายตอนส่ง event ถ้า viewer ปิดไปแล้ว */
    ev_emit("exec_fail", "A", "execvp()", "\"name\":\"%s\",\"errno\":%d,\"error\":\"%s\"",
            sc->argv[0], e, strerror(e));
    if (e == ENOENT)
        fprintf(stderr, "seesh: %s: command not found\n", sc->argv[0]);
    else
        fprintf(stderr, "seesh: %s: %s\n", sc->argv[0], strerror(e));
    _exit(e == ENOENT ? 127 : 126);             /* 127 = หาไม่เจอ, 126 = เจอแต่รันไม่ได้ (เหมือน bash) */
}

static const char *builtin_syscall(const char *name) {
    if (strcmp(name, "cd") == 0)  return "chdir()";
    if (strcmp(name, "pwd") == 0) return "getcwd()";
    return "";
}

/* built-in ที่มี redirect: เปลี่ยน stdin/stdout ของ shell ชั่วคราว แล้วคืนค่าเดิม */
static int run_builtin_redirected(simple_cmd_t *sc) {
    if (!sc->in_file && !sc->out_file)
        return run_builtin(sc);

    fflush(stdout);                             /* ล้างข้อความค้างก่อนเปลี่ยนทางออก */
    int saved_in  = dup(STDIN_FILENO);          /* จำทางเข้า-ออกเดิมของ shell ไว้ */
    int saved_out = dup(STDOUT_FILENO);

    int status = 1;
    int applied = (apply_redirect(sc) == 0);
    if (applied) {
        status = run_builtin(sc);
        fflush(stdout);                         /* เขียนลงไฟล์ให้หมดก่อนคืนทางออก */
    }

    dup2(saved_in, STDIN_FILENO);               /* คืนทางเข้า-ออกเดิม: ไม่อย่างนั้น prompt จะไปลงไฟล์ด้วย */
    dup2(saved_out, STDOUT_FILENO);
    close(saved_in);
    close(saved_out);
    if (applied && sc->out_file)                /* ส่ง event เฉพาะตอนที่เปลี่ยนทางออกไปจริง */
        ev_emit("dup2", "B", "dup2()", "\"fd\":\"stdout\",\"target\":\"terminal\"");
    return status;
}

int execute(command_t *cmd) {
    simple_cmd_t *sc = &cmd->cmds[0];

    if (cmd->background && cmd->ncmds > 1) {    /* ยังไม่รองรับ */
        fprintf(stderr, "seesh: background pipeline ยังไม่รองรับ\n");
        return 1;
    }

    if (cmd->ncmds > 1)                         /* มี | → ให้ pipeline จัดการ */
        return run_pipeline(cmd);

    if (is_builtin(sc->argv[0])) {              /* built-in ทำใน shell เอง */
        ev_emit("builtin", "A", builtin_syscall(sc->argv[0]),
                "\"name\":\"%s\"", sc->argv[0]);
        return run_builtin_redirected(sc);
    }

    pid_t pid = fork();                         /* สร้าง process ลูก */
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {                             /* ---- ฝั่งลูก ---- */
        if (cmd->background) setpgid(0, 0);     /* แยกกลุ่ม: Ctrl+C จะไม่โดน job เบื้องหลัง */
        exec_simple(sc);
    }

    ev_emit("fork", "A", "fork()", "\"child\":%d,\"name\":\"%s\",\"index\":0",
            (int)pid, sc->argv[0]);

    if (cmd->background) {                      /* ---- แม่: background ไม่รอ ---- */
        setpgid(pid, pid);
        jobs_add(pid, cmd->raw);
        return 0;
    }

    int status;                                 /* ---- แม่: foreground รอ ---- */
    if (waitpid(pid, &status, 0) < 0) { perror("waitpid"); return 1; }
    int code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    ev_emit("wait", "A", "waitpid()", "\"child\":%d,\"status\":%d", (int)pid, code);
    return code;
}
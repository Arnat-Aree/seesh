/* jobs.c (C) — เก็บและเก็บกวาด background job */
#include "jobs.h"
#include "signals.h"
#include "events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_JOBS 32

typedef struct {
    int   used;
    pid_t pid;
    char *cmd;
    int   ev_id;     /* เลขคำสั่งของ event: ใช้ส่ง event ตอน job จบทีหลัง */
} job_t;

static job_t jobs[MAX_JOBS];   /* หมายเลข job = ตำแหน่ง + 1 */

int jobs_add(pid_t pid, const char *raw) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].used) {
            jobs[i].used = 1;
            jobs[i].pid = pid;
            jobs[i].cmd = strdup(raw);
            jobs[i].ev_id = ev_cmd();
            printf("[%d] %d\n", i + 1, (int)pid);
            ev_emit("bg_start", "C", "", "\"job\":%d,\"child\":%d", i + 1, (int)pid);
            return i + 1;
        }
    }
    fprintf(stderr, "seesh: job เต็มแล้ว\n");
    return -1;
}

void jobs_reap(void) {
    if (!g_sigchld) return;          /* ไม่มีลูกจบ ไม่ต้องทำอะไร */
    g_sigchld = 0;

    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].used) continue;
        int status;
        pid_t r = waitpid(jobs[i].pid, &status, WNOHANG);   /* ไม่รอ ถ้ายังไม่จบ */
        if (r == jobs[i].pid) {
            int code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
            ev_emit_cmd(jobs[i].ev_id, "signal", "C", "SIGCHLD",
                        "\"sig\":\"SIGCHLD\",\"child\":%d", (int)jobs[i].pid);
            ev_emit_cmd(jobs[i].ev_id, "bg_done", "C", "waitpid(WNOHANG)",
                        "\"job\":%d,\"status\":%d", i + 1, code);
            printf("[%d]  Done    %s\n", i + 1, jobs[i].cmd);
            free(jobs[i].cmd);
            jobs[i].used = 0;
        }
    }
}

void jobs_print(void) {
    jobs_reap();                     /* เก็บกวาด job ที่จบแล้วก่อน จะได้ไม่แสดงว่ายัง Running */
    for (int i = 0; i < MAX_JOBS; i++)
        if (jobs[i].used)
            printf("[%d]  Running    %s\n", i + 1, jobs[i].cmd);
}
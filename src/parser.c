/* parser.c (A) — แยกข้อความที่ผู้ใช้พิมพ์เป็น command_t */
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKENS 256

static int is_op(const char *t) {
    return strcmp(t, "|") == 0 || strcmp(t, "<") == 0 || strcmp(t, ">") == 0 ||
           strcmp(t, ">>") == 0 || strcmp(t, "&") == 0;
}

/* แยกบรรทัดเป็น token: คำธรรมดา, "ข้อความในเครื่องหมายคำพูด", หรือ | < > >> & */
static int tokenize(const char *line, char *tok[], int max) {
    int n = 0;
    const char *p = line;
    while (*p && n < max) {
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') break;

        if (p[0] == '>' && p[1] == '>') {
            tok[n++] = strdup(">>"); p += 2;
        } else if (strchr("|<>&", *p)) {
            char s[2] = { *p, '\0' };
            tok[n++] = strdup(s); p++;
        } else if (*p == '"' || *p == '\'') {
            char q = *p++;
            const char *start = p;
            while (*p && *p != q) p++;
            tok[n++] = strndup(start, p - start);
            if (*p == q) p++;
        } else {
            const char *start = p;
            while (*p && !isspace((unsigned char)*p) && !strchr("|<>&", *p)) p++;
            tok[n++] = strndup(start, p - start);
        }
    }
    return n;
}

int parse_line(const char *line, command_t *cmd) {
    char *tok[MAX_TOKENS];
    int n = tokenize(line, tok, MAX_TOKENS);
    int i, err = 0;

    memset(cmd, 0, sizeof(*cmd));
    cmd->raw = strdup(line);
    cmd->ncmds = 1;
    simple_cmd_t *cur = &cmd->cmds[0];

    for (i = 0; i < n && !err; i++) {
        char *t = tok[i];
        if (strcmp(t, "|") == 0) {                       /* เริ่มคำสั่งย่อยใหม่ */
            if (cur->argc == 0 || cmd->ncmds >= MAX_CMDS) err = 1;
            else cur = &cmd->cmds[cmd->ncmds++];
            free(t);
        } else if (t[0] == '<' || t[0] == '>') {         /* redirect: token ถัดไปคือชื่อไฟล์ */
            if (i + 1 >= n || is_op(tok[i + 1])) { err = 1; free(t); continue; }
            char *file = tok[++i];
            if (t[0] == '<') { free(cur->in_file); cur->in_file = file; }
            else { free(cur->out_file); cur->out_file = file; cur->append = (t[1] == '>'); }
            free(t);
        } else if (strcmp(t, "&") == 0) {                /* & ต้องอยู่ท้ายสุด */
            if (i != n - 1) err = 1; else cmd->background = 1;
            free(t);
        } else {                                         /* คำธรรมดา = argument */
            if (cur->argc < MAX_ARGS - 1) cur->argv[cur->argc++] = t;
            else { free(t); err = 1; }
        }
    }
    for (; i < n; i++) free(tok[i]);                     /* token ที่เหลือตอนเจอ error */

    if (!err && cur->argc == 0) err = 1;                 /* เช่น "ls |" */
    for (int k = 0; k < cmd->ncmds && !err; k++) {       /* < ได้เฉพาะตัวแรก, > ได้เฉพาะตัวสุดท้าย */
        if (k > 0 && cmd->cmds[k].in_file) err = 1;
        if (k < cmd->ncmds - 1 && cmd->cmds[k].out_file) err = 1;
    }

    if (err) { free_command(cmd); return -1; }
    return 0;
}

void free_command(command_t *cmd) {
    for (int k = 0; k < cmd->ncmds; k++) {
        simple_cmd_t *sc = &cmd->cmds[k];
        for (int a = 0; a < sc->argc; a++) free(sc->argv[a]);
        free(sc->in_file);
        free(sc->out_file);
    }
    free(cmd->raw);
    memset(cmd, 0, sizeof(*cmd));
}
/* parser.h (A) — สัญญากลาง: โครงสร้างคำสั่งที่ทุกส่วนใช้ร่วมกัน */
#ifndef PARSER_H
#define PARSER_H

#define MAX_ARGS 64   /* argument สูงสุดต่อคำสั่งย่อย */
#define MAX_CMDS 16   /* คำสั่งย่อยสูงสุดใน pipeline  */

/* คำสั่งย่อย 1 ตัว (ส่วนที่คั่นด้วย |) */
typedef struct {
    char *argv[MAX_ARGS]; /* argv[0] = ชื่อโปรแกรม, ปิดท้ายด้วย NULL */
    int   argc;
    char *in_file;        /* ไฟล์หลัง <  (NULL = ไม่มี) */
    char *out_file;       /* ไฟล์หลัง > หรือ >> (NULL = ไม่มี) */
    int   append;         /* 1 = >>, 0 = > */
} simple_cmd_t;

/* คำสั่งทั้งบรรทัด */
typedef struct {
    simple_cmd_t cmds[MAX_CMDS];
    int   ncmds;          /* จำนวนคำสั่งย่อย (1 = ไม่มี pipe) */
    int   background;     /* 1 = ลงท้ายด้วย & */
    char *raw;            /* ข้อความเดิมที่ผู้ใช้พิมพ์ */
} command_t;

/* คืน 0 = สำเร็จ, -1 = รูปแบบคำสั่งผิด */
int  parse_line(const char *line, command_t *cmd);
void free_command(command_t *cmd);

#endif
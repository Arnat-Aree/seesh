/* history.c (C) — เก็บคำสั่งล่าสุด 100 คำสั่งแบบวนทับ */
#include "history.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_MAX 100

static char *hist[HISTORY_MAX];
static int   count = 0;           /* จำนวนคำสั่งทั้งหมดที่เคยเพิ่ม (ใช้เป็นเลขลำดับ) */

void history_add(const char *line) {
    int slot = count % HISTORY_MAX;   /* เต็มแล้ววนกลับไปทับช่องที่เก่าที่สุด */
    free(hist[slot]);
    hist[slot] = strdup(line);
    count++;
}

void history_print(void) {
    int start = count > HISTORY_MAX ? count - HISTORY_MAX : 0;
    for (int i = start; i < count; i++)
        printf("%5d  %s\n", i + 1, hist[i % HISTORY_MAX]);
}
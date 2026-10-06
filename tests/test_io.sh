#!/usr/bin/env bash
# test_io.sh — redirect และ pipe
source "$(dirname "$0")/lib.sh"
section "I/O: redirect และ pipe"

NUMS='printf "3\n1\n2\n3\n" > n.txt'

same     "> เขียนทับ"                 ""            $'echo Hello > f.txt\ncat f.txt'
same     ">> เขียนต่อท้าย"            ""            $'echo A > f.txt\necho B >> f.txt\ncat f.txt'
same     "< อ่านจากไฟล์"              "$NUMS"       'sort < n.txt'
same     "pipe 2 ขั้น"                'touch a.c b.c x.txt' 'ls | wc -l'
same     "pipe 3 ขั้น"                'touch a.c b.c x.txt' 'ls | grep c | wc -l'
same     "pipe 4 ขั้น"                'touch a.c b.c x.txt' 'ls | grep c | sort -r | head -1'
same     "pipe + redirect"           "$NUMS"       $'cat < n.txt | sort | uniq > out.txt\ncat out.txt'
same     "ข้อมูลเยอะผ่านท่อ (ไม่ค้าง)"   ""            'seq 1 100000 | wc -l'
same     "โปรแกรมอ่านไม่หมด (ไม่ค้าง)"  ""            'yes | head -3'
same     "built-in + redirect"       ""            $'pwd > p.txt\nwc -l < p.txt'
same     "คืนทางออกหลัง built-in"     ""            $'pwd > p.txt\necho visible'
contains "history + redirect"        ""            $'echo hi\nhistory > h.txt\ncat h.txt' "history > h.txt"
contains "เปิดไฟล์ไม่ได้"              ""            'cat < nofile.txt' "No such file"
contains "เขียนไฟล์ในโฟลเดอร์ที่ไม่มี"  ""            'echo x > nodir/x.txt' "No such file"

summary
#!/usr/bin/env bash
# test_signals.sh — background job และการเก็บกวาด process
# หมายเหตุ: Ctrl+C ต้องทดสอบด้วยมือ
source "$(dirname "$0")/lib.sh"
section "Signals: background job"

contains "& แจ้งหมายเลข job"         ""  'sleep 1 &'                          "[1]"
contains "jobs แสดงงานที่ยังทำอยู่"    ""  $'sleep 3 &\njobs'                    "Running"
contains "แจ้ง Done เมื่องานจบ"       ""  $'sleep 1 &\nsleep 2\necho next'     "Done"
lacks    "ไม่มี zombie หลงเหลือ"      ""  $'sleep 1 &\nsleep 2\nps -o stat='  "Z"
contains "shell ไม่รองานเบื้องหลัง"    ""  $'sleep 3 &\necho immediately'       "immediately"

summary
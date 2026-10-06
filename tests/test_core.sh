#!/usr/bin/env bash
# test_core.sh — รับคำสั่ง, รันโปรแกรม, built-in
source "$(dirname "$0")/lib.sh"
section "Core: คำสั่งพื้นฐานและ built-in"

same     "echo"                     ""            'echo hello world'
same     "ข้อความในเครื่องหมายคำพูด"   ""            'echo "Hello   OS"'
same     "ls"                       'touch a b c' 'ls'
same     "cd เข้าโฟลเดอร์ย่อย"         'mkdir sub; touch sub/x' $'cd sub\nls'
same     "cd ไม่ใส่อะไร = กลับ home"   ""            $'cd\npwd'
same     "exit หยุดทันที"             ""            $'echo a\nexit\necho b'
same     "บรรทัดว่างไม่ error"          ""            $'\n\necho ok'
contains "หาโปรแกรมไม่เจอ"           ""            'abcxyz'           "command not found"
contains "เจอแต่รันไม่ได้"             ""            '/tmp'             "Permission denied"
contains "syntax error"             ""            'ls |'             "syntax error"
contains "shell ทำงานต่อหลัง error"  ""            $'abcxyz\necho alive' "alive"
contains "help มี history"           ""            'help'             "history"
contains "history บันทึกคำสั่ง"       ""            $'echo hi\nhistory' "2  history"

summary
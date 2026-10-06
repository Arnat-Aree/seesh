#!/usr/bin/env bash
# lib.sh — ฟังก์ชันช่วยทดสอบ: ป้อนคำสั่งเข้า seesh และ bash แล้วเทียบผล

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SEESH="$ROOT/seesh"
PASS=0
FAIL=0
GREEN=$'\033[32m'; RED=$'\033[31m'; RESET=$'\033[0m'

# รันคำสั่งในโฟลเดอร์ว่างใหม่ทุกครั้ง แล้วลบทิ้ง
#   $1 = shell ที่ใช้รัน   $2 = คำสั่งเตรียมไฟล์ (รันด้วย bash)
#   $3 = คำสั่งทดสอบ      $4 = 1 ถ้าต้องการเก็บ error ด้วย
run_fresh() {
  local dir out code
  dir=$(mktemp -d)
  out=$(
    cd "$dir" || exit 1
    [ -n "$2" ] && printf '%s\n' "$2" | bash >/dev/null 2>&1
    if [ "$4" = 1 ]; then
      printf '%s\n' "$3" | perl -e 'alarm 5; exec @ARGV' "$1" 2>&1   # เกิน 5 วินาที = ค้าง
    else
      printf '%s\n' "$3" | perl -e 'alarm 5; exec @ARGV' "$1" 2>/dev/null
    fi
  )
  code=$?
  rm -rf "$dir"
  [ $code -eq 142 ] && out="__TIMEOUT__"     # 142 = ถูกตัดเพราะเกินเวลา (128 + SIGALRM)
  printf '%s' "$out"
}

ok()  { PASS=$((PASS + 1)); echo "  ${GREEN}✓${RESET} $1"; }
bad() { FAIL=$((FAIL + 1)); echo "  ${RED}✗${RESET} $1"
        echo "      คาดหวัง: $2"
        echo "      ได้จริง: $3"; }

section() { echo; echo "== $1 =="; }

# same "ชื่อ" "เตรียมไฟล์" "คำสั่ง" — ผลของ seesh ต้องเหมือน bash ทุกตัวอักษร
same() {
  local want got
  want=$(run_fresh bash "$2" "$3" 0)
  got=$(run_fresh "$SEESH" "$2" "$3" 0)
  if [ "$want" = "$got" ]; then ok "$1"; else bad "$1" "$want" "$got"; fi
}

# contains "ชื่อ" "เตรียมไฟล์" "คำสั่ง" "ข้อความ" — ผลของ seesh (รวม error) ต้องมีข้อความนี้
contains() {
  local got
  got=$(run_fresh "$SEESH" "$2" "$3" 1)
  if [[ "$got" == *"$4"* ]]; then ok "$1"; else bad "$1" "มีข้อความ \"$4\"" "$got"; fi
}

# lacks "ชื่อ" "เตรียมไฟล์" "คำสั่ง" "ข้อความ" — ผลของ seesh ต้องไม่มีข้อความนี้ และต้องไม่ค้าง
lacks() {
  local got
  got=$(run_fresh "$SEESH" "$2" "$3" 1)
  if [[ "$got" != *"$4"* && "$got" != "__TIMEOUT__" ]]; then ok "$1"; else bad "$1" "ไม่มีข้อความ \"$4\"" "$got"; fi
}

# สรุปผล: คืนค่า 0 ถ้าผ่านทุกข้อ
summary() {
  echo "  ผ่าน $PASS / $((PASS + FAIL))"
  [ $FAIL -eq 0 ]
}
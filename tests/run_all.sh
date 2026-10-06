#!/usr/bin/env bash
# run_all.sh — รันชุดทดสอบทั้งหมด
cd "$(dirname "$0")" || exit 1

failed=0
for t in test_core.sh test_io.sh test_signals.sh; do
  bash "$t" || failed=1
done

echo
if [ $failed -eq 0 ]; then
  echo "ผ่านทั้งหมด ✓"
else
  echo "มีบางข้อไม่ผ่าน ✗"
  exit 1
fi
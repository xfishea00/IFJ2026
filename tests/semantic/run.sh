#!/bin/sh

set -u

parser=${1:-./ifj2026}
passed=0
failed=0

for test_file in tests/semantic/invalid/*.scala; do
  [ -f "$test_file" ] || continue

  output=$("$parser" < "$test_file" 2>&1)
  actual_status=$?
  expected_status=6

  if [ "$actual_status" -eq "$expected_status" ]; then
    printf 'PASS %s\n' "$test_file"
    passed=$((passed + 1))
  else
    printf 'FAIL %s (expected exit %s, got %s)\n' \
      "$test_file" "$expected_status" "$actual_status"
    if [ -n "$output" ]; then
      printf '%s\n' "$output" | sed 's/^/  /'
    fi
    failed=$((failed + 1))
  fi
done

printf '%s passed, %s failed\n' "$passed" "$failed"
[ "$failed" -eq 0 ]

#!/bin/sh

set -u

parser=${1:-./ifj2026}
passed=0
failed=0

run_group() {
  directory=$1
  expected_status=$2

  for test_file in "$directory"/*.scala; do
    [ -f "$test_file" ] || continue
    if [ "$test_file" = "tests/syntax/invalid/duplicate_definition.scala" ]; then
      continue
    fi

    output=$("$parser" < "$test_file" 2>&1)
    actual_status=$?

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
}

run_group tests/syntax/valid 0
run_group tests/syntax/invalid 2

duplicate_test="tests/syntax/invalid/duplicate_definition.scala"
output=$("$parser" < "$duplicate_test" 2>&1)
actual_status=$?
if [ "$actual_status" -eq 4 ]; then
  printf 'PASS %s\n' "$duplicate_test"
  passed=$((passed + 1))
else
  printf 'FAIL %s (expected exit 4, got %s)\n' \
    "$duplicate_test" "$actual_status"
  if [ -n "$output" ]; then
    printf '%s\n' "$output" | sed 's/^/  /'
  fi
  failed=$((failed + 1))
fi

generated_output=$("$parser" < tests/syntax/valid/funexp2.scala 2>/dev/null)
if printf '%s\n' "$generated_output" | grep -q '^\.IFJcode26$' &&
  printf '%s\n' "$generated_output" | grep -q '^CALL \$function_'; then
  printf 'PASS generator emits function code\n'
  passed=$((passed + 1))
else
  printf 'FAIL generator emits function code\n'
  failed=$((failed + 1))
fi

printf '%s passed, %s failed\n' "$passed" "$failed"
[ "$failed" -eq 0 ]

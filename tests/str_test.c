/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    str_test.c
 * Authors: Michal Řezníček (xreznim00)
 *
 * AI-generated test (Claude Opus 5.5). Prompt: "napis test pro str.c, over
 * init, pridavani znaku at se buffer zvetsi, clear, take a pridavani po take a
 * at vypise co selhalo"
 */

#include "../src/str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

// prints a message and counts a failure when the condition does not hold
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL line %d: %s\n", __LINE__, msg);                    \
      failures++;                                                              \
    }                                                                          \
  } while (0)

int main(void) {
  str_t s;

  // init gives an empty string
  CHECK(str_init(&s) == 0, "str_init returns 0");
  CHECK(s.length == 0, "new string has length 0");
  CHECK(strcmp(s.data, "") == 0, "new string is empty");

  // append many characters so the buffer has to grow several times
  for (int i = 0; i < 1000; i++) {
    CHECK(str_append(&s, (char)('a' + i % 26)) == 0, "str_append returns 0");
  }
  CHECK(s.length == 1000, "length is 1000 after 1000 appends");
  CHECK(s.capacity > s.length, "capacity leaves room for '\\0'");
  CHECK(strlen(s.data) == 1000, "data is NUL-terminated");
  CHECK(s.data[0] == 'a' && s.data[25] == 'z' && s.data[26] == 'a',
        "characters are stored in order");

  // clear keeps the memory but empties the content
  size_t capacity_before = s.capacity;
  str_clear(&s);
  CHECK(s.length == 0, "length is 0 after clear");
  CHECK(strcmp(s.data, "") == 0, "string is empty after clear");
  CHECK(s.capacity == capacity_before, "clear keeps capacity");

  // take hands over the content
  str_append(&s, 'x');
  str_append(&s, 'y');
  char *taken = str_take(&s);
  CHECK(taken != NULL && strcmp(taken, "xy") == 0, "take returns content");
  CHECK(s.length == 0, "string is empty after take");
  free(taken);

  // appending after take must work again (the scanner does this every token)
  CHECK(str_append(&s, 'z') == 0, "append after take returns 0");
  CHECK(strcmp(s.data, "z") == 0, "append after take stores the char");
  free(str_take(&s));

  // take with nothing appended since the last take (empty "" literal)
  // must return an empty string, not NULL
  char *empty = str_take(&s);
  CHECK(empty != NULL, "take of nothing returns non-NULL");
  CHECK(empty == NULL || strcmp(empty, "") == 0,
        "take of nothing returns empty string");
  free(empty);

  str_free(&s);
  CHECK(s.data == NULL, "data is NULL after free");

  if (failures == 0) {
    printf("str_test: OK\n");
  }
  return failures == 0 ? 0 : 1;
}
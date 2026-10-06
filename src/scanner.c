/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    scanner.c
 * Authors: Michal Řezníček (xreznim00)
 */

#include "scanner.h"
#include "error.h"
#include "str.h"

#include <ctype.h>
#include <stdlib.h>

#define PUSHBACK_SIZE 4 // how many characters can be returned to the input

static FILE *input;                 // where to read the input from
static int line;                    // current line, starts at 1
static str_t lexeme;                // text of the token being read
static int pushback[PUSHBACK_SIZE]; // characters returned by unget_char
static int pushback_count;          // how many characters are in pushback

// returns the next character of the input or EOF
static int get_char(void) {
  int c;
  if (pushback_count > 0) {
    c = pushback[--pushback_count];
  } else {
    c = fgetc(input);
  }
  if (c == '\n') {
    line++;
  }
  return c;
}

// returns a character back, next get_char will return it again
static void unget_char(int c) {
  if (c == EOF || pushback_count >= PUSHBACK_SIZE) {
    return; // do not push back
  }
  pushback[pushback_count++] = c;
  if (c == '\n') {
    line--;
  }
}

int scanner_init(FILE *in) {
  input = in;
  line = 1;
  pushback_count = 0;
  return str_init(&lexeme);
}

void scanner_free(void) { str_free(&lexeme); }
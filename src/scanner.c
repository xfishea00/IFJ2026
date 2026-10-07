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

// states of finite automaton
typedef enum {
  STATE_START,  // S
  STATE_LT,     // f1 after '<'
  STATE_GT,     // f3 after '>'
  STATE_ASSIGN, // f5 after '='
  STATE_EXCL,   // q1 after '!'
  STATE_AT,     // q2 after '@'
  STATE_AT_M,   // q3 after "@m"
  STATE_AT_MA,  // q4 after "@ma"
  STATE_AT_MAI, // q5 after "@mai"
} state_t;

int scanner_next(token_t *out) {
  *out = (token_t){0}; // empty token
  str_clear(&lexeme);
  state_t state = STATE_START;

  while (1) {
    int c = get_char();

    switch (state) {
    case STATE_START:
      out->line = line;
      if (c == ' ' || c == '\t') {
        // whitespace, stay in STATE_START
      } else if (c == '\n') {
        out->type = TOK_NEWLINE;
        return ERR_OK;
      } else if (c == EOF) {
        out->type = TOK_EOF;
        return ERR_OK;
      } else if (c == '+') {
        out->type = TOK_PLUS;
        return ERR_OK;
      } else if (c == '-') {
        out->type = TOK_MINUS;
        return ERR_OK;
      } else if (c == '*') {
        out->type = TOK_MUL;
        return ERR_OK;
      } else if (c == '/') {
        out->type = TOK_DIV;
        return ERR_OK;
      } else if (c == ':') {
        out->type = TOK_COLON;
        return ERR_OK;
      } else if (c == ',') {
        out->type = TOK_COMMA;
        return ERR_OK;
      } else if (c == '.') {
        out->type = TOK_DOT;
        return ERR_OK;
      } else if (c == ';') {
        out->type = TOK_SEMICOLON;
        return ERR_OK;
      } else if (c == '(') {
        out->type = TOK_LPAR;
        return ERR_OK;
      } else if (c == ')') {
        out->type = TOK_RPAR;
        return ERR_OK;
      } else if (c == '<') {
        state = STATE_LT;
      } else if (c == '>') {
        state = STATE_GT;
      } else if (c == '=') {
        state = STATE_ASSIGN;
      } else if (c == '!') {
        state = STATE_EXCL;
      } else if (c == '@') {
        state = STATE_AT;
      } else
        return ERR_LEXICAL; // invalid character
      break;
    default:
      return ERR_INTERNAL; // unknown state
    }
  }
}
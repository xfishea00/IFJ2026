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
#include <string.h>

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
  STATE_START,      // S
  STATE_LT,         // f1 after '<'
  STATE_GT,         // f3 after '>'
  STATE_ASSIGN,     // f5 after '='
  STATE_EXCL,       // q1 after '!'
  STATE_AT,         // q2 after '@'
  STATE_AT_M,       // q3 after "@m"
  STATE_AT_MA,      // q4 after "@ma"
  STATE_AT_MAI,     // q5 after "@mai"
  STATE_UNDERSCORE, // q6 after "_"
  STATE_ID,         // f19 identifier
} state_t;

// keywords of IFJ26 and their token types
static const struct {
  const char *name;
  token_type_t type;
} keywords[] = {
    {"def", TOK_KW_DEF},
    {"do", TOK_KW_DO},
    {"while", TOK_KW_WHILE},
    {"Int", TOK_KW_INT},
    {"Double", TOK_KW_DOUBLE},
    {"String", TOK_KW_STRING},
    {"if", TOK_KW_IF},
    {"else", TOK_KW_ELSE},
    {"then", TOK_KW_THEN},
    {"return", TOK_KW_RETURN},
    {"import", TOK_KW_IMPORT},
    {"var", TOK_KW_VAR},
    {"val", TOK_KW_VAL},
    {"Unit", TOK_KW_UNIT},
    // extension keywords
    {"for", TOK_KW_FOR},
    {"Boolean", TOK_KW_BOOLEAN},
    {"match", TOK_KW_MATCH},
    {"List", TOK_KW_LIST},
    {"class", TOK_KW_CLASS},
    {"case", TOK_KW_CASE},
    {"extension", TOK_KW_EXTENSION},
    {"type", TOK_KW_TYPE},
};

// returns the keyword token type for the text, or TOK_ID if it is not a keyword
static token_type_t keyword_type(const char *text) {
  for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
    if (strcmp(text, keywords[i].name) == 0) {
      return keywords[i].type;
    }
  }
  return TOK_ID; // not a keyword
}

int scanner_next(token_t *out) {
  *out = (token_t){0}; // empty token
  str_clear(&lexeme);
  state_t state = STATE_START;

  while (1) {
    int c_line = line; // line of the char we are about to read
    int c = get_char();

    switch (state) {
    case STATE_START:
      out->line = c_line;
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
      } else if (isalpha(c)) {
        if (str_append(&lexeme, (char)c) != ERR_OK) {
          return ERR_INTERNAL;
        }
        state = STATE_ID;
      } else if (c == '_') {
        if (str_append(&lexeme, (char)c) != ERR_OK) {
          return ERR_INTERNAL;
        }
        state = STATE_UNDERSCORE;
      } else {
        return ERR_LEXICAL; // invalid character
      }
      break;

    case STATE_LT:
      if (c == '=') {
        out->type = TOK_LE;
        return ERR_OK;
      } else {
        unget_char(c);
        out->type = TOK_LT;
        return ERR_OK;
      }

    case STATE_GT:
      if (c == '=') {
        out->type = TOK_GE;
        return ERR_OK;
      } else {
        unget_char(c);
        out->type = TOK_GT;
        return ERR_OK;
      }

    case STATE_ASSIGN:
      if (c == '=') {
        out->type = TOK_EQ;
        return ERR_OK;
      } else {
        unget_char(c);
        out->type = TOK_ASSIGN;
        return ERR_OK;
      }

    case STATE_EXCL:
      if (c == '=') {
        out->type = TOK_NEQ;
        return ERR_OK;
      } else {
        return ERR_LEXICAL; // ! alone is not a valid token
      }

    case STATE_AT:
      if (c == 'm') {
        state = STATE_AT_M;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_AT_M:
      if (c == 'a') {
        state = STATE_AT_MA;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_AT_MA:
      if (c == 'i') {
        state = STATE_AT_MAI;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_AT_MAI:
      if (c == 'n') {
        out->type = TOK_AT_MAIN;
        return ERR_OK;
      } else {
        return ERR_LEXICAL;
      }

    case STATE_UNDERSCORE:
      if (isalnum(c) || c == '_') {
        if (str_append(&lexeme, (char)c) != ERR_OK) {
          return ERR_INTERNAL;
        }
        state = STATE_ID;
      } else {
        return ERR_LEXICAL; // '_' alone is not a valid token
      }
      break;

    case STATE_ID:
      if (isalnum(c) || c == '_') {
        if (str_append(&lexeme, (char)c) != ERR_OK) {
          return ERR_INTERNAL;
        }
      } else {
        // end of identifier
        unget_char(c);
        out->type = keyword_type(lexeme.data);
        if (out->type == TOK_ID) {
          out->string_val = str_take(&lexeme); // token owns the text
          if (out->string_val == NULL) {
            return ERR_INTERNAL;
          }
        }
        return ERR_OK;
      }
      break;
    }
  }
}

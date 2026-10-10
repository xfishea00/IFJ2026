/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    scanner.c
 * Authors: Michal Řezníček (xreznim00)
 */

#include "scanner.h"
#include "error.h"
#include "str.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PUSHBACK_SIZE 4 // how many characters can be returned to the input

static FILE *input;                 // where to read the input from
static int line;                    // current line, starts at 1
static str_t lexeme;                // text of the token being read
static int pushback[PUSHBACK_SIZE]; // characters returned by unget_char
static int pushback_count;          // how many characters are in pushback

// macro for appending a character to the lexeme
#define APPEND(c)                                                              \
  do {                                                                         \
    if (str_append(&lexeme, c) != ERR_OK) {                                    \
      return ERR_INTERNAL;                                                     \
    }                                                                          \
  } while (0)

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
    return; // nothing to push back
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
  STATE_START,              // S
  STATE_LT,                 // f1 after '<'
  STATE_GT,                 // f3 after '>'
  STATE_ASSIGN,             // f5 after '='
  STATE_EXCL,               // q1 after '!'
  STATE_AT,                 // q2 after '@'
  STATE_AT_M,               // q3 after "@m"
  STATE_AT_MA,              // q4 after "@ma"
  STATE_AT_MAI,             // q5 after "@mai"
  STATE_UNDERSCORE,         // q6 after '_'
  STATE_ID,                 // f19 identifier
  STATE_ZERO,               // f20 after '0'
  STATE_INT,                // f23 integer literal
  STATE_INT_UNDERSCORE,     // q9 after '_' in integer part
  STATE_INT_DOT,            // q7 after '.' in number
  STATE_DECIMAL,            // f21 decimal part
  STATE_DECIMAL_UNDERSCORE, // q10 after '_' in decimal part
  STATE_EXP,                // q8 after 'e'/'E'
  STATE_EXP_SIGN,           // q12 after sign of exponent
  STATE_EXP_DIGITS,         // f22 digits of exponent
  STATE_EXP_UNDERSCORE,     // q11 after '_' in exponent
  STATE_DOT,                // f13 after '.' at start of token
  STATE_STR_START,          // q13 after the opening '"'
  STATE_STR,                // q14 inside single line string
  STATE_STR_ESC,            // q15 after '\' in string
  STATE_STR_EMPTY,          // f26 after '""', empty string or start of '"""'
  STATE_ML,                 // q16 inside multi line string
  STATE_ML_Q1,              // q17 one '"' in multi line string
  STATE_ML_Q2,              // q18 two '"' in multi line string
  STATE_DIV,                // f10 after '/'
  STATE_LINE_COMMENT,       // q19 inside "//" comment
  STATE_BLOCK,              // q20 inside block comment
  STATE_BLOCK_SLASH,        // q21 '/' inside block comment
  STATE_BLOCK_STAR,         // q22 '*' inside block comment
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

// returns true if next char can be part of an identifier (letter, digit, '_')
static bool next_is_id_char(void) {
  int next = get_char();
  unget_char(next);
  return isalnum(next) || next == '_';
}

// sets the type of number token and converts lexeme to value
static int finish_number(token_t *out, token_type_t type) {
  if (next_is_id_char()) {
    return ERR_LEXICAL;
  }
  out->type = type;
  if (type == TOK_INT_LIT) {
    out->int_val = strtoll(lexeme.data, NULL, 10);
  } else {
    errno = 0;
    double value = strtod(lexeme.data, NULL);
    if (errno == ERANGE && (value == HUGE_VAL || value == -HUGE_VAL)) {
      return ERR_LEXICAL; // too large for double
    }
    out->double_val = value;
  }
  return ERR_OK;
}

int scanner_next(token_t *out) {
  *out = (token_t){0}; // empty token
  str_clear(&lexeme);
  state_t state = STATE_START;
  int depth = 0;   // nesting of block comments
  bool nl = false; // block comment with '\n'

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
        state = STATE_DIV;
      } else if (c == ':') {
        out->type = TOK_COLON;
        return ERR_OK;
      } else if (c == ',') {
        out->type = TOK_COMMA;
        return ERR_OK;
      } else if (c == '.') {
        APPEND(c);
        state = STATE_DOT;
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
        APPEND(c);
        state = STATE_ID;
      } else if (c == '_') {
        APPEND(c);
        state = STATE_UNDERSCORE;
      } else if (c == '0') {
        APPEND(c);
        state = STATE_ZERO;
      } else if (isdigit(c)) { // c is 1-9
        APPEND(c);
        state = STATE_INT;
      } else if (c == '"') {
        state = STATE_STR_START;
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
        if (next_is_id_char()) {
          return ERR_LEXICAL;
        }
        out->type = TOK_AT_MAIN;
        return ERR_OK;
      } else {
        return ERR_LEXICAL;
      }

    case STATE_UNDERSCORE:
      if (isalnum(c) || c == '_') {
        APPEND(c);
        state = STATE_ID;
      } else {
        return ERR_LEXICAL; // '_' alone is not a valid token
      }
      break;

    case STATE_ID:
      if (isalnum(c) || c == '_') {
        APPEND(c);
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

    case STATE_ZERO:
      if (c == '.') {
        APPEND(c);
        state = STATE_INT_DOT;
      } else if (c == 'e' || c == 'E') {
        APPEND(c);
        state = STATE_EXP;
      } else if (c == 'd' || c == 'D') {
        return finish_number(out, TOK_DOUBLE_LIT); // 'd' is not stored
      } else {
        unget_char(c);
        return finish_number(out, TOK_INT_LIT);
      }
      break;

    case STATE_INT:
      if (isdigit(c)) {
        APPEND(c);
      } else if (c == '_') {
        state = STATE_INT_UNDERSCORE; // '_' is not stored
      } else if (c == '.') {
        APPEND(c);
        state = STATE_INT_DOT;
      } else if (c == 'e' || c == 'E') {
        APPEND(c);
        state = STATE_EXP;
      } else if (c == 'd' || c == 'D') {
        return finish_number(out, TOK_DOUBLE_LIT); // 'd' is not stored
      } else {
        unget_char(c);
        return finish_number(out, TOK_INT_LIT);
      }
      break;

    case STATE_INT_UNDERSCORE:
      if (c == '_') {
        state = STATE_INT_UNDERSCORE;
      } else if (isdigit(c)) {
        APPEND(c);
        state = STATE_INT;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_INT_DOT:
      if (isdigit(c)) {
        APPEND(c);
        state = STATE_DECIMAL;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_DECIMAL:
      if (isdigit(c)) {
        APPEND(c);
      } else if (c == '_') {
        state = STATE_DECIMAL_UNDERSCORE; // '_' is not stored
      } else if (c == 'e' || c == 'E') {
        APPEND(c);
        state = STATE_EXP;
      } else if (c == 'd' || c == 'D') {
        return finish_number(out, TOK_DOUBLE_LIT); // 'd' is not stored
      } else {
        unget_char(c);
        return finish_number(out, TOK_DOUBLE_LIT);
      }
      break;

    case STATE_DECIMAL_UNDERSCORE:
      if (c == '_') {
        state = STATE_DECIMAL_UNDERSCORE;
      } else if (isdigit(c)) {
        APPEND(c);
        state = STATE_DECIMAL;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_EXP:
      if (isdigit(c)) {
        APPEND(c);
        state = STATE_EXP_DIGITS;
      } else if (c == '+' || c == '-') {
        APPEND(c);
        state = STATE_EXP_SIGN;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_EXP_SIGN:
      if (isdigit(c)) {
        APPEND(c);
        state = STATE_EXP_DIGITS;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_EXP_DIGITS:
      if (isdigit(c)) {
        APPEND(c);
      } else if (c == '_') {
        state = STATE_EXP_UNDERSCORE; // '_' is not stored
      } else if (c == 'd' || c == 'D') {
        return finish_number(out, TOK_DOUBLE_LIT); // 'd' is not stored
      } else {
        unget_char(c);
        return finish_number(out, TOK_DOUBLE_LIT);
      }
      break;

    case STATE_EXP_UNDERSCORE:
      if (c == '_') {
        state = STATE_EXP_UNDERSCORE;
      } else if (isdigit(c)) {
        APPEND(c);
        state = STATE_EXP_DIGITS;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_DOT:
      if (isdigit(c)) {
        APPEND(c);
        state = STATE_DECIMAL;
      } else {
        unget_char(c);
        out->type = TOK_DOT;
        return ERR_OK;
      }
      break;

    case STATE_STR_START:
      if (c == '"') {
        state = STATE_STR_EMPTY;
      } else if (c == '\\') {
        state = STATE_STR_ESC;
      } else if (c == '\n' || c == EOF) {
        return ERR_LEXICAL;
      } else {
        APPEND(c);
        state = STATE_STR;
      }
      break;

    case STATE_STR:
      if (c == '"') {
        out->type = TOK_STRING_LIT;
        out->string_val = str_take(&lexeme);
        if (out->string_val == NULL) {
          return ERR_INTERNAL;
        }
        return ERR_OK;
      } else if (c == '\\') {
        state = STATE_STR_ESC;
      } else if (c == '\n' || c == EOF) {
        return ERR_LEXICAL;
      } else {
        APPEND(c);
      }
      break;

    case STATE_STR_ESC:
      if (c == 'n') {
        APPEND('\n');
        state = STATE_STR;
      } else if (c == 't') {
        APPEND('\t');
        state = STATE_STR;
      } else if (c == 'r') {
        APPEND('\r');
        state = STATE_STR;
      } else if (c == '"') {
        APPEND('"');
        state = STATE_STR;
      } else if (c == '\\') {
        APPEND('\\');
        state = STATE_STR;
      } else {
        return ERR_LEXICAL;
      }
      break;

    case STATE_STR_EMPTY:
      if (c == '"') {
        state = STATE_ML;
      } else {
        unget_char(c);
        out->type = TOK_STRING_LIT;
        out->string_val = str_take(&lexeme);
        if (out->string_val == NULL) {
          return ERR_INTERNAL;
        }
        return ERR_OK;
      }
      break;

    case STATE_ML:
      if (c == '"') {
        state = STATE_ML_Q1;
      } else if (c == EOF) {
        return ERR_LEXICAL;
      } else {
        APPEND(c);
      }
      break;

    case STATE_ML_Q1:
      if (c == '"') {
        state = STATE_ML_Q2;
      } else if (c == EOF) {
        return ERR_LEXICAL;
      } else {
        // the '"' was part of the content
        APPEND('"');
        APPEND(c);
        state = STATE_ML;
      }
      break;

    case STATE_ML_Q2:
      if (c == '"') {
        out->type = TOK_STRING_LIT;
        out->string_val = str_take(&lexeme);
        if (out->string_val == NULL) {
          return ERR_INTERNAL;
        }
        return ERR_OK;
      } else if (c == EOF) {
        return ERR_LEXICAL;
      } else {
        // both '"' were part of the content
        APPEND('"');
        APPEND('"');
        APPEND(c);
        state = STATE_ML;
      }
      break;

    case STATE_DIV:
      if (c == '/') {
        state = STATE_LINE_COMMENT;
      } else if (c == '*') {
        depth = 0;
        nl = false;
        state = STATE_BLOCK;
      } else {
        unget_char(c);
        out->type = TOK_DIV;
        return ERR_OK;
      }
      break;

    case STATE_LINE_COMMENT:
      if (c == '\n' || c == EOF) {
        unget_char(c);
        state = STATE_START;
      }
      break;

    case STATE_BLOCK:
      if (c == '*') {
        state = STATE_BLOCK_STAR;
      } else if (c == '/') {
        state = STATE_BLOCK_SLASH;
      } else if (c == '\n') {
        nl = true;
      } else if (c == EOF) {
        return ERR_LEXICAL;
      }
      break;

    case STATE_BLOCK_SLASH:
      if (c == '*') {
        depth++;
        state = STATE_BLOCK;
      } else if (c == '\n') {
        nl = true;
        state = STATE_BLOCK;
      } else if (c == '/') {
        state = STATE_BLOCK_SLASH;
      } else if (c == EOF) {
        return ERR_LEXICAL;
      } else {
        state = STATE_BLOCK;
      }
      break;

    case STATE_BLOCK_STAR:
      if (c == '/') {
        if (depth > 0) {
          depth--; // end of nested comment
          state = STATE_BLOCK;
        } else if (nl) {
          out->type = TOK_NEWLINE; // comment with '\n' acts as end of line
          return ERR_OK;
        } else {
          state = STATE_START; // comment without '\n' acts as whitespace
        }
      } else if (c == '*') {
        state = STATE_BLOCK_STAR;
      } else if (c == '\n') {
        nl = true;
        state = STATE_BLOCK;
      } else if (c == EOF) {
        return ERR_LEXICAL;
      } else {
        state = STATE_BLOCK;
      }
      break;
    }
  }
}

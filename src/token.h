/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    token.h
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   token types and token structure
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
  TOK_EOF,
  TOK_NEWLINE,
  TOK_INDENT, // indentation token
  TOK_DEDENT, // dedentation token

  TOK_ID,
  TOK_INT_LIT,
  TOK_DOUBLE_LIT,
  TOK_STRING_LIT,
  TOK_AT_MAIN, // @main as single token

  // keywords
  TOK_KW_FIRST,
  TOK_KW_DEF = TOK_KW_FIRST,
  TOK_KW_DO,
  TOK_KW_WHILE,
  TOK_KW_INT,
  TOK_KW_DOUBLE,
  TOK_KW_STRING,
  TOK_KW_IF,
  TOK_KW_ELSE,
  TOK_KW_THEN,
  TOK_KW_RETURN,
  TOK_KW_IMPORT,
  TOK_KW_VAR,
  TOK_KW_VAL,
  TOK_KW_UNIT,
  // extension keywords
  TOK_KW_FOR,
  TOK_KW_BOOLEAN,
  TOK_KW_MATCH,
  TOK_KW_LIST,
  TOK_KW_CLASS,
  TOK_KW_CASE,
  TOK_KW_EXTENSION,
  TOK_KW_TYPE,
  TOK_KW_LAST = TOK_KW_TYPE,

  // operators
  TOK_LT,        // <
  TOK_LE,        // <=
  TOK_GT,        // >
  TOK_GE,        // >=
  TOK_ASSIGN,    // =
  TOK_EQ,        // ==
  TOK_PLUS,      // +
  TOK_MINUS,     // -
  TOK_MUL,       // *
  TOK_DIV,       // /
  TOK_COLON,     // :
  TOK_COMMA,     // ,
  TOK_DOT,       // .
  TOK_SEMICOLON, // ;
  TOK_NEQ,       // !=
  TOK_LPAR,      // (
  TOK_RPAR,      // )
} token_type_t;

// returns true if the token type is a keyword, false otherwise
static inline bool token_is_keyword(token_type_t type) {
  return type >= TOK_KW_FIRST && type <= TOK_KW_LAST;
}

// struct to represent a token
typedef struct {
  token_type_t type;
  int64_t int_val;   // for TOK_INT_LIT
  double double_val; // for TOK_DOUBLE_LIT
  char *string_val;  // for TOK_STRING_LIT and TOK_ID; allocated by scanner,
                     // freed by receiver; NULL for all other token types
  int line;
} token_t;

// frees the string of token
static inline void token_free(token_t *tok) {
  if (tok == NULL) {
    return;
  }
  free(tok->string_val);
  tok->string_val = NULL;
}

#endif // TOKEN_H
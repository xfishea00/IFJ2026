/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    symbol.h
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   symbols of the semantic analysis
 */

#ifndef SYMBOL_H
#define SYMBOL_H

#include "ast.h"
#include <stdbool.h>

// symbol kinds
typedef enum {
  SYM_VAR,   // variable, parameter, for variable, pattern variable
  SYM_FUNC,  // function, also builtin
  SYM_CLASS, // case class
  SYM_ALIAS, // type alias
} symbol_kind_t;

// state of function body analysis
typedef enum {
  FUNC_NOT_DONE,
  FUNC_IN_PROGRESS, // calling it means recursion
  FUNC_DONE,
} func_state_t;

struct symbol {
  symbol_kind_t kind;
  char *name;
  type_t *type;
  char *unique_name;  // name for generator
  ast_node_t *decl;   // NULL for builtin function
  bool is_mutable;    // SYM_VAR only
  bool is_builtin;    // SYM_FUNC only
  func_state_t state; // SYM_FUNC only
};

#endif // SYMBOL_H
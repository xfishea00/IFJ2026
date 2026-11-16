/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    symbol.h
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   symbols of the semantic analysis
 */

#ifndef SYMBOL_H
#define SYMBOL_H

#include "ast.h"
#include "error.h"
#include <stdbool.h>

// symbol kinds
typedef enum {
  SYM_VAR,
  SYM_FUNC,
  SYM_CLASS,
  SYM_ALIAS,
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
  int param_count;    // builtin SYM_FUNC only
};

// adds new symbol to the global list, name is not copied
symbol_t *symbol_new(symbol_kind_t kind, char *name, ast_node_t *decl);

// releases all symbols
void symbols_free_all(void);

// global scope with builtin functions
bool scope_init(void);

// opens a new scope on top
bool scope_push(void);

// closes the top scope, symbols stay valid
void scope_pop(void);

// defines symbol in the top scope, ERR_SEM_REDEFif already there
int scope_define(symbol_t *sym);

// finds symbol from the top scope, NULL if not found
symbol_t *scope_lookup(const char *name);

// releases all scopes, not symbols
void scope_free(void);

#endif // SYMBOL_H
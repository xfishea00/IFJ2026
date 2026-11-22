/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    symbol.c
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   symbols of the semantic analysis
 */

#include "symbol.h"
#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOLS_INIT_CAPACITY 16

// global list of all symbols, released only at the end (AST points to them)
static symbol_t **symbols;
static int symbol_count;
static int symbol_capacity;
static unsigned int unique_name_counter;

static char *make_unique_name(symbol_kind_t kind) {
  char buffer[32];
  char prefix = kind == SYM_FUNC ? 'f' : 'v';
  int length =
      snprintf(buffer, sizeof(buffer), "%c%u", prefix, unique_name_counter++);
  if (length < 0 || (size_t)length >= sizeof(buffer)) {
    return NULL;
  }
  char *name = malloc((size_t)length + 1);
  if (name != NULL) {
    memcpy(name, buffer, (size_t)length + 1);
  }
  return name;
}

symbol_t *symbol_new(symbol_kind_t kind, char *name, ast_node_t *decl) {
  if (symbol_count >= symbol_capacity) {
    int new_capacity = symbol_capacity * 2;
    if (new_capacity == 0) {
      new_capacity = SYMBOLS_INIT_CAPACITY;
    }
    symbol_t **new_symbols =
        realloc(symbols, new_capacity * sizeof(symbol_t *));
    if (new_symbols == NULL) {
      return NULL;
    }
    symbols = new_symbols;
    symbol_capacity = new_capacity;
  }
  symbol_t *sym = malloc(sizeof(symbol_t));
  if (sym == NULL) {
    return NULL;
  }
  char *unique_name = NULL;
  if (kind == SYM_FUNC || kind == SYM_VAR) {
    unique_name = make_unique_name(kind);
    if (unique_name == NULL) {
      free(sym);
      return NULL;
    }
  }
  sym->kind = kind;
  sym->name = name;
  sym->type = NULL;
  sym->unique_name = unique_name;
  sym->decl = decl;
  sym->is_mutable = false;
  sym->is_builtin = false;
  sym->state = FUNC_NOT_DONE;
  sym->param_count = 0;
  symbols[symbol_count++] = sym;
  return sym;
}

void symbols_free_all(void) {
  for (int i = 0; i < symbol_count; i++) {
    free(symbols[i]->unique_name);
    free(symbols[i]);
  }
  free(symbols);
  symbols = NULL;
  symbol_count = 0;
  symbol_capacity = 0;
  unique_name_counter = 0;
}

// one scope = one symbol table, linked to the enclosing scope
struct scope {
  symtable_t *table;
  struct scope *parent; // NULL for global scope
};

static scope_t *current; // innermost scope
static scope_t *global;

// builtin functions, inserted into the global scope by scope_init
static const struct {
  const char *name;
  int param_count;
} builtin_functions[] = {{"print", 1},
                         {"readLine", 0},
                         {"readInt", 0},
                         {"readDouble", 0},
                         {"toStr", 1},
                         {"toInt", 1},
                         {"strLength", 1},
                         {"subStr", 3},
                         {"ord", 2},
                         {"chr", 1},
                         {"readBoolean", 0},
                         {"readIntMaybe", 0},
                         {"readDoubleMaybe", 0}};

bool scope_init(void) {
  global = malloc(sizeof(scope_t));
  if (global == NULL) {
    return false;
  }
  global->table = symtable_init();
  if (global->table == NULL) {
    free(global);
    return false;
  }
  global->parent = NULL;
  current = global;

  for (size_t i = 0;
       i < sizeof(builtin_functions) / sizeof(builtin_functions[0]); i++) {
    const char *name = builtin_functions[i].name;
    int param_count = builtin_functions[i].param_count;
    symbol_t *sym = symbol_new(SYM_FUNC, (char *)name, NULL);
    if (sym == NULL) {
      return false;
    }
    sym->is_builtin = true;
    sym->param_count = param_count;
    sym->state = FUNC_DONE; // no body to analyze
    if (scope_define(sym) != ERR_OK) {
      return false;
    }
  }
  return true;
}

bool scope_push(void) {
  scope_t *new_scope = malloc(sizeof(scope_t));
  if (new_scope == NULL) {
    return false;
  }
  new_scope->table = symtable_init();
  if (new_scope->table == NULL) {
    free(new_scope);
    return false;
  }
  new_scope->parent = current;
  current = new_scope;
  return true;
}

void scope_pop(void) {
  if (current != NULL) {
    scope_t *temp = current;
    current = current->parent;
    symtable_free(temp->table);
    free(temp);
  }
}

int scope_define(symbol_t *sym) {
  if (current == NULL) {
    return ERR_INTERNAL;
  }
  if (symtable_search(current->table, sym->name) != NULL) {
    return ERR_SEM_REDEF;
  }
  if (!symtable_insert(current->table, sym->name, sym)) {
    return ERR_INTERNAL;
  }
  return ERR_OK;
}

symbol_t *scope_lookup(const char *name) {
  scope_t *scope = current;
  while (scope != NULL) {
    symbol_t *sym = symtable_search(scope->table, name);
    if (sym != NULL) {
      return sym;
    }
    scope = scope->parent;
  }
  return NULL;
}

void scope_free(void) {
  while (current != NULL) {
    scope_pop();
  }
  global = NULL;
}

scope_t *scope_enter_global(void) {
  scope_t *saved = current;
  current = global;
  return saved;
}

void scope_restore(scope_t *saved) { current = saved; }

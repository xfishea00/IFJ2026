/**
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    symtable.h
 * Authors: Jaroslav Baček (xbacekj00)
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H
#include <stdbool.h>
#include <stdlib.h>

// symbol type options
typedef enum {
  TYPE_INT,
  TYPE_DOUBLE,
  TYPE_STRING,
  TYPE_UNIT,
  TYPE_BOOLEAN
} symbol_type_t;

// symbol mode options
typedef enum { SYM_VARIABLE, SYM_VALUE, SYM_FUNCTION } symbol_mode_t;

// symbol structure
typedef struct {
  symbol_type_t type;
  symbol_mode_t mode;
  int parameter_count;
  symbol_type_t *parameter_types;
} symbol_info_t;

// symbol table structure
typedef struct symtable symtable_t;

// initialize symbol table
symtable_t *symtable_init(void);

// insert a symbol into the table
bool symtable_insert(symtable_t *table, const char *key, symbol_info_t info);

// search for a symbol in the table
symbol_info_t *symtable_search(symtable_t *table, const char *key);

// delete a symbol from the table
bool symtable_delete(symtable_t *table, const char *key);

// free the symbol table
void symtable_free(symtable_t *table);

#endif

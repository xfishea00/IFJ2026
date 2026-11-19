/**
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    symtable.h
 * Authors: Jaroslav Baček (xbacekj00)
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H
#include "symbol.h"
#include <stdbool.h>
#include <stdlib.h>

// symbol table structure
typedef struct symtable symtable_t;

// initialize symbol table
symtable_t *symtable_init(void);

// insert a symbol into the table
bool symtable_insert(symtable_t *table, const char *key, symbol_t *symbol);

// search for a symbol in the table
symbol_t *symtable_search(symtable_t *table, const char *key);

// delete a symbol from the table
bool symtable_delete(symtable_t *table, const char *key);

// free the symbol table
void symtable_free(symtable_t *table);

#endif

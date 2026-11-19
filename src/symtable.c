/**
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    symtable.c
 * Authors: Jaroslav Baček (xbacekj00)
 */

#include "symtable.h"
#include <stdint.h>
#include <string.h>
#define INITIAL_SIZE 17

// symtable state options
typedef enum { STATE_FREE, STATE_OCCUPIED, STATE_DELETED } symbol_state_t;

// symbol structure
typedef struct {
  char *key;
  symbol_t *symbol;
  symbol_state_t state;
} intern_symbol_t;

// symbol table structure
struct symtable {
  intern_symbol_t *symbols;
  size_t size;
  size_t max_len;
};

// djb2 hash function to hash strings and return an index in the symbol table
static size_t hash(const char *key, size_t max_len) {
  size_t h = 5381;
  unsigned int c;

  while ((c = *key++)) {
    h = ((h << 5) + h) + c;
  }

  return h % max_len;
}

// function to initialize symbol table
symtable_t *symtable_init(void) {
  symtable_t *symtable = malloc(sizeof(struct symtable));
  if (symtable == NULL) {
    return NULL;
  }

  symtable->size = 0;
  symtable->max_len = INITIAL_SIZE;

  symtable->symbols = malloc(sizeof(intern_symbol_t) * symtable->max_len);
  if (symtable->symbols == NULL) {
    free(symtable);
    return NULL;
  }

  for (size_t i = 0; i < symtable->max_len; i++) {
    symtable->symbols[i].key = NULL;
    symtable->symbols[i].symbol = NULL;
    symtable->symbols[i].state = STATE_FREE;
  }

  return symtable;
}

// function to rehash the symbol table
static bool symtable_rehash(symtable_t *table) {
  size_t new_len = table->max_len * 2 + 1;

  intern_symbol_t *new_symbols = malloc(sizeof(intern_symbol_t) * new_len);
  if (new_symbols == NULL) {
    return false;
  }

  for (size_t i = 0; i < new_len; i++) {
    new_symbols[i].key = NULL;
    new_symbols[i].symbol = NULL;
    new_symbols[i].state = STATE_FREE;
  }

  // rehash all STATE_OCCUPIED symbols into the new table
  for (size_t i = 0; i < table->max_len; i++) {
    if (table->symbols[i].state == STATE_OCCUPIED) {
      size_t idx = hash(table->symbols[i].key, new_len);
      while (new_symbols[idx].state == STATE_OCCUPIED) {
        idx = (idx + 1) % new_len;
      }
      new_symbols[idx] = table->symbols[i];
    }
  }

  free(table->symbols);
  table->symbols = new_symbols;
  table->max_len = new_len;
  return true;
}

// function to insert a symbol into the table
bool symtable_insert(symtable_t *table, const char *key, symbol_t *symbol) {
  symbol_t *existing_symbol = symtable_search(table, key);
  if (existing_symbol != NULL) {
    return false;
  }

  size_t max_optimal_size = (table->max_len * 2) / 3;

  if (table->size >= max_optimal_size) {
    if (symtable_rehash(table) == false) {
      return false;
    }
  }

  size_t idx = hash(key, table->max_len);
  intern_symbol_t *first_deleted = NULL;

  // look for a free slot or a deleted slot
  for (size_t i = 0; i < table->max_len; i++) {
    intern_symbol_t *position = &table->symbols[idx];

    if (position->state == STATE_FREE) {
      break;
    }
    if (position->state == STATE_DELETED) {
      if (first_deleted == NULL) {
        first_deleted = position;
      }
    } else if (strcmp(position->key, key) == 0) {
      return false;
    }
    idx = (idx + 1) % table->max_len;
  }

  // insert the new symbol at found position
  intern_symbol_t *insert_position;
  if (first_deleted != NULL) {
    insert_position = first_deleted;
  } else {
    insert_position = &table->symbols[idx];
  }

  // allocate memory for the key and copy it
  char *temp_key = malloc(strlen(key) + 1);
  if (temp_key == NULL) {
    return false;
  }
  strcpy(temp_key, key);

  insert_position->key = temp_key;
  insert_position->symbol = symbol;
  insert_position->state = STATE_OCCUPIED;
  table->size = table->size + 1;

  return true;
}

// function to search for a symbol in the table
symbol_t *symtable_search(symtable_t *table, const char *key) {
  size_t idx = hash(key, table->max_len);

  for (size_t i = 0; i < table->max_len; i++) {
    intern_symbol_t *position = &table->symbols[idx];

    if (position->state == STATE_FREE) {
      return NULL;
    }
    if (position->state == STATE_OCCUPIED && strcmp(position->key, key) == 0) {
      return position->symbol;
    }
    idx = (idx + 1) % table->max_len;
  }
  return NULL;
}

// function to delete a symbol from the table
bool symtable_delete(symtable_t *table, const char *key) {
  size_t idx = hash(key, table->max_len);

  for (size_t i = 0; i < table->max_len; i++) {
    intern_symbol_t *position = &table->symbols[idx];

    if (position->state == STATE_FREE) {
      return false;
    }
    if (position->state == STATE_OCCUPIED && strcmp(position->key, key) == 0) {
      free(position->key);
      position->key = NULL;
      position->symbol = NULL;
      position->state = STATE_DELETED;
      table->size--;
      return true;
    }
    idx = (idx + 1) % table->max_len;
  }
  return false;
}

// function to free the symbol table
void symtable_free(symtable_t *table) {
  if (table == NULL) {
    return;
  }

  for (size_t i = 0; i < table->max_len; i++) {
    if (table->symbols[i].state == STATE_OCCUPIED) {
      free(table->symbols[i].key);
    }
  }
  free(table->symbols);
  free(table);
}

/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    type.c
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   data types of the language
 */

#include "type.h"

#include <stdlib.h>

// one static instance for each base type, never freed
static type_t int_type = {.kind = T_INT};
static type_t double_type = {.kind = T_DOUBLE};
static type_t string_type = {.kind = T_STRING};
static type_t bool_type = {.kind = T_BOOL};
static type_t unit_type = {.kind = T_UNIT};

#define TYPES_INIT_CAPACITY 16

// all types created by type_named, released only at the end
static type_t **types;
static int type_count;
static int type_capacity;

type_t *type_int(void) { return &int_type; }
type_t *type_double(void) { return &double_type; }
type_t *type_string(void) { return &string_type; }
type_t *type_bool(void) { return &bool_type; }
type_t *type_unit(void) { return &unit_type; }

type_t *type_from_token(token_type_t token) {
  switch (token) {
  case TOK_KW_INT:
    return type_int();
  case TOK_KW_DOUBLE:
    return type_double();
  case TOK_KW_STRING:
    return type_string();
  case TOK_KW_BOOLEAN:
    return type_bool();
  case TOK_KW_UNIT:
    return type_unit();
  default:
    return NULL; // list, union and class names are not base types
  }
}

bool type_equal(type_t *a, type_t *b) {
  if (a == NULL || b == NULL) {
    return false;
  }
  return a->kind == b->kind;
}

bool type_compatible(type_t *expected, type_t *actual) {
  if (type_equal(expected, actual)) {
    return true;
  }
  if (expected != NULL && actual != NULL && expected->kind == T_DOUBLE &&
      actual->kind == T_INT) {
    return true; // implicit conversion from Int to Double
  }
  return false;
}

type_t *type_named(char *name) {
  if (type_count >= type_capacity) {
    int new_capacity;
    if (type_capacity == 0) {
      new_capacity = TYPES_INIT_CAPACITY;
    } else {
      new_capacity = type_capacity * 2;
    }
    type_t **tmp = realloc(types, new_capacity * sizeof(type_t *));
    if (tmp == NULL) {
      return NULL;
    }
    types = tmp;
    type_capacity = new_capacity;
  }

  type_t *new_type = malloc(sizeof(type_t));
  if (new_type == NULL) {
    return NULL;
  }
  new_type->kind = T_NAMED;
  new_type->name = name;
  types[type_count++] = new_type;
  return new_type;
}

void types_free_all(void) {
  for (int i = 0; i < type_count; i++) {
    free(types[i]->name);
    free(types[i]);
  }
  free(types);
  types = NULL;
  type_count = 0;
  type_capacity = 0;
}

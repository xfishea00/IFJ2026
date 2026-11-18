/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    type.c
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   data types of the language
 */

#include "type.h"

#include <stdlib.h>

// one static instance for each base type, never freed
static type_t int_type = {T_INT};
static type_t double_type = {T_DOUBLE};
static type_t string_type = {T_STRING};
static type_t bool_type = {T_BOOL};
static type_t unit_type = {T_UNIT};

type_t *type_int(void) { return &int_type; }
type_t *type_double(void) { return &double_type; }
type_t *type_string(void) { return &string_type; }
type_t *type_bool(void) { return &bool_type; }
type_t *type_unit(void) { return &unit_type; }

bool type_equal(type_t *a, type_t *b) {
  if (a == NULL || b == NULL) {
    return false;
  }
  return a->kind == b->kind;
}

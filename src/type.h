/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    type.h
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   data types of the language
 */

#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

// type kinds
typedef enum {
  T_INT,
  T_DOUBLE,
  T_STRING,
  T_BOOL,
  T_UNIT,
  T_LIST,    // List[T]
  T_UNION,   // A | B
  T_NAMED,   // type alias or class
  T_ANY,     // parameter of print and toStr
  T_RANGE,   // 1 to 10
  T_UNKNOWN, // not yet known type
} type_kind_t;

// same name as the forward declaration in ast.h
typedef struct type type_t;

struct type {
  type_kind_t kind;
  // TODO: fields for List, union and named types
};

// base types, each returns the one static instance
type_t *type_int(void);
type_t *type_double(void);
type_t *type_string(void);
type_t *type_bool(void);
type_t *type_unit(void);

// true if both types are the same
bool type_equal(type_t *a, type_t *b);

#endif // TYPE_H

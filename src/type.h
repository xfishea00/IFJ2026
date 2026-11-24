/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    type.h
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   data types of the language
 */

#ifndef TYPE_H
#define TYPE_H

#include "token.h"

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
  char *name; // class or alias name
  type_kind_t list_type;
  // TODO: fields for List, union types
};

// base types, each returns the one static instance
type_t *type_int(void);
type_t *type_double(void);
type_t *type_string(void);
type_t *type_bool(void);
type_t *type_unit(void);
// base type for a type keyword token, NULL for other tokens
type_t *type_from_token(token_type_t token);
// new T_NAMED type, name is taken over
type_t *type_named(char *name);
// new T_LIST type with the element kind from element_type
type_t *type_list(type_t *element_type);
// releases all dynamically allocated types
void types_free_all(void);

// true if both types are the same
bool type_equal(type_t *a, type_t *b);

// true if a value of type actual can be used where expected is required
bool type_compatible(type_t *expected, type_t *actual);

#endif // TYPE_H

/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    error.h
 * Authors: Michal Řezníček (xreznim00)
 */

#ifndef ERROR_H
#define ERROR_H

// exit codes for the compiler
typedef enum {
  ERR_OK = 0,            // success
  ERR_LEXICAL = 1,       // lexical error
  ERR_SYNTAX = 2,        // syntax error
  ERR_SEM_UNDEF = 3,     // undefined function, variable or type; missing @main
  ERR_SEM_REDEF = 4,     // redefinition of identifier
  ERR_SEM_ARGS = 5,      // wrong number or type of arguments; @main params
  ERR_SEM_TYPE = 6,      // type mismatch
  ERR_SEM_IMMUTABLE = 7, // assignment to immutable value
  ERR_SEM_OTHER = 10,    // other semantic error
  ERR_INTERNAL = 99      // internal error (memory allocation, ...)
} err_code_t;

#endif // ERROR_H
/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    expression_parser.h
 * Authors: Maros Magoč (xmagocm00)
 * Brief:   Expression syntax analysis interface
 */

#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "token.h"

typedef enum {
    PRE_ID,     // id

    PRE_PLUS,   // +
    PRE_MINUS,  // -
    PRE_CONCAT, // ++
    PRE_MUL,    // *
    PRE_DIV,    // /

    PRE_LT,     // <
    PRE_GT,     // >
    PRE_LE,     // <=
    PRE_GE,     // >=
    PRE_EQ,     // ==
    PRE_NEQ,    // !=

    PRE_NOT,    // !
    PRE_UMINUS, // unary -

    PRE_AND,    // &&
    PRE_OR,     // ||
    
    PRE_DOT,    // .
    PRE_LPAR,   // (
    PRE_RPAR,   // )

    PRE_END,     // $
    PRE_INVALID = -1
} precedence_terminal_t;

typedef enum {
    STACK_TERMINAL,
    STACK_MARKER,
    STACK_EXPRESSION
} stack_element_type_t;

typedef enum {
    PRE_LOWER =  '<',
    PRE_HIGHER = '>',
    PRE_EQUAL =  '=',
    PRE_ERROR =  ' ',
} precedence_t;

typedef struct {
    stack_element_type_t type;
    precedence_terminal_t terminal;
} stack_element_t;

typedef struct {
    int n;
    stack_element_t left_side;
    const char *right_side;
    precedence_terminal_t operator;
} rule_t;

typedef int (*advance_token_f)(token_t *);

#define L PRE_LOWER
#define H PRE_HIGHER
#define E PRE_EQUAL
#define X PRE_ERROR

#define TERMINATOR_NUM 20 // number of rows and columns in the table
#define RULE_NUM 18       // number of rules

precedence_terminal_t map_token_to_precedence(token_t);                                // maps tokens to precedence terminals
bool is_boundary(token_type_t t);                                                      // returns true for tokens that are not a part of an expression
int parse_non_id_expression(token_t *current_token, advance_token_f advance_token);    // parses expression

#endif
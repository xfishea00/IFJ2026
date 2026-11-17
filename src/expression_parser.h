/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    expression_parser.h
 * Authors: Maros Magoč (xmagocm00)
 * Brief:   Expression syntax analysis interface
 */

#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "token.h"
#include "stack.h"

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

    PRE_AND,    // &&
    PRE_OR,     // ||
    
    PRE_DOT,    // .
    PRE_LPAR,   // (
    PRE_RPAR,   // )

    PRE_END,     // $
    PRE_INVALID = -1
} precedence_terminal_t;

typedef enum {
    PRE_LOWER =  '<',
    PRE_HIGHER = '>',
    PRE_EQUAL =  '=',
    PRE_ERROR =  ' ',
} precedence_t;

typedef struct {
    int n;
    stack_element_t left_side;
    const char *right_side;
    precedence_terminal_t operator;
} rule_t;

#define L PRE_LOWER
#define H PRE_HIGHER
#define E PRE_EQUAL
#define X PRE_ERROR

#define TERMINATOR_NUM 19 // number of rows and columns in the table
#define RULE_NUM 17       // number of rules

precedence_terminal_t map_token_to_precedence(token_t);

#endif
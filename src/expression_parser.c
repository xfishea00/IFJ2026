/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    expression_parser.c
 * Authors: Maros Magoč (xmagocm00)
 * Brief:   Expression syntax analysis
 */

#include "expression_parser.h"
#include "token.h"

precedence_terminal_t map_token_to_precedence(token_t t) {
    switch (t.type) {
        case TOK_ID:         return PRE_ID;
        case TOK_INT_LIT:    return PRE_ID;
        case TOK_DOUBLE_LIT: return PRE_ID;
        case TOK_STRING_LIT: return PRE_ID;
        case TOK_KW_TRUE:    return PRE_ID;
        case TOK_KW_FALSE:   return PRE_ID;
        case TOK_PLUS:       return PRE_PLUS;
        case TOK_MINUS:      return PRE_MINUS;
        case TOK_CONCAT:     return PRE_CONCAT;
        case TOK_MUL:        return PRE_MUL;
        case TOK_DIV:        return PRE_DIV;
        case TOK_LT:         return PRE_LT;
        case TOK_GT:         return PRE_GT;
        case TOK_LE:         return PRE_LE;
        case TOK_GE:         return PRE_GE;
        case TOK_EQ:         return PRE_EQ;
        case TOK_NEQ:        return PRE_NEQ;
        case TOK_NOT:        return PRE_NOT;
        case TOK_AND:        return PRE_AND;
        case TOK_OR:         return PRE_OR;
        case TOK_DOT:        return PRE_DOT;
        case TOK_LPAR:       return PRE_LPAR;
        case TOK_RPAR:       return PRE_RPAR;
        case TOK_NEWLINE:    return PRE_END;
        case TOK_EOF:        return PRE_END;
        default:             return PRE_INVALID;
    }
}


/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    semantic.h
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   semantic analysis
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

// checks program, fills in type and symbol fields, returns ERR_OK or error code
int semantic_analyze(ast_node_t *root);

#endif // SEMANTIC_H

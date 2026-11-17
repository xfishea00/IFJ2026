/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    generator.h
 * Authors: Jaroslav Baček (xbacekj00)
 * Brief:   interface of the code generator
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"

/**
 * @brief Generates code from the AST tree.
 *
 * @return int 0 on success, 99 on internal error, else error code from the
 * semantic analysis
 */
int generate_code(/* AST tree */);

#endif // GENERATOR_H
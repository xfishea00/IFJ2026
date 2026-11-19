/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    generator.h
 * Authors: Jaroslav Baček (xbacekj00), Maros Magoč (xmagocm00)
 * Brief:   interface of the code generator
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"

#include <stdio.h>

/*
 * ----------------------------------------------------------------
 * GENERATOR STATE
 * ----------------------------------------------------------------
 */

/**
 * @brief State of the code generator.
 */
typedef struct {
  FILE *output;
  unsigned int label_counter;
  unsigned int temp_counter;
  bool error;
} generator_state_t;

/**
 * @brief Generates code from the AST tree.
 *
 * @return int 0 on success, 99 on internal error, else error code from the
 * semantic analysis
 */
int generate_code(ast_node_t *root);

#endif // GENERATOR_H
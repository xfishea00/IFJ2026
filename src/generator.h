/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    generator.h
 * Authors: Jaroslav Baček (xbacekj00), Maros Magoč (xmagocm00)
 * Brief:   interface of the code generator
 */

#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"

#include <stddef.h>
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
  bool buffering_output;
  char *output_buffer;
  size_t output_buffer_length;
  size_t output_buffer_capacity;
} generator_state_t;

/**
 * @brief Generates code from the AST tree.
 *
 * @return int ERR_OK on success or ERR_INTERNAL on code generation failure.
 */
int generate_code(ast_node_t *root);

#endif // GENERATOR_H
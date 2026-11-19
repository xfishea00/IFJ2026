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

static void generate_error(generator_state_t *state, const char *error_message, ...);
static void generate_function(generator_state_t *state, ast_node_t *node);
static void generate_function_parameters(generator_state_t *state, ast_node_t *node);
static void generate_block(generator_state_t *state, ast_node_t *node);
static void generate_variable_declaration(generator_state_t *state, ast_node_t *node);
static void generate_assignment(generator_state_t *state, ast_node_t *node);
static void generate_return(generator_state_t *state, ast_node_t *node);
static void generate_function_return(generator_state_t *state, ast_node_t *node);
static void generate_literal(generator_state_t *state, ast_node_t *node);
static void generate_variable(generator_state_t *state, ast_node_t *node);
static void generate_unary_expression(generator_state_t *state, ast_node_t *node);
static void generate_binary_expression(generator_state_t *state, ast_node_t *node);
static void generate_function_call(generator_state_t *state, ast_node_t *node);
static void generate_if(generator_state_t *state, ast_node_t *node);
static void generate_while(generator_state_t *state, ast_node_t *node);
static void generate_string(generator_state_t *state, ast_node_t *node);
static void generate_builtin_call(generator_state_t *state, ast_node_t *node);
static void generate_print(generator_state_t *state, ast_node_t *node);
static void generate_builtin_read(generator_state_t *state, const char *type);
static void generate_toStr(generator_state_t *state);
static void generate_toInt(generator_state_t *state);
static void generate_strLength(generator_state_t *state);
static void generate_subStr(generator_state_t *state);
static void generate_ord(generator_state_t *state);
static void generate_chr(generator_state_t *state);
static void generate_for(generator_state_t *state, ast_node_t *node);
static void generate_match(generator_state_t *state, ast_node_t *node);
static void generate_readBoolean(generator_state_t *state);
static void generate_readIntMaybe(generator_state_t *state);
static void generate_readDoubleMaybe(generator_state_t *state);

/**
 * @brief Generates code from the AST tree.
 *
 * @return int 0 on success, 99 on internal error, else error code from the
 * semantic analysis
 */
int generate_code(ast_node_t *root);

#endif // GENERATOR_H
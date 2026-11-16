/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    generator.c
 * Authors: Jaroslav Baček (xbacekj00)
 * Brief:   implementation of the code generator
 */

#include "generator.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/*
 * ----------------------------------------------------------------
 * HELPER FUNCTIONS
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates a formatted string to the output.
 * @param state The generator state.
 * @param format The format string.
 * @param ... The arguments to format.
 */
static void generate_output(generator_state_t *state, const char *format, ...) {
  if (state == NULL) {
    return;
  }
  va_list args;
  va_start(args, format);
  int result = vfprintf(state->output, format, args);
  va_end(args);

  if (result < 0 || fputc('\n', state->output) == EOF) {
    state->error = true;
  }
}

/**
 * @brief Generates a label to the output.
 * @param state The generator state.
 * @param label The label to generate.
 */
static void generate_label(generator_state_t *state, const char *label) {
  if (state == NULL) {
    return;
  }
  generate_output(state, "LABEL %s", label);
}

/**
 * @brief Generates a unique temporary variable name.
 * @param state The generator state.
 * @param buffer The buffer to store the generated temporary variable name.
 * @param buffer_size The size of the buffer.
 */
static void generate_temp_name(generator_state_t *state, char *buffer,
                               size_t buffer_size) {
  if (state == NULL || buffer == NULL || buffer_size == 0) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  int n = snprintf(buffer, buffer_size, "$temp@%u", state->temp_counter++);
  if (n < 0 || (size_t)n >= buffer_size) {
    generate_error(state, "Temporary variable name buffer overflow");
  }
}

/**
 * @brief Generates a unique label name.
 * @param state The generator state.
 * @param buffer The buffer to store the generated label name.
 * @param buffer_size The size of the buffer.
 */
static void generate_label_name(generator_state_t *state, char *buffer,
                                size_t buffer_size) {
  if (state == NULL || buffer == NULL || buffer_size == 0) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  int n = snprintf(buffer, buffer_size, "$label_%u", state->label_counter++);
  if (n < 0 || (size_t)n >= buffer_size) {
    generate_error(state, "Label name buffer overflow");
  }
}

/**
 * @brief Generates an error message and sets the error flag in the generator
 * state.
 * @param state The generator state.
 * @param error_message The error message to generate.
 */
static void generate_error(generator_state_t *state,
                           const char *error_message) {
  if (state == NULL) {
    return;
  }
  fprintf(stderr, "Code generation error: %s\n", error_message);
  state->error = true;
}

/*
 * ----------------------------------------------------------------
 * MAIN CODE HANDLERS
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates the program code from the AST.
 * @param state The generator state.
 * @param root The root of the AST.
 */
static void generate_program(generator_state_t *state, ast_node_t *root) {
  if (state == NULL) {
    return;
  }
  if (root == NULL || root->kind != AST_PROGRAM) {
    state->error = true;
    return;
  }
  generate_output(state, ".IFJcode26\n");

  ast_list_t *defs = &root->program.defs;

  for (int i = 0; i < defs->count; i++) {
    ast_node_t *def = defs->items[i];

    if (def == NULL || def->kind != AST_FUNC_DEF) {
      state->error = true;
      return;
    }
    generate_function(state, def);

    if (state->error) {
      return;
    }
  }
}

/**
 * @brief Generates a statement definition from the AST.
 * @param state The generator state.
 * @param node The AST node representing the statement definition.
 */
static void generate_statement(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  switch (node->kind) {
    case AST_BLOCK:
        generate_block(state, node);
        break;
    case AST_VAR_DEF:
        generate_variable_declaration(state, node);
        break;
    case AST_ASSIGN:
        generate_assignment(state, node);
        break;
    case AST_IF:
        generate_if(state, node);
        break;
    case AST_WHILE:
        generate_while(state, node);
        break;
    case AST_RETURN:
        generate_return(state, node);
        break;
    case AST_FOR:
        generate_for(state, node);
        break;
    case AST_MATCH:
        generate_match(state, node);
        break;
    default:
        state->error = true;
        break;
  }
}

/**
 * @brief Generates an expression from the AST.
 * @param state The generator state.
 * @param node The AST node representing the expression.
 */
static void generate_expression(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  switch (node->kind){
    case AST_LITERAL:
        generate_literal(state, node);
        break;
    case AST_ID:
        generate_variable(state, node);
        break;
    case AST_BINARY:
        generate_binary_expression(state, node);
        break;
    case AST_UNARY:
        generate_unary_expression(state, node);
        break;
    case AST_CALL:
        generate_function_call(state, node);
        break;
    default:
        state->error = true;
        break;
  }
}

/*
 * ----------------------------------------------------------------
 * PROGRAM STRUCTURE
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates a function definition from the AST.
 * @param state The generator state.
 * @param node The AST node representing the function definition.
 */
static void generate_function(generator_state_t *state, ast_node_t *node) {
    if(state == NULL || node == NULL || node->kind != AST_FUNC_DEF) {
        if(state != NULL) {
            state->error = true;
        }
        return;
    }
    
}

/**
 * @brief Generates the parameters of a function from the AST.
 * @param state The generator state.
 * @param node The AST node representing the function parameters.
 */
static void generate_function_parameters(generator_state_t *state,
                                         ast_node_t *node) {}

/*
 * ----------------------------------------------------------------
 * STATEMENTS
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates a block of statements from the AST.
 * @param state The generator state.
 * @param node The AST node representing the block of statements.
 */
static void generate_block(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates variable declaration from the AST.
 * @param state The generator state.
 * @param node The AST node representing the declaration statement.
 */
static void generate_variable_declaration(generator_state_t *state,
                                          ast_node_t *node) {}

/**
 * @brief Generates an assignment statement from the AST.
 * @param state The generator state.
 * @param node The AST node representing the assignment statement.
 */
static void generate_assignment(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a return statement from the AST.
 * @param state The generator state.
 * @param node The AST node representing the return statement.
 */
static void generate_return(generator_state_t *state, ast_node_t *node) {}

/*
 * ----------------------------------------------------------------
 * EXPRESSIONS
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates a literal value from the AST.
 * @param state The generator state.
 * @param node The AST node representing the literal value.
 */
static void generate_literal(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a variable reference from the AST.
 * @param state The generator state.
 * @param node The AST node representing the variable reference.
 */
static void generate_variable(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a unary expression from the AST.
 * @param state The generator state.
 * @param node The AST node representing the unary expression.
 */
static void generate_unary_expression(generator_state_t *state,
                                      ast_node_t *node) {}

/**
 * @brief Generates a binary expression from the AST.
 * @param state The generator state.
 * @param node The AST node representing the binary expression.
 */
static void generate_binary_expression(generator_state_t *state,
                                       ast_node_t *node) {}

/**
 * @brief Generates a function call from the AST.
 * @param state The generator state.
 * @param node The AST node representing the function call.
 */
static void generate_function_call(generator_state_t *state, ast_node_t *node) {
}

/**
 * @brief Generates a boolean expression from the AST.
 * @param state The generator state.
 * @param node The AST node representing the boolean expression.
 */
static void generate_boolean_expression(generator_state_t *state,
                                        ast_node_t *node) {}

/**
 * @brief Generates an if expression from the AST.
 * @param state The generator state.
 * @param node The AST node representing the if expression.
 */
static void generate_if_expression(generator_state_t *state, ast_node_t *node) {
}

/*
 * ----------------------------------------------------------------
 * FLOW CONTROL
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates an if statement from the AST.
 * @param state The generator state.
 * @param node The AST node representing the if statement.
 */
static void generate_if(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a while loop from the AST.
 * @param state The generator state.
 * @param node The AST node representing the while loop.
 */
static void generate_while(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a for loop from the AST.
 * @param state The generator state.
 * @param node The AST node representing the for loop.
 */
static void generate_for(generator_state_t *state, ast_node_t *node) {}

/*
 * ----------------------------------------------------------------
 * LISTS
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates a list from the AST.
 * @param state The generator state.
 * @param node The AST node representing the list.
 */
static void generate_list(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a list access from the AST.
 * @param state The generator state.
 * @param node The AST node representing the list access.
 */
static void generate_list_access(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a list operation from the AST.
 * @param state The generator state.
 * @param node The AST node representing the list operation.
 */
static void generate_list_operation(generator_state_t *state,
                                    ast_node_t *node) {}

/*
 * ----------------------------------------------------------------
 * DATA CLASSES
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates a data class definition from the AST.
 * @param state The generator state.
 * @param node The AST node representing the data class definition.
 */
static void generate_data_class_definition(generator_state_t *state,
                                           ast_node_t *node) {}

/**
 * @brief Generates a data class constructor from the AST.
 * @param state The generator state.
 * @param node The AST node representing the data class constructor.
 */
static void generate_data_class_constructor(generator_state_t *state,
                                            ast_node_t *node) {}

/**
 * @brief Generates a data class member access from the AST.
 * @param state The generator state.
 * @param node The AST node representing the data class member access.
 */
static void generate_member_access(generator_state_t *state, ast_node_t *node) {
}

/**
 * @brief Generates a data class member call from the AST.
 * @param state The generator state.
 * @param node The AST node representing the data class member call.
 */
static void generate_member_call(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a union type definition from the AST.
 * @param state The generator state.
 * @param node The AST node representing the union type definition.
 */
static void generate_union_type(generator_state_t *state, ast_node_t *node) {}

/*
 * ----------------------------------------------------------------
 * MATCHING
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates a match statement from the AST.
 * @param state The generator state.
 * @param node The AST node representing the match statement.
 */
static void generate_match(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a match case from the AST.
 * @param state The generator state.
 * @param node The AST node representing the match case.
 */
static void generate_match_case(generator_state_t *state, ast_node_t *node) {}

/**
 * @brief Generates a pattern from the AST.
 * @param state The generator state.
 * @param node The AST node representing the pattern.
 */
static void generate_pattern(generator_state_t *state, ast_node_t *node) {}

/*
 * ----------------------------------------------------------------
 * BUIL-IN FUNCTIONS
 * ----------------------------------------------------------------
 */

static void generate_builtin_call(generator_state_t *state, ast_node_t *node) {}

static void generate_print(generator_state_t *state, ast_node_t *node) {}
static void generate_read_line(generator_state_t *state, ast_node_t *node) {}
static void generate_read_int(generator_state_t *state, ast_node_t *node) {}
static void generate_read_double(generator_state_t *state, ast_node_t *node) {}
static void generate_to_string(generator_state_t *state, ast_node_t *node) {}
static void generate_to_int(generator_state_t *state, ast_node_t *node) {}
static void generate_string_length(generator_state_t *state, ast_node_t *node) {
}
static void generate_string_substring(generator_state_t *state,
                                      ast_node_t *node) {}
static void generate_string_ord(generator_state_t *state, ast_node_t *node) {}
static void generate_string_chr(generator_state_t *state, ast_node_t *node) {}

/*
 * ----------------------------------------------------------------
 * MAIN START
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates code from the AST and writes it to the output file.
 * @param root The root of the AST.
 * @return true if code generation was successful, false otherwise.
 */
int generate_code(ast_node_t *root) {
  if (root == NULL) {
    return false;
  }
  generator_state_t state;
  state.output = stdout;
  state.temp_counter = 0;
  state.label_counter = 0;
  state.error = false;

  generate_program(&state, root);

  return !state.error;
}
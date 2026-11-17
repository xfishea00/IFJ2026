/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    generator.c
 * Authors: Jaroslav Baček (xbacekj00)
 * Brief:   implementation of the code generator
 */

#include "generator.h"

#include <inttypes.h>
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

  ast_list_t *defs = &root->program.defs;
  ast_node_t *main_func = NULL;

  // Find the main function - only one is allowed
  for (int i = 0; i < defs->count; i++) {
    ast_node_t *def = defs->items[i];

    if (def == NULL || def->kind != AST_FUNC_DEF) {
      state->error = true;
      return;
    }
    if (def->func_def.is_main) {
      if (main_func != NULL) {
        generate_error(state, "Multiple main functions defined");
        state->error = true;
        return;
      }
      main_func = def;
    }
  }
  // Check if the main is defined
  if (main_func == NULL || main_func->func_def.symbol == NULL ||
      main_func->func_def.symbol->unique_name == NULL) {
    generate_error(state, "Main function not defined");
    state->error = true;
    return;
  }
  // Generate the IFJcode26 header
  generate_output(state, ".IFJcode26\n");

  // Generate the main function call
  generate_output(state, "CREATEFRAME\n");
  generate_output(state, "CALL $%s\n", main_func->func_def.symbol->unique_name);
  generate_output(state, "EXIT int@0\n\n");

  if (state->error) {
    return;
  }
  // Generate code for each function definition
  for (int i = 0; i < defs->count; i++) {
    ast_node_t *def = defs->items[i];
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
  switch (node->kind) {
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
  if (state == NULL || node == NULL || node->kind != AST_FUNC_DEF) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  symbol_t *symbol = node->func_def.symbol;
  if (symbol == NULL || symbol->unique_name == NULL) {
    state->error = true;
    return;
  }

  // Generate the function label
  char function_label[256];
  int n = snprintf(function_label, sizeof(function_label), "function@%s",
                   symbol->unique_name);
  if (n < 0 || (size_t)n >= sizeof(function_label)) {
    generate_error(state, "Function label buffer overflow");
    state->error = true;
    return;
  }
  generate_label(state, function_label);
  if (state->error) {
    return;
  }
  generate_output(state, "PUSHFRAME\n");
  if (state->error) {
    return;
  }
  generate_function_parameters(state, node);
  if (state->error) {
    return;
  }
  generate_block(state, node->func_def.body);
  if (state->error) {
    return;
  }
  generate_function_return(state, node);
  if (state->error) {
    return;
  }
}

/**
 * @brief Generates the parameters of a function from the AST.
 * @param state The generator state.
 * @param node The AST node representing the function parameters.
 */
static void generate_function_parameters(generator_state_t *state,
                                         ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_FUNC_DEF) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  // declare all parameters
  for (int i = 0; i < node->func_def.param_list_count; i++) {
    ast_list_t *params = &node->func_def.param_lists[i];

    for (int j = 0; j < params->count; j++) {
      ast_node_t *param = params->items[j];

      if (param == NULL || param->kind != AST_VAR_DEF ||
          param->param.symbol == NULL ||
          param->param.symbol->unique_name == NULL) {
        state->error = true;
        return;
      }
      generate_output(state, "DEFVAR LF@%s\n",
                      param->param.symbol->unique_name);
      if (state->error) {
        return;
      }
    }
  }
  // pop all parameters from the stack in reverse order
  for (int i = node->func_def.param_list_count - 1; i >= 0; i--) {
    ast_list_t *params = &node->func_def.param_lists[i];

    for (int j = params->count - 1; j >= 0; j--) {
      ast_node_t *param = params->items[j];

      generate_output(state, "POPS LF@%s\n", param->param.symbol->unique_name);
      if (state->error) {
        return;
      }
    }
  }
}

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
static void generate_block(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_BLOCK) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  ast_list_t *stmts = &node->block.stmts;

  // Generate code for each statement in the block
  for (int i = 0; i < stmts->count; i++) {
    ast_node_t *stmt = stmts->items[i];

    if (stmt == NULL) {
      state->error = true;
      return;
    }
    generate_statement(state, stmt);

    if (state->error) {
      return;
    }
  }
}

/**
 * @brief Generates variable declaration from the AST.
 * @param state The generator state.
 * @param node The AST node representing the declaration statement.
 */
static void generate_variable_declaration(generator_state_t *state,
                                          ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_VAR_DEF) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  if (node->var_def.symbol == NULL ||
      node->var_def.symbol->unique_name == NULL) {
    state->error = true;
    return;
  }

  if (node->var_def.init == NULL) {
    return;
  }

  // Generate the initialization expression
  generate_expression(state, node->var_def.init);

  if (state->error) {
    return;
  }

  // Generate the variable declaration
  generate_output(state, "POPS LF@%s\n", node->var_def.symbol->unique_name);
}

/**
 * @brief Generates an assignment statement from the AST.
 * @param state The generator state.
 * @param node The AST node representing the assignment statement.
 */
static void generate_assignment(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_ASSIGN) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  if (node->assign.symbol == NULL || node->assign.symbol->unique_name == NULL ||
      node->assign.value == NULL) {
    state->error = true;
    return;
  }

  // Generate the assignment value expression
  generate_expression(state, node->assign.value);

  if (state->error) {
    return;
  }

  // Generate the assignment statement
  generate_output(state, "POPS LF@%s\n", node->assign.symbol->unique_name);
}

/**
 * @brief Generates a return statement from the AST.
 * @param state The generator state.
 * @param node The AST node representing the return statement.
 */
static void generate_return(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_RETURN) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  // Generate the return value expression if it exists, otherwise push nil
  if (node->return_stmt.value != NULL) {
    generate_expression(state, node->return_stmt.value);
  } else {
    generate_output(state, "PUSHS nil@nil\n");
  }
  if (state->error) {
    return;
  }
  // Pop the frame and return
  generate_output(state, "POPFRAME\n");
  generate_output(state, "RETURN\n");
}

/**
 * @brief Generates the return value of a function from the AST.
 * @param state The generator state.
 * @param node The AST node representing the return value.
 */
static void generate_function_return(generator_state_t *state,
                                     ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_FUNC_DEF) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  generate_output(state, "PUSHS nil@nil\n");
  generate_output(state, "POPFRAME\n");
  generate_output(state, "RETURN\n");
}

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
static void generate_literal(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_LITERAL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  switch (node->literal.kind) {
  case LIT_INT:
    generate_output(state, "PUSHS int@%" PRId64 "\n", node->literal.int_val);
    break;

  case LIT_DOUBLE:
    generate_output(state, "PUSHS float@%a\n", node->literal.double_val);
    break;

  case LIT_STRING:
    generate_string(state, node);
    break;

  case LIT_BOOL:
    if (node->literal.bool_val) {
      generate_output(state, "PUSHS bool@true\n");
    } else {
      generate_output(state, "PUSHS bool@false\n");
    }
    break;

  case LIT_UNIT:
    generate_output(state, "PUSHS nil@nil\n");
    break;

  default:
    state->error = true;
    break;
  }
}

/**
 * @brief Generates a variable reference from the AST.
 * @param state The generator state.
 * @param node The AST node representing the variable reference.
 */
static void generate_variable(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_ID) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  if (node->id.symbol == NULL || node->id.symbol->unique_name == NULL) {
    state->error = true;
    return;
  }

  // Generate the variable reference
  generate_output(state, "PUSHS LF@%s\n", node->id.symbol->unique_name);
}

/**
 * @brief Generates a unary expression from the AST.
 * @param state The generator state.
 * @param node The AST node representing the unary expression.
 */
static void generate_unary_expression(generator_state_t *state,
                                      ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_UNARY ||
      node->unary.operand == NULL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  generate_expression(state, node->unary.operand);

  if (state->error) {
    return;
  }

  switch (node->unary.op) {
  case OP_NOT:
    generate_output(state, "NOTS\n");
    break;
  case OP_NEG:
    // TODO: Implement negation
    break;
  default:
    state->error = true;
    break;
  }
}

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
static void generate_if(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_IF) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  if (node->if_stmt.cond == NULL || node->if_stmt.then_branch == NULL) {
    state->error = true;
    return;
  }

  char else_label[256];
  char end_label[256];

  // Generate unique labels for the else and end of the if statement
  generate_label_name(state, else_label, sizeof(else_label));
  generate_label_name(state, end_label, sizeof(end_label));
  if (state->error) {
    return;
  }

  generate_expression(state, node->if_stmt.cond);
  if (state->error) {
    return;
  }

  // Generate the conditional jump to the else label if the condition is false
  generate_output(state, "PUSHS bool@false\n");
  generate_output(state, "JUMPIFNEQS %s\n", else_label);

  generate_block(state, node->if_stmt.then_branch);
  if (state->error) {
    return;
  }

  generate_output(state, "JUMP %s\n", end_label);

  generate_label(state, else_label);
  if (node->if_stmt.else_branch != NULL) {
    generate_block(state, node->if_stmt.else_branch);
    if (state->error) {
      return;
    }
  }
  generate_label(state, end_label);
}

/**
 * @brief Generates a while loop from the AST.
 * @param state The generator state.
 * @param node The AST node representing the while loop.
 */
static void generate_while(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_WHILE) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  if (node->while_stmt.cond == NULL || node->while_stmt.body == NULL) {
    state->error = true;
    return;
  }

  char start_label[64];
  char end_label[64];

  generate_label_name(state, start_label, sizeof(start_label));
  generate_label_name(state, end_label, sizeof(end_label));
  if (state->error) {
    return;
  }

  generate_label(state, start_label);

  generate_expression(state, node->while_stmt.cond);
  if (state->error) {
    return;
  }

  generate_output(state, "PUSHS bool@false\n");
  generate_output(state, "JUMPIFNEQS %s\n", end_label);

  generate_block(state, node->while_stmt.body);
  if (state->error) {
    return;
  }

  generate_output(state, "JUMP %s\n", start_label);

  generate_label(state, end_label);
}

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
 * ADDITIONAL FUNCTIONS, BUILTINS
 * ----------------------------------------------------------------
 */

/**
 * @brief Generates a string literal from the AST.
 * @param state The generator state.
 * @param node The AST node representing the string literal.
 */
static void generate_string(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_LITERAL ||
      node->literal.kind != LIT_STRING || node->literal.string_val == NULL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  generate_output(state, "PUSHS string@");
  if (state->error) {
    return;
  }

  int len = strlen(node->literal.string_val);
  for (int i = 0; i < len; i++) {
    char c = node->literal.string_val[i];
    if ((unsigned char)c <= 32 || c == '\\' || c == '#') {
      generate_output(state, "\\%03d", (unsigned char)c);
    } else {
      generate_output(state, "%c", c);
    }
    if (state->error) {
      return;
    }
  }
  generate_output(state, "\n");
  if (state->error) {
    return;
  }
}

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
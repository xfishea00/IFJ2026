/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    generator.c
 * Authors: Jaroslav Baček (xbacekj00), Maros Magoč (xmagocm00)
 * Brief:   implementation of the code generator
 */

#include "generator.h"
#include "error.h"
#include "symbol.h"
#include "type.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void generate_error(generator_state_t *state, const char *error_message,
                           ...);
static void generate_function(generator_state_t *state, ast_node_t *node);
static void generate_function_parameters(generator_state_t *state,
                                         ast_node_t *node);
static void generate_discard(generator_state_t *state);
static void generate_function_body(generator_state_t *state, ast_node_t *node);
static void buffer_function_code(generator_state_t *state, const char *code,
                                 size_t code_length);
static void generate_block(generator_state_t *state, ast_node_t *node);
static void generate_expression(generator_state_t *state, ast_node_t *node);
static void generate_variable_declaration(generator_state_t *state,
                                          ast_node_t *node);
static void generate_assignment(generator_state_t *state, ast_node_t *node);
static void generate_return(generator_state_t *state, ast_node_t *node);
static void generate_function_return(generator_state_t *state, ast_node_t *node,
                                     bool has_result);
static void generate_return_epilogue(generator_state_t *state);
static void generate_literal(generator_state_t *state, ast_node_t *node);
static void generate_variable(generator_state_t *state, ast_node_t *node);
static void generate_unary_expression(generator_state_t *state,
                                      ast_node_t *node);
static void generate_binary_expression(generator_state_t *state,
                                       ast_node_t *node);
static void generate_boolean_expression(generator_state_t *state,
                                        ast_node_t *node);
static void generate_function_call(generator_state_t *state, ast_node_t *node);
static void generate_if_expression(generator_state_t *state, ast_node_t *node);
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
  if (state->buffering_output) {
    va_list count_args;
    va_copy(count_args, args);
    int required = vsnprintf(NULL, 0, format, count_args);
    va_end(count_args);
    if (required < 0 || state->output_buffer_length == SIZE_MAX ||
        (size_t)required > SIZE_MAX - state->output_buffer_length - 1) {
      va_end(args);
      state->error = true;
      return;
    }

    size_t needed = state->output_buffer_length + (size_t)required + 1;
    if (needed > state->output_buffer_capacity) {
      size_t capacity;
      if(state->output_buffer_capacity == 0){
        capacity = 256;
      } else{
        capacity = state->output_buffer_capacity;
      }

      while (capacity < needed) {
        if (capacity > SIZE_MAX / 2) {
          capacity = needed;
          break;
        }
        capacity *= 2;
      }
      char *buffer = realloc(state->output_buffer, capacity);
      if (buffer == NULL) {
        va_end(args);
        state->error = true;
        return;
      }
      state->output_buffer = buffer;
      state->output_buffer_capacity = capacity;
    }

    int written =
        vsnprintf(state->output_buffer + state->output_buffer_length,
                  state->output_buffer_capacity - state->output_buffer_length,
                  format, args);
    if (written != required) {
      va_end(args);
      state->error = true;
      return;
    }
    state->output_buffer_length += (size_t)written;
  } else {
    int result = vfprintf(state->output, format, args);
    if (result < 0) {
      state->error = true;
    }
  }
  va_end(args);
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
  generate_output(state, "LABEL %s\n", label);
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
  int n = snprintf(buffer, buffer_size, "$temp_%u", state->temp_counter++);
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
static void generate_error(generator_state_t *state, const char *error_message,
                           ...) {
  if (state == NULL) {
    return;
  }
  fprintf(stderr, "Code generation error: ");
  va_list args;
  va_start(args, error_message);
  vfprintf(stderr, error_message, args);
  va_end(args);
  fputc('\n', stderr);
  state->error = true;
}

static void generate_discard(generator_state_t *state) {
  char temp_name[64];
  generate_temp_name(state, temp_name, sizeof(temp_name));
  if (state->error) {
    return;
  }

  generate_output(state, "DEFVAR LF@%s\n", temp_name);
  generate_output(state, "POPS LF@%s\n", temp_name);
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
    generate_error(state, "Expected program node, got AST kind %d",
                   root == NULL ? -1 : (int)root->kind);
    return;
  }

  ast_list_t *defs = &root->program.defs;
  ast_node_t *main_func = NULL;

  // Find the main function - only one is allowed
  for (int i = 0; i < defs->count; i++) {
    ast_node_t *def = defs->items[i];

    if (def == NULL || def->kind != AST_FUNC_DEF) {
      generate_error(state,
                     "Unsupported top-level definition at index %d "
                     "(AST kind %d)",
                     i, def == NULL ? -1 : (int)def->kind);
      return;
    }
    if (def->func_def.is_main) {
      if (main_func != NULL) {
        generate_error(state, "Multiple main functions defined");
        return;
      }
      main_func = def;
    }
  }
  // Check if the main is defined
  if (main_func == NULL || main_func->func_def.symbol == NULL ||
      main_func->func_def.symbol->unique_name == NULL) {
    generate_error(state, "Main function not defined");
    return;
  }
  // Generate the IFJcode26 header
  generate_output(state, ".IFJcode26\n");

  // Generate the main function call
  generate_output(state, "CREATEFRAME\n");
  generate_output(state, "CALL $function_%s\n",
                  main_func->func_def.symbol->unique_name);
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
      generate_error(state, "Null statement node");
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
  case AST_CALL:
    generate_function_call(state, node);
    if (state->error) {
      return;
    }

    char temp_name[64];
    generate_temp_name(state, temp_name, sizeof(temp_name));
    if (state->error) {
      return;
    }

    generate_output(state, "DEFVAR LF@%s\n", temp_name);
    generate_output(state, "POPS LF@%s\n", temp_name);
    break;
  case AST_ID:
  case AST_LITERAL:
  case AST_BINARY:
  case AST_UNARY:
    generate_expression(state, node);
    if (state->error) {
      return;
    }
    generate_discard(state);
    break;
  default:
    generate_error(state, "Unsupported statement AST kind %d", (int)node->kind);
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
      generate_error(state, "Null expression node");
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
  case AST_IF:
    generate_if_expression(state, node);
    break;
  default:
    generate_error(state, "Unsupported expression AST kind %d",
                   (int)node->kind);
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
  int n = snprintf(function_label, sizeof(function_label), "$function_%s",
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

  bool previous_buffering = state->buffering_output;
  char *previous_buffer = state->output_buffer;
  size_t previous_length = state->output_buffer_length;
  size_t previous_capacity = state->output_buffer_capacity;
  state->buffering_output = true;
  state->output_buffer = NULL;
  state->output_buffer_length = 0;
  state->output_buffer_capacity = 0;

  generate_function_parameters(state, node);
  if (!state->error) {
    generate_function_body(state, node);
  }

  char *function_code = state->output_buffer;
  size_t function_code_length = state->output_buffer_length;
  state->buffering_output = previous_buffering;
  state->output_buffer = previous_buffer;
  state->output_buffer_length = previous_length;
  state->output_buffer_capacity = previous_capacity;

  if (state->error) {
    free(function_code);
    return;
  }
  buffer_function_code(state, function_code, function_code_length);
  free(function_code);
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

      if (param == NULL || param->kind != AST_PARAM ||
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

static bool is_function_result_expression(const ast_node_t *node) {
  if (node == NULL) {
    return false;
  }
  switch (node->kind) {
  case AST_LITERAL:
  case AST_ID:
  case AST_BINARY:
  case AST_UNARY:
  case AST_CALL:
    return true;
  case AST_IF:
    return node->if_stmt.else_branch != NULL;
  default:
    return false;
  }
}

static bool is_if_branch_result_expression(const ast_node_t *node) {
  return is_function_result_expression(node);
}

static void generate_if_branch_value(generator_state_t *state,
                                     ast_node_t *branch) {
  if (state == NULL || branch == NULL || branch->kind != AST_BLOCK ||
      branch->block.stmts.count == 0) {
    if (state != NULL) {
      generate_error(state, "If-expression branch has no result");
    }
    return;
  }

  ast_list_t *stmts = &branch->block.stmts;
  for (int i = 0; i < stmts->count - 1; i++) {
    if (stmts->items[i] == NULL) {
      generate_error(state, "Null statement in if-expression branch");
      return;
    }
    generate_statement(state, stmts->items[i]);
    if (state->error) {
      return;
    }
  }

  ast_node_t *result = stmts->items[stmts->count - 1];
  if (result != NULL && result->kind == AST_RETURN) {
    generate_statement(state, result);
    return;
  }
  if (!is_if_branch_result_expression(result)) {
    generate_error(state,
                   "If-expression branch does not end with an expression");
    return;
  }
  generate_expression(state, result);
}

static void generate_function_body(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_FUNC_DEF ||
      node->func_def.body == NULL || node->func_def.body->kind != AST_BLOCK) {
    if (state != NULL) {
      generate_error(state, "Function has an invalid body");
    }
    return;
  }

  ast_node_t *body = node->func_def.body;
  ast_list_t *stmts = &body->block.stmts;
  bool returns_value =
      !node->func_def.is_main && (node->func_def.ret_type == NULL ||
                                  node->func_def.ret_type->kind != T_UNIT);
  bool has_result = false;

  for (int i = 0; i < stmts->count; i++) {
    ast_node_t *stmt = stmts->items[i];
    if (stmt == NULL) {
      generate_error(state, "Null statement in function body at index %d", i);
      return;
    }

    bool is_last = i == stmts->count - 1;
    if (is_last && returns_value && is_function_result_expression(stmt)) {
      generate_expression(state, stmt);
      if (state->error) {
        return;
      }
      has_result = true;
    } else if (is_last && !returns_value &&
               is_function_result_expression(stmt)) {
      generate_expression(state, stmt);
      if (state->error) {
        return;
      }
      generate_discard(state);
      if (state->error) {
        return;
      }
    } else {
      generate_statement(state, stmt);
      if (state->error) {
        return;
      }
    }
  }

  if (stmts->count > 0 && stmts->items[stmts->count - 1]->kind == AST_RETURN) {
    return;
  }
  generate_function_return(state, node, has_result);
}

static void buffer_function_code(generator_state_t *state, const char *code,
                                 size_t code_length) {
  typedef struct {
    char **names;
    size_t count;
    size_t capacity;
  } defvars_t;
  defvars_t declared = {0};
  bool needs_cleanup = false;

  static const char prefix[] = "DEFVAR LF@";

  size_t position = 0;
  while (!needs_cleanup && position < code_length) {
    size_t line_start = position;
    const char *newline = memchr(code + position, '\n', code_length - position);

    size_t line_length;
    if (newline == NULL) {
      line_length = code_length - position;
    } else {
      line_length = (size_t)(newline - (code + position)) + 1;
    }

    position += line_length;
    if (line_length < sizeof(prefix) - 1 ||
        memcmp(code + line_start, prefix, sizeof(prefix) - 1) != 0) {
      continue;
    }

    const char *name = code + line_start + sizeof(prefix) - 1;
    size_t name_length = 0;
    while (sizeof(prefix) - 1 + name_length < line_length &&
           name[name_length] != ' ' && name[name_length] != '\t' &&
           name[name_length] != '\r' && name[name_length] != '\n') {
      name_length++;
    }
    if (name_length == 0) {
      generate_error(state, "Encountered empty DEFVAR name in function code");
      needs_cleanup = true;
      break;
    }
    bool duplicate = false;
    for (size_t i = 0; i < declared.count; i++) {
      if (strlen(declared.names[i]) == name_length &&
          memcmp(declared.names[i], name, name_length) == 0) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      continue;
    }

    if (declared.count == declared.capacity) {
      size_t capacity = declared.capacity == 0 ? 8 : declared.capacity * 2;
      char **names = realloc(declared.names, capacity * sizeof(*names));
      if (names == NULL) {
        generate_error(state, "Could not allocate function variable list");
        needs_cleanup = true;
        break;
      }
      declared.names = names;
      declared.capacity = capacity;
    }
    declared.names[declared.count] = malloc(name_length + 1);
    if (declared.names[declared.count] == NULL) {
      generate_error(state, "Could not allocate function variable name");
      needs_cleanup = true;
      break;
    }
    memcpy(declared.names[declared.count], name, name_length);
    declared.names[declared.count][name_length] = '\0';
    declared.count++;
  }

  for (size_t i = 0; !needs_cleanup && i < declared.count; i++) {
    generate_output(state, "DEFVAR LF@%s\n", declared.names[i]);
    if (state->error) {
      needs_cleanup = true;
    }
  }

  position = 0;
  while (!needs_cleanup && position < code_length) {
    size_t line_start = position;
    const char *newline = memchr(code + position, '\n', code_length - position);

    size_t line_length;
    if (newline == NULL) {
      line_length = code_length - position;
    } else {
      line_length = (size_t)(newline - (code + position)) + 1;
    }

    position += line_length;
    bool is_defvar = line_length >= sizeof(prefix) - 1 &&
                     memcmp(code + line_start, prefix, sizeof(prefix) - 1) == 0;
    if (!is_defvar && fwrite(code + line_start, 1, line_length,
                             state->output) != line_length) {
      generate_error(state, "Could not write generated function code");
      needs_cleanup = true;
    }
  }

  for (size_t i = 0; i < declared.count; i++) {
    free(declared.names[i]);
  }
  free(declared.names);
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

  generate_output(state, "DEFVAR LF@%s\n", node->var_def.symbol->unique_name);
  if (state->error) {
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
  generate_return_epilogue(state);
}

static void generate_return_epilogue(generator_state_t *state) {
  if (state == NULL) {
    return;
  }
  generate_output(state, "POPFRAME\n");
  generate_output(state, "RETURN\n");
}

/**
 * @brief Generates the return value of a function from the AST.
 * @param state The generator state.
 * @param node The AST node representing the return value.
 */
static void generate_function_return(generator_state_t *state, ast_node_t *node,
                                     bool has_result) {
  if (state == NULL || node == NULL || node->kind != AST_FUNC_DEF) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  if (!has_result) {
    generate_output(state, "PUSHS nil@nil\n");
  }
  generate_return_epilogue(state);
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
    if (node->unary.operand->type == NULL) {
      state->error = true;
      return;
    }
    switch (node->unary.operand->type->kind) {
    case T_INT:
      generate_output(state, "PUSHS int@-1\n"
                             "MULS\n");
      break;
    case T_DOUBLE:
      generate_output(state, "PUSHS float@-0x1p+0\n"
                             "MULS\n");
      break;
    default:
      state->error = true;
      break;
    }
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
                                       ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_BINARY ||
      node->binary.left == NULL || node->binary.right == NULL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  if (node->binary.op == OP_AND || node->binary.op == OP_OR) {
    generate_boolean_expression(state, node);
    return;
  }

  generate_expression(state, node->binary.left);

  if (state->error) {
    return;
  }

  generate_expression(state, node->binary.right);

  if (state->error) {
    return;
  }
  
  char temp0[64];
  char temp1[64];
  char temp2[64];

  switch (node->binary.op) {
  case OP_ADD:
    generate_output(state, "ADDS\n");
    break;
  case OP_SUB:
    generate_output(state, "SUBS\n");
    break;
  case OP_MUL:
    generate_output(state, "MULS\n");
    break;
  case OP_DIV:
    generate_output(state, "DIVS\n");
    break;
  case OP_LT:
    generate_output(state, "LTS\n");
    break;
  case OP_GT:
    generate_output(state, "GTS\n");
    break;
  case OP_LE:
    generate_output(state, "GTS\n"
                           "NOTS\n");
    break;
  case OP_GE:
    generate_output(state, "LTS\n"
                           "NOTS\n");
    break;
  case OP_EQ:
    generate_output(state, "EQS\n");
    break;
  case OP_NEQ:
    generate_output(state, "EQS\n"
                           "NOTS\n");
    break;
  case OP_CONCAT:
    generate_temp_name(state, temp0, sizeof(temp0));
    generate_temp_name(state, temp1, sizeof(temp1));
    generate_temp_name(state, temp2, sizeof(temp2));
    generate_output(state, "DEFVAR LF@%s\n", temp0);
    generate_output(state, "DEFVAR LF@%s\n", temp1);
    generate_output(state, "DEFVAR LF@%s\n", temp2);
    generate_output(state, "POPS LF@%s\n", temp0);
    generate_output(state, "POPS LF@%s\n", temp1);
    generate_output(state, "CONCAT LF@%s LF@%s LF@%s\n", temp2, temp1, temp0);
    generate_output(state, "PUSHS LF@%s\n", temp2);
    break;
  default:
    state->error = true;
    break;
  }
}

/**
 * @brief Generates a function call from the AST.
 * @param state The generator state.
 * @param node The AST node representing the function call.
 */
static void generate_function_call(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_CALL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  if (node->call.symbol == NULL) {
    state->error = true;
    return;
  }

  // builtin functions generation
  if (node->call.symbol->is_builtin) {
    generate_builtin_call(state, node);
    return;
  }

  if (node->call.symbol->unique_name == NULL ||
      node->call.symbol->decl == NULL ||
      node->call.symbol->decl->kind != AST_FUNC_DEF) {
    state->error = true;
    return;
  }

  // Generate code for each argument in the function call
  for (int i = 0; i < node->call.arg_list_count; i++) {
    ast_list_t *args = &node->call.arg_lists[i];

    for (int j = 0; j < args->count; j++) {
      ast_node_t *arg = args->items[j];

      if (arg == NULL) {
        state->error = true;
        return;
      }

      generate_expression(state, arg);
      if (state->error) {
        return;
      }
    }
  }

  generate_output(state, "CREATEFRAME\n");

  if (state->error) {
    return;
  }

  generate_output(state, "CALL $function_%s\n", node->call.symbol->unique_name);
}

/**
 * @brief Generates a boolean expression from the AST.
 * @param state The generator state.
 * @param node The AST node representing the boolean expression.
 */
static void generate_boolean_expression(generator_state_t *state,
                                        ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_BINARY ||
      (node->binary.op != OP_AND && node->binary.op != OP_OR) ||
      node->binary.left == NULL || node->binary.right == NULL) {
    if (state != NULL) {
      generate_error(state, "Invalid boolean expression");
    }
    return;
  }

  char short_eval_label[64];
  char end_label[64];
  generate_label_name(state, short_eval_label,
                      sizeof(short_eval_label));
  generate_label_name(state, end_label, sizeof(end_label));
  if (state->error) {
    return;
  }

  bool is_and = node->binary.op == OP_AND;
  const char *short_value;
  if(is_and){
    short_value = "false";
  } else {
    short_value = "true";
  }
  generate_expression(state, node->binary.left);
  if (state->error) {
    return;
  }
  generate_output(state, "PUSHS bool@%s\n", short_value);
  generate_output(state, "JUMPIFEQS %s\n", short_eval_label);

  generate_expression(state, node->binary.right);
  if (state->error) {
    return;
  }
  generate_output(state, "JUMP %s\n", end_label);

  generate_label(state, short_eval_label);
  generate_output(state, "PUSHS bool@%s\n", short_value);
  generate_label(state, end_label);
}

/**
 * @brief Generates an if expression from the AST.
 * @param state The generator state.
 * @param node The AST node representing the if expression.
 */
static void generate_if_expression(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_IF ||
      node->if_stmt.cond == NULL || node->if_stmt.then_branch == NULL ||
      node->if_stmt.else_branch == NULL) {
    if (state != NULL) {
      generate_error(state,
                     "If-expression requires a condition and both branches");
    }
    return;
  }

  char else_label[64];
  char end_label[64];
  generate_label_name(state, else_label, sizeof(else_label));
  generate_label_name(state, end_label, sizeof(end_label));
  if (state->error) {
    return;
  }

  generate_expression(state, node->if_stmt.cond);
  if (state->error) {
    return;
  }
  generate_output(state, "PUSHS bool@false\n");
  generate_output(state, "JUMPIFEQS %s\n", else_label);

  generate_if_branch_value(state, node->if_stmt.then_branch);
  if (state->error) {
    return;
  }
  generate_output(state, "JUMP %s\n", end_label);

  generate_label(state, else_label);
  generate_if_branch_value(state, node->if_stmt.else_branch);
  if (state->error) {
    return;
  }
  generate_label(state, end_label);
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
  generate_output(state, "JUMPIFEQS %s\n", end_label);

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
static void generate_for(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_FOR ||
      node->for_stmt.source == NULL ||
      node->for_stmt.source->kind != AST_RANGE || node->for_stmt.body == NULL ||
      node->for_stmt.body->kind != AST_BLOCK || node->for_stmt.symbol == NULL ||
      node->for_stmt.symbol->unique_name == NULL) {
    if (state != NULL) {
      generate_error(state,
                     "For-loop requires an integer range and a valid body");
    }
    return;
  }

  ast_node_t *range = node->for_stmt.source;
  if (range->range.from == NULL || range->range.end == NULL) {
    generate_error(state, "For-loop range is missing a bound");
    return;
  }

  char start_value[64];
  char end_value[64];
  char step_value[64];
  char loop_label[64];
  char positive_label[64];
  char body_label[64];
  char increment_label[64];
  char end_label[64];
  char nonzero_step_label[64];

  generate_temp_name(state, start_value, sizeof(start_value));
  generate_temp_name(state, end_value, sizeof(end_value));
  generate_temp_name(state, step_value, sizeof(step_value));
  generate_label_name(state, loop_label, sizeof(loop_label));
  generate_label_name(state, positive_label, sizeof(positive_label));
  generate_label_name(state, body_label, sizeof(body_label));
  generate_label_name(state, increment_label, sizeof(increment_label));
  generate_label_name(state, end_label, sizeof(end_label));
  generate_label_name(state, nonzero_step_label, sizeof(nonzero_step_label));
  if (state->error) {
    return;
  }

  generate_output(state, "DEFVAR LF@%s\n", start_value);
  generate_output(state, "DEFVAR LF@%s\n", end_value);
  generate_output(state, "DEFVAR LF@%s\n", step_value);
  generate_output(state, "DEFVAR LF@%s\n", node->for_stmt.symbol->unique_name);

  generate_expression(state, range->range.from);
  if (state->error) {
    return;
  }
  generate_output(state, "POPS LF@%s\n", start_value);

  generate_expression(state, range->range.end);
  if (state->error) {
    return;
  }
  generate_output(state, "POPS LF@%s\n", end_value);

  if (range->range.step == NULL) {
    generate_output(state, "MOVE LF@%s int@1\n", step_value);
  } else {
    generate_expression(state, range->range.step);
    if (state->error) {
      return;
    }
    generate_output(state, "POPS LF@%s\n", step_value);
  }

  generate_output(state, "JUMPIFNEQ %s LF@%s int@0\n", nonzero_step_label,
                  step_value);
  generate_output(state, "EXIT int@40\n");
  generate_label(state, nonzero_step_label);
  generate_output(state, "MOVE LF@%s LF@%s\n",
                  node->for_stmt.symbol->unique_name, start_value);

  generate_label(state, loop_label);
  generate_output(state, "PUSHS LF@%s\n", step_value);
  generate_output(state, "PUSHS int@0\n");
  generate_output(state, "GTS\n");
  generate_output(state, "PUSHS bool@false\n");
  generate_output(state, "JUMPIFNEQS %s\n", positive_label);

  generate_output(state, "PUSHS LF@%s\n", node->for_stmt.symbol->unique_name);
  generate_output(state, "PUSHS LF@%s\n", end_value);

  if (range->range.is_until) {
    generate_output(state, "GTS\n");
  } else {
    generate_output(state, "LTS\n");
    generate_output(state, "NOTS\n");
  }
  generate_output(state, "PUSHS bool@true\n");
  generate_output(state, "JUMPIFNEQS %s\n", end_label);
  generate_output(state, "JUMP %s\n", body_label);

  generate_label(state, positive_label);
  generate_output(state, "PUSHS LF@%s\n", node->for_stmt.symbol->unique_name);
  generate_output(state, "PUSHS LF@%s\n", end_value);
  if (range->range.is_until) {
    generate_output(state, "LTS\n");
  } else {
    generate_output(state, "GTS\n");
    generate_output(state, "NOTS\n");
  }
  generate_output(state, "PUSHS bool@true\n");
  generate_output(state, "JUMPIFNEQS %s\n", end_label);

  generate_label(state, body_label);
  for (int i = 0; i < node->for_stmt.guards.count; i++) {
    ast_node_t *guard = node->for_stmt.guards.items[i];
    if (guard == NULL) {
      generate_error(state, "Null guard in for-loop at index %d", i);
      return;
    }
    generate_expression(state, guard);
    if (state->error) {
      return;
    }
    generate_output(state, "PUSHS bool@true\n");
    generate_output(state, "JUMPIFNEQS %s\n", increment_label);
  }

  generate_block(state, node->for_stmt.body);
  if (state->error) {
    return;
  }

  generate_label(state, increment_label);
  generate_output(state, "PUSHS LF@%s\n", node->for_stmt.symbol->unique_name);
  generate_output(state, "PUSHS LF@%s\n", step_value);
  generate_output(state, "ADDS\n");
  generate_output(state, "POPS LF@%s\n", node->for_stmt.symbol->unique_name);
  generate_output(state, "JUMP %s\n", loop_label);
  generate_label(state, end_label);
}

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
static void generate_list(generator_state_t *state, ast_node_t *node) {
  (void)node;
  generate_error(state, "List construction code generation is not implemented");
}

/**
 * @brief Generates a list access from the AST.
 * @param state The generator state.
 * @param node The AST node representing the list access.
 */
static void generate_list_access(generator_state_t *state, ast_node_t *node) {
  (void)node;
  generate_error(state, "List access code generation is not implemented");
}

/**
 * @brief Generates a list operation from the AST.
 * @param state The generator state.
 * @param node The AST node representing the list operation.
 */
static void generate_list_operation(generator_state_t *state,
                                    ast_node_t *node) {
  (void)node;
  generate_error(state, "List operation code generation is not implemented");
}

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
                                           ast_node_t *node) {
  (void)node;
  generate_error(state,
                 "Data class definition code generation is not implemented");
}

/**
 * @brief Generates a data class constructor from the AST.
 * @param state The generator state.
 * @param node The AST node representing the data class constructor.
 */
static void generate_data_class_constructor(generator_state_t *state,
                                            ast_node_t *node) {
  (void)node;
  generate_error(state,
                 "Data class constructor code generation is not implemented");
}

/**
 * @brief Generates a data class member access from the AST.
 * @param state The generator state.
 * @param node The AST node representing the data class member access.
 */
static void generate_member_access(generator_state_t *state, ast_node_t *node) {
  (void)node;
  generate_error(state, "Data class member access is not implemented");
}

/**
 * @brief Generates a data class member call from the AST.
 * @param state The generator state.
 * @param node The AST node representing the data class member call.
 */
static void generate_member_call(generator_state_t *state, ast_node_t *node) {
  (void)node;
  generate_error(state, "Data class member call is not implemented");
}

/**
 * @brief Generates a union type definition from the AST.
 * @param state The generator state.
 * @param node The AST node representing the union type definition.
 */
static void generate_union_type(generator_state_t *state, ast_node_t *node) {
  (void)node;
  generate_error(state, "Union type code generation is not implemented");
}

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
static void generate_match(generator_state_t *state, ast_node_t *node) {
  (void)node;
  generate_error(state, "Match code generation is not implemented");
}

/**
 * @brief Generates a match case from the AST.
 * @param state The generator state.
 * @param node The AST node representing the match case.
 */
static void generate_match_case(generator_state_t *state, ast_node_t *node) {
  (void)node;
  generate_error(state, "Match case code generation is not implemented");
}

/**
 * @brief Generates a pattern from the AST.
 * @param state The generator state.
 * @param node The AST node representing the pattern.
 */
static void generate_pattern(generator_state_t *state, ast_node_t *node) {
  (void)node;
  generate_error(state, "Pattern code generation is not implemented");
}

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

static void generate_builtin_call(generator_state_t *state, ast_node_t *node) {
  if (state == NULL || node == NULL || node->kind != AST_CALL ||
      node->call.symbol == NULL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }
  const char *builtin_name = node->call.symbol->name;

  if (strcmp(builtin_name, "readInt") == 0) {
    generate_builtin_read(state, "int");
    return;
  }
  if (strcmp(builtin_name, "readDouble") == 0) {
    generate_builtin_read(state, "float");
    return;
  }
  if (strcmp(builtin_name, "readLine") == 0) {
    generate_builtin_read(state, "string");
    return;
  }

  // builtin functions with 1 argument call generate_expression before they are
  // called
  if (strcmp(builtin_name, "toStr") == 0 ||
      strcmp(builtin_name, "toInt") == 0 ||
      strcmp(builtin_name, "strLength") == 0 ||
      strcmp(builtin_name, "chr") == 0) {

    generate_expression(state, node->call.arg_lists[0].items[0]);
    if (state->error) {
      return;
    }
  }

  if (strcmp(builtin_name, "ord") == 0) {
    generate_expression(state, node->call.arg_lists[0].items[0]);
    if (state->error) {
      return;
    }
    generate_expression(state, node->call.arg_lists[0].items[1]);
    if (state->error) {
      return;
    }
  }

  if (strcmp(builtin_name, "subStr") == 0) {
    generate_expression(state, node->call.arg_lists[0].items[0]);
    if (state->error) {
      return;
    }
    generate_expression(state, node->call.arg_lists[0].items[1]);
    if (state->error) {
      return;
    }
    generate_expression(state, node->call.arg_lists[0].items[2]);
    if (state->error) {
      return;
    }
  }

  if (strcmp(builtin_name, "print") == 0) {
    generate_print(state, node);
  } else if (strcmp(builtin_name, "toStr") == 0) {
    generate_toStr(state);
  } else if (strcmp(builtin_name, "toInt") == 0) {
    generate_toInt(state);
  } else if (strcmp(builtin_name, "strLength") == 0) {
    generate_strLength(state);
  } else if (strcmp(builtin_name, "subStr") == 0) {
    generate_subStr(state);
  } else if (strcmp(builtin_name, "ord") == 0) {
    generate_ord(state);
  } else if (strcmp(builtin_name, "chr") == 0) {
    generate_chr(state);
  } else if (strcmp(builtin_name, "readBoolean") == 0) {
    generate_readBoolean(state);
  } else if (strcmp(builtin_name, "readIntMaybe") == 0) {
    generate_readIntMaybe(state);
  } else if (strcmp(builtin_name, "readDoubleMaybe") == 0) {
    generate_readDoubleMaybe(state);
  } else {
    state->error = true;
    generate_error(state, "Unknown builtin function: %s", builtin_name);
    return;
  }
}

static void generate_print(generator_state_t *state, ast_node_t *node) {
  if (state == NULL) {
    return;
  }

  generate_expression(state, node->call.arg_lists[0].items[0]);
  if (state->error) {
    return;
  }

  char temp_name[256];
  char type_name[256];
  char unit_label[64];
  char end_label[64];
  generate_temp_name(state, temp_name, sizeof(temp_name));
  if (state->error) {
    return;
  }
  generate_temp_name(state, type_name, sizeof(type_name));
  if (state->error) {
    return;
  }
  generate_label_name(state, unit_label, sizeof(unit_label));
  generate_label_name(state, end_label, sizeof(end_label));
  if (state->error) {
    return;
  }

  generate_output(state, "DEFVAR LF@%s\n", temp_name);
  generate_output(state, "DEFVAR LF@%s\n", type_name);
  generate_output(state, "POPS LF@%s\n", temp_name);
  generate_output(state, "TYPE LF@%s LF@%s\n", type_name, temp_name);
  generate_output(state, "JUMPIFEQ %s LF@%s string@nil\n", unit_label,
                  type_name);
  generate_output(state, "WRITE LF@%s\n", temp_name);
  generate_output(state, "JUMP %s\n", end_label);
  generate_label(state, unit_label);
  generate_output(state, "WRITE string@()\n");
  generate_label(state, end_label);

  generate_output(state, "PUSHS nil@nil\n");
}

static void generate_builtin_read(generator_state_t *state, const char *type) {
  if (state == NULL || type == NULL) {
    if (state != NULL) {
      state->error = true;
    }
    return;
  }

  char temp_name[256];
  char type_name[256];
  char valid_label[64];
  generate_temp_name(state, temp_name, sizeof(temp_name));
  if (state->error) {
    return;
  }
  if (strcmp(type, "int") == 0) {
    generate_temp_name(state, type_name, sizeof(type_name));
    if (state->error) {
      return;
    }
    generate_label_name(state, valid_label, sizeof(valid_label));
    if (state->error) {
      return;
    }
  }

  generate_output(state, "DEFVAR LF@%s\n", temp_name);
  if (strcmp(type, "int") == 0) {
    generate_output(state, "DEFVAR LF@%s\n", type_name);
  }
  generate_output(state, "READ LF@%s %s\n", temp_name, type);
  if (strcmp(type, "int") == 0) {
    generate_output(state, "TYPE LF@%s LF@%s\n", type_name, temp_name);
    generate_output(state, "JUMPIFNEQ %s LF@%s string@nil\n", valid_label,
                    type_name);
    generate_output(state, "MOVE LF@%s int@0\n", temp_name);
    generate_label(state, valid_label);
  }
  generate_output(state, "PUSHS LF@%s\n", temp_name);
}

static void generate_toStr(generator_state_t *state) {
  if (state == NULL) {
    return;
  }

  char value[256];
  char type[256];
  char result[256];

  char label_string[256];
  char label_bool[256];
  char label_int[256];
  char label_float[256];
  char label_end[256];
  char label_unit[256];
  char label_error[256];

  generate_temp_name(state, value, sizeof(value));
  if (state->error) {
    return;
  }
  generate_temp_name(state, type, sizeof(type));
  if (state->error) {
    return;
  }
  generate_temp_name(state, result, sizeof(result));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_string, sizeof(label_string));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_bool, sizeof(label_bool));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_int, sizeof(label_int));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_float, sizeof(label_float));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_end, sizeof(label_end));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_unit, sizeof(label_unit));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_error, sizeof(label_error));
  if (state->error) {
    return;
  }

  // declare variables for value and type
  generate_output(state, "DEFVAR LF@%s\n", value);
  generate_output(state, "DEFVAR LF@%s\n", type);
  generate_output(state, "DEFVAR LF@%s\n", result);
  generate_output(state, "POPS LF@%s\n", value);
  generate_output(state, "TYPE LF@%s LF@%s\n", type, value);

  generate_output(state, "JUMPIFEQ %s LF@%s string@string\n", label_string,
                  type);
  generate_output(state, "JUMPIFEQ %s LF@%s string@bool\n", label_bool, type);
  generate_output(state, "JUMPIFEQ %s LF@%s string@int\n", label_int, type);
  generate_output(state, "JUMPIFEQ %s LF@%s string@float\n", label_float, type);
  generate_output(state, "JUMPIFEQ %s LF@%s string@nil\n", label_unit, type);

  generate_output(state, "JUMP %s\n", label_error);

  // generate code for string
  generate_output(state, "LABEL %s\n", label_string);
  generate_output(state, "PUSHS LF@%s\n", value);
  generate_output(state, "JUMP %s\n", label_end);

  // generate code for bool
  char bool_true[256];
  char bool_false[256];

  generate_output(state, "LABEL %s\n", label_bool);

  generate_temp_name(state, bool_true, sizeof(bool_true));
  if (state->error) {
    return;
  }

  generate_temp_name(state, bool_false, sizeof(bool_false));
  if (state->error) {
    return;
  }

  generate_output(state, "JUMPIFEQ %s LF@%s bool@true\n", bool_true, value);
  generate_output(state, "JUMP %s\n", bool_false);

  generate_output(state, "LABEL %s\n", bool_true);
  generate_output(state, "PUSHS string@true\n");
  generate_output(state, "JUMP %s\n", label_end);

  generate_output(state, "LABEL %s\n", bool_false);
  generate_output(state, "PUSHS string@false\n");
  generate_output(state, "JUMP %s\n", label_end);

  // generate code for int
  generate_output(state, "LABEL %s\n", label_int);
  generate_output(state, "INT2STR LF@%s LF@%s\n", result, value);
  generate_output(state, "PUSHS LF@%s\n", result);
  generate_output(state, "JUMP %s\n", label_end);

  // generate code for float
  generate_output(state, "LABEL %s\n", label_float);
  generate_output(state, "FLOAT2STR LF@%s LF@%s\n", result, value);
  generate_output(state, "PUSHS LF@%s\n", result);
  generate_output(state, "JUMP %s\n", label_end);

  // generate code for unit
  generate_output(state, "LABEL %s\n", label_unit);
  generate_output(state, "PUSHS string@()\n");
  generate_output(state, "JUMP %s\n", label_end);

  // generate code for error
  generate_output(state, "LABEL %s\n", label_error);
  generate_output(state, "EXIT int@40\n");
  generate_output(state, "LABEL %s\n", label_end);

  // TODO: implement toStr for other types (lists,...)
}

static void generate_toInt(generator_state_t *state) {
  if (state == NULL) {
    return;
  }

  char temp_name[256];
  generate_temp_name(state, temp_name, sizeof(temp_name));
  if (state->error) {
    return;
  }

  generate_output(state, "DEFVAR LF@%s\n", temp_name);
  generate_output(state, "POPS LF@%s\n", temp_name);
  generate_output(state, "FLOAT2INT LF@%s LF@%s\n", temp_name, temp_name);
  generate_output(state, "PUSHS LF@%s\n", temp_name);
}

static void generate_strLength(generator_state_t *state) {
  if (state == NULL) {
    return;
  }

  char value[256];
  char length[256];

  generate_temp_name(state, value, sizeof(value));
  if (state->error) {
    return;
  }

  generate_temp_name(state, length, sizeof(length));
  if (state->error) {
    return;
  }

  generate_output(state, "DEFVAR LF@%s\n", value);
  generate_output(state, "DEFVAR LF@%s\n", length);

  generate_output(state, "POPS LF@%s\n", value);
  generate_output(state, "STRLEN LF@%s LF@%s\n", length, value);
  generate_output(state, "PUSHS LF@%s\n", length);
}

static void generate_subStr(generator_state_t *state) {
  if (state == NULL) {
    return;
  }

  char value[256];
  char start[256];
  char length[256];
  char end[256];
  char index[256];
  char result[256];
  char condition[256];
  char character[256];
  char character_code[256];

  char label_loop[256];
  char label_end[256];
  char label_error[256];
  char label_return[256];

  generate_temp_name(state, value, sizeof(value));
  if (state->error) {
    return;
  }
  generate_temp_name(state, start, sizeof(start));
  if (state->error) {
    return;
  }
  generate_temp_name(state, length, sizeof(length));
  if (state->error) {
    return;
  }
  generate_temp_name(state, end, sizeof(end));
  if (state->error) {
    return;
  }
  generate_temp_name(state, index, sizeof(index));
  if (state->error) {
    return;
  }
  generate_temp_name(state, result, sizeof(result));
  if (state->error) {
    return;
  }
  generate_temp_name(state, condition, sizeof(condition));
  if (state->error) {
    return;
  }
  generate_temp_name(state, character, sizeof(character));
  if (state->error) {
    return;
  }
  generate_temp_name(state, character_code, sizeof(character_code));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_loop, sizeof(label_loop));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_end, sizeof(label_end));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_error, sizeof(label_error));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_return, sizeof(label_return));
  if (state->error) {
    return;
  }

  // declare variables
  generate_output(state, "DEFVAR LF@%s\n", value);
  generate_output(state, "DEFVAR LF@%s\n", start);
  generate_output(state, "DEFVAR LF@%s\n", length);
  generate_output(state, "DEFVAR LF@%s\n", end);
  generate_output(state, "DEFVAR LF@%s\n", index);
  generate_output(state, "DEFVAR LF@%s\n", result);
  generate_output(state, "DEFVAR LF@%s\n", condition);
  generate_output(state, "DEFVAR LF@%s\n", character);
  generate_output(state, "DEFVAR LF@%s\n", character_code);

  // pop arguments from the stack
  generate_output(state, "POPS LF@%s\n", end);
  generate_output(state, "POPS LF@%s\n", start);
  generate_output(state, "POPS LF@%s\n", value);

  // get the length of the string
  generate_output(state, "STRLEN LF@%s LF@%s\n", length, value);

  // start < 0
  generate_output(state, "LT LF@%s LF@%s int@0\n", condition, start);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@true\n", label_error,
                  condition);

  // end < 0
  generate_output(state, "LT LF@%s LF@%s int@0\n", condition, end);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@true\n", label_error,
                  condition);

  // start >= length
  generate_output(state, "LT LF@%s LF@%s LF@%s\n", condition, start, length);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@false\n", label_error,
                  condition);

  // start > end
  generate_output(state, "GT LF@%s LF@%s LF@%s\n", condition, start, end);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@true\n", label_error,
                  condition);

  // end >= length
  generate_output(state, "LT LF@%s LF@%s LF@%s\n", condition, end, length);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@false\n", label_error,
                  condition);

  // init result, index
  generate_output(state, "MOVE LF@%s string@\n", result);
  generate_output(state, "MOVE LF@%s LF@%s\n", index, start);

  // loop
  generate_output(state, "LABEL %s\n", label_loop);

  // stop if index > end
  generate_output(state, "GT LF@%s LF@%s LF@%s\n", condition, index, end);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@true\n", label_end,
                  condition);

  // get char on index
  generate_output(state, "STRI2INT LF@%s LF@%s LF@%s\n", character_code, value,
                  index);
  generate_output(state, "INT2CHAR LF@%s LF@%s\n", character, character_code);

  // concat char to result
  generate_output(state, "CONCAT LF@%s LF@%s LF@%s\n", result, result,
                  character);

  // index++
  generate_output(state, "ADD LF@%s LF@%s int@1\n", index, index);

  generate_output(state, "JUMP %s\n", label_loop);

  // end of loop
  generate_output(state, "LABEL %s\n", label_end);
  generate_output(state, "PUSHS LF@%s\n", result);
  generate_output(state, "JUMP %s\n", label_return);

  // exit 40
  generate_output(state, "LABEL %s\n", label_error);
  generate_output(state, "EXIT int@40\n");

  // return
  generate_output(state, "LABEL %s\n", label_return);
}

static void generate_ord(generator_state_t *state) {
  if (state == NULL) {
    return;
  }

  char value[256];
  char index[256];
  char length[256];
  char result[256];
  char condition[256];

  char label_error[256];
  char label_end[256];

  generate_temp_name(state, value, sizeof(value));
  if (state->error) {
    return;
  }
  generate_temp_name(state, index, sizeof(index));
  if (state->error) {
    return;
  }
  generate_temp_name(state, length, sizeof(length));
  if (state->error) {
    return;
  }
  generate_temp_name(state, result, sizeof(result));
  if (state->error) {
    return;
  }
  generate_temp_name(state, condition, sizeof(condition));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_error, sizeof(label_error));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_end, sizeof(label_end));
  if (state->error) {
    return;
  }

  generate_output(state, "DEFVAR LF@%s\n", value);
  generate_output(state, "DEFVAR LF@%s\n", index);
  generate_output(state, "DEFVAR LF@%s\n", length);
  generate_output(state, "DEFVAR LF@%s\n", result);
  generate_output(state, "DEFVAR LF@%s\n", condition);

  generate_output(state, "POPS LF@%s\n", index);
  generate_output(state, "POPS LF@%s\n", value);

  // get string length
  generate_output(state, "STRLEN LF@%s LF@%s\n", length, value);

  // check empty string
  generate_output(state, "JUMPIFEQ %s LF@%s int@0\n", label_error, length);

  // index < 0
  generate_output(state, "LT LF@%s LF@%s int@0\n", condition, index);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@true\n", label_error,
                  condition);

  // index >= length
  generate_output(state, "LT LF@%s LF@%s LF@%s\n", condition, index, length);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@false\n", label_error,
                  condition);

  // get ascii code
  generate_output(state, "STRI2INT LF@%s LF@%s LF@%s\n", result, value, index);

  generate_output(state, "JUMP %s\n", label_end);

  // invalid index or empty string
  generate_output(state, "LABEL %s\n", label_error);
  generate_output(state, "MOVE LF@%s int@-1\n", result);

  // return
  generate_output(state, "LABEL %s\n", label_end);
  generate_output(state, "PUSHS LF@%s\n", result);
}

static void generate_chr(generator_state_t *state) {
  if (state == NULL) {
    return;
  }

  char value[256];
  char result[256];
  char condition[256];
  char label_error[256];
  char label_end[256];

  generate_temp_name(state, value, sizeof(value));
  if (state->error) {
    return;
  }
  generate_temp_name(state, result, sizeof(result));
  if (state->error) {
    return;
  }
  generate_temp_name(state, condition, sizeof(condition));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_error, sizeof(label_error));
  if (state->error) {
    return;
  }
  generate_temp_name(state, label_end, sizeof(label_end));
  if (state->error) {
    return;
  }

  generate_output(state, "DEFVAR LF@%s\n", value);
  generate_output(state, "DEFVAR LF@%s\n", result);
  generate_output(state, "DEFVAR LF@%s\n", condition);

  generate_output(state, "POPS LF@%s\n", value);

  // value < 0
  generate_output(state, "LT LF@%s LF@%s int@0\n", condition, value);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@true\n", label_error,
                  condition);

  // value > 127
  generate_output(state, "GT LF@%s LF@%s int@127\n", condition, value);
  generate_output(state, "JUMPIFEQ %s LF@%s bool@true\n", label_error,
                  condition);

  // int to char
  generate_output(state, "INT2CHAR LF@%s LF@%s\n", result, value);

  // result
  generate_output(state, "PUSHS LF@%s\n", result);
  generate_output(state, "JUMP %s\n", label_end);

  // exit 40
  generate_output(state, "LABEL %s\n", label_error);
  generate_output(state, "EXIT int@40\n");

  generate_output(state, "LABEL %s\n", label_end);
}

// TODO: implement the additional functions
static void generate_readBoolean(generator_state_t *state) {
  generate_builtin_read(state, "bool");
}

static void generate_readIntMaybe(generator_state_t *state) {
  generate_builtin_read(state, "int");
}

static void generate_readDoubleMaybe(generator_state_t *state) {
  generate_error(state, "readDoubleMaybe code generation is not implemented");
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
    return ERR_INTERNAL;
  }
  generator_state_t state;
  state.output = stdout;
  state.temp_counter = 0;
  state.label_counter = 0;
  state.error = false;
  state.buffering_output = false;
  state.output_buffer = NULL;
  state.output_buffer_length = 0;
  state.output_buffer_capacity = 0;

  generate_program(&state, root);

  return state.error ? ERR_INTERNAL : ERR_OK;
}
/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    ast.h
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   abstract syntax tree
 */

#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stdint.h>

// AST node kinds
typedef enum {
  AST_PROGRAM,
  AST_FUNC_DEF,
  AST_PARAM,
  AST_BLOCK,
  AST_VAR_DEF,
  AST_ASSIGN,
  AST_IF,
  AST_WHILE,
  AST_RETURN,
  AST_LITERAL,
  AST_ID,
  AST_BINARY,
  AST_UNARY,
  AST_CALL,
} ast_node_kind_t;

// operators of binary and unary
typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_LT,
  OP_GT,
  OP_LE,
  OP_GE,
  OP_EQ,
  OP_NEQ,
  OP_AND,
  OP_OR,
  OP_NOT,
  OP_NEG,
} ast_op_t;

// forward declarations, type and symbol are defined in other modules
typedef struct ast_node ast_node_t;
typedef struct type type_t;
typedef struct symbol symbol_t;

// list of AST nodes
typedef struct {
  ast_node_t **items;
  int count;
  int capacity;
} ast_list_t;

// type and symbol fields are filled in by semantics, parser leaves them NULL
struct ast_node {
  ast_node_kind_t kind;
  int line;
  type_t *type; // result type of expression

  union {
    struct {
      ast_list_t defs;
    } program;

    struct {
      char *name;
      bool is_main;
      type_t *ret_type; // NULL if not given
      ast_list_t *param_lists;
      int param_list_count; // 0 = def answer, 1 = def foo()
      ast_node_t *body;     // always BLOCK
      symbol_t *symbol;
    } func_def;

    struct {
      char *name;
      type_t *decl_type;
      symbol_t *symbol;
    } param;

    struct {
      ast_list_t stmts;
    } block;

    struct {
      bool is_mutable; // var = true, val = false
      char *name;
      type_t *decl_type; // NULL if not given
      ast_node_t *init;
      symbol_t *symbol;
    } var_def;

    struct {
      char *name;
      ast_node_t *value;
      symbol_t *symbol;
    } assign;

    struct {
      ast_node_t *cond;
      ast_node_t *then_branch; // always BLOCK
      ast_node_t *else_branch; // BLOCK, NULL without else
    } if_stmt;

    struct {
      ast_node_t *cond;
      ast_node_t *body; // always BLOCK
    } while_stmt;

    struct {
      ast_node_t *value; // NULL for bare return
    } return_stmt;

    // valid field is given by node->type
    struct {
      int64_t int_val;
      double double_val;
      char *string_val;
      bool bool_val;
    } literal;

    struct {
      char *name;
      symbol_t *symbol;
    } id;

    struct {
      ast_op_t op;
      ast_node_t *left;
      ast_node_t *right;
    } binary;

    struct {
      ast_op_t op;
      ast_node_t *operand;
    } unary;

    struct {
      char *name;
      ast_list_t *arg_lists; // NULL if count is 0
      int arg_list_count;    // 0 = answer, 1 = foo()
      symbol_t *symbol;
    } call;
  };
};

// constructors take over strings, nodes and lists, ast_free releases them
ast_node_t *ast_new_program(int line);
ast_node_t *ast_new_func_def(char *name, bool is_main, type_t *ret_type,
                             ast_list_t *param_lists, int param_list_count,
                             ast_node_t *body, int line);
ast_node_t *ast_new_param(char *name, type_t *decl_type, int line);
ast_node_t *ast_new_block(int line);
ast_node_t *ast_new_var_def(bool is_mutable, char *name, type_t *decl_type,
                            ast_node_t *init, int line);
ast_node_t *ast_new_assign(char *name, ast_node_t *value, int line);
ast_node_t *ast_new_if(ast_node_t *cond, ast_node_t *then_branch,
                       ast_node_t *else_branch, int line);
ast_node_t *ast_new_while(ast_node_t *cond, ast_node_t *body, int line);
ast_node_t *ast_new_return(ast_node_t *value, int line);

// literals, each sets node->type
ast_node_t *ast_new_int(int64_t value, int line);
ast_node_t *ast_new_double(double value, int line);
ast_node_t *ast_new_string(char *value, int line);
ast_node_t *ast_new_bool(bool value, int line);
ast_node_t *ast_new_unit(int line);

ast_node_t *ast_new_id(char *name, int line);
ast_node_t *ast_new_binary(ast_op_t op, ast_node_t *left, ast_node_t *right,
                           int line);
ast_node_t *ast_new_unary(ast_op_t op, ast_node_t *operand, int line);
ast_node_t *ast_new_call(char *name, ast_list_t *arg_lists, int arg_list_count,
                         int line);

bool ast_list_append(ast_list_t *list, ast_node_t *node);

// adds new empty list to array of lists and returns it, NULL on failure
// returned pointer is valid only until the next call
ast_list_t *ast_lists_add(ast_list_t **lists, int *count);

// releases node with its whole subtree, NULL is allowed
void ast_free(ast_node_t *node);

#endif // AST_H

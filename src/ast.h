/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    ast.h
 * Authors: Michal Řezníček (xreznim00), Adam James Fisher (xfishea00)
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
  AST_FOR,
  AST_RANGE,
  AST_LIST_LIT,
  AST_LIST_RANGE,
  AST_MEMBER,
  AST_TYPE_ALIAS,
  AST_CASE_CLASS,
  AST_MATCH,
  AST_CASE,
  AST_PAT_TYPED,
  AST_PAT_CLASS,
  AST_PAT_WILDCARD,
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
  OP_CONCAT,
} ast_op_t;

// kinds of literal
typedef enum {
  LIT_INT,
  LIT_DOUBLE,
  LIT_STRING,
  LIT_BOOL,
  LIT_UNIT,
} ast_literal_kind_t;

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

    // valid field is given by kind
    struct {
      ast_literal_kind_t kind;
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

    struct {
      char *var_name;
      ast_node_t *source; // RANGE or List expression
      ast_list_t guards;
      ast_node_t *body; // always BLOCK
      symbol_t *symbol;
    } for_stmt;

    struct {
      bool is_until; // false = to, true = until
      ast_node_t *from;
      ast_node_t *end;
      ast_node_t *step; // NULL if not given
    } range;

    struct {
      ast_list_t items;
    } list_lit;

    struct {
      ast_node_t *start;
      ast_node_t *end;
      ast_node_t *step; // NULL if not given
    } list_range;

    struct {
      char *name;
      ast_node_t *object;
      bool has_parens; // lst.drop(2) = true, lst.head = false
      ast_list_t args; // empty without parentheses
    } member;

    struct {
      char *name;
      type_t *aliased;
      symbol_t *symbol;
    } type_alias;

    struct {
      char *name;
      ast_list_t attrs;
      symbol_t *symbol;
    } case_class;

    struct {
      ast_node_t *subject;
      ast_list_t cases;
    } match;

    struct {
      ast_node_t *pattern; // LITERAL, PAT_TYPED, PAT_CLASS or PAT_WILDCARD
      ast_node_t *guard;   // NULL if not given
      ast_node_t *body;    // always BLOCK
    } case_branch;

    struct {
      char *name;
      type_t *decl_type;
      symbol_t *symbol;
    } pat_typed;

    struct {
      char *name;
      ast_list_t bindings; // ID nodes, name NULL for _
    } pat_class;
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
ast_node_t *ast_new_for(char *var_name, ast_node_t *source, ast_list_t guards,
                        ast_node_t *body, int line);
ast_node_t *ast_new_range(bool is_until, ast_node_t *from, ast_node_t *end,
                          ast_node_t *step, int line);
ast_node_t *ast_new_list_lit(ast_list_t items, int line);
ast_node_t *ast_new_list_range(ast_node_t *start, ast_node_t *end,
                               ast_node_t *step, int line);
ast_node_t *ast_new_member(char *name, ast_node_t *object, bool has_parens,
                           ast_list_t args, int line);
ast_node_t *ast_new_type_alias(char *name, type_t *aliased, int line);
ast_node_t *ast_new_case_class(char *name, ast_list_t attrs, int line);
ast_node_t *ast_new_match(ast_node_t *subject, ast_list_t cases, int line);
ast_node_t *ast_new_case(ast_node_t *pattern, ast_node_t *guard,
                         ast_node_t *body, int line);
ast_node_t *ast_new_pat_typed(char *name, type_t *decl_type, int line);
ast_node_t *ast_new_pat_class(char *name, ast_list_t bindings, int line);
ast_node_t *ast_new_pat_wildcard(int line);

bool ast_list_append(ast_list_t *list, ast_node_t *node);

// adds new empty list to array of lists and returns it, NULL on failure
// returned pointer is valid only until the next call
ast_list_t *ast_lists_add(ast_list_t **lists, int *count);

// releases node with its whole subtree, NULL is allowed
void ast_free(ast_node_t *node);

// prints the tree to stdout
void ast_print(const ast_node_t *node);

#endif // AST_H

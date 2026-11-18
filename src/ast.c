/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    ast.c
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   abstract syntax tree
 */

#include "ast.h"
#include "type.h"

#include <stdlib.h>

#define LIST_INIT_CAPACITY 4

// AI-generated skeleton (Claude Opus 5.5): node_new, ast_new_while and the
// structure of ast_free; the rest written by hand.
// Prompt: "dej mi kostru pomocne funkce node_new, jednoho konstruktoru jako
//          priklad a struktury ast_free"

// allocates zeroed node of given kind
static ast_node_t *node_new(ast_node_kind_t kind, int line) {
  ast_node_t *node = calloc(1, sizeof(ast_node_t));
  if (node == NULL) {
    return NULL;
  }
  node->kind = kind;
  node->line = line;
  return node;
}

ast_node_t *ast_new_program(int line) { return node_new(AST_PROGRAM, line); }

ast_node_t *ast_new_func_def(char *name, bool is_main, type_t *ret_type,
                             ast_list_t *param_lists, int param_list_count,
                             ast_node_t *body, int line) {
  ast_node_t *node = node_new(AST_FUNC_DEF, line);
  if (node != NULL) {
    node->func_def.name = name;
    node->func_def.is_main = is_main;
    node->func_def.ret_type = ret_type;
    node->func_def.param_lists = param_lists;
    node->func_def.param_list_count = param_list_count;
    node->func_def.body = body;
  }
  return node;
}

ast_node_t *ast_new_param(char *name, type_t *decl_type, int line) {
  ast_node_t *node = node_new(AST_PARAM, line);
  if (node != NULL) {
    node->param.name = name;
    node->param.decl_type = decl_type;
  }
  return node;
}

ast_node_t *ast_new_block(int line) { return node_new(AST_BLOCK, line); }

ast_node_t *ast_new_var_def(bool is_mutable, char *name, type_t *decl_type,
                            ast_node_t *init, int line) {
  ast_node_t *node = node_new(AST_VAR_DEF, line);
  if (node != NULL) {
    node->var_def.is_mutable = is_mutable;
    node->var_def.name = name;
    node->var_def.decl_type = decl_type;
    node->var_def.init = init;
  }
  return node;
}

ast_node_t *ast_new_assign(char *name, ast_node_t *value, int line) {
  ast_node_t *node = node_new(AST_ASSIGN, line);
  if (node != NULL) {
    node->assign.name = name;
    node->assign.value = value;
  }
  return node;
}

ast_node_t *ast_new_if(ast_node_t *cond, ast_node_t *then_branch,
                       ast_node_t *else_branch, int line) {
  ast_node_t *node = node_new(AST_IF, line);
  if (node != NULL) {
    node->if_stmt.cond = cond;
    node->if_stmt.then_branch = then_branch;
    node->if_stmt.else_branch = else_branch;
  }
  return node;
}

ast_node_t *ast_new_while(ast_node_t *cond, ast_node_t *body, int line) {
  ast_node_t *node = node_new(AST_WHILE, line);
  if (node != NULL) {
    node->while_stmt.cond = cond;
    node->while_stmt.body = body;
  }
  return node;
}

ast_node_t *ast_new_return(ast_node_t *value, int line) {
  ast_node_t *node = node_new(AST_RETURN, line);
  if (node != NULL) {
    node->return_stmt.value = value;
  }
  return node;
}

ast_node_t *ast_new_int(int64_t value, int line) {
  ast_node_t *node = node_new(AST_LITERAL, line);
  if (node != NULL) {
    node->literal.kind = LIT_INT;
    node->literal.int_val = value;
    node->type = type_int();
  }
  return node;
}

ast_node_t *ast_new_double(double value, int line) {
  ast_node_t *node = node_new(AST_LITERAL, line);
  if (node != NULL) {
    node->literal.kind = LIT_DOUBLE;
    node->literal.double_val = value;
    node->type = type_double();
  }
  return node;
}

ast_node_t *ast_new_string(char *value, int line) {
  ast_node_t *node = node_new(AST_LITERAL, line);
  if (node != NULL) {
    node->literal.kind = LIT_STRING;
    node->literal.string_val = value;
    node->type = type_string();
  }
  return node;
}

ast_node_t *ast_new_bool(bool value, int line) {
  ast_node_t *node = node_new(AST_LITERAL, line);
  if (node != NULL) {
    node->literal.kind = LIT_BOOL;
    node->literal.bool_val = value;
    node->type = type_bool();
  }
  return node;
}

ast_node_t *ast_new_unit(int line) {
  ast_node_t *node = node_new(AST_LITERAL, line);
  if (node != NULL) {
    node->literal.kind = LIT_UNIT;
    node->type = type_unit();
  }
  return node;
}

ast_node_t *ast_new_id(char *name, int line) {
  ast_node_t *node = node_new(AST_ID, line);
  if (node != NULL) {
    node->id.name = name;
  }
  return node;
}

ast_node_t *ast_new_binary(ast_op_t op, ast_node_t *left, ast_node_t *right,
                           int line) {
  ast_node_t *node = node_new(AST_BINARY, line);
  if (node != NULL) {
    node->binary.op = op;
    node->binary.left = left;
    node->binary.right = right;
  }
  return node;
}

ast_node_t *ast_new_unary(ast_op_t op, ast_node_t *operand, int line) {
  ast_node_t *node = node_new(AST_UNARY, line);
  if (node != NULL) {
    node->unary.op = op;
    node->unary.operand = operand;
  }
  return node;
}

ast_node_t *ast_new_call(char *name, ast_list_t *arg_lists, int arg_list_count,
                         int line) {
  ast_node_t *node = node_new(AST_CALL, line);
  if (node != NULL) {
    node->call.name = name;
    node->call.arg_lists = arg_lists;
    node->call.arg_list_count = arg_list_count;
  }
  return node;
}

ast_node_t *ast_new_for(char *var_name, ast_node_t *source, ast_list_t guards,
                        ast_node_t *body, int line) {
  ast_node_t *node = node_new(AST_FOR, line);
  if (node != NULL) {
    node->for_stmt.var_name = var_name;
    node->for_stmt.source = source;
    node->for_stmt.guards = guards;
    node->for_stmt.body = body;
  }
  return node;
}

ast_node_t *ast_new_range(bool is_until, ast_node_t *from, ast_node_t *end,
                          ast_node_t *step, int line) {
  ast_node_t *node = node_new(AST_RANGE, line);
  if (node != NULL) {
    node->range.is_until = is_until;
    node->range.from = from;
    node->range.end = end;
    node->range.step = step;
  }
  return node;
}

ast_node_t *ast_new_list_lit(ast_list_t items, int line) {
  ast_node_t *node = node_new(AST_LIST_LIT, line);
  if (node != NULL) {
    node->list_lit.items = items;
  }
  return node;
}

ast_node_t *ast_new_list_range(ast_node_t *start, ast_node_t *end,
                               ast_node_t *step, int line) {
  ast_node_t *node = node_new(AST_LIST_RANGE, line);
  if (node != NULL) {
    node->list_range.start = start;
    node->list_range.end = end;
    node->list_range.step = step;
  }
  return node;
}

ast_node_t *ast_new_member(char *name, ast_node_t *object, bool has_parens,
                           ast_list_t args, int line) {
  ast_node_t *node = node_new(AST_MEMBER, line);
  if (node != NULL) {
    node->member.name = name;
    node->member.object = object;
    node->member.has_parens = has_parens;
    node->member.args = args;
  }
  return node;
}

ast_node_t *ast_new_type_alias(char *name, type_t *aliased, int line) {
  ast_node_t *node = node_new(AST_TYPE_ALIAS, line);
  if (node != NULL) {
    node->type_alias.name = name;
    node->type_alias.aliased = aliased;
  }
  return node;
}

ast_node_t *ast_new_case_class(char *name, ast_list_t attrs, int line) {
  ast_node_t *node = node_new(AST_CASE_CLASS, line);
  if (node != NULL) {
    node->case_class.name = name;
    node->case_class.attrs = attrs;
  }
  return node;
}

ast_node_t *ast_new_match(ast_node_t *subject, ast_list_t cases, int line) {
  ast_node_t *node = node_new(AST_MATCH, line);
  if (node != NULL) {
    node->match.subject = subject;
    node->match.cases = cases;
  }
  return node;
}

ast_node_t *ast_new_case(ast_node_t *pattern, ast_node_t *guard,
                         ast_node_t *body, int line) {
  ast_node_t *node = node_new(AST_CASE, line);
  if (node != NULL) {
    node->case_branch.pattern = pattern;
    node->case_branch.guard = guard;
    node->case_branch.body = body;
  }
  return node;
}

ast_node_t *ast_new_pat_typed(char *name, type_t *decl_type, int line) {
  ast_node_t *node = node_new(AST_PAT_TYPED, line);
  if (node != NULL) {
    node->pat_typed.name = name;
    node->pat_typed.decl_type = decl_type;
  }
  return node;
}

ast_node_t *ast_new_pat_class(char *name, ast_list_t bindings, int line) {
  ast_node_t *node = node_new(AST_PAT_CLASS, line);
  if (node != NULL) {
    node->pat_class.name = name;
    node->pat_class.bindings = bindings;
  }
  return node;
}

ast_node_t *ast_new_pat_wildcard(int line) {
  ast_node_t *node = node_new(AST_PAT_WILDCARD, line);
  return node;
}

bool ast_list_append(ast_list_t *list, ast_node_t *node) {
  if (list->count >= list->capacity) {
    int new_capacity = list->capacity * 2;
    if (new_capacity == 0) {
      new_capacity = LIST_INIT_CAPACITY;
    }
    ast_node_t **new_items =
        realloc(list->items, new_capacity * sizeof(ast_node_t *));
    if (new_items == NULL) {
      return false;
    }
    list->items = new_items;
    list->capacity = new_capacity;
  }
  list->items[list->count++] = node;
  return true;
}

ast_list_t *ast_lists_add(ast_list_t **lists, int *count) {
  ast_list_t *new_lists = realloc(*lists, (*count + 1) * sizeof(ast_list_t));
  if (new_lists == NULL) {
    return NULL;
  }
  *lists = new_lists;
  ast_list_t *new_list = &(*lists)[*count];
  new_list->items = NULL;
  new_list->count = 0;
  new_list->capacity = 0;
  (*count)++;
  return new_list;
}

// releases nodes of list and its items, not list itself
static void list_free(ast_list_t *list) {
  if (list == NULL) {
    return;
  }
  for (int i = 0; i < list->count; i++) {
    ast_free(list->items[i]);
  }
  free(list->items);
}

// types and symbols are not freed here
void ast_free(ast_node_t *node) {
  if (node == NULL) {
    return;
  }
  switch (node->kind) {
  case AST_PROGRAM:
    list_free(&node->program.defs);
    break;

  case AST_FUNC_DEF:
    free(node->func_def.name);
    for (int i = 0; i < node->func_def.param_list_count; i++) {
      list_free(&node->func_def.param_lists[i]);
    }
    free(node->func_def.param_lists);
    ast_free(node->func_def.body);
    break;

  case AST_PARAM:
    free(node->param.name);
    break;

  case AST_BLOCK:
    list_free(&node->block.stmts);
    break;

  case AST_VAR_DEF:
    free(node->var_def.name);
    ast_free(node->var_def.init);
    break;

  case AST_ASSIGN:
    free(node->assign.name);
    ast_free(node->assign.value);
    break;

  case AST_IF:
    ast_free(node->if_stmt.cond);
    ast_free(node->if_stmt.then_branch);
    ast_free(node->if_stmt.else_branch);
    break;

  case AST_WHILE:
    ast_free(node->while_stmt.cond);
    ast_free(node->while_stmt.body);
    break;

  case AST_RETURN:
    ast_free(node->return_stmt.value);
    break;

  case AST_LITERAL:
    free(node->literal.string_val);
    break;

  case AST_ID:
    free(node->id.name);
    break;

  case AST_BINARY:
    ast_free(node->binary.left);
    ast_free(node->binary.right);
    break;

  case AST_UNARY:
    ast_free(node->unary.operand);
    break;

  case AST_CALL:
    for (int i = 0; i < node->call.arg_list_count; i++) {
      list_free(&node->call.arg_lists[i]);
    }
    free(node->call.arg_lists);
    free(node->call.name);
    break;

  case AST_FOR:
    free(node->for_stmt.var_name);
    ast_free(node->for_stmt.source);
    list_free(&node->for_stmt.guards);
    ast_free(node->for_stmt.body);
    break;

  case AST_RANGE:
    ast_free(node->range.from);
    ast_free(node->range.end);
    ast_free(node->range.step);
    break;

  case AST_LIST_LIT:
    list_free(&node->list_lit.items);
    break;

  case AST_LIST_RANGE:
    ast_free(node->list_range.start);
    ast_free(node->list_range.end);
    ast_free(node->list_range.step);
    break;

  case AST_MEMBER:
    free(node->member.name);
    ast_free(node->member.object);
    list_free(&node->member.args);
    break;

  case AST_TYPE_ALIAS:
    free(node->type_alias.name);
    break;

  case AST_CASE_CLASS:
    free(node->case_class.name);
    list_free(&node->case_class.attrs);
    break;

  case AST_MATCH:
    ast_free(node->match.subject);
    list_free(&node->match.cases);
    break;

  case AST_CASE:
    ast_free(node->case_branch.pattern);
    ast_free(node->case_branch.guard);
    ast_free(node->case_branch.body);
    break;

  case AST_PAT_TYPED:
    free(node->pat_typed.name);
    break;

  case AST_PAT_CLASS:
    free(node->pat_class.name);
    list_free(&node->pat_class.bindings);
    break;

  case AST_PAT_WILDCARD:
    break;
  }
  free(node);
}

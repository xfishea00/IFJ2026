/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    ast.c
 * Authors: Michal Řezníček (xreznim00), Adam James Fisher (xfishea00)
 * Brief:   abstract syntax tree
 */

#include "ast.h"
#include "type.h"

#include <stdio.h>
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

static void print_indent(int depth) {
  for (int i = 0; i < depth; i++) {
    fputs("  ", stdout);
  }
}

static void print_basic_type(const type_t *type) {
  if (type == NULL) {
    fputs("<inferred>", stdout);
    return;
  }
  switch (type->kind) {
  case T_INT:
    fputs("Int", stdout);
    break;
  case T_DOUBLE:
    fputs("Double", stdout);
    break;
  case T_STRING:
    fputs("String", stdout);
    break;
  case T_BOOL:
    fputs("Boolean", stdout);
    break;
  case T_UNIT:
    fputs("Unit", stdout);
    break;
  default:
    fputs("<non-basic>", stdout);
    break;
  }
}

static const char *operator_name(ast_op_t op) {
  switch (op) {
  case OP_ADD:
    return "+";
  case OP_SUB:
    return "-";
  case OP_MUL:
    return "*";
  case OP_DIV:
    return "/";
  case OP_LT:
    return "<";
  case OP_GT:
    return ">";
  case OP_LE:
    return "<=";
  case OP_GE:
    return ">=";
  case OP_EQ:
    return "==";
  case OP_NEQ:
    return "!=";
  case OP_AND:
    return "&&";
  case OP_OR:
    return "||";
  case OP_NOT:
    return "!";
  case OP_NEG:
    return "unary -";
  case OP_CONCAT:
    return "++";
  }
  return "unknown";
}

static void print_string(const char *value) {
  putchar('"');
  for (const unsigned char *p = (const unsigned char *)value; *p != '\0'; p++) {
    switch (*p) {
    case '\\':
      fputs("\\\\", stdout);
      break;
    case '"':
      fputs("\\\"", stdout);
      break;
    case '\n':
      fputs("\\n", stdout);
      break;
    case '\r':
      fputs("\\r", stdout);
      break;
    case '\t':
      fputs("\\t", stdout);
      break;
    default:
      putchar(*p);
      break;
    }
  }
  putchar('"');
}

static void print_node(const ast_node_t *node, int depth, const char *label) {
  if (node == NULL) {
    return;
  }
  print_indent(depth);
  if (label != NULL) {
    printf("%s: ", label);
  }

  switch (node->kind) {
  case AST_PROGRAM:
    printf("Program (line %d)\n", node->line);
    for (int i = 0; i < node->program.defs.count; i++) {
      print_node(node->program.defs.items[i], depth + 1, NULL);
    }
    break;
  case AST_FUNC_DEF:
    printf("Function %s%s", node->func_def.name,
           node->func_def.is_main ? " [main]" : "");
    if (node->func_def.ret_type != NULL) {
      fputs(" -> ", stdout);
      print_basic_type(node->func_def.ret_type);
    }
    printf(" (line %d)\n", node->line);
    for (int i = 0; i < node->func_def.param_list_count; i++) {
      print_indent(depth + 1);
      printf("Parameters (group %d)\n", i + 1);
      for (int j = 0; j < node->func_def.param_lists[i].count; j++) {
        print_node(node->func_def.param_lists[i].items[j], depth + 2, NULL);
      }
    }
    print_node(node->func_def.body, depth + 1, "Body");
    break;
  case AST_PARAM:
    printf("Parameter %s: ", node->param.name);
    print_basic_type(node->param.decl_type);
    printf(" (line %d)\n", node->line);
    break;
  case AST_BLOCK:
    printf("Block (line %d)\n", node->line);
    for (int i = 0; i < node->block.stmts.count; i++) {
      print_node(node->block.stmts.items[i], depth + 1, NULL);
    }
    break;
  case AST_VAR_DEF:
    printf("%s %s", node->var_def.is_mutable ? "Var" : "Val",
           node->var_def.name);
    if (node->var_def.decl_type != NULL) {
      fputs(": ", stdout);
      print_basic_type(node->var_def.decl_type);
    }
    printf(" (line %d)\n", node->line);
    print_node(node->var_def.init, depth + 1, "Initializer");
    break;
  case AST_ASSIGN:
    printf("Assignment to %s (line %d)\n", node->assign.name, node->line);
    print_node(node->assign.value, depth + 1, "Value");
    break;
  case AST_IF:
    printf("If (line %d)\n", node->line);
    print_node(node->if_stmt.cond, depth + 1, "Condition");
    print_node(node->if_stmt.then_branch, depth + 1, "Then");
    print_node(node->if_stmt.else_branch, depth + 1, "Else");
    break;
  case AST_WHILE:
    printf("While (line %d)\n", node->line);
    print_node(node->while_stmt.cond, depth + 1, "Condition");
    print_node(node->while_stmt.body, depth + 1, "Body");
    break;
  case AST_RETURN:
    printf("Return (line %d)\n", node->line);
    print_node(node->return_stmt.value, depth + 1, "Value");
    break;
  case AST_LITERAL:
    switch (node->literal.kind) {
    case LIT_INT:
      printf("Integer %lld (line %d)\n", (long long)node->literal.int_val,
             node->line);
      break;
    case LIT_DOUBLE:
      printf("Double %g (line %d)\n", node->literal.double_val, node->line);
      break;
    case LIT_STRING:
      printf("String ");
      print_string(node->literal.string_val);
      printf(" (line %d)\n", node->line);
      break;
    case LIT_BOOL:
      printf("Boolean %s (line %d)\n",
             node->literal.bool_val ? "true" : "false", node->line);
      break;
    case LIT_UNIT:
      printf("Unit (line %d)\n", node->line);
      break;
    }
    break;
  case AST_ID:
    printf("Identifier %s (line %d)\n",
           node->id.name != NULL ? node->id.name : "_", node->line);
    break;
  case AST_BINARY:
    printf("Binary %s (line %d)\n", operator_name(node->binary.op), node->line);
    print_node(node->binary.left, depth + 1, "Left");
    print_node(node->binary.right, depth + 1, "Right");
    break;
  case AST_UNARY:
    printf("Unary %s (line %d)\n", operator_name(node->unary.op), node->line);
    print_node(node->unary.operand, depth + 1, "Operand");
    break;
  case AST_CALL:
    printf("Call %s (line %d)\n", node->call.name, node->line);
    for (int i = 0; i < node->call.arg_list_count; i++) {
      print_indent(depth + 1);
      printf("Arguments (group %d)\n", i + 1);
      for (int j = 0; j < node->call.arg_lists[i].count; j++) {
        print_node(node->call.arg_lists[i].items[j], depth + 2, NULL);
      }
    }
    break;
  case AST_FOR:
    printf("For %s (line %d)\n", node->for_stmt.var_name, node->line);
    print_node(node->for_stmt.source, depth + 1, "Source");
    for (int i = 0; i < node->for_stmt.guards.count; i++) {
      print_node(node->for_stmt.guards.items[i], depth + 1, "Guard");
    }
    print_node(node->for_stmt.body, depth + 1, "Body");
    break;
  case AST_RANGE:
    printf("Range (%s) (line %d)\n", node->range.is_until ? "until" : "to",
           node->line);
    print_node(node->range.from, depth + 1, "From");
    print_node(node->range.end, depth + 1, "End");
    print_node(node->range.step, depth + 1, "Step");
    break;
  case AST_LIST_LIT:
    printf("List literal (line %d)\n", node->line);
    for (int i = 0; i < node->list_lit.items.count; i++) {
      print_node(node->list_lit.items.items[i], depth + 1, NULL);
    }
    break;
  case AST_LIST_RANGE:
    printf("List range (line %d)\n", node->line);
    print_node(node->list_range.start, depth + 1, "Start");
    print_node(node->list_range.end, depth + 1, "End");
    print_node(node->list_range.step, depth + 1, "Step");
    break;
  case AST_MEMBER:
    printf("Member %s%s (line %d)\n", node->member.name,
           node->member.has_parens ? "()" : "", node->line);
    print_node(node->member.object, depth + 1, "Object");
    for (int i = 0; i < node->member.args.count; i++) {
      print_node(node->member.args.items[i], depth + 1, "Argument");
    }
    break;
  case AST_TYPE_ALIAS:
    printf("Type alias %s (line %d)\n", node->type_alias.name, node->line);
    break;
  case AST_CASE_CLASS:
    printf("Case class %s (line %d)\n", node->case_class.name, node->line);
    for (int i = 0; i < node->case_class.attrs.count; i++) {
      print_node(node->case_class.attrs.items[i], depth + 1, NULL);
    }
    break;
  case AST_MATCH:
    printf("Match (line %d)\n", node->line);
    print_node(node->match.subject, depth + 1, "Subject");
    for (int i = 0; i < node->match.cases.count; i++) {
      print_node(node->match.cases.items[i], depth + 1, NULL);
    }
    break;
  case AST_CASE:
    printf("Case (line %d)\n", node->line);
    print_node(node->case_branch.pattern, depth + 1, "Pattern");
    print_node(node->case_branch.guard, depth + 1, "Guard");
    print_node(node->case_branch.body, depth + 1, "Body");
    break;
  case AST_PAT_TYPED:
    printf("Typed pattern %s (line %d)\n", node->pat_typed.name, node->line);
    break;
  case AST_PAT_CLASS:
    printf("Class pattern %s (line %d)\n", node->pat_class.name, node->line);
    for (int i = 0; i < node->pat_class.bindings.count; i++) {
      print_node(node->pat_class.bindings.items[i], depth + 1, NULL);
    }
    break;
  case AST_PAT_WILDCARD:
    printf("Wildcard pattern (line %d)\n", node->line);
    break;
  }
}

//AI-Generated Code
//Applies to all functions related to printing the AST tree for debugging purposes
//Prompt: can you make a function that displays the structure of the finished ast in the terminal
void ast_print(const ast_node_t *node) { print_node(node, 0, NULL); }
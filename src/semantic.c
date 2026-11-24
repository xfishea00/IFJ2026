/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    semantic.c
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   semantic analysis
 */

#include "semantic.h"
#include "error.h"
#include "symbol.h"
#include "type.h"

#include <stdbool.h>
#include <string.h>

static int analyze_function(symbol_t *func);
static int sem_block(ast_node_t *block);
static int sem_stmt(ast_node_t *stmt);
static int sem_match(ast_node_t *match);
static int sem_expr(ast_node_t *expr);

static symbol_t *current_function;

// defines all functions, classes and aliases in the global scope
static int collect_definitions(ast_node_t *program) {
  for (int i = 0; i < program->program.defs.count; i++) {
    ast_node_t *def = program->program.defs.items[i];

    switch (def->kind) {
    case AST_FUNC_DEF: {
      symbol_t *sym = symbol_new(SYM_FUNC, def->func_def.name, def);
      if (sym == NULL) {
        return ERR_INTERNAL;
      }
      int rc = scope_define(sym);
      if (rc != ERR_OK) {
        return rc;
      }
      def->func_def.symbol = sym;
      break;
    }

    case AST_TYPE_ALIAS: {
      symbol_t *sym = symbol_new(SYM_ALIAS, def->type_alias.name, def);
      if (sym == NULL) {
        return ERR_INTERNAL;
      }
      int rc = scope_define(sym);
      if (rc != ERR_OK) {
        return rc;
      }
      def->type_alias.symbol = sym;
      break;
    }

    case AST_CASE_CLASS: {
      ast_list_t *attrs = &def->case_class.attrs;
      for (int a = 0; a < attrs->count; a++) {
        for (int b = a + 1; b < attrs->count; b++) {
          if (strcmp(attrs->items[a]->param.name,
                     attrs->items[b]->param.name) == 0) {
            return ERR_SEM_REDEF; // attribute names must be unique
          }
        }
      }
      symbol_t *sym = symbol_new(SYM_CLASS, def->case_class.name, def);
      if (sym == NULL) {
        return ERR_INTERNAL;
      }
      int rc = scope_define(sym);
      if (rc != ERR_OK) {
        return rc;
      }
      def->case_class.symbol = sym;
      break;
    }

    default:
      break;
    }
  }
  return ERR_OK;
}

// exactly one @main, no parameters
static int check_main(ast_node_t *program) {
  int main_count = 0;

  for (int i = 0; i < program->program.defs.count; i++) {
    ast_node_t *def = program->program.defs.items[i];
    if (def->kind != AST_FUNC_DEF || !def->func_def.is_main) {
      continue; // not @main
    }
    main_count++;
    if (main_count > 1) {
      return ERR_SEM_REDEF; // more than one @main
    }
    for (int j = 0; j < def->func_def.param_list_count; j++) {
      if (def->func_def.param_lists[j].count > 0) {
        return ERR_SEM_ARGS; // @main has parameters
      }
    }
    type_t *ret = def->func_def.ret_type;
    if (ret != NULL && ret->kind != T_UNIT) {
      return ERR_SEM_TYPE;
    }
  }
  if (main_count == 0) {
    return ERR_SEM_UNDEF; // no @main
  }
  return ERR_OK;
}

// T_NAMED must be a class or alias
static int check_type(type_t *type) {
  if (type == NULL || type->kind != T_NAMED) {
    return ERR_OK;
  }
  symbol_t *sym = scope_lookup(type->name);
  if (sym == NULL || (sym->kind != SYM_CLASS && sym->kind != SYM_ALIAS)) {
    return ERR_SEM_UNDEF;
  }
  return ERR_OK;
}

// analyzes function body
static int analyze_function(symbol_t *func) {
  symbol_t *saved_function = current_function;
  current_function = func;
  func->state = FUNC_IN_PROGRESS;
  scope_t *saved = scope_enter_global();
  if (!scope_push()) {
    scope_restore(saved);
    return ERR_INTERNAL;
  }
  int rc = check_type(func->decl->func_def.ret_type);

  for (int i = 0; rc == ERR_OK && i < func->decl->func_def.param_list_count;
       i++) {
    ast_list_t *param_list = &func->decl->func_def.param_lists[i];
    for (int j = 0; rc == ERR_OK && j < param_list->count; j++) {
      ast_node_t *param = param_list->items[j];
      rc = check_type(param->param.decl_type);
      if (rc != ERR_OK) {
        break;
      }
      symbol_t *sym = symbol_new(SYM_VAR, param->param.name, param);
      if (sym == NULL) {
        rc = ERR_INTERNAL;
        break;
      }
      sym->is_mutable = false; // parameters are immutable
      sym->type = param->param.decl_type;
      rc = scope_define(sym);
      if (rc != ERR_OK) {
        break;
      }
      param->param.symbol = sym;
    }
  }

  // body in the same scope as parameters
  ast_list_t *stmts = &func->decl->func_def.body->block.stmts;
  for (int i = 0; rc == ERR_OK && i < stmts->count; i++) {
    rc = sem_stmt(stmts->items[i]);
  }
  scope_pop();
  scope_restore(saved);
  current_function = saved_function;
  if (rc == ERR_OK) {
    func->state = FUNC_DONE;
  } else {
    func->state = FUNC_NOT_DONE;
  }
  return rc;
}

// block with its own scope
static int sem_block(ast_node_t *block) {
  if (!scope_push()) {
    return ERR_INTERNAL;
  }
  int rc = ERR_OK;

  for (int i = 0; rc == ERR_OK && i < block->block.stmts.count; i++) {
    rc = sem_stmt(block->block.stmts.items[i]);
  }
  scope_pop();
  return rc;
}

// checks a statement
static int sem_stmt(ast_node_t *stmt) {
  switch (stmt->kind) {
  case AST_VAR_DEF: {
    // init first, val x = x + 1 uses outer x
    int rc = sem_expr(stmt->var_def.init);
    if (rc != ERR_OK) {
      return rc;
    }
    rc = check_type(stmt->var_def.decl_type);
    if (rc != ERR_OK) {
      return rc;
    }
    // init must match the declared type
    if (stmt->var_def.decl_type != NULL && stmt->var_def.init->type != NULL &&
        !type_compatible(stmt->var_def.decl_type, stmt->var_def.init->type)) {
      return ERR_SEM_TYPE;
    }

    symbol_t *sym = symbol_new(SYM_VAR, stmt->var_def.name, stmt);
    if (sym == NULL) {
      return ERR_INTERNAL;
    }
    sym->is_mutable = stmt->var_def.is_mutable;
    if (stmt->var_def.decl_type != NULL) {
      sym->type = stmt->var_def.decl_type;
    } else {
      sym->type = stmt->var_def.init->type;
    }
    rc = scope_define(sym);
    if (rc != ERR_OK) {
      return rc;
    }
    stmt->var_def.symbol = sym;
    return ERR_OK;
  }

  case AST_ASSIGN: {
    int rc = sem_expr(stmt->assign.value);
    if (rc != ERR_OK) {
      return rc;
    }
    symbol_t *sym = scope_lookup(stmt->assign.name);
    if (sym == NULL) {
      return ERR_SEM_UNDEF;
    }
    if (sym->kind != SYM_VAR || !sym->is_mutable) {
      return ERR_SEM_IMMUTABLE;
    }
    // value must match the type of the variable
    if (sym->type != NULL && stmt->assign.value->type != NULL &&
        !type_compatible(sym->type, stmt->assign.value->type)) {
      return ERR_SEM_TYPE;
    }
    stmt->assign.symbol = sym;
    return ERR_OK;
  }

  case AST_IF: {
    int rc = sem_expr(stmt->if_stmt.cond);
    if (rc != ERR_OK) {
      return rc;
    }
    rc = sem_block(stmt->if_stmt.then_branch);
    if (rc != ERR_OK) {
      return rc;
    }
    if (stmt->if_stmt.else_branch != NULL) {
      rc = sem_block(stmt->if_stmt.else_branch);
      if (rc != ERR_OK) {
        return rc;
      }
    }
    return ERR_OK;
  }

  case AST_WHILE: {
    int rc = sem_expr(stmt->while_stmt.cond);
    if (rc != ERR_OK) {
      return rc;
    }
    rc = sem_block(stmt->while_stmt.body);
    if (rc != ERR_OK) {
      return rc;
    }
    return ERR_OK;
  }

  case AST_FOR: {
    // source first, control variable is not visible yet
    int rc = sem_expr(stmt->for_stmt.source);
    if (rc != ERR_OK) {
      return rc;
    }
    if (!scope_push()) {
      return ERR_INTERNAL;
    }
    symbol_t *sym = symbol_new(SYM_VAR, stmt->for_stmt.var_name, stmt);
    if (sym == NULL) {
      rc = ERR_INTERNAL;
    } else {
      sym->is_mutable = false;
      rc = scope_define(sym);
      stmt->for_stmt.symbol = sym;
    }

    for (int i = 0; rc == ERR_OK && i < stmt->for_stmt.guards.count; i++) {
      rc = sem_expr(stmt->for_stmt.guards.items[i]);
    }
    if (rc == ERR_OK) {
      rc = sem_block(stmt->for_stmt.body);
    }
    scope_pop();
    return rc;
  }

  case AST_RETURN: {
    if (stmt->return_stmt.value != NULL) {
      int rc = sem_expr(stmt->return_stmt.value);
      if (rc != ERR_OK) {
        return rc;
      }
    }
    if (current_function->decl->func_def.ret_type == NULL) {
      return ERR_SEM_TYPE;
    }
    type_t *ret = current_function->decl->func_def.ret_type;
    if (stmt->return_stmt.value == NULL && ret->kind != T_UNIT) {
      return ERR_SEM_OTHER;
    }
    return ERR_OK;
  }

  default:
    return sem_expr(stmt); // expression as statement
  }
}

// defines variables of a case pattern in the current scope
static int sem_pattern(ast_node_t *pat) {
  switch (pat->kind) {
  case AST_LITERAL:
  case AST_PAT_WILDCARD:
    return ERR_OK; // defines nothing

  case AST_PAT_TYPED: {
    int rc = check_type(pat->pat_typed.decl_type);
    if (rc != ERR_OK) {
      return rc;
    }
    symbol_t *sym = symbol_new(SYM_VAR, pat->pat_typed.name, pat);
    if (sym == NULL) {
      return ERR_INTERNAL;
    }
    sym->is_mutable = false;
    rc = scope_define(sym);
    if (rc != ERR_OK) {
      return rc;
    }
    pat->pat_typed.symbol = sym;
    return ERR_OK;
  }

  case AST_PAT_CLASS: {
    symbol_t *sym = scope_lookup(pat->pat_class.name);
    if (sym == NULL || sym->kind != SYM_CLASS) {
      return ERR_SEM_UNDEF;
    }
    if (sym->decl->case_class.attrs.count != pat->pat_class.bindings.count) {
      return ERR_SEM_ARGS;
    }
    for (int i = 0; i < pat->pat_class.bindings.count; i++) {
      ast_node_t *binding = pat->pat_class.bindings.items[i];
      if (binding->id.name == NULL) {
        continue; // _ defines nothing
      }
      symbol_t *var = symbol_new(SYM_VAR, binding->id.name, binding);
      if (var == NULL) {
        return ERR_INTERNAL;
      }
      var->is_mutable = false;
      int rc = scope_define(var);
      if (rc != ERR_OK) {
        return rc;
      }
      binding->id.symbol = var;
    }
    return ERR_OK;
  }

  default:
    return ERR_INTERNAL;
  }
}

// match expression, every case has its own scope
static int sem_match(ast_node_t *match) {
  int rc = sem_expr(match->match.subject);
  if (rc != ERR_OK) {
    return rc;
  }
  bool has_default = false;

  for (int i = 0; i < match->match.cases.count; i++) {
    ast_node_t *branch = match->match.cases.items[i];
    if (!scope_push()) {
      return ERR_INTERNAL;
    }
    rc = sem_pattern(branch->case_branch.pattern);
    if (rc != ERR_OK) {
      scope_pop();
      return rc;
    }
    if (branch->case_branch.guard != NULL) {
      rc = sem_expr(branch->case_branch.guard);
      if (rc != ERR_OK) {
        scope_pop();
        return rc;
      }
    }
    rc = sem_block(branch->case_branch.body);
    if (rc != ERR_OK) {
      scope_pop();
      return rc;
    }

    scope_pop();
    if (branch->case_branch.pattern->kind == AST_PAT_WILDCARD &&
        branch->case_branch.guard == NULL) {
      has_default = true;
    }
  }

  if (!has_default) {
    return ERR_SEM_OTHER; // no default case
  }
  return ERR_OK;
}

// checks an expression
static int sem_expr(ast_node_t *expr) {
  switch (expr->kind) {
  case AST_ID: {
    symbol_t *sym = scope_lookup(expr->id.name);
    if (sym == NULL) {
      return ERR_SEM_UNDEF;
    }
    expr->id.symbol = sym;
    if (sym->kind == SYM_VAR) {
      expr->type = sym->type;
    }

    if (sym->kind == SYM_FUNC) {
      // function used without parentheses
      if (sym->is_builtin || sym->decl->func_def.param_list_count != 0) {
        return ERR_SEM_ARGS;
      }
      if (sym->decl->func_def.ret_type == NULL) {
        if (sym->state == FUNC_IN_PROGRESS) {
          return ERR_SEM_TYPE;
        }
        if (sym->state == FUNC_NOT_DONE) {
          return analyze_function(sym);
        }
      }
    }
    return ERR_OK;
  }

  case AST_CALL: {
    symbol_t *sym = scope_lookup(expr->call.name);
    if (sym == NULL) {
      return ERR_SEM_UNDEF;
    }
    expr->call.symbol = sym;

    if (sym->kind == SYM_CLASS) { // constructor Point(1, 2)
      if (expr->call.arg_list_count != 1 ||
          expr->call.arg_lists[0].count != sym->decl->case_class.attrs.count) {
        return ERR_SEM_ARGS;
      }
    } else if (sym->kind == SYM_VAR) { // list index lst(2)
      if (expr->call.arg_list_count != 1 ||
          expr->call.arg_lists[0].count != 1) {
        return ERR_SEM_ARGS;
      }
    } else if (sym->kind == SYM_ALIAS) {
      return ERR_SEM_UNDEF;
    } else if (sym->is_builtin) {
      if (expr->call.arg_list_count != 1 ||
          expr->call.arg_lists[0].count != sym->param_count) {
        return ERR_SEM_ARGS;
      }
    } else {
      ast_node_t *def = sym->decl;
      if (expr->call.arg_list_count != def->func_def.param_list_count) {
        return ERR_SEM_ARGS;
      }
      for (int i = 0; i < expr->call.arg_list_count; i++) {
        if (expr->call.arg_lists[i].count !=
            def->func_def.param_lists[i].count) {
          return ERR_SEM_ARGS;
        }
      }

      // no return type, IN_PROGRESS means recursion
      if (def->func_def.ret_type == NULL) {
        if (sym->state == FUNC_IN_PROGRESS) {
          return ERR_SEM_TYPE;
        }
        if (sym->state == FUNC_NOT_DONE) {
          int rc = analyze_function(sym);
          if (rc != ERR_OK) {
            return rc;
          }
        }
      }
    }

    for (int i = 0; i < expr->call.arg_list_count; i++) {
      ast_list_t *arg_list = &expr->call.arg_lists[i];
      for (int j = 0; j < arg_list->count; j++) {
        int rc = sem_expr(arg_list->items[j]);
        if (rc != ERR_OK) {
          return rc;
        }
        if (sym->kind == SYM_FUNC && !sym->is_builtin) {
          type_t *param_type =
              sym->decl->func_def.param_lists[i].items[j]->param.decl_type;
          if (param_type != NULL && arg_list->items[j]->type != NULL &&
              !type_compatible(param_type, arg_list->items[j]->type)) {
            return ERR_SEM_ARGS;
          }
        }
      }
    }
    return ERR_OK;
  }

  case AST_BINARY: {
    int rc = sem_expr(expr->binary.left);
    if (rc == ERR_OK) {
      rc = sem_expr(expr->binary.right);
    }
    return rc;
  }

  case AST_UNARY:
    return sem_expr(expr->unary.operand);

  case AST_LITERAL:
    return ERR_OK;

  case AST_IF:
    return sem_stmt(expr); // if as expression, same checks as statement

  case AST_RANGE: {
    int rc = sem_expr(expr->range.from);
    if (rc == ERR_OK) {
      rc = sem_expr(expr->range.end);
    }
    if (rc == ERR_OK && expr->range.step != NULL) {
      rc = sem_expr(expr->range.step);
    }
    return rc;
  }

  case AST_LIST_LIT: {
    for (int i = 0; i < expr->list_lit.items.count; i++) {
      int rc = sem_expr(expr->list_lit.items.items[i]);
      if (rc != ERR_OK) {
        return rc;
      }
    }
    return ERR_OK;
  }

  case AST_LIST_RANGE: {
    int rc = sem_expr(expr->list_range.start);
    if (rc == ERR_OK) {
      rc = sem_expr(expr->list_range.end);
    }
    if (rc == ERR_OK && expr->list_range.step != NULL) {
      rc = sem_expr(expr->list_range.step);
    }
    return rc;
  }

  case AST_MEMBER: {
    int rc = sem_expr(expr->member.object);
    for (int i = 0; rc == ERR_OK && i < expr->member.args.count; i++) {
      rc = sem_expr(expr->member.args.items[i]);
    }
    return rc;
  }

  case AST_MATCH:
    return sem_match(expr);

  default:
    return ERR_OK;
  }
}

int semantic_analyze(ast_node_t *root) {
  if (!scope_init()) {
    scope_free(); // init may fail after the global scope was created
    return ERR_INTERNAL;
  }
  int rc = collect_definitions(root);
  if (rc == ERR_OK) {
    rc = check_main(root);
  }

  for (int i = 0; rc == ERR_OK && i < root->program.defs.count; i++) {
    ast_node_t *def = root->program.defs.items[i];
    if (def->kind != AST_FUNC_DEF) {
      continue;
    }
    symbol_t *sym = def->func_def.symbol;
    if (sym->state == FUNC_NOT_DONE) {
      rc = analyze_function(sym);
    }
  }
  scope_free(); // symbols stay for the generator
  return rc;
}

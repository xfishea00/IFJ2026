/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    parser.c
 * Authors: Adam James Fisher (xfishea00)
 * Brief:   Syntax analysis
 */

#include "error.h"
#include "scanner.h"
#include "symtable.h"
#include "token.h"
#include "type.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "expression_parser.h"

#define current_line parser.current_token.line

// Global / Parser Context
typedef struct {
  token_t current_token;
  bool debug;
} Parser;

static Parser parser;

// Helper error exit function
void syntax_error(const char *msg) {
  fprintf(stderr, "Syntax Error: %s (token_t type: %d, Line: %d)\n", msg,
          parser.current_token.type, parser.current_token.line);
  exit(ERR_SYNTAX);
}

static void internal_error(void) {
  fprintf(stderr, "Parser: memory allocation failed\n");
  exit(ERR_INTERNAL);
}

static bool check_identifier(const char *name) {
  return parser.current_token.type == TOK_ID &&
         parser.current_token.string_val != NULL &&
         strcmp(parser.current_token.string_val, name) == 0;
}

// helper functions
static void advance(void) {
  token_free(&parser.current_token);
  int rc = scanner_next(&parser.current_token);
  if (rc != ERR_OK) {
    fprintf(stderr, "Scanner error while parsing (code: %d)\n", rc);
    exit(rc);
  }
}

static int advance_expression_token(token_t *token) {
  (void)token;
  advance();
  return ERR_OK;
}


static token_type_t peek(void) { return parser.current_token.type; }

static bool check(token_type_t type) { return peek() == type; }

static bool check_literal(void) {
  return check(TOK_INT_LIT) || check(TOK_STRING_LIT) || check(TOK_DOUBLE_LIT) 
         || check(TOK_KW_TRUE) || check(TOK_KW_FALSE);
}

static void match(token_type_t expected) {
  if (check(expected)) {
    advance();
  } else {
    char err_msg[100];
    snprintf(err_msg, sizeof(err_msg), "Expected token type %d, but got %d",
             expected, peek());
    syntax_error(err_msg);
  }
}

void match_literal() {
  if (check_literal()) {
    advance();
  } else {
    char err_msg[100];
    snprintf(err_msg, sizeof(err_msg), "Expected token type %s, but got %d",
             "LITERAL", parser.current_token.type);
    syntax_error(err_msg);
  }
}

type_t *match_type(void) {
  type_t *type = type_from_token(peek());
  switch (peek()) {
  case TOK_KW_INT:
  case TOK_KW_DOUBLE:
  case TOK_KW_STRING:
  case TOK_KW_UNIT:
  case TOK_KW_BOOLEAN:
  case TOK_ID:
    advance();
    break;
  case TOK_KW_LIST:
    match(TOK_KW_LIST);
    match(TOK_LBRACKET);
    match_type();
    match(TOK_RBRACKET);
    break;
  default:
    syntax_error("Unexpected token in expect_type");
  }
  if (check(TOK_PIPE)) {
    match(TOK_PIPE);
    match_type();
    type = NULL;
    // TODO: union types
  }
  return type;
}

static void append_node(ast_list_t *list, ast_node_t *node) {
  if (node != NULL && !ast_list_append(list, node)) {
    ast_free(node);
    internal_error();
  }
}

static char *take_string(void) {
  char *value = parser.current_token.string_val;
  parser.current_token.string_val = NULL;
  advance();
  return value;
}

// Forward Declarations of Parsing Functions
static ast_node_t *parse_program(void);
void parse_prologue(void);
void parse_top_level_list(ast_list_t *defs);
static ast_node_t *parse_func_declaration(bool is_main);
void parse_entry_function(void);
static ast_node_t * parse_type_definition(void);
static ast_node_t * parse_class(void);

static ast_node_t *parse_block(void);
void parse_statement_list(ast_node_t *block);
void parse_statement_list_tail(ast_node_t *block);
static ast_node_t *parse_statement(void);
static ast_node_t *parse_id_stmt_tail(void);
static ast_node_t *parse_block_statement(void);

static ast_node_t *parse_var_definition(void);
type_t *parse_expect_type(void);
static ast_node_t *parse_return_statement(void);

static ast_node_t * parse_if_statement(void);
static ast_node_t * parse_else_tail(void);

static ast_node_t *parse_while_statement(void);
static ast_node_t *parse_for_loop(void);
static ast_node_t * parse_for_inline(int line);
static ast_node_t * parse_for_multiline(int line);
void parse_inline_guards(ast_list_t *list);
void parse_multiline_guards(ast_list_t *list);
static ast_node_t *parse_gen_source(int line, char *var_name);

static void parse_param_groups(ast_list_t **param_lists, int *list_count);
static ast_node_t * parse_param(void);

void parse_arg_groups(ast_list_t **lists, int *list_count);
static ast_node_t *parse_arg_list(void);

static ast_node_t* parse_expression(void);
static ast_node_t *parse_expr_match_tail(ast_node_t* subject);
static ast_node_t *parse_literals(void);
static ast_node_t *parse_list_def(void);
void parse_list_elements(void);
void parse_list_tail(void);
void parse_list_next(void);

void parse_case_list(ast_node_t *subject, ast_list_t *cases);
static ast_node_t *parse_model(void);
void parse_separator(void);

// Implementation of Grammar Rules

ast_node_t* parse_program(void) {
  int line = current_line;
  parse_prologue();

  ast_node_t *program = ast_new_program(line);
  if (program == NULL) {
    internal_error();
  }
  parse_top_level_list(&program->program.defs);
  match(TOK_EOF);
  return program;
}

void parse_prologue(void) {
  match(TOK_KW_IMPORT);
  match(TOK_ID);
  match(TOK_DOT);
  match(TOK_MUL);
  match(TOK_NEWLINE);
}

void parse_top_level_list(ast_list_t *defs) {
  while (!check(TOK_EOF)){
    switch (peek()) {
    case TOK_KW_DEF:
      append_node(defs, parse_func_declaration(false));
      break;
    case TOK_AT_MAIN:
      advance();
      append_node(defs, parse_func_declaration(true));
      break;
    case TOK_KW_CASE:
      append_node(defs, parse_class());
      break;
    case TOK_KW_TYPE:
      append_node(defs, parse_type_definition());
      break;
    case TOK_EOF:
      // epsilon
      break;
    case TOK_NEWLINE:
      match(TOK_NEWLINE);
      break;
    default:
      syntax_error("Unexpected token in top-level list");
    }
  }
}


ast_node_t *parse_func_declaration(bool is_main) {
  match(TOK_KW_DEF);
  int line = current_line;
  char* name;
  if(check(TOK_ID)) name = take_string();
  ast_list_t *param_lists = NULL;
  int param_list_count = 0;
  parse_param_groups(&param_lists, &param_list_count);
  
  type_t *ret_type = parse_expect_type();
  match(TOK_ASSIGN);
  ast_node_t *body = parse_block_statement();

  ast_node_t *function = ast_new_func_def(name, is_main, ret_type, param_lists, param_list_count, body, line);
  
  //free 
  if (function == NULL) {
    free(name);
    for (int i = 0; i < param_list_count; i++) {
      for (int j = 0; j < param_lists[i].count; j++) {
        ast_free(param_lists[i].items[j]);
      }
      free(param_lists[i].items);
    }
    free(param_lists);
    ast_free(body);
    internal_error();
  }

  return function;
}

type_t *parse_expect_type(void) {
  if (check(TOK_COLON)) {
    match(TOK_COLON);
    return match_type();
  }
  return NULL; // type not given
}

static void parse_param_groups(ast_list_t **param_lists, int *list_count) {
  while (check(TOK_LPAR)) {
    match(TOK_LPAR);
    ast_list_t *params = ast_lists_add(param_lists, list_count);
    if (params == NULL) {
      internal_error();
    }
    while (!check(TOK_RPAR)) {
      append_node(params, parse_param());
      if (!check(TOK_COMMA)) {
        break;
      }
      match(TOK_COMMA);
      if(check(TOK_NEWLINE)){
        match(TOK_NEWLINE);
      }else if(check(TOK_RPAR)){
        syntax_error("No newline between trailing comma and RPAR");
      }
      
    }
    match(TOK_RPAR);
  }
}

static ast_node_t *parse_param(void) {
  if (!check(TOK_ID)) {
    syntax_error("Expected parameter name");
  }
  int line = parser.current_token.line;
  char *name = take_string();
  match(TOK_COLON);
  type_t *type = match_type();
  ast_node_t *param = ast_new_param(name, type, line);
  if (param == NULL) {
    free(name);
    internal_error();
  }
  return param;
}

ast_node_t * parse_block_statement(void) {
  if (check(TOK_NEWLINE)) {
    return parse_block();
  } else {
    return parse_statement();
  }
}

static ast_node_t *parse_block(void) {
  int line = current_line;
  match(TOK_NEWLINE);
  match(TOK_INDENT);

  ast_node_t *block = ast_new_block(line);
  if (block == NULL) {
    internal_error();
  }

  while (!check(TOK_DEDENT)){ // unsure
    if (check(TOK_EOF)) {
      ast_free(block);
      syntax_error("Unterminated block");
    }
    parse_statement_list(block);
  }
  match(TOK_DEDENT);
  return block;
}

void parse_statement_list(ast_node_t *block) {
  append_node(&block->block.stmts, parse_statement());
  parse_statement_list_tail(block);
}

void parse_statement_list_tail(ast_node_t *block) {
  if (check(TOK_NEWLINE) || check(TOK_SEMICOLON)) {
    parse_separator();
    if (check(TOK_DEDENT)) {
      // End of block reached after separator
      return;
    }
    parse_statement_list(block);
  }
  // else epsilon (e.g. directly before DEDENT)
}

void parse_separator(void) {
  if (check(TOK_SEMICOLON)) {
    match(TOK_SEMICOLON);
  } else if (check(TOK_NEWLINE)) {
    while (check(TOK_NEWLINE)) {
      match(TOK_NEWLINE);
    }
  } else {
    syntax_error("Expected separator (; or NL)");
  }
}

static ast_node_t *parse_statement(void) {
  switch (peek()) {
  case TOK_ID:
    return parse_id_stmt_tail();
  case TOK_KW_VAL:
  case TOK_KW_VAR:
    return parse_var_definition();
  case TOK_KW_IF:
    return parse_if_statement();
  case TOK_KW_WHILE:
    return parse_while_statement();
  case TOK_KW_FOR:
    return parse_for_loop();
  case TOK_KW_RETURN:
    return parse_return_statement();
  default:
    return parse_expression();
  }
  return NULL; //temp
}

static ast_node_t *parse_id_stmt_tail(void) {
  char *name = take_string();
  int line = current_line;
  if (check(TOK_ASSIGN)) {
    // Assignment: id = expression
    match(TOK_ASSIGN);
    ast_node_t *value = parse_expression();
    ast_node_t *assign = ast_new_assign(name, value, line);
    if (assign == NULL) {
        free(name);
        ast_free(value);
        internal_error();
    }
    return assign;
  } else if (check(TOK_LPAR)) {
    // Function call: id(args)...
    int list_count = 0;
    ast_list_t *list = NULL;
    parse_arg_groups(&list, &list_count);
    ast_node_t *call = ast_new_call(name, list, list_count, line);

    if (call == NULL) {
    free(name);
    for (int i = 0; i < list_count; i++) {
      for (int j = 0; j < list[i].count; j++) {
        ast_free(list[i].items[j]);
      }
      free(list[i].items);
    }
    free(list);
    internal_error();
    }
    if (check(TOK_KW_MATCH)) {
      return parse_expr_match_tail(call);
    }
    return call;

  } else if (check(TOK_KW_MATCH)) {
    ast_node_t *subject = ast_new_id(name, line);
    if (subject == NULL) {
      free(name);
      internal_error();
    }
    return parse_expr_match_tail(subject);
  } else {
     ast_node_t *left = ast_new_id(name, line);
     if (left == NULL) {
       internal_error();
     }
     ast_op_t op;
     bool has_operator = true;
     switch (peek()) {
     case TOK_PLUS: op = OP_ADD; break;
     case TOK_MINUS: op = OP_SUB; break;
     case TOK_CONCAT: op = OP_CONCAT; break;
     case TOK_MUL: op = OP_MUL; break;
     case TOK_DIV: op = OP_DIV; break;
     case TOK_LT: op = OP_LT; break;
     case TOK_GT: op = OP_GT; break;
     case TOK_LE: op = OP_LE; break;
     case TOK_GE: op = OP_GE; break;
     case TOK_EQ: op = OP_EQ; break;
     case TOK_NEQ: op = OP_NEQ; break;
     case TOK_AND: op = OP_AND; break;
     case TOK_OR: op = OP_OR; break;
     default: has_operator = false; break;
     }
     if (!has_operator) {
       return left;
     }
     advance();
     ast_node_t *right = parse_expression();
     ast_node_t *binary = ast_new_binary(op, left, right, line);
     if (binary == NULL) {
       ast_free(left);
       ast_free(right);
       internal_error();
     }
     return binary;
  }
  return NULL; //temporary
}

static ast_node_t *parse_var_definition(void) {
  bool mutable = check(TOK_KW_VAR);
  int line = current_line;
  advance();
  if (!check(TOK_ID)) {
    syntax_error("Expected variable name");
  }
  char *name = take_string();
  type_t *decl_type = parse_expect_type();
  match(TOK_ASSIGN);
  ast_node_t *init = parse_expression();
  ast_node_t *var = ast_new_var_def(mutable, name, decl_type, init, line);

  if (var == NULL) {
    free(name);
    ast_free(init);
    internal_error();
  }

  return var;
}

static ast_node_t *parse_return_statement(void) {
  int line = current_line;
  match(TOK_KW_RETURN);
  ast_node_t *value = NULL;
  if (!check(TOK_NEWLINE) && !check(TOK_SEMICOLON) && !check(TOK_DEDENT) &&
      !check(TOK_EOF)) {
    value = parse_expression();
  }
  ast_node_t *ret = ast_new_return(value, line);
  if (ret == NULL) {
    ast_free(value);
    internal_error();
  }
  return ret;
}

static ast_node_t *parse_if_statement(void) {
  int line = current_line;
  match(TOK_KW_IF);
  ast_node_t *condition = parse_expression();
  match(TOK_KW_THEN);
  ast_node_t * then_branch = parse_block_statement();
  ast_node_t * else_branch = NULL;
  while (check(TOK_KW_ELSE)){
    match(TOK_KW_ELSE);
    else_branch = parse_else_tail();
  }

  ast_node_t *node = ast_new_if(condition, then_branch, else_branch, line);
  if (node == NULL) {
    ast_free(condition);
    ast_free(then_branch);
    ast_free(else_branch);
    internal_error();
  }
  return node;
}

ast_node_t *parse_else_tail(void) {
  ast_node_t *node;
  if (check(TOK_KW_IF)) {
    node = parse_if_statement();
  } else {
    node = parse_block_statement();
  }
  return node;
}

ast_node_t *parse_while_statement(void) {
  int line = current_line;
  match(TOK_KW_WHILE);
  ast_node_t *condition = parse_expression();
  match(TOK_KW_DO);
  ast_node_t *body = parse_block();
  ast_node_t *node = ast_new_while(condition, body, line);
  if (node == NULL) {
    ast_free(condition);
    ast_free(body);
    internal_error();
  }
  return node;
}


ast_node_t *parse_for_loop(void) {
  int line = current_line;
  match(TOK_KW_FOR);

  ast_node_t *loop = NULL;
  if (check(TOK_NEWLINE)) {
    loop = parse_for_multiline(line);
  } else {
    loop = parse_for_inline(line);
  }
  return loop;
}

ast_node_t *parse_for_inline(int line) {
  if (!check(TOK_ID)) {
    syntax_error("Expected for-loop variable");
  }
  char *var_name = take_string();
  match(TOK_LARROW);
  ast_node_t *source = parse_gen_source(line, var_name);
  ast_list_t guards = {0};
  parse_inline_guards(&guards);
  match(TOK_KW_DO);
  ast_node_t *body = parse_block_statement();
  ast_node_t *loop = ast_new_for(var_name, source, guards, body, line);

  if (loop == NULL) {
    free(var_name);
    ast_free(source);
    for (int i = 0; i < guards.count; i++) {
      ast_free(guards.items[i]);
    }
    free(guards.items);
    ast_free(body);
    internal_error();
  }
  return loop;
}

ast_node_t *parse_for_multiline(int line) {
  while (check(TOK_NEWLINE))
    match(TOK_NEWLINE);
  match(TOK_INDENT);
  if (!check(TOK_ID)) {
    syntax_error("Expected for-loop variable");
  }
  char *var_name = take_string();
  match(TOK_LARROW);
  ast_node_t *source = parse_gen_source(line, var_name);
  ast_list_t guards = {0};
  parse_multiline_guards(&guards);
  match(TOK_DEDENT);
  while (check(TOK_NEWLINE))
    match(TOK_NEWLINE);
  match(TOK_KW_DO);
  ast_node_t *body = parse_block_statement();
  ast_node_t *loop = ast_new_for(var_name, source, guards, body, line);

  if (loop == NULL) {
    free(var_name);
    ast_free(source);
    for (int i = 0; i < guards.count; i++) {
      ast_free(guards.items[i]);
    }
    free(guards.items);
    ast_free(body);
    internal_error();
  }
  return loop;
}

void parse_inline_guards(ast_list_t *guards) {
  if (check(TOK_KW_IF)) {
    match(TOK_KW_IF);
    append_node(guards, parse_expression());
    parse_inline_guards(guards);
  }
  // else epsilon
}

void parse_multiline_guards(ast_list_t *guards) {
  if (check(TOK_NEWLINE)) {
    while (check(TOK_NEWLINE))
      match(TOK_NEWLINE);
    if (check(TOK_KW_IF)) {
      match(TOK_KW_IF);
      append_node(guards, parse_expression());
      parse_multiline_guards(guards);
    }
  }
  // else epsilon
}

ast_node_t *parse_gen_source(int line, char *var_name) {
  ast_node_t *range = NULL;
  ast_node_t *from = parse_expression();
  if (check_identifier("to") || check_identifier("until")) {
    bool is_until = check_identifier("until");
    match(TOK_ID);
    ast_node_t *end = parse_expression();
    ast_node_t *step = NULL;
    if (check_identifier("by")) {
      match(TOK_ID);
      step = parse_expression();
    }
    range = ast_new_range(is_until, from, end, step, line);
    if (range == NULL) {
      ast_free(from);
      ast_free(end);
      ast_free(step);
      free(var_name);
      internal_error();
    }  
  }
  return range;
}


void parse_arg_groups(ast_list_t **list, int *list_count) {
  while (check(TOK_LPAR)){
    match(TOK_LPAR);
    ast_list_t *args = ast_lists_add(list, list_count);
    if (args == NULL) {
      internal_error();
    }
    while (!check(TOK_RPAR)) {
      append_node(args, parse_arg_list());
      if (!check(TOK_COMMA)) {
        break;
      }
      match(TOK_COMMA);
      if(check(TOK_NEWLINE)){
        match(TOK_NEWLINE);
      }else if(check(TOK_RPAR)){
        syntax_error("No newline between trailing comma and RPAR");
      }
      
    }
    match(TOK_RPAR);
  }
  // else epsilon
}

static ast_node_t *parse_arg_list(void) {
  ast_node_t *arg = parse_expression();
  return arg;
}

static ast_node_t *parse_expression_primary(void) {
  int line = current_line;

  if (check(TOK_KW_LIST)) {
    match(TOK_KW_LIST);
    if (check(TOK_DOT)) {
      char *list_name = malloc(5);
      if (list_name == NULL) {
        internal_error();
      }
      strcpy(list_name, "List");
      ast_node_t *object = ast_new_id(list_name, line);
      match(TOK_DOT);
      if (!check(TOK_ID)) {
        ast_free(object);
        syntax_error("Expected list member name");
      }
      char *member_name = take_string();
      ast_list_t args = {0};
      bool has_parens = check(TOK_LPAR);
      if (has_parens) {
        int list_count = 0;
        ast_list_t *list = NULL;
        parse_arg_groups(&list, &list_count);
        if (list_count > 0) {
          args = list[0];
          free(list);
        }
      }
      ast_node_t *member = ast_new_member(member_name, object, has_parens,
                                          args, line);
      if (member == NULL) {
        ast_free(object);
        free(member_name);
        internal_error();
      }
      return member;
    }
    if (check(TOK_ID)) {
      char *list_name = malloc(5);
      if (list_name == NULL) {
        internal_error();
      }
      strcpy(list_name, "List");
      ast_node_t *object = ast_new_id(list_name, line);
      char *member_name = take_string();
      ast_list_t args = {0};
      ast_node_t *member = ast_new_member(member_name, object, false, args,
                                          line);
      if (member == NULL) {
        ast_free(object);
        free(member_name);
        internal_error();
      }
      return member;
    }
    match(TOK_LPAR);
    ast_list_t owned_items = {0};
    while (check(TOK_NEWLINE)) {
      match(TOK_NEWLINE);
    }
    while (!check(TOK_RPAR)) {
      ast_node_t *item;
      if (check(TOK_KW_LIST)) {
        item = parse_list_def();
      } else if (check_literal()) {
        item = parse_literals();
      } else if (check(TOK_ID)) {
        item = ast_new_id(take_string(), line);
      } else if (check(TOK_COMMA)) {
        match(TOK_COMMA);
        while (check(TOK_NEWLINE)) {
          match(TOK_NEWLINE);
        }
        continue;
      } else if (check(TOK_NEWLINE)) {
        match(TOK_NEWLINE);
        continue;
      } else {
        syntax_error("Invalid list element");
      }
      append_node(&owned_items, item);
    }
    match(TOK_RPAR);
    ast_node_t *list = ast_new_list_lit(owned_items, line);
    if (list == NULL) {
      internal_error();
    }
    return list;
  }
  if (check(TOK_LPAR)) {
    advance();
    if (check(TOK_RPAR)) {
      advance();
      return ast_new_unit(line);
    }
    if (check(TOK_ID)) {
      char *lambda_name = take_string();
      if (check(TOK_COLON)) {
        advance();
        match_type();
        match(TOK_RPAR);
        match(TOK_ARROW);
        ast_node_t *body = parse_expression();
        free(lambda_name);
        return body;
      }
      ast_node_t *inner = ast_new_id(lambda_name, line);
      ast_op_t op;
      bool found = false;
      switch (peek()) {
      case TOK_PLUS: op = OP_ADD; found = true; break;
      case TOK_MINUS: op = OP_SUB; found = true; break;
      case TOK_MUL: op = OP_MUL; found = true; break;
      case TOK_DIV: op = OP_DIV; found = true; break;
      case TOK_LT: op = OP_LT; found = true; break;
      case TOK_GT: op = OP_GT; found = true; break;
      case TOK_LE: op = OP_LE; found = true; break;
      case TOK_GE: op = OP_GE; found = true; break;
      case TOK_EQ: op = OP_EQ; found = true; break;
      case TOK_NEQ: op = OP_NEQ; found = true; break;
      case TOK_AND: op = OP_AND; found = true; break;
      case TOK_OR: op = OP_OR; found = true; break;
      default: break;
      }
      if (found) {
        advance();
        ast_node_t *right = parse_expression_primary();
        inner = ast_new_binary(op, inner, right, line);
      }
      match(TOK_RPAR);
      return inner;
    }
    ast_node_t *inner = parse_expression();
    if (check(TOK_ID) &&
        (check_identifier("to") || check_identifier("until"))) {
      bool is_until = check_identifier("until");
      match(TOK_ID);
      ast_node_t *end = parse_expression();
      ast_node_t *step = NULL;
      if (check(TOK_ID) && check_identifier("by")) {
        match(TOK_ID);
        step = parse_expression();
      }
      ast_node_t *range = ast_new_range(is_until, inner, end, step, line);
      if (range == NULL) {
        ast_free(inner);
        ast_free(end);
        ast_free(step);
        internal_error();
      }
      inner = range;
    }
    match(TOK_RPAR);
    while (check(TOK_DOT)) {
      advance();
      if (!check(TOK_ID)) {
        ast_free(inner);
        syntax_error("Expected member name");
      }
      char *member_name = take_string();
      ast_list_t args = {0};
      bool has_parens = check(TOK_LPAR);
      if (has_parens) {
        int list_count = 0;
        ast_list_t *list = NULL;
        parse_arg_groups(&list, &list_count);
        if (list_count > 0) {
          args = list[0];
          free(list);
        }
      }
      ast_node_t *member = ast_new_member(member_name, inner, has_parens,
                                          args, line);
      if (member == NULL) {
        ast_free(inner);
        free(member_name);
        internal_error();
      }
      inner = member;
    }
    return inner;
  }
  if (check(TOK_ID)) {
    char *name = take_string();
    if (check(TOK_LPAR)) {
      int list_count = 0;
      ast_list_t *list = NULL;
      parse_arg_groups(&list, &list_count);
      return ast_new_call(name, list, list_count, line);
    }
    ast_node_t *id = ast_new_id(name, line);
    while (check(TOK_DOT)) {
      advance();
      if (!check(TOK_ID)) {
        ast_free(id);
        syntax_error("Expected member name");
      }
      char *member_name = take_string();
      ast_list_t args = {0};
      bool has_parens = check(TOK_LPAR);
      if (has_parens) {
        int list_count = 0;
        ast_list_t *list = NULL;
        parse_arg_groups(&list, &list_count);
        if (list_count > 0) {
          args = list[0];
          free(list);
        }
      }
      ast_node_t *member = ast_new_member(member_name, id, has_parens, args,
                                          line);
      if (member == NULL) {
        ast_free(id);
        free(member_name);
        internal_error();
      }
      id = member;
    }
    return id;
  }
  if (check(TOK_INT_LIT)) {
    int64_t value = parser.current_token.int_val;
    advance();
    return ast_new_int(value, line);
  }
  if (check(TOK_DOUBLE_LIT)) {
    double value = parser.current_token.double_val;
    advance();
    return ast_new_double(value, line);
  }
  if (check(TOK_STRING_LIT)) {
    return ast_new_string(take_string(), line);
  }
  if (check(TOK_KW_TRUE) || check(TOK_KW_FALSE)) {
    bool value = check(TOK_KW_TRUE);
    advance();
    return ast_new_bool(value, line);
  }
  syntax_error("Expected expression");
  return NULL;
}

static ast_node_t *parse_expression_operators(ast_node_t *left, int min_precedence) {
  while (true) {
    ast_op_t op;
    int precedence;
    switch (peek()) {
    case TOK_MUL: op = OP_MUL; precedence = 5; break;
    case TOK_DIV: op = OP_DIV; precedence = 5; break;
    case TOK_CONCAT: op = OP_CONCAT; precedence = 4; break;
    case TOK_PLUS: op = OP_ADD; precedence = 4; break;
    case TOK_MINUS: op = OP_SUB; precedence = 4; break;
    case TOK_LT: op = OP_LT; precedence = 3; break;
    case TOK_GT: op = OP_GT; precedence = 3; break;
    case TOK_LE: op = OP_LE; precedence = 3; break;
    case TOK_GE: op = OP_GE; precedence = 3; break;
    case TOK_EQ: op = OP_EQ; precedence = 3; break;
    case TOK_NEQ: op = OP_NEQ; precedence = 3; break;
    case TOK_AND: op = OP_AND; precedence = 2; break;
    case TOK_OR: op = OP_OR; precedence = 1; break;
    default: return left;
    }
    if (precedence < min_precedence) {
      return left;
    }
    advance();
    ast_node_t *right = parse_expression_primary();
    right = parse_expression_operators(right, precedence + 1);
    ast_node_t *binary = ast_new_binary(op, left, right, current_line);
    if (binary == NULL) {
      ast_free(left);
      ast_free(right);
      internal_error();
    }
    left = binary;
  }
}

static ast_node_t *parse_expression(void) {
  int line = current_line;

  if (check(TOK_KW_MATCH)) {
    syntax_error("Match expression requires a subject");
  }
  if (check(TOK_KW_IF)) {
    return parse_if_statement();
  }

  if (check(TOK_KW_LIST) || check(TOK_ID) || check_literal() ||
      check(TOK_LPAR)) {
    ast_node_t *left = parse_expression_primary();
    if (left == NULL) {
      internal_error();
    }
    ast_node_t *expression = parse_expression_operators(left, 1);
    if (check(TOK_KW_MATCH)) {
      return parse_expr_match_tail(expression);
    }
    return expression;
  }

  int rc = parse_non_id_expression(&parser.current_token,
                                    advance_expression_token);
  if (rc == ERR_SYNTAX) {
    syntax_error("Invalid expression");
  }
  if (rc != ERR_OK) {
    exit(rc);
  }

  return ast_new_unit(line);
}

static ast_node_t *parse_expr_match_tail(ast_node_t* subject) {
  int line = current_line; 
  match(TOK_KW_MATCH);
  while (check(TOK_NEWLINE)){
    match(TOK_NEWLINE);
  }
  // makeshift
  match(TOK_INDENT);
  ast_list_t cases = {0};
  

  parse_case_list(subject, &cases);
  


  match(TOK_DEDENT);
  ast_node_t *match_node = ast_new_match(subject, cases, line);
  if (match_node == NULL) {
  ast_free(subject);
  for (int i = 0; i < cases.count; i++) {
    ast_free(cases.items[i]);
  }
  free(cases.items);
  internal_error();
  }
  return match_node;

  // else epsilon
}

void parse_case_list(ast_node_t *subject, ast_list_t *cases) {
  
  if (check(TOK_KW_CASE)) {
    int line = current_line;
    match(TOK_KW_CASE);
    ast_node_t *pattern = parse_model();
    ast_node_t *guard = NULL;
    if (check(TOK_KW_IF)) {
      match(TOK_KW_IF);
      guard = parse_expression();
    }
    match(TOK_ARROW);
    ast_node_t *body = NULL;
    if(!check(TOK_NEWLINE)){
      body = parse_expression();
    } else{
      body = parse_block();
    }
    ast_node_t *case_node = ast_new_case(pattern, guard, body, line);
    if (case_node == NULL) {
      ast_free(pattern);
      ast_free(guard);
      ast_free(body);
      ast_free(subject);
      for (int i = 0; i < cases->count; i++) {
        ast_free(cases->items[i]);
      }
      free(cases->items);
      internal_error();
    }
    
    append_node(cases, case_node);
    while (check(TOK_NEWLINE))
      match(TOK_NEWLINE);
    parse_case_list(subject, cases);
  }
  // else epsilon
  
}

// Reminder to add class constructor variant
static ast_node_t *parse_model(void) {
  int line = current_line;
  if(check(TOK_UNDERSCORE)){
    match(TOK_UNDERSCORE);
    ast_node_t *pattern = ast_new_pat_wildcard(line);
    if (pattern == NULL) {
      internal_error();
    }
    return pattern;
  }
  if(check_literal()){
    return parse_literals();
  }
 if (!check(TOK_ID)) {
    syntax_error("Invalid pattern in match case");
  }

  char *name = take_string();
    if (check(TOK_COLON)) {
      match(TOK_COLON);
      type_t *type = match_type();
      ast_node_t *pattern = ast_new_pat_typed(name, type, line);
      if (pattern == NULL) {
      free(name);
      internal_error();
      }
      return pattern;
    } 
    
    if (check(TOK_LPAR)) {
      // class constructor
      match(TOK_LPAR);
      ast_list_t bindings = {0};
      while (!check(TOK_RPAR)) {
        int binding_line = current_line;
        char *binding_name = NULL;
        if (check(TOK_ID)) {
          binding_name = take_string();
        } else if (check(TOK_UNDERSCORE)) {
          match(TOK_UNDERSCORE);
        } else {
          free(name);
          for (int i = 0; i < bindings.count; i++) {
            ast_free(bindings.items[i]);
          }
          free(bindings.items);
          syntax_error("Expected class-pattern binding");
        }
        
        ast_node_t *binding = ast_new_id(binding_name, binding_line);
        if (binding == NULL) {
        free(binding_name);
        free(name);
        for (int i = 0; i < bindings.count; i++) {
          ast_free(bindings.items[i]);
        }
        free(bindings.items);
        internal_error();
        }
        append_node(&bindings, binding);
        if (!check(TOK_COMMA)) {
        break;
        }
        match(TOK_COMMA);
      }
      match(TOK_RPAR);
      ast_node_t *pattern = ast_new_pat_class(name, bindings, line);
      if (pattern == NULL) {
      free(name);
      for (int i = 0; i < bindings.count; i++) {
        ast_free(bindings.items[i]);
      }
      free(bindings.items);
      internal_error();
      }
      return pattern;
    }
  free(name);
  syntax_error("Expected ':' or '(' after match-pattern identifier");
  return NULL;
}  

static ast_node_t *parse_literals(void){
  int line = current_line;
  
  if (check(TOK_INT_LIT)) {
    int64_t value = parser.current_token.int_val;
    advance();
    ast_node_t *node = ast_new_int(value, line);
    if (node == NULL) {
      internal_error();
    }
    return node;
  }

  if (check(TOK_DOUBLE_LIT)) {
    double value = parser.current_token.double_val;
    advance();
    ast_node_t *node = ast_new_double(value, line);
    if (node == NULL) {
      internal_error();
    }
    return node;
  }
  if (check(TOK_STRING_LIT)) {
    char *value = take_string();
    ast_node_t *node = ast_new_string(value, line);
    if (node == NULL) {
      free(value);
      internal_error();
    }
    return node;
  }
  if (check(TOK_KW_TRUE) || check(TOK_KW_FALSE)) {
    bool value = check(TOK_KW_TRUE);
    advance();
    ast_node_t *node = ast_new_bool(value, line);
    if (node == NULL) {
      internal_error();
    }
    return node;
  }

  return NULL;
}

static ast_node_t *parse_list_def(void) {
  int line = current_line;
  match(TOK_KW_LIST);
  match(TOK_LPAR);
  ast_list_t owned_items = {0};
  while (check(TOK_NEWLINE)) {
    match(TOK_NEWLINE);
  }
  while(!check(TOK_RPAR)){
    if(check_literal()){
      ast_list_append(&owned_items, parse_literals());
    }else if(check(TOK_ID)){
      ast_list_append(&owned_items, ast_new_id(take_string(), line));
    }else if(check(TOK_KW_LIST)){
      ast_list_append(&owned_items, parse_list_def());
    }else if(check(TOK_COMMA)){
      match(TOK_COMMA);
      while (check(TOK_NEWLINE)) {
        match(TOK_NEWLINE);
      }
    }else if(check(TOK_NEWLINE)){
      match(TOK_NEWLINE);
    }else{
      syntax_error("Invalid list element");
    }
  }
  match(TOK_RPAR);

  ast_node_t *list = ast_new_list_lit(owned_items, line);
  if (list == NULL) {
    for (int i = 0; i < owned_items.count; i++) {
      ast_free(owned_items.items[i]);
    }
    free(owned_items.items);
    internal_error();
  }
  return list;
}


ast_node_t *parse_class(void) {
  int line = current_line;
  match(TOK_KW_CASE);
  match(TOK_KW_CLASS);
  if (!check(TOK_ID)) {
    syntax_error("Expected case class name");
  }
  char *name = take_string();
  match(TOK_LPAR);
  ast_list_t attrs = {0};
  while(!check(TOK_RPAR)){
    append_node(&attrs, parse_param());
    if (!check(TOK_COMMA)){
      break;
    }
    match(TOK_COMMA);
    if(!check(TOK_NEWLINE) && !check(TOK_ID)){
      syntax_error("Unexpected token in class attributes");
    }
    if(check(TOK_NEWLINE)){
      match(TOK_NEWLINE);
      break;
    }
  }
  match(TOK_RPAR);
  ast_node_t *class_node = ast_new_case_class(name, attrs, line);

  //Free if error
  if (class_node == NULL) {
    free(name);
    for (int i = 0; i < attrs.count; i++) {
      ast_free(attrs.items[i]);
    }
    free(attrs.items);
    internal_error();
  }
  return class_node;
}

static ast_node_t * parse_type_definition(void) {
  int line = current_line;
  match(TOK_KW_TYPE);
  if (!check(TOK_ID)) {
    syntax_error("Expected type alias name");
  }
  char *name = take_string();
  match(TOK_ASSIGN);
  match_type();
  while (!check(TOK_NEWLINE)) {
    match(TOK_PIPE);
    match_type();
  }
  while (check(TOK_NEWLINE))
    match(TOK_NEWLINE);
  ast_node_t *type_node = ast_new_type_alias(name, NULL, line);
  return type_node;
}

int main(void) {
  int rc = scanner_init(stdin);
  if (rc != ERR_OK) {
    fprintf(stderr, "Failed to initialize scanner.\n");
    return rc;
  }
  advance();
  ast_node_t *ast_root = parse_program();
  ast_print(ast_root);
  scanner_free();
  ast_free(ast_root);
  return ERR_OK;
} 

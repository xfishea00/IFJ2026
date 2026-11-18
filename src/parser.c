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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
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

static token_type_t peek(void) { return parser.current_token.type; }

static bool check(token_type_t type) { return peek() == type; }

static bool check_literal(void) {
  return check(TOK_INT_LIT) || check(TOK_STRING_LIT) || check(TOK_DOUBLE_LIT);
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

void match_type() {
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
  }
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
void parse_expect_type(void);
static ast_node_t *parse_return_statement(void);

static ast_node_t * parse_if_statement(void);
static ast_node_t * parse_else_tail(void);
void parse_else_continuation(void);

static ast_node_t *parse_while_statement(void);
static ast_node_t *parse_for_loop(void);
void parse_for_body(void);
static ast_node_t * parse_for_inline(int line);
static ast_node_t * parse_for_multiline(int line);
void parse_inline_guards(ast_list_t *list);
void parse_multiline_guards(ast_list_t *list);
static ast_node_t *parse_gen_source(int line, char *var_name);

static void parse_param_groups(ast_list_t **param_lists, int *list_count);
static ast_node_t * parse_param(void);

void parse_func_call(void);
void parse_arg_groups(ast_list_t **lists, int *list_count);
static ast_node_t *parse_arg_list(void);
void parse_arg_tail(void);
void parse_arg_next(void);

ast_node_t* parse_expression(void);
//void parse_non_id_expression(void);
void parse_expr_match_tail(void);
void parse_list_def(void);
void parse_list_elements(void);
void parse_list_tail(void);
void parse_list_next(void);

void parse_case_list(void);
void parse_model(void);
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
  
  parse_expect_type();
  match(TOK_ASSIGN);
  ast_node_t *body = parse_block_statement();

  ast_node_t *function = ast_new_func_def(name, is_main, NULL, param_lists, param_list_count, body, line);
  
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

void parse_expect_type(void) {
  if (check(TOK_COLON)) {
    match(TOK_COLON);
    match_type();
  }
  // else epsilon
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
  match_type();
  ast_node_t *param = ast_new_param(name, NULL, line);
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
   return call;

  } else if (check(TOK_KW_MATCH)) {
    // makeshift solution
    parse_expr_match_tail();
  } else {
    // Id expression or operation tail
    if (parser.current_token.type <= 59 &&
        parser.current_token.type >= 33) { // makeshift operators
      return parse_expression();
    }
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
  parse_expect_type();
  match(TOK_ASSIGN);
  ast_node_t *init = parse_expression();
  ast_node_t *var = ast_new_var_def(mutable, name, NULL, NULL, line);

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

ast_node_t *parse_expression(void) {
  if (check(TOK_KW_LIST)) {
    parse_list_def();
  } else {
    // filters out expressions for testing purposes
    int lpar_cnt = 0;
    while ((token_is_keyword(parser.current_token.type) != true ||
            (check(TOK_KW_TRUE) || check(TOK_KW_FALSE)) ||
            check(TOK_KW_MATCH)) &&
           !(check(TOK_RPAR) && lpar_cnt == 0)) {
      if (parser.current_token.type == TOK_NEWLINE && lpar_cnt == 0) {
        break;
      } else if (parser.current_token.type == TOK_NEWLINE && lpar_cnt != 0) {
        advance();
        if(parser.current_token.type != TOK_RPAR) {
          syntax_error("Provisional expression parser reject expression");
        }
        continue;
      } else if (check(TOK_RPAR)) {
        lpar_cnt -= 1;
      } else if (check(TOK_LPAR)) {
        lpar_cnt += 1;
      } else if (check(TOK_KW_IF)) {
        parse_if_statement();
      } else if (check(TOK_KW_MATCH)) {
        parse_expr_match_tail();
      } else if (check_identifier("until") || check_identifier("by") || check_identifier("to")){
        break;
      }
      advance();
    }
    // parse_non_id_expression();
    // parse_expr_match_tail();
  }
  char* test = (char*)malloc(5*sizeof(char));
  strcpy(test, "test");
  return ast_new_id(test, 42); //temporary return for testing
}

//void parse_non_id_expression(void) {
  /*
  if (check(TOK_KW_LIST)) {
    parse_list_def();
  } else if (check_literal()) {
    match_literal();
  } else {
    parse_expression(); //makeshift
    //advance();
  }
  */
 // parse_expression();
//}

void parse_expr_match_tail(void) {
  if (check(TOK_KW_MATCH)) {
    match(TOK_KW_MATCH);
    while (check(TOK_NEWLINE))
      match(TOK_NEWLINE);
    // makeshift
    match(TOK_INDENT);
    parse_case_list();
    match(TOK_DEDENT);
  }
  // else epsilon
}

void parse_list_def(void) {
  match(TOK_KW_LIST);
  match(TOK_LPAR);
  parse_list_elements();
  match(TOK_RPAR);
}

void parse_list_elements(void) {
  if (check(TOK_RPAR)) {
    return; // epsilon
  }
  parse_expression();
  parse_list_tail();
}

void parse_list_tail(void) {
  if (check(TOK_COMMA)) {
    match(TOK_COMMA);
    parse_list_next();
  }
  // else epsilon
}

void parse_list_next(void) {
  if (check(TOK_NEWLINE)) {
    match(TOK_NEWLINE);
    parse_list_elements();
  } else {
    parse_expression();
    parse_list_tail();
  }
}

void parse_case_list(void) {
  if (check(TOK_KW_CASE)) {
    match(TOK_KW_CASE);
    parse_model();
    if (check(TOK_KW_IF)) {
      match(TOK_KW_IF);
      parse_expression();
    }
    match(TOK_ARROW);
    parse_expression();
    while (check(TOK_NEWLINE))
      match(TOK_NEWLINE);
    parse_case_list();
  }
  // else epsilon
}

// Reminder to add class constructor variant
void parse_model(void) {
  if (check_literal()) {
    match_literal();
  } else if (check(TOK_UNDERSCORE)) {
    match(TOK_UNDERSCORE);
  } else if (check(TOK_ID)) {
    match(TOK_ID);
    if (check(TOK_COLON)) {
      match(TOK_COLON);
      match_type();
    } else if (check(TOK_LPAR)) {
      // class constructor
      match(TOK_LPAR);
      while (!check(TOK_RPAR)) {
        match(TOK_ID);
        if (!check(TOK_RPAR))
          match(TOK_COMMA);
      }
      match(TOK_RPAR);
    }
  } else {
    syntax_error("Invalid pattern model in match case");
  }
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

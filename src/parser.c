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
ast_node_t *parse_program(void);
void parse_prologue(void);
void parse_top_level_list(ast_list_t *defs);
ast_node_t *parse_func_declaration(bool is_main);
void parse_entry_function(void);
void parse_type_definition(void);
void parse_class(void);

void parse_block(void);
void parse_statement_list(void);
void parse_statement_list_tail(void);
void parse_statement(void);
void parse_id_stmt_tail(void);
ast_node_t * parse_block_statement(void);

void parse_var_definition(void);
void parse_expect_type(void);
void parse_return_statement(void);
void parse_return_tail(void);

void parse_if_statement(void);
void parse_else_tail(void);
void parse_else_continuation(void);

void parse_while_statement(void);
void parse_for_loop(void);
void parse_for_body(void);
void parse_for_inline(void);
void parse_for_multiline(void);
void parse_inline_guards(void);
void parse_multiline_guards(void);
void parse_gen_source(void);
void parse_step(void);

static void parse_param_groups(ast_list_t **param_lists, int *list_count);
static ast_node_t * parse_param(void);

void parse_func_call(void);
void parse_arg_groups(void);
void parse_arg_list(void);
void parse_arg_tail(void);
void parse_arg_next(void);

void parse_expression(void);
void parse_non_id_expression(void);
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
  parse_prologue();

  ast_node_t *program = ast_new_program(current_line);
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
      parse_class();
      break;
    case TOK_KW_TYPE:
      parse_type_definition();
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
  char* name;
  if(check(TOK_ID)) name = take_string();
  ast_list_t *param_lists = NULL;
  int param_list_count = 0;
  parse_param_groups(&param_lists, &param_list_count);
  
  parse_expect_type();
  match(TOK_ASSIGN);
  ast_node_t *body = parse_block_statement();

  ast_node_t *function = ast_new_func_def(name, is_main, NULL, param_lists, param_list_count, NULL, current_line);
  
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
    parse_block();
  } else {
    parse_statement();
  }
  return;
}

void parse_block(void) {
  match(TOK_NEWLINE);
  match(TOK_INDENT);
  parse_statement_list();
  while (!check(TOK_DEDENT)) // unsure
    parse_statement_list();
  match(TOK_DEDENT);
}

void parse_statement_list(void) {
  parse_statement();
  parse_statement_list_tail();
}

void parse_statement_list_tail(void) {
  if (check(TOK_NEWLINE) || check(TOK_SEMICOLON)) {
    parse_separator();
    if (check(TOK_DEDENT)) {
      // End of block reached after separator
      return;
    }
    parse_statement_list();
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

void parse_statement(void) {
  switch (peek()) {
  case TOK_ID:
    match(TOK_ID);
    parse_id_stmt_tail();
    break;
  case TOK_KW_VAL:
  case TOK_KW_VAR:
    parse_var_definition();
    break;
  case TOK_KW_IF:
    parse_if_statement();
    break;
  case TOK_KW_WHILE:
    parse_while_statement();
    break;
  case TOK_KW_FOR:
    parse_for_loop();
    break;
  case TOK_KW_RETURN:
    parse_return_statement();
    break;
  default:
    parse_non_id_expression();
    break;
  }
}

void parse_id_stmt_tail(void) {
  if (check(TOK_ASSIGN)) {
    // Assignment: id = expression
    match(TOK_ASSIGN);
    parse_expression();
  } else if (check(TOK_LPAR)) {
    // Function call: id(args)...
    parse_arg_groups();
  } else if (check(TOK_KW_MATCH)) {
    // makeshift solution
    parse_expr_match_tail();
  } else {
    // Id expression or operation tail
    if (parser.current_token.type <= 59 &&
        parser.current_token.type >= 33) { // makeshift operators
      parse_expression();
    }
  }
}

void parse_var_definition(void) {
  if (check(TOK_KW_VAL)) {
    match(TOK_KW_VAL);
  } else {
    match(TOK_KW_VAR);
  }
  match(TOK_ID);
  parse_expect_type();
  match(TOK_ASSIGN);
  parse_expression();
}

void parse_return_statement(void) {
  match(TOK_KW_RETURN);
  parse_return_tail();
}

void parse_return_tail(void) {
  if (check(TOK_NEWLINE) || check(TOK_SEMICOLON) || check(TOK_DEDENT) ||
      check(TOK_EOF)) {
    return; // epsilon
  }
  parse_expression();
}

void parse_if_statement(void) {
  match(TOK_KW_IF);
  parse_expression();
  match(TOK_KW_THEN);
  parse_block_statement();
  parse_else_tail();
}

void parse_else_tail(void) {
  if (check(TOK_KW_ELSE)) {
    match(TOK_KW_ELSE);
    parse_else_continuation();
  }
  // else epsilon
}

void parse_else_continuation(void) {
  if (check(TOK_KW_IF)) {
    match(TOK_KW_IF);
    parse_expression();
    match(TOK_KW_THEN);
    parse_block_statement();
    parse_else_tail();
  } else {
    parse_block_statement();
  }
}

void parse_while_statement(void) {
  match(TOK_KW_WHILE);
  parse_expression();
  match(TOK_KW_DO);
  parse_block();
}

void parse_for_loop(void) {
  match(TOK_KW_FOR);
  parse_for_body();
}

void parse_for_body(void) {
  if (check(TOK_NEWLINE)) {
    parse_for_multiline();
  } else {
    parse_for_inline();
  }
}

void parse_for_inline(void) {
  match(TOK_ID);
  match(TOK_LARROW);
  parse_gen_source();
  parse_inline_guards();
  match(TOK_KW_DO);
  parse_block_statement();
}

void parse_for_multiline(void) {
  while (check(TOK_NEWLINE))
    match(TOK_NEWLINE);
  match(TOK_INDENT);
  match(TOK_ID);
  match(TOK_LARROW);
  parse_gen_source();
  parse_multiline_guards();
  match(TOK_DEDENT);
  while (check(TOK_NEWLINE))
    match(TOK_NEWLINE);
  match(TOK_KW_DO);
  parse_block_statement();
}

void parse_inline_guards(void) {
  if (check(TOK_KW_IF)) {
    match(TOK_KW_IF);
    parse_expression();
    parse_inline_guards();
  }
  // else epsilon
}

void parse_multiline_guards(void) {
  if (check(TOK_NEWLINE)) {
    while (check(TOK_NEWLINE))
      match(TOK_NEWLINE);
    if (check(TOK_KW_IF)) {
      match(TOK_KW_IF);
      parse_expression();
      parse_multiline_guards();
    }
  }
  // else epsilon
}

void parse_gen_source(void) {
  parse_expression();
  if (check(TOK_ID)) {
    match(TOK_ID);
    parse_expression();
    parse_step();
  }
}

void parse_step(void) {
  if (check(TOK_ID)) {
    match(TOK_ID);
    parse_expression();
  }
  // else epsilon
}

void parse_arg_groups(void) {
  if (check(TOK_LPAR)) {
    match(TOK_LPAR);
    parse_arg_list();
    match(TOK_RPAR);
    parse_arg_groups();
  }
  // else epsilon
}

void parse_arg_list(void) {
  if (check(TOK_RPAR)) {
    return; // epsilon
  }
  parse_expression();
  parse_arg_tail();
}

void parse_arg_tail(void) {
  if (check(TOK_COMMA)) {
    match(TOK_COMMA);
    parse_arg_next();
  }
  // else epsilon
}

void parse_arg_next(void) {
  if (check(TOK_NEWLINE)) {
    match(TOK_NEWLINE);
    parse_arg_list();
  } else {
    parse_expression();
    parse_arg_tail();
  }
}

void parse_expression(void) {
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
      }
      advance();
    }
    // parse_non_id_expression();
    // parse_expr_match_tail();
  }
}

void parse_non_id_expression(void) {
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
  parse_expression();
}

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

void parse_class(void) {
  match(TOK_KW_CASE);
  match(TOK_KW_CLASS);
  match(TOK_ID);
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
}

void parse_type_definition(void) {
  match(TOK_KW_TYPE);
  match(TOK_ID);
  match(TOK_ASSIGN);
  match_type();
  while (!check(TOK_NEWLINE)) {
    match(TOK_PIPE);
    match_type();
  }
  while (check(TOK_NEWLINE))
    match(TOK_NEWLINE);
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

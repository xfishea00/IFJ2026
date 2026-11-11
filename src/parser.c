/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    parser.c
 * Authors: Adam James Fisher (xfishea00)
 * Brief:   Syntax analysis
 */

#include "scanner.h"
#include "token.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Error codes 
#define ERR_OK 0
#define ERR_LEXICAL 1
#define ERR_SYNTAX 2
#define ERR_SEM_UNDEFINED 3
#define ERR_SEM_TYPE 4

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

// helper functions
void advance(void) { scanner_next(&(parser.current_token)); }

token_type_t peek(void) { return parser.current_token.type; }

bool check(token_type_t type) { return parser.current_token.type == type; }

bool check_literal() {
  return parser.current_token.type == TOK_INT_LIT ||
         parser.current_token.type == TOK_STRING_LIT ||
         parser.current_token.type == TOK_DOUBLE_LIT;
}


void match(token_type_t expected) {
  if (check(expected)) {
    advance();
  } else {
    char err_msg[100];
    snprintf(err_msg, sizeof(err_msg), "Expected token type %d, but got %d",
             expected, parser.current_token.type);
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

void match_type(){
    switch (peek())
    {
    case TOK_KW_INT:
        match(TOK_KW_INT);
        break;
    case TOK_KW_DOUBLE:
        match(TOK_KW_DOUBLE);
        break;
    case TOK_KW_STRING:
        match(TOK_KW_STRING);
        break;
    case TOK_KW_UNIT:
        match(TOK_KW_UNIT);
        break;
    case TOK_ID:
        match(TOK_ID);
        break;
    case TOK_KW_BOOLEAN:
        match(TOK_KW_BOOLEAN);
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
    if (peek() == TOK_PIPE){
      match(TOK_PIPE);
      match_type();
    }
}

// Forward Declarations of Parsing Functions 
void parse_program(void);
void parse_prologue(void);
void parse_top_level_list(void);
void parse_func_declaration(void);
void parse_entry_function(void);
void parse_type_definition(void);
void parse_class(void);

void parse_block(void);
void parse_statement_list(void);
void parse_statement_list_tail(void);
void parse_statement(void);
void parse_id_stmt_tail(void);
void parse_block_statement(void);

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

void parse_param_groups(void);
void parse_param_list(void);
void parse_param_tail(void);
void parse_param_next(void);
void parse_param(void);

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

void parse_program(void) {
  parse_prologue();
  parse_top_level_list();
  match(TOK_EOF);
}

void parse_prologue(void) {
  match(TOK_KW_IMPORT);
  match(TOK_ID);
  match(TOK_DOT);
  match(TOK_MUL);
  match(TOK_NEWLINE);
}

void parse_top_level_list(void) {
  switch (peek()) {
  case TOK_KW_DEF:
    parse_func_declaration();
    parse_top_level_list();
    break;
  case TOK_AT_MAIN:
    parse_entry_function();
    parse_top_level_list();
    break;
  case TOK_KW_CASE:
    parse_class();
    parse_top_level_list();
    break;
  case TOK_KW_TYPE:
    parse_type_definition();
    parse_top_level_list();
    break;
  case TOK_EOF:
    // epsilon
    break;
  case TOK_NEWLINE:
    match(TOK_NEWLINE);
    parse_top_level_list();
    break;
  default:
    syntax_error("Unexpected token in top-level list");
  }
}

void parse_entry_function(void) {
  match(TOK_AT_MAIN);
  parse_func_declaration();
}

void parse_func_declaration(void) {
  match(TOK_KW_DEF);
  match(TOK_ID);
  parse_param_groups();
  parse_expect_type();
  match(TOK_ASSIGN);
  parse_block_statement();
}

void parse_expect_type(void) {
  if (check(TOK_COLON)) {
    match(TOK_COLON);
    match_type();
  }
  // else epsilon
}

void parse_param_groups(void) {
  if (check(TOK_LPAR)) {
    match(TOK_LPAR);
    parse_param_list();
    match(TOK_RPAR);
    parse_param_groups();
  }
  // else epsilon
}

void parse_param_list(void) {
  if (check(TOK_ID)) {
    parse_param();
    parse_param_tail();
  }
  // else epsilon
}

void parse_param_tail(void) {
  if (check(TOK_COMMA)) {
    match(TOK_COMMA);
    parse_param_next();
  }
  // else epsilon
}

void parse_param_next(void) {
  if (check(TOK_NEWLINE)) {
    match(TOK_NEWLINE);
    parse_param_list();
  } else if (check(TOK_ID)) {
    parse_param();
    parse_param_tail();
  } else {
    syntax_error("Invalid sequence after comma in parameter list");
  }
}

void parse_param(void) {
  match(TOK_ID);
  match(TOK_COLON);
  match_type();
}

void parse_block_statement(void) {
  if (check(TOK_NEWLINE)) {
    parse_block();
  } else {
    parse_statement();
  }
}

void parse_block(void) {
  match(TOK_NEWLINE);
  match(TOK_INDENT);
  parse_statement_list();
  while(!check(TOK_DEDENT)) //unsure
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
  } else if (check(TOK_KW_MATCH)){
    //makeshift solution
    parse_expr_match_tail();
  } else {
    // Id expression or operation tail
    if(parser.current_token.type <= 59 && parser.current_token.type >= 33){ //makeshift operators
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
    //filters out expressions for testing purposes
    int lpar_cnt = 0;
    while((token_is_keyword(parser.current_token.type) != true || (check(TOK_KW_TRUE) || check(TOK_KW_FALSE)) || check(TOK_KW_MATCH)) && !(check(TOK_RPAR) && lpar_cnt == 0)){
      if(parser.current_token.type == TOK_NEWLINE){
        break;
      } else if (check(TOK_RPAR)){
        lpar_cnt -= 1;
      } else if (check(TOK_LPAR)){
        lpar_cnt += 1;
      } else if (check(TOK_KW_IF)){
        parse_if_statement();
      } else if (check(TOK_KW_MATCH)){
        parse_expr_match_tail();
      }
      advance();
    }
    //parse_non_id_expression();
    //parse_expr_match_tail();
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
      //makeshift
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

//Reminder to add class constructor variant
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
    } else if (check(TOK_LPAR)){
      //class constructor
      match(TOK_LPAR);
      while(!check(TOK_RPAR)){
        match(TOK_ID);
        if(!check(TOK_RPAR))
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
  parse_param_list();
  match(TOK_RPAR);
}

void parse_type_definition(void) {
  match(TOK_KW_TYPE);
  match(TOK_ID);
  match(TOK_ASSIGN);
  match_type();
  while(!check(TOK_NEWLINE)){
    match(TOK_PIPE);
    match_type();
  }
  while (check(TOK_NEWLINE))
      match(TOK_NEWLINE);
}

int main(void) {
  if (scanner_init(stdin) != ERR_OK) {
    fprintf(stderr, "Failed to initialize scanner.\n");
    return ERR_LEXICAL;
  }
  advance();
  parse_program();
  scanner_free();
  return ERR_OK;
}

/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    main.c
 * Brief:   compiler entry point
 */

#include "ast.h"
#include "error.h"
#include "generator.h"
#include "parser.h"
#include "scanner.h"
#include "semantic.h"
#include "symbol.h"
#include "type.h"
#include <stdio.h>

int main(void) {
  int rc = scanner_init(stdin);
  if (rc != ERR_OK) {
    return rc;
  }

  // TODO: parser interface
  ast_node_t *root = parse();
  rc = semantic_analyze(root);
  if (rc == ERR_OK) {
    rc = generate_code(root);
  }
  ast_free(root);
  symbols_free_all();
  types_free_all();
  scanner_free();

  return rc;
}

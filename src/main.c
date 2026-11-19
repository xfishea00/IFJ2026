/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    main.c
 * Brief:   compiler entry point
 */

#include "ast.h"
#include "error.h"
// #include "generator.h"
#include "parser.h"
#include "scanner.h"
#include "semantic.h"
#include "symbol.h"
#include <stdio.h>

int main(void) {
  int rc = scanner_init(stdin);
  if (rc != ERR_OK) {
    return rc;
  }

  // TODO: parser interface
  ast_node_t *root = parse();

  /*
  rc = semantic_analyze(root); // 3-10, 99
  if (rc == ERR_OK) {
    // TODO: rc = generate_code(root); - IFJcode26 to stdout
  }
  */
  ast_free(root);
  //symbols_free_all(); // symbols live until the generator is done
  scanner_free();
  return rc;
}

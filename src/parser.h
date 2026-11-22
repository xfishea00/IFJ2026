#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

// parses the program from the scanner, returns the AST root
ast_node_t *parse(void);

#endif // PARSER_H
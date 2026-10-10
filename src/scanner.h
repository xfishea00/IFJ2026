/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    scanner.h
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   interface of the lexical analyzer
 */

#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

#include <stdio.h>

// prepares the scanner to read from the given input
// returns 0 on success, 99 on allocation error
int scanner_init(FILE *input);

// reads the next token into *out
// returns 0 on success, 1 on lexical error, 99 on allocation error
int scanner_next(token_t *out);

// frees memory used by scanner
void scanner_free(void);

#endif // SCANNER_H

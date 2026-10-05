/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    str.h
 * Authors: Michal Řezníček (xreznim00)
 */

#ifndef STR_H
#define STR_H

#include <stddef.h>

// dynamic string that grows as characters are added
typedef struct {
  char *data;      // content, always ends with '\0'
  size_t length;   // number of characters without '\0'
  size_t capacity; // allocated size of data in bytes
} str_t;

// allocate an empty string; returns 0 on success, 99 on allocation error
int str_init(str_t *str);

// frees the string memory
void str_free(str_t *str);

// makes the string empty, keeps allocated memory
void str_clear(str_t *str);

// adds one character to the end, grows the buffer if needed
// returns 0 on success, 99 on allocation error
int str_append(str_t *str, char c);

// returns the content and leaves the string empty (caller must free it)
// returns NULL on allocation error
char *str_take(str_t *str);

#endif // STR_H

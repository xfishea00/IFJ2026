/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    str.c
 * Authors: Michal Řezníček (xreznim00)
 */

#include "str.h"

#include <stdlib.h>

#define STR_INIT_CAPACITY 16 // initial buffer size in bytes

int str_init(str_t *str) {
  str->data = malloc(STR_INIT_CAPACITY);
  if (str->data == NULL) {
    return 99; // allocation error
  }
  str->data[0] = '\0';
  str->length = 0;
  str->capacity = STR_INIT_CAPACITY;
  return 0; // success
}

void str_free(str_t *str) {
  free(str->data);
  str->data = NULL;
  str->length = 0;
  str->capacity = 0;
}

void str_clear(str_t *str) {
  str->length = 0;
  if (str->data != NULL) {
    str->data[0] = '\0';
  }
}

int str_append(str_t *str, char c) {
  if (str->length + 1 >= str->capacity) {
    size_t new_capacity;
    if (str->capacity == 0) {
      new_capacity = STR_INIT_CAPACITY;
    } else {
      new_capacity = str->capacity * 2;
    }
    char *new_data = realloc(str->data, new_capacity);
    if (new_data == NULL) {
      return 99; // allocation error
    }
    str->data = new_data;
    str->capacity = new_capacity;
  }
  str->data[str->length] = c;
  str->length++;
  str->data[str->length] = '\0';
  return 0; // success
}

char *str_take(str_t *str) {
  if (str->data == NULL) { // nothing append since last take
    char *empty = malloc(1);
    if (empty != NULL) {
      empty[0] = '\0';
    }
    return empty; // NULL only if malloc failed
  }
  char *result = str->data;
  str->data = NULL;
  str->length = 0;
  str->capacity = 0;
  return result; // caller must free the returned string
}
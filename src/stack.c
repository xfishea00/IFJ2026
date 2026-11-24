/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    stack.c
 * Authors: Maros Magoč (xmagocm00)
 * Brief:   Stack implementation
 */

#include "stack.h"
#include "error.h"
#include "expression_parser.h"
#include <stdio.h>
#include <stdlib.h>

void stack_error(const char *message) {
  fprintf(stderr, "%s", message);
  exit(ERR_INTERNAL);
}

void stack_init(stack_t *s) { s->top = -1; }

bool is_empty(stack_t *s) { return s->top == -1; }

bool is_full(stack_t *s) { return s->top >= MAX_SIZE - 1; }

void stack_push(stack_t *s, stack_element_t t) {
  if (is_full(s)) {
    stack_error("Stack overflow\n");
  }
  s->elements[++s->top] = t;
}

stack_element_t stack_pop(stack_t *s) {
  if (is_empty(s)) {
    stack_error("Stack underflow\n");
  }
  return s->elements[s->top--];
}

stack_element_t stack_peek(stack_t *s) {
  if (is_empty(s)) {
    stack_error("Empty stack\n");
  }
  return s->elements[s->top];
}

int stack_top_element_type_pos(stack_t *s, stack_element_type_t type) {
  if (is_empty(s)) {
    stack_error("Empty stack\n");
  }
  for (int i = s->top; i >= 0; i--) {
    if (s->elements[i].type == type) {
      return i;
    }
  }
  return -1;
}

void stack_insert(stack_t *s, stack_element_t t, int pos) {
  if (pos < 0 || is_full(s) || pos > s->top + 1) {
    stack_error("Stack overflox\n");
  }
  for (int i = ++s->top; i > pos; i--) {
    s->elements[i] = s->elements[i - 1];
  }
  s->elements[pos] = t;
}

bool is_lpar_in_stack(stack_t *s) {
  if (is_empty(s)) {
    stack_error("Empty stack\n");
  }
  for (int i = s->top; i >= 0; i--) {
    if (s->elements[i].type == STACK_TERMINAL &&
        s->elements[i].terminal == PRE_LPAR) {
      return true;
    }
  }
  return false;
}
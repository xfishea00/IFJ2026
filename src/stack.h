/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    stack.h
 * Authors: Maros Magoč (xmagocm00)
 * Brief:   Stack implementation interface
 */

#ifndef STACK_H
#define STACK_H

#include "expression_parser.h"

#define MAX_SIZE 200

typedef struct {
  stack_element_t elements[MAX_SIZE];
  int top;
} stack_t;

void stack_error(const char *message);          // helper error function
void stack_init(stack_t *s);                    // initializes stack
bool is_empty(stack_t *s);                      // checks if stack is empty
bool is_full(stack_t *s);                       // checks if stack is full
void stack_push(stack_t *s, stack_element_t t); // add element to stack
stack_element_t
stack_pop(stack_t *s); // remove the top element from stack, returns the element
stack_element_t
stack_peek(stack_t *s); // return the top element from stack without removing it
int stack_top_element_type_pos(
    stack_t *s,
    stack_element_type_t type); // finds the position of the topmost element of
                                // the given type on the stack
void stack_insert(
    stack_t *s, stack_element_t t,
    int pos); // inserts an element at a specific position on the stack

#endif
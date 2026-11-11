/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    stack.h
 * Authors: Maros Magoč (xmagocm00)
 * Brief:   Stack implementation interface
 */

#ifndef STACK_H
#define STACK_H

#include "token.h"
#include <stdio.h>
#include <stdbool.h>

#define MAX_SIZE = 100

typedef struct {
    precedence_terminal_t terminals[MAX_SIZE];
    int top;
} Stack;

void stack_init(Stack *s);            // initializes stack
void stack_push(Stack *s, precedence_terminal_t t); // add token to stack
precedence_terminal_t stack_pop(Stack *s);          // remove the top token from stack, returns the token
precedence_terminal_t stack_peek(Stack *s);         // return the top token from stack without removing it
bool is_empty(Stack *s);              // checks if stack is empty
bool is_full(Stack *s);               // checks if stack is full

#endif
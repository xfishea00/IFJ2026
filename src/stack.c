/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    stack.c
 * Authors: Maros Magoč (xmagocm00)
 * Brief:   Stack implementation
 */

#include "stack.h"
#include "token.h"
#include "scanner.h"
#include <stdio.h>
#include <stdbool.h>

#define stack_error(const char *message);

void stack_init(Stack *s) {
    stack->top = -1;
}

bool is_empty(Stack *s) {
    return s->top == -1;
}

bool is_full(Stack *s) {
    return s->top >= MAX_SIZE - 1;
}

void stack_push(Stack *s, precedence_terminal_t t) {
    if (is_full(s)) {
        stack_error("Stack overflow\n");
        return;
    }
    s->terminals[++s->top] = t;
}

precedence_terminal_t stack_pop(Stack *s) {
    if (is_empty(s)) {
        stack_error("Stack underflow\n");
        return -1;
    }
    return s->terminals[s->top--];
}

precedence_terminal_t stack_peek(Stack *s) {
    if (is_empty(s)) {
        stack_error("Empty stack\n")
        return -1;
    }
    return s->terminals[s->top];
}
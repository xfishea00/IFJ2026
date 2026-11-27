/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    expression_parser.c
 * Authors: Maros Magoč (xmagocm00)
 * Brief:   Expression syntax analysis
 */

#include "expression_parser.h"
#include "error.h"
#include "stack.h"
#include "token.h"
#include <string.h>

// 

precedence_terminal_t map_token_to_precedence(token_t t) {
  if (t.type == TOK_ID && t.string_val != NULL &&
      ( strcmp(t.string_val, "to") == 0 || 
        strcmp(t.string_val, "until") == 0 ||
        strcmp(t.string_val, "by") == 0)) {
    return PRE_END;
  }
  switch (t.type) {
    case TOK_ID:         return PRE_ID;
    case TOK_INT_LIT:    return PRE_ID;
    case TOK_DOUBLE_LIT: return PRE_ID;
    case TOK_STRING_LIT: return PRE_ID;
    case TOK_KW_TRUE:    return PRE_ID;
    case TOK_KW_FALSE:   return PRE_ID;
    case TOK_KW_LIST:    return PRE_ID;
    case TOK_PLUS:       return PRE_PLUS;
    case TOK_MINUS:      return PRE_MINUS;
    case TOK_CONCAT:     return PRE_CONCAT;
    case TOK_MUL:        return PRE_MUL;
    case TOK_DIV:        return PRE_DIV;
    case TOK_LT:         return PRE_LT;
    case TOK_GT:         return PRE_GT;
    case TOK_LE:         return PRE_LE;
    case TOK_GE:         return PRE_GE;
    case TOK_EQ:         return PRE_EQ;
    case TOK_NEQ:        return PRE_NEQ;
    case TOK_NOT:        return PRE_NOT;
    case TOK_AND:        return PRE_AND;
    case TOK_OR:         return PRE_OR;
    case TOK_DOT:        return PRE_DOT;
    case TOK_LPAR:       return PRE_LPAR;
    case TOK_RPAR:       return PRE_RPAR;
    case TOK_NEWLINE:    return PRE_END;
    case TOK_DEDENT:     return PRE_END;
    case TOK_COMMA:      return PRE_END;
    case TOK_COLON:      return PRE_END;
    case TOK_SEMICOLON:  return PRE_END;
    case TOK_KW_THEN:    return PRE_END;
    case TOK_KW_DO:      return PRE_END;
    case TOK_KW_ELSE:    return PRE_END;
    case TOK_LARROW:     return PRE_END;
    case TOK_RBRACKET:   return PRE_END;
    case TOK_LBRACKET:   return PRE_END;
    case TOK_ARROW:      return PRE_END;
    case TOK_PIPE:       return PRE_END;
    case TOK_KW_MATCH:   return PRE_END;
    case TOK_EOF:        return PRE_END;
    default:             return PRE_INVALID;
  }
}

const precedence_t table[TERMINATOR_NUM][TERMINATOR_NUM] = {
    //       id  +  - ++  *  /  <  > <= >= == !=  ! u- && ||  .  (  )  $
    /* id */ {X, H, H, H, H, H, H, H, H, H, H, H, X, X, H, H, H, X, H, H},
    /* +  */ {L, H, H, H, L, L, H, H, H, H, H, H, L, L, H, H, L, L, H, H},
    /* -  */ {L, H, H, H, L, L, H, H, H, H, H, H, L, L, H, H, L, L, H, H},
    /* ++ */ {L, H, H, H, L, L, H, H, H, H, H, H, L, L, H, H, L, L, H, H},
    /* *  */ {L, H, H, H, H, H, H, H, H, H, H, H, L, L, H, H, L, L, H, H},
    /* /  */ {L, H, H, H, H, H, H, H, H, H, H, H, L, L, H, H, L, L, H, H},
    /* <  */ {L, L, L, L, L, L, X, X, X, X, X, X, L, L, H, H, L, L, H, H},
    /* >  */ {L, L, L, L, L, L, X, X, X, X, X, X, L, L, H, H, L, L, H, H},
    /* <= */ {L, L, L, L, L, L, X, X, X, X, X, X, L, L, H, H, L, L, H, H},
    /* >= */ {L, L, L, L, L, L, X, X, X, X, X, X, L, L, H, H, L, L, H, H},
    /* == */ {L, L, L, L, L, L, X, X, X, X, X, X, L, L, H, H, L, L, H, H},
    /* != */ {L, L, L, L, L, L, X, X, X, X, X, X, L, L, H, H, L, L, H, H},
    /* !  */ {L, H, H, H, H, H, H, H, H, H, H, H, L, L, H, H, L, L, H, H},
    /* u- */ {L, H, H, H, H, H, H, H, H, H, H, H, L, L, H, H, L, L, H, H},
    /* && */ {L, L, L, L, L, L, L, L, L, L, L, L, L, L, H, H, L, L, H, H},
    /* || */ {L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, H, L, L, H, H},
    /* .  */ {L, H, H, H, H, H, H, H, H, H, H, H, H, H, H, H, H, L, H, H},
    /* (  */ {L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, E, X},
    /* )  */ {X, H, H, H, H, H, H, H, H, H, H, H, X, X, H, H, H, X, H, H},
    /* $  */ {L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, X, X}};

const stack_element_t end_marker = {.type = STACK_TERMINAL,
                                    .terminal = PRE_END};
const stack_element_t less_marker = {.type = STACK_MARKER};

// AI-generated part; prompt: "Add a way to handle the freeing of ast nodes in the stack"
void free_stack(stack_t *stack) {
  for (int i = 0; i <= stack->top; i++) {
    if (stack->elements[i].type == STACK_EXPRESSION) {
      ast_free(stack->elements[i].node);
      stack->elements[i].node = NULL;
    }
  }
}
// End of AI-generated part

bool is_boundary(token_t t) {
  if (t.type == TOK_ID && t.string_val != NULL &&
      (strcmp(t.string_val, "to") == 0 ||
       strcmp(t.string_val, "until") == 0 ||
       strcmp(t.string_val, "by") == 0)) {
    return true;
  }
  switch (t.type) {
  case TOK_RPAR:
  case TOK_NEWLINE:
  case TOK_DEDENT:
  case TOK_COMMA:
  case TOK_COLON:
  case TOK_SEMICOLON:
  case TOK_KW_THEN:
  case TOK_KW_DO:
  case TOK_KW_ELSE:
  case TOK_LARROW:
  case TOK_RBRACKET:
  case TOK_ARROW:
  case TOK_PIPE:
  case TOK_KW_MATCH:
  case TOK_EOF:
    return true;
  default:
    return false;
  }
}

bool is_not_expression_end(stack_t s, token_t current_token) {
  if (s.top >= 1 && 
      s.elements[s.top - 1].type == STACK_TERMINAL &&
      s.elements[s.top - 1].terminal == PRE_END &&
      s.elements[s.top].type == STACK_EXPRESSION &&
      is_boundary(current_token)) {
    return false;
  }
  return true;
}

bool is_binary_operator(precedence_terminal_t term, ast_op_t *op) {
  switch (term) {
    case PRE_PLUS:
      *op = OP_ADD;
      return true;
    case PRE_MINUS:
      *op = OP_SUB;
      return true;
    case PRE_CONCAT:
      *op = OP_CONCAT;
      return true;
    case PRE_MUL:
      *op = OP_MUL;
      return true;
    case PRE_DIV:
      *op = OP_DIV;
      return true;
    case PRE_LT:
      *op = OP_LT;
      return true;
    case PRE_GT:
      *op = OP_GT;
      return true;
    case PRE_LE:
      *op = OP_LE;
      return true;
    case PRE_GE:
      *op = OP_GE;
      return true;
    case PRE_EQ:
      *op = OP_EQ;
      return true;
    case PRE_NEQ:
      *op = OP_NEQ;
      return true;
    case PRE_AND:
      *op = OP_AND;
      return true;
    case PRE_OR:
      *op = OP_OR;
      return true;
    default:
      return false;
  }
}

int parse_non_id_expression(token_t *current_token,
                            advance_token_f advance_token,
                            parse_primary_f parse_primary, // part of the AI-generated code used later
                            ast_node_t *initial_expression, // part of the AI-generated code used later
                            ast_node_t **expression) {
  
  if (current_token == NULL || advance_token == NULL ||
      parse_primary == NULL || expression == NULL) {
    ast_free(initial_expression);
    return ERR_INTERNAL;
  }
  stack_t stack;
  stack_init(&stack);
  stack_push(&stack, end_marker);
  // AI-generated part; prompt: "Help me figure out why the tests are failing"
  if (initial_expression != NULL) {
    stack_element_t initial = { .type = STACK_EXPRESSION,
                                .node = initial_expression };
    stack_push(&stack, initial);
  }
  // End of AI-generated part

  int rc;

  while (is_not_expression_end(stack, *current_token)) {
    int top_terminal_pos = stack_top_element_type_pos(&stack, STACK_TERMINAL);
    if (top_terminal_pos == -1) {
      free_stack(&stack);
      return ERR_INTERNAL;
    }

    stack_element_t topmost_terminal = stack.elements[top_terminal_pos];
    stack_element_t input_terminal = {
        .type = STACK_TERMINAL,
        .terminal = map_token_to_precedence(*current_token),
        .node = NULL,
        .line = current_token->line};
    if (input_terminal.terminal == PRE_INVALID) {
      free_stack(&stack);
      return ERR_SYNTAX;
    } else if (input_terminal.terminal == PRE_MINUS &&
               stack.elements[stack.top].type != STACK_EXPRESSION) {
      input_terminal.terminal = PRE_UMINUS;
    }
    precedence_t precedence = table[topmost_terminal.terminal][input_terminal.terminal];

    switch (precedence) {
      case PRE_LOWER:
        // AI-generated part; prompt: "Help me figure out why the tests are failing"
        if (input_terminal.terminal == PRE_ID) {
          ast_node_t *operand = parse_primary();
          if (operand == NULL) {
            free_stack(&stack);
            return ERR_INTERNAL;
          }
          stack_element_t expression = { .type = STACK_EXPRESSION, .node = operand };
          stack_push(&stack, expression);
        // End of AI-generated part

        } else {
          stack_insert(&stack, less_marker, top_terminal_pos + 1);
          stack_push(&stack, input_terminal);
          rc = advance_token(current_token);
          if (rc != ERR_OK) {
            free_stack(&stack);
            return rc;
          }
        }
        break;

      case PRE_HIGHER: {
        int top_marker_pos = stack_top_element_type_pos(&stack, STACK_MARKER);
        if (top_marker_pos == -1) {
          free_stack(&stack);
          return ERR_SYNTAX;
        }
        stack_element_t expression = { .type = STACK_EXPRESSION, .node = NULL };
        int expression_len = stack.top - top_marker_pos;
        switch (expression_len) {
          case 3: {
            stack_element_t a = stack.elements[top_marker_pos + 1];
            stack_element_t b = stack.elements[top_marker_pos + 2];
            stack_element_t c = stack.elements[top_marker_pos + 3];
            ast_op_t op;

            if (a.type == STACK_TERMINAL &&
                a.terminal == PRE_LPAR &&
                b.type == STACK_EXPRESSION &&
                c.type == STACK_TERMINAL &&
                c.terminal == PRE_RPAR) {
              expression = b;
            } else {
                if (a.type != STACK_EXPRESSION ||
                        b.type != STACK_TERMINAL ||
                        c.type != STACK_EXPRESSION ||
                        !is_binary_operator(b.terminal, &op)) {
                  free_stack(&stack);
                  return ERR_SYNTAX;
                }
                expression.node = ast_new_binary(op, a.node, c.node, b.line);
            }
            break;
          }

          case 2: {
            stack_element_t a = stack.elements[top_marker_pos + 1];
            stack_element_t b = stack.elements[top_marker_pos + 2];
            ast_op_t op;

            if (a.type == STACK_TERMINAL &&
                a.terminal == PRE_LPAR &&
                b.type == STACK_TERMINAL &&
                b.terminal == PRE_RPAR) {
              expression.node = ast_new_unit(a.line);
              break;
            }

            if (a.type != STACK_TERMINAL ||
                b.type != STACK_EXPRESSION) {
              free_stack(&stack);
              return ERR_SYNTAX;
            }
            if (a.terminal == PRE_NOT) {
              op = OP_NOT;
            } else if (a.terminal == PRE_UMINUS) {
              op = OP_NEG;
            } else {
              free_stack(&stack);
              return ERR_SYNTAX;
            }

            expression.node = ast_new_unary(op, b.node, a.line);
            break;
          }

          case 1: {
            stack_element_t a = stack.elements[top_marker_pos + 1];

            if (a.type != STACK_EXPRESSION) {
              free_stack(&stack);
              return ERR_SYNTAX;
            }
            
            expression = a;
            break;
          }

          default:
            free_stack(&stack);
            return ERR_SYNTAX;
          }

        if (expression.node == NULL) {
          free_stack(&stack);
          return ERR_INTERNAL;
        }
        stack.elements[top_marker_pos] = expression;
        stack.top = top_marker_pos;
        break;
      }

      case PRE_EQUAL:
        stack_push(&stack, input_terminal);
        rc = advance_token(current_token);
        if (rc != ERR_OK) {
          free_stack(&stack);
          return rc;
        }
        break;

      case PRE_ERROR:
        free_stack(&stack);
        return ERR_SYNTAX;

      default:
        free_stack(&stack);
        return ERR_INTERNAL;
    }
  }
  *expression = stack.elements[stack.top].node;
  return ERR_OK;
}
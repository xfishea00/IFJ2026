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
    case TOK_EOF:        return PRE_END;
    default:             return PRE_INVALID;
  }
}

const precedence_t table[TERMINATOR_NUM][TERMINATOR_NUM] = {
    //       id  +  - ++  *  /  <  > <= >= == !=  ! && ||  .  (  )  $
    /* id */ {X, H, H, H, H, H, H, H, H, H, H, H, X, H, H, H, X, H, H},
    /* +  */ {L, H, H, H, L, L, H, H, H, H, H, H, L, H, H, L, L, H, H},
    /* -  */ {L, H, H, H, L, L, H, H, H, H, H, H, L, H, H, L, L, H, H},
    /* ++ */ {L, H, H, H, L, L, H, H, H, H, H, H, L, H, H, L, L, H, H},
    /* *  */ {L, H, H, H, H, H, H, H, H, H, H, H, L, H, H, L, L, H, H},
    /* /  */ {L, H, H, H, H, H, H, H, H, H, H, H, L, H, H, L, L, H, H},
    /* <  */ {L, L, L, L, L, L, X, X, X, X, X, X, L, H, H, L, L, H, H},
    /* >  */ {L, L, L, L, L, L, X, X, X, X, X, X, L, H, H, L, L, H, H},
    /* <= */ {L, L, L, L, L, L, X, X, X, X, X, X, L, H, H, L, L, H, H},
    /* >= */ {L, L, L, L, L, L, X, X, X, X, X, X, L, H, H, L, L, H, H},
    /* == */ {L, L, L, L, L, L, X, X, X, X, X, X, L, H, H, L, L, H, H},
    /* != */ {L, L, L, L, L, L, X, X, X, X, X, X, L, H, H, L, L, H, H},
    /* !  */ {L, H, H, H, H, H, H, H, H, H, H, H, L, H, H, L, L, H, H},
    /* && */ {L, L, L, L, L, L, L, L, L, L, L, L, L, H, H, L, L, H, H},
    /* || */ {L, L, L, L, L, L, L, L, L, L, L, L, L, L, H, L, L, H, H},
    /* .  */ {L, H, H, H, H, H, H, H, H, H, H, H, H, H, H, H, L, H, H},
    /* (  */ {L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, E, X},
    /* )  */ {X, H, H, H, H, H, H, H, H, H, H, H, X, H, H, H, X, H, H},
    /* $  */ {L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, L, X, X}};

const stack_element_t end_marker = {.type = STACK_TERMINAL,
                                    .terminal = PRE_END};
const stack_element_t less_marker = {.type = STACK_MARKER};
const stack_element_t expression = {.type = STACK_EXPRESSION};

const rule_t rules[RULE_NUM] = {
    {.n = 1,  .left_side = expression, .right_side = "E + E"},
    {.n = 2,  .left_side = expression, .right_side = "E - E"},
    {.n = 3,  .left_side = expression, .right_side = "E ++ E"},
    {.n = 4,  .left_side = expression, .right_side = "E * E"},
    {.n = 5,  .left_side = expression, .right_side = "E / E"},
    {.n = 6,  .left_side = expression, .right_side = "E < E"},
    {.n = 7,  .left_side = expression, .right_side = "E > E"},
    {.n = 8,  .left_side = expression, .right_side = "E <= E"},
    {.n = 9,  .left_side = expression, .right_side = "E >= E"},
    {.n = 10, .left_side = expression, .right_side = "E == E"},
    {.n = 11, .left_side = expression, .right_side = "E != E"},
    {.n = 12, .left_side = expression, .right_side = "!E"},
    {.n = 13, .left_side = expression, .right_side = "E && E"},
    {.n = 14, .left_side = expression, .right_side = "E || E"},
    {.n = 15, .left_side = expression, .right_side = "E.E"},
    {.n = 16, .left_side = expression, .right_side = "(E)"},
    {.n = 17, .left_side = expression, .right_side = "id"}};

bool is_boundary(token_type_t t, stack_t s) {
  switch (t) {
  case TOK_RPAR:
    if (is_lpar_in_stack(&s)) return true;
    else return false;
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
  case TOK_EOF:
    return true;
  default:
    return false;
  }
}

bool is_not_expression_end(stack_t s, token_type_t current_token_type) {
  if (s.top >= 1 && 
      s.elements[s.top - 1].type == STACK_TERMINAL &&
      s.elements[s.top - 1].terminal == PRE_END &&
      s.elements[s.top].type == STACK_EXPRESSION &&
      is_boundary(current_token_type, s)) {
    return false;
  }
  return true;
}

int parse_non_id_expression(token_t *current_token,
                            advance_token_f advance_token) {
  stack_t stack;
  stack_init(&stack);
  stack_push(&stack, end_marker);
  int rc;

  while (is_not_expression_end(stack, current_token->type)) {
    int top_terminal_pos = stack_top_element_type_pos(&stack, STACK_TERMINAL);
    if (top_terminal_pos == -1) {
      return ERR_INTERNAL;
    }

    stack_element_t topmost_terminal = stack.elements[top_terminal_pos];
    stack_element_t input_terminal = {
        .type = STACK_TERMINAL,
        .terminal = map_token_to_precedence(*current_token)};
    if (input_terminal.terminal == PRE_INVALID) {
      return ERR_SYNTAX;
    }
    precedence_t precedence =
        table[topmost_terminal.terminal][input_terminal.terminal];

    switch (precedence) {
    case PRE_LOWER:
      stack_insert(&stack, less_marker, top_terminal_pos + 1);
      stack_push(&stack, input_terminal);
      rc = advance_token(current_token);
      if (rc != ERR_OK) {
        return rc;
      }
      break;

    case PRE_HIGHER: {
      rule_t rule = {.n = -1};
      int top_marker_pos = stack_top_element_type_pos(&stack, STACK_MARKER);
      if (top_marker_pos == -1) {
        return ERR_SYNTAX;
      }
      int expression_len = stack.top - top_marker_pos;
      switch (expression_len) {
      case 3: {
        stack_element_t a = stack.elements[top_marker_pos + 1];
        stack_element_t b = stack.elements[top_marker_pos + 2];
        stack_element_t c = stack.elements[top_marker_pos + 3];

        if (a.type == expression.type && b.type == STACK_TERMINAL &&
            b.terminal == PRE_PLUS && c.type == expression.type) {
          rule = rules[0];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_MINUS && c.type == expression.type) {
          rule = rules[1];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_CONCAT && c.type == expression.type) {
          rule = rules[2];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_MUL && c.type == expression.type) {
          rule = rules[3];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_DIV && c.type == expression.type) {
          rule = rules[4];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_LT && c.type == expression.type) {
          rule = rules[5];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_GT && c.type == expression.type) {
          rule = rules[6];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_LE && c.type == expression.type) {
          rule = rules[7];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_GE && c.type == expression.type) {
          rule = rules[8];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_EQ && c.type == expression.type) {
          rule = rules[9];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_NEQ && c.type == expression.type) {
          rule = rules[10];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_AND && c.type == expression.type) {
          rule = rules[12];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_OR && c.type == expression.type) {
          rule = rules[13];
        } else if (a.type == expression.type && b.type == STACK_TERMINAL &&
                   b.terminal == PRE_DOT && c.type == expression.type) {
          rule = rules[14];
        } else if (a.type == STACK_TERMINAL && a.terminal == PRE_LPAR &&
                   b.type == expression.type && c.type == STACK_TERMINAL &&
                   c.terminal == PRE_RPAR) {
          rule = rules[15];
        } else {
          return ERR_SYNTAX;
        }
        break;
      }

      case 2: {
        stack_element_t a = stack.elements[top_marker_pos + 1];
        stack_element_t b = stack.elements[top_marker_pos + 2];

        if (a.type == STACK_TERMINAL && a.terminal == PRE_NOT &&
            b.type == expression.type) {
          rule = rules[11];
        } else if (a.type == STACK_TERMINAL && a.terminal == PRE_MINUS &&
                   b.type == expression.type) {
          rule = (rule_t){.n = -1, .left_side = expression};
        } else {
          return ERR_SYNTAX;
        }
        break;
      }

      case 1: {
        stack_element_t a = stack.elements[top_marker_pos + 1];

        if (a.type == STACK_TERMINAL && a.terminal == PRE_ID) {
          rule = rules[16];
        } else {
          return ERR_SYNTAX;
        }
        break;
      }

      default:
        return ERR_SYNTAX;
      }
      stack.elements[top_marker_pos] = rule.left_side;
      stack.top = top_marker_pos;
      break;
    }

    case PRE_EQUAL:
      stack_push(&stack, input_terminal);
      rc = advance_token(current_token);
      if (rc != ERR_OK) {
        return rc;
      }
      break;

    case PRE_ERROR:
      return ERR_SYNTAX;

    default:
      return ERR_INTERNAL;
    }
  }
  return ERR_OK;
}
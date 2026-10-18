/*
 * Project: Implementace překladače imperativního jazyka IFJ26
 * File:    scanner_test.c
 * Authors: Michal Řezníček (xreznim00)
 * Brief:   tests of the scanner
 *
 * AI-generated test (Claude Opus 5.5). Prompt: "napis test pro scanner, over
 * operatory, @main, identifikatory a klicova slova, cisla, retezce, komentare
 * a odsazeni vcetne chybnych vstupu a at vypise co selhalo"
 */

#define _POSIX_C_SOURCE 200809L // fmemopen

#include "../src/error.h"
#include "../src/scanner.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define GOT_SIZE 2048

static int failures = 0;

// keyword names in the order of token.h
static const char *kw_names[] = {
    "def",   "do",   "while",     "Int",     "Double", "String",
    "if",    "else", "then",      "return",  "import", "var",
    "val",   "Unit", "for",       "Boolean", "match",  "List",
    "class", "case", "extension", "type",
};

static const char *type_name(token_type_t t) {
  switch (t) {
  case TOK_EOF:
    return "EOF";
  case TOK_NEWLINE:
    return "NEWLINE";
  case TOK_INDENT:
    return "INDENT";
  case TOK_DEDENT:
    return "DEDENT";
  case TOK_AT_MAIN:
    return "AT_MAIN";
  case TOK_LT:
    return "LT";
  case TOK_LE:
    return "LE";
  case TOK_GT:
    return "GT";
  case TOK_GE:
    return "GE";
  case TOK_ASSIGN:
    return "ASSIGN";
  case TOK_EQ:
    return "EQ";
  case TOK_NEQ:
    return "NEQ";
  case TOK_PLUS:
    return "PLUS";
  case TOK_MINUS:
    return "MINUS";
  case TOK_MUL:
    return "MUL";
  case TOK_DIV:
    return "DIV";
  case TOK_COLON:
    return "COLON";
  case TOK_COMMA:
    return "COMMA";
  case TOK_DOT:
    return "DOT";
  case TOK_SEMICOLON:
    return "SEMICOLON";
  case TOK_LPAR:
    return "LPAR";
  case TOK_RPAR:
    return "RPAR";
  default:
    return "?";
  }
}

// appends formatted text to buf
static void add(char *buf, const char *fmt, ...) {
  size_t len = strlen(buf);
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf + len, GOT_SIZE - len, fmt, args);
  va_end(args);
}

// appends s to buf, special characters written as escape sequences
static void add_escaped(char *buf, const char *s) {
  for (; *s != '\0'; s++) {
    if (*s == '\n') {
      add(buf, "\\n");
    } else if (*s == '\t') {
      add(buf, "\\t");
    } else if (*s == '\r') {
      add(buf, "\\r");
    } else if (*s == '\\') {
      add(buf, "\\\\");
    } else {
      add(buf, "%c", *s);
    }
  }
}

// appends one token in the form used by the expected strings
static void add_token(char *buf, const token_t *tok) {
  if (tok->type == TOK_ID) {
    add(buf, "ID(%s)", tok->string_val);
  } else if (tok->type == TOK_INT_LIT) {
    add(buf, "INT(%" PRId64 ")", tok->int_val);
  } else if (tok->type == TOK_DOUBLE_LIT) {
    add(buf, "DOUBLE(%g)", tok->double_val);
  } else if (tok->type == TOK_STRING_LIT) {
    add(buf, "STR(");
    add_escaped(buf, tok->string_val);
    add(buf, ")");
  } else if (token_is_keyword(tok->type)) {
    add(buf, "KW(%s)", kw_names[tok->type - TOK_KW_FIRST]);
  } else {
    add(buf, "%s", type_name(tok->type));
  }
}

// runs the scanner on input and compares the tokens with expected
// with_lines: every token is prefixed with its line number, e.g. "2:ID(x)"
static void test(const char *name, const char *input, const char *expected,
                 int with_lines) {
  FILE *f = fmemopen((void *)input, strlen(input), "r");
  char got[GOT_SIZE] = "";

  if (f == NULL || scanner_init(f) != ERR_OK) {
    add(got, "init failed");
  } else {
    for (int i = 0; i < 100; i++) {
      token_t tok;
      int rc = scanner_next(&tok);
      if (i > 0) {
        add(got, " ");
      }
      if (rc != ERR_OK) {
        add(got, "ERR(%d)", rc);
        token_free(&tok);
        break;
      }
      if (with_lines) {
        add(got, "%d:", tok.line);
      }
      add_token(got, &tok);
      token_free(&tok);
      if (tok.type == TOK_EOF) {
        break;
      }
    }
    scanner_free();
  }
  if (f != NULL) {
    fclose(f);
  }

  if (strcmp(got, expected) == 0) {
    printf("ok   %s\n", name);
  } else {
    failures++;
    printf("FAIL %s\n     got:      %s\n     expected: %s\n", name, got,
           expected);
  }
}

#define T(name, input, expected) test(name, input, expected, 0)
#define TL(name, input, expected) test(name, input, expected, 1)

int main(void) {
  // inputs without '\n' at the end get NEWLINE before EOF (last line ends)

  printf("--- operators and @main\n");
  T("all operators", "<= < >= > == = != ( ) : , ; . + - * /",
    "LE LT GE GT EQ ASSIGN NEQ LPAR RPAR COLON COMMA SEMICOLON DOT PLUS "
    "MINUS MUL DIV NEWLINE EOF");
  T("no spaces", "a<=b", "ID(a) LE ID(b) NEWLINE EOF");
  T("<<", "<<", "LT LT NEWLINE EOF");
  T("===", "===", "EQ ASSIGN NEWLINE EOF");
  T("! alone", "!", "ERR(1)");
  T("! space =", "! =", "ERR(1)");
  T("@main", "@main def", "AT_MAIN KW(def) NEWLINE EOF");
  T("@main(", "@main(", "AT_MAIN LPAR NEWLINE EOF");
  T("@mainx", "@mainx", "ERR(1)");
  T("@ main", "@ main", "ERR(1)");
  T("@mai", "@mai", "ERR(1)");
  T("unknown char #", "#", "ERR(1)");
  T("unknown char after id", "a $", "ID(a) ERR(1)");
  T("CR", "a\r\n", "ID(a) ERR(1)");

  printf("--- identifiers and keywords\n");
  T("identifiers", "abc _a a_1 A9 __x",
    "ID(abc) ID(_a) ID(a_1) ID(A9) ID(__x) NEWLINE EOF");
  T("_ alone", "_", "ERR(1)");
  T("_ then space", "_ a", "ERR(1)");
  T("all keywords",
    "def do while Int Double String if else then return import var val Unit "
    "for Boolean match List class case extension type",
    "KW(def) KW(do) KW(while) KW(Int) KW(Double) KW(String) KW(if) KW(else) "
    "KW(then) KW(return) KW(import) KW(var) KW(val) KW(Unit) KW(for) "
    "KW(Boolean) KW(match) KW(List) KW(class) KW(case) KW(extension) "
    "KW(type) NEWLINE EOF");
  T("case sensitive", "int Def DEF", "ID(int) ID(Def) ID(DEF) NEWLINE EOF");
  T("keyword prefix", "defx do_ Ints", "ID(defx) ID(do_) ID(Ints) NEWLINE EOF");
  T("true is ID", "true false", "ID(true) ID(false) NEWLINE EOF");

  printf("--- numbers\n");
  T("integers", "0 42 1_000 1__0",
    "INT(0) INT(42) INT(1000) INT(10) NEWLINE EOF");
  T("decimals", "0.5 .5 0.0 1_0.2_5",
    "DOUBLE(0.5) DOUBLE(0.5) DOUBLE(0) DOUBLE(10.25) NEWLINE EOF");
  T("exponents", "1e5 1E5 1e+5 1e-3 1.5e-3 0e0 .5e1 1e1_0",
    "DOUBLE(100000) DOUBLE(100000) DOUBLE(100000) DOUBLE(0.001) "
    "DOUBLE(0.0015) DOUBLE(0) DOUBLE(5) DOUBLE(1e+10) NEWLINE EOF");
  T("suffix d", "1d 1D 0d 1.5d 1e2d",
    "DOUBLE(1) DOUBLE(1) DOUBLE(0) DOUBLE(1.5) DOUBLE(100) NEWLINE EOF");
  T("minus is operator", "-5 1-2",
    "MINUS INT(5) INT(1) MINUS INT(2) NEWLINE EOF");
  T("int then .5", "1 .5", "INT(1) DOUBLE(0.5) NEWLINE EOF");
  T("member access", "x.y", "ID(x) DOT ID(y) NEWLINE EOF");
  T("underflow -> 0 (our decision)", "1e-400", "DOUBLE(0) NEWLINE EOF");
  T("01", "01", "ERR(1)");
  T("0_1", "0_1", "ERR(1)");
  T("00.5", "00.5", "ERR(1)");
  T("1_", "1_", "ERR(1)");
  T("1.", "1.", "ERR(1)");
  T("1._5", "1._5", "ERR(1)");
  T("1_.5", "1_.5", "ERR(1)");
  T("1.5_", "1.5_", "ERR(1)");
  T("1e", "1e", "ERR(1)");
  T("1e+", "1e+", "ERR(1)");
  T("1e_5", "1e_5", "ERR(1)");
  T("1e5_", "1e5_", "ERR(1)");
  T("123abc", "123abc", "ERR(1)");
  T("1.5f", "1.5f", "ERR(1)");
  T("1dd", "1dd", "ERR(1)");
  T("0x1", "0x1", "ERR(1)");
  T("1e400 overflow", "1e400", "ERR(1)");

  printf("--- strings\n");
  T("simple", "\"abc\"", "STR(abc) NEWLINE EOF");
  T("empty", "\"\" x", "STR() ID(x) NEWLINE EOF");
  T("all escapes", "\"\\n\\t\\r\\\"\\\\\"", "STR(\\n\\t\\r\"\\\\) NEWLINE EOF");
  T("real tab inside", "\"a\tb\"", "STR(a\\tb) NEWLINE EOF");
  T("bad escape", "\"a\\x\"", "ERR(1)");
  T("unterminated", "\"abc", "ERR(1)");
  T("LF inside", "\"a\nb\"", "ERR(1)");
  T("backslash at end", "\"a\\", "ERR(1)");
  T("multiline", "\"\"\"abc\"\"\"", "STR(abc) NEWLINE EOF");
  T("multiline empty", "\"\"\"\"\"\"", "STR() NEWLINE EOF");
  T("multiline with LF", "\"\"\"a\nb\"\"\"", "STR(a\\nb) NEWLINE EOF");
  T("multiline no escapes", "\"\"\"a\\nb\"\"\"", "STR(a\\\\nb) NEWLINE EOF");
  T("multiline quotes inside", "\"\"\"a\"b\"\"c\"\"\"",
    "STR(a\"b\"\"c) NEWLINE EOF");
  T("multiline unterminated", "\"\"\"a", "ERR(1)");
  T("multiline unterminated \"", "\"\"\"a\"", "ERR(1)");
  T("multiline unterminated \"\"", "\"\"\"a\"\"", "ERR(1)");
  TL("lines after multiline", "\"\"\"a\nb\"\"\" x",
     "1:STR(a\\nb) 2:ID(x) 2:NEWLINE 2:EOF");

  printf("--- lines\n");
  TL("newlines", "a\nb\n", "1:ID(a) 1:NEWLINE 2:ID(b) 2:NEWLINE 3:EOF");

  printf("--- comments\n");
  T("division", "a / b", "ID(a) DIV ID(b) NEWLINE EOF");
  T("division at end", "a /", "ID(a) DIV NEWLINE EOF");
  T("line comment + LF", "a // x\nb", "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("line comment + EOF", "a // x", "ID(a) NEWLINE EOF");
  T("comment-only line", "// only\n", "EOF");
  T("two line comments", "a // x\n// y\nb", "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("//* is line comment", "a //* x\nb", "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("empty block", "a /**/ b", "ID(a) ID(b) NEWLINE EOF");
  T("block", "a /* x */ b", "ID(a) ID(b) NEWLINE EOF");
  T("block without spaces", "x/*a*/y", "ID(x) ID(y) NEWLINE EOF");
  T("/*/ does not close", "a /*/ x */ b", "ID(a) ID(b) NEWLINE EOF");
  T("stars inside", "a /* ** */ b", "ID(a) ID(b) NEWLINE EOF");
  T("/***/", "a /***/ b", "ID(a) ID(b) NEWLINE EOF");
  T("nested", "a /* /* x */ */ b", "ID(a) ID(b) NEWLINE EOF");
  T("nested twice", "a /* /* /* x */ */ */ b", "ID(a) ID(b) NEWLINE EOF");
  T("block then division", "a /* x */ / b", "ID(a) DIV ID(b) NEWLINE EOF");
  T("block-only line", "/* a */\n", "EOF");
  T("block with LF", "a /* x\ny */\nb", "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("block with LF at EOF", "a /* x\ny */", "ID(a) NEWLINE EOF");
  T("nested with LF", "a /* /* x\n */ */\nb",
    "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("LF only in nested part", "a /* x /* \n */ y */\nb",
    "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("LF after star", "a /* *\n*/\nb", "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("LF after slash", "a /* /\n*/\nb", "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("multiline block-only lines", "a\n/* x\ny */\nb",
    "ID(a) NEWLINE ID(b) NEWLINE EOF");
  TL("lines after block", "a /*\n\n*/\nb",
     "1:ID(a) 1:NEWLINE 4:ID(b) 4:NEWLINE 4:EOF");
  T("comment in string", "\"/* x */\"", "STR(/* x */) NEWLINE EOF");
  T("unterminated", "/* x", "ERR(1)");
  T("unterminated after star", "/* x *", "ERR(1)");
  T("unterminated after slash", "/* x /", "ERR(1)");
  T("unterminated nested", "/* /* x */", "ERR(1)");
  T("unterminated with LF", "a /* x\n", "ID(a) ERR(1)");

  printf("--- indentation\n");
  T("empty file", "", "EOF");
  T("only spaces", "   ", "EOF");
  T("only LFs", "\n\n", "EOF");
  T("same level", "a\nb\n", "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("simple block", "a\n  b\nc\n",
    "ID(a) NEWLINE INDENT ID(b) NEWLINE DEDENT ID(c) NEWLINE EOF");
  T("two levels, dedent both", "a\n  b\n    c\nd\n",
    "ID(a) NEWLINE INDENT ID(b) NEWLINE INDENT ID(c) NEWLINE DEDENT DEDENT "
    "ID(d) NEWLINE EOF");
  T("dedent one level", "a\n  b\n    c\n  d\n",
    "ID(a) NEWLINE INDENT ID(b) NEWLINE INDENT ID(c) NEWLINE DEDENT ID(d) "
    "NEWLINE DEDENT EOF");
  T("DEDENTs at EOF", "a\n  b\n    c\n",
    "ID(a) NEWLINE INDENT ID(b) NEWLINE INDENT ID(c) NEWLINE DEDENT DEDENT "
    "EOF");
  T("EOF without LF in block", "a\n  b",
    "ID(a) NEWLINE INDENT ID(b) NEWLINE DEDENT EOF");
  T("first line indented", "  a\n", "INDENT ID(a) NEWLINE DEDENT EOF");
  T("empty lines ignored", "a\n\n  \n  b\n\nc\n",
    "ID(a) NEWLINE INDENT ID(b) NEWLINE DEDENT ID(c) NEWLINE EOF");
  T("comment lines ignored", "a\n    // x\n  b\n/* y */\nc\n",
    "ID(a) NEWLINE INDENT ID(b) NEWLINE DEDENT ID(c) NEWLINE EOF");
  T("inconsistent dedent", "a\n    b\n  c\n",
    "ID(a) NEWLINE INDENT ID(b) NEWLINE ERR(1)");
  T("tab in indentation", "a\n\tb\n", "ID(a) NEWLINE ERR(1)");
  T("spaces and tab in indentation", "a\n  \tb\n", "ID(a) NEWLINE ERR(1)");
  T("tab on empty line is ok", "a\n\t\nb\n", "ID(a) NEWLINE ID(b) NEWLINE EOF");
  T("tab in the middle is ok", "a\tb\n", "ID(a) ID(b) NEWLINE EOF");
  T("LF inside parens", "f(a,\n    b)\nc\n",
    "ID(f) LPAR ID(a) COMMA ID(b) RPAR NEWLINE ID(c) NEWLINE EOF");
  T("nested parens with LF", "f((a)\n)\nb\n",
    "ID(f) LPAR LPAR ID(a) RPAR RPAR NEWLINE ID(b) NEWLINE EOF");
  T("block starting with (", "a\n  (b)\nc\n",
    "ID(a) NEWLINE INDENT LPAR ID(b) RPAR NEWLINE DEDENT ID(c) NEWLINE EOF");
  T("multiline string in block", "a\n  \"\"\"x\ny\"\"\"\nb\n",
    "ID(a) NEWLINE INDENT STR(x\\ny) NEWLINE DEDENT ID(b) NEWLINE EOF");
  T("program",
    "def f() =\n"
    "  if x then\n"
    "    a\n"
    "  else\n"
    "    b\n"
    "c\n",
    "KW(def) ID(f) LPAR RPAR ASSIGN NEWLINE INDENT KW(if) ID(x) KW(then) "
    "NEWLINE INDENT ID(a) NEWLINE DEDENT KW(else) NEWLINE INDENT ID(b) "
    "NEWLINE DEDENT DEDENT ID(c) NEWLINE EOF");
  TL("lines of INDENT/DEDENT", "a\n  b\nc\n",
     "1:ID(a) 1:NEWLINE 2:INDENT 2:ID(b) 2:NEWLINE 3:DEDENT 3:ID(c) "
     "3:NEWLINE 4:EOF");

  if (failures == 0) {
    printf("\nall OK\n");
  } else {
    printf("\n%d FAILED\n", failures);
  }
  return failures != 0;
}

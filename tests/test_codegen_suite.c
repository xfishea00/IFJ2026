/*
 * These tests were created by AI (Copilot Fusion gpt-6-luna)
 * Promt: "Based on inclueded files and the project structure create complete test file,
 * so that i can see the expected output and actual output and tests summary."
 * Build from the repository root:
 *   gcc -std=c11 -pedantic -Wall -Wextra tests/test_codegen_suite.c \
 *       -o /tmp/test_codegen_suite
 * Run after building ifj2026:
 *   /tmp/test_codegen_suite [-i path/to/ic26int] [path/to/ifj2026]
 */
#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define VALID_TEST_DIR "tests/syntax/valid"
#define MAX_LINE 4096

typedef struct {
  char **items;
  size_t count;
  size_t capacity;
} string_list_t;

typedef struct {
  const char *name;
  const char *source;
  const char *input;
  const char *expected_output;
  int expected_status;
} runtime_test_t;

static const runtime_test_t RUNTIME_TESTS[] = {
    {"hello", "@main def main(): Unit =\n  print(\"Hello, IFJ26!\")\n", "",
     "Hello, IFJ26!", 0},
    {"integer_literal", "@main def main(): Unit =\n  print(42)\n", "", "42", 0},
    {"arithmetic_precedence", "@main def main(): Unit =\n  print(7 + 2 * 3)\n",
     "", "13", 0},
    {"parenthesized_arithmetic",
     "@main def main(): Unit =\n  print((7 + 2) * 3)\n", "", "27", 0},
    {"integer_division", "@main def main(): Unit =\n  print(7 / 2)\n", "", "3",
     0},
    {"unary_minus", "@main def main(): Unit =\n  print(-7)\n", "", "-7", 0},
    {"double_arithmetic",
     "@main def main(): Unit =\n  print(toStr(1.5 + 2.0))\n", "", "3.50", 0},
    {"string_concatenation",
     "@main def main(): Unit =\n  print(\"IFJ\" + \"26\")\n", "", "IFJ26", 0},
    {"string_escape", "@main def main(): Unit =\n  print(\"first\\nsecond\")\n",
     "", "first\nsecond", 0},
    {"comparison",
     "@main def main(): Unit =\n  print(toStr(3 < 4))\n  print(\" \")\n"
     "  print(toStr(4 == 5))\n",
     "", "true false", 0},
    {"if_else",
     "@main def main(): Unit =\n  if 3 < 4 then\n    print(\"yes\")\n"
     "  else\n    print(\"no\")\n",
     "", "yes", 0},
    {"while_loop",
     "@main def main(): Unit =\n  var i = 1\n  while i <= 3 do\n"
     "    print(i)\n    i = i + 1\n",
     "", "123", 0},
    {"boolean_logic",
     "@main def main(): Unit =\n"
     "  print(toStr(true && !false))\n"
     "  print(toStr(false || true))\n",
     "", "truetrue", 0},
    {"function_call",
     "def twice(x: Int): Int = x * 2\n"
     "@main def main(): Unit =\n  print(twice(21))\n",
     "", "42", 0},
    {"recursive_function",
     "def factorial(n: Int): Int =\n"
     "  if n <= 1 then 1 else n * factorial(n - 1)\n"
     "@main def main(): Unit =\n  print(factorial(5))\n",
     "", "120", 0},
    {"function_evaluation_order",
     "def show(value: Int): Int =\n"
     "  print(value)\n"
     "  value\n"
     "@main def main(): Unit =\n"
     "  print(show(1) + show(2))\n",
     "", "123", 0},
    {"string_builtins",
     "@main def main(): Unit =\n  print(strLength(\"hello\"))\n"
     "  print(subStr(\"abcdef\", 1, 3))\n",
     "", "5bcd", 0},
    {"character_builtins",
     "@main def main(): Unit =\n  print(ord(\"A\", 0))\n  print(chr(66))\n", "",
     "65B", 0},
    {"read_integer",
     "@main def main(): Unit =\n  val value = readInt()\n"
     "  print(value + 1)\n",
     "41\n", "42", 0},
    {"read_line", "@main def main(): Unit =\n  print(readLine())\n", "hello\n",
     "hello\n", 0},
    {"read_integer_invalid", "@main def main(): Unit =\n  print(readInt())\n",
     "not-an-integer\n", "0", 0},
    {"short_circuit",
     "def side(): Boolean =\n  print(\"S\")\n  true\n"
     "@main def main(): Unit =\n  val left = false && side()\n"
     "  val right = true || side()\n  print(toStr(left))\n"
     "  print(toStr(right))\n",
     "", "falsetrue", 0},
    {"for_to_range",
     "@main def main(): Unit =\n"
     "  for i <- 1 to 5 do\n"
     "    print(i)\n",
     "", "12345", 0},
    {"for_until_range",
     "@main def main(): Unit =\n"
     "  for i <- 0 until 5 do\n"
     "    print(i)\n",
     "", "01234", 0},
    {"for_descending_range",
     "@main def main(): Unit =\n"
     "  for i <- 5 to 1 by -2 do\n"
     "    print(i)\n",
     "", "531", 0},
    {"list_index",
     "@main def main(): Unit =\n"
     "  val values: List[Int] = List(10, 20, 30)\n"
     "  print(values(1))\n",
     "", "20", 0},
    {"list_to_string",
     "@main def main(): Unit =\n"
     "  print(List(1, 2, 3))\n",
     "", "List(1, 2, 3)", 0},
    {"empty_list",
     "@main def main(): Unit =\n"
     "  val values: List[Int] = List()\n"
     "  print(values)\n",
     "", "List()", 0},
    {"unit_output",
     "@main def main(): Unit =\n"
     "  print(())\n",
     "", "()", 0},
    {"runtime_substr_error",
     "@main def main(): Unit =\n  print(\"before\\n\")\n"
     "  subStr(\"abc\", 2, 1)\n",
     "", "before\n", 40},
    {"runtime_list_index_error",
     "@main def main(): Unit =\n"
     "  print(\"before\\n\")\n"
     "  val values: List[Int] = List(1, 2)\n"
     "  print(values(5))\n",
     "", "before\n", 40},
};

static void fail_allocation(void) {
  perror("allocation");
  exit(EXIT_FAILURE);
}

static char *duplicate_string(const char *text) {
  char *copy = strdup(text);
  if (copy == NULL) {
    fail_allocation();
  }
  return copy;
}

static void list_append(string_list_t *list, const char *text) {
  if (list->count == list->capacity) {
    size_t capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    char **items = realloc(list->items, capacity * sizeof(*items));
    if (items == NULL) {
      fail_allocation();
    }
    list->items = items;
    list->capacity = capacity;
  }
  list->items[list->count++] = duplicate_string(text);
}

static int compare_strings(const void *left, const void *right) {
  const char *const *a = left;
  const char *const *b = right;
  return strcmp(*a, *b);
}

static bool has_suffix(const char *text, const char *suffix) {
  size_t text_length = strlen(text);
  size_t suffix_length = strlen(suffix);
  return text_length >= suffix_length &&
         strcmp(text + text_length - suffix_length, suffix) == 0;
}

static void free_list(string_list_t *list) {
  for (size_t i = 0; i < list->count; i++) {
    free(list->items[i]);
  }
  free(list->items);
}

static int load_valid_tests(string_list_t *tests) {
  DIR *directory = opendir(VALID_TEST_DIR);
  if (directory == NULL) {
    perror(VALID_TEST_DIR);
    return -1;
  }

  struct dirent *entry;
  while ((entry = readdir(directory)) != NULL) {
    if (!has_suffix(entry->d_name, ".scala")) {
      continue;
    }
    char path[MAX_LINE];
    int length =
        snprintf(path, sizeof(path), "%s/%s", VALID_TEST_DIR, entry->d_name);
    if (length < 0 || (size_t)length >= sizeof(path)) {
      fprintf(stderr, "Test path is too long: %s\n", entry->d_name);
      closedir(directory);
      return -1;
    }
    list_append(tests, path);
  }
  closedir(directory);
  qsort(tests->items, tests->count, sizeof(*tests->items), compare_strings);
  return 0;
}

static char *compile_test(const char *compiler, const char *source,
                          int *exit_code) {
  FILE *input = fopen(source, "rb");
  if (input == NULL) {
    perror(source);
    return NULL;
  }

  char output_path[] = "/tmp/ifj_codegen_suite_XXXXXX";
  int output_fd = mkstemp(output_path);
  if (output_fd < 0) {
    perror("mkstemp");
    fclose(input);
    return NULL;
  }

  pid_t child = fork();
  if (child < 0) {
    perror("fork");
    close(output_fd);
    fclose(input);
    unlink(output_path);
    return NULL;
  }

  if (child == 0) {
    if (dup2(fileno(input), STDIN_FILENO) < 0 ||
        dup2(output_fd, STDOUT_FILENO) < 0) {
      perror("dup2");
      _exit(126);
    }
    fclose(input);
    close(output_fd);
    execlp(compiler, compiler, (char *)NULL);
    perror(compiler);
    _exit(127);
  }

  fclose(input);
  close(output_fd);

  int status;
  while (waitpid(child, &status, 0) < 0) {
    if (errno != EINTR) {
      perror("waitpid");
      unlink(output_path);
      return NULL;
    }
  }
  if (!WIFEXITED(status)) {
    *exit_code = -1;
  } else {
    *exit_code = WEXITSTATUS(status);
  }

  FILE *output = fopen(output_path, "rb");
  unlink(output_path);
  if (output == NULL) {
    perror(output_path);
    return NULL;
  }
  if (fseek(output, 0, SEEK_END) != 0) {
    perror("fseek");
    fclose(output);
    return NULL;
  }
  long output_size = ftell(output);
  if (output_size < 0 || fseek(output, 0, SEEK_SET) != 0) {
    perror("reading compiler output");
    fclose(output);
    return NULL;
  }

  char *code = malloc((size_t)output_size + 1);
  if (code == NULL) {
    fail_allocation();
  }
  size_t bytes_read = fread(code, 1, (size_t)output_size, output);
  fclose(output);
  if (bytes_read != (size_t)output_size) {
    fprintf(stderr, "Could not read all compiler output for %s\n", source);
    free(code);
    return NULL;
  }
  code[bytes_read] = '\0';
  return code;
}

static bool write_all(int fd, const char *data, size_t length) {
  while (length > 0) {
    ssize_t written = write(fd, data, length);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      return false;
    }
    if (written == 0) {
      errno = EIO;
      return false;
    }
    data += written;
    length -= (size_t)written;
  }
  return true;
}

static char *write_temp_file(const char *prefix, const char *contents) {
  char path[MAX_LINE];
  int length = snprintf(path, sizeof(path), "%sXXXXXX", prefix);
  if (length < 0 || (size_t)length >= sizeof(path)) {
    fprintf(stderr, "Temporary file path is too long\n");
    return NULL;
  }
  int fd = mkstemp(path);
  if (fd < 0) {
    perror("mkstemp");
    return NULL;
  }
  if (!write_all(fd, contents, strlen(contents))) {
    perror("writing temporary file");
    close(fd);
    unlink(path);
    return NULL;
  }
  if (close(fd) != 0) {
    perror("closing temporary file");
    unlink(path);
    return NULL;
  }
  return duplicate_string(path);
}

static char *compile_source(const char *compiler, const char *source,
                            int *exit_code) {
  static const char import[] = "import ifj26.*\n";
  size_t source_length = strlen(source);
  char *complete_source = malloc(sizeof(import) + source_length);
  if (complete_source == NULL) {
    fail_allocation();
  }
  memcpy(complete_source, import, sizeof(import) - 1);
  memcpy(complete_source + sizeof(import) - 1, source, source_length + 1);
  char *source_path =
      write_temp_file("/tmp/ifj_codegen_source_", complete_source);
  free(complete_source);
  if (source_path == NULL) {
    return NULL;
  }
  char *code = compile_test(compiler, source_path, exit_code);
  unlink(source_path);
  free(source_path);
  return code;
}

static char *read_file(const char *path, size_t *length) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    perror(path);
    return NULL;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    perror("fseek");
    fclose(file);
    return NULL;
  }
  long size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    perror("reading output");
    fclose(file);
    return NULL;
  }
  char *contents = malloc((size_t)size + 1);
  if (contents == NULL) {
    fail_allocation();
  }
  size_t bytes_read = fread(contents, 1, (size_t)size, file);
  fclose(file);
  if (bytes_read != (size_t)size) {
    fprintf(stderr, "Could not read all interpreter output\n");
    free(contents);
    return NULL;
  }
  contents[bytes_read] = '\0';
  if (length != NULL) {
    *length = bytes_read;
  }
  return contents;
}

static char *run_interpreter(const char *interpreter, const char *code,
                             const char *input, int *exit_code,
                             size_t *output_length) {
  char *code_path = write_temp_file("/tmp/ifj_codegen_code_", code);
  char *input_path = write_temp_file("/tmp/ifj_codegen_input_", input);
  if (code_path == NULL || input_path == NULL) {
    if (code_path != NULL) {
      unlink(code_path);
      free(code_path);
    }
    if (input_path != NULL) {
      unlink(input_path);
      free(input_path);
    }
    return NULL;
  }

  char output_path[] = "/tmp/ifj_codegen_output_XXXXXX";
  int output_fd = mkstemp(output_path);
  if (output_fd < 0) {
    perror("mkstemp");
    unlink(code_path);
    unlink(input_path);
    free(code_path);
    free(input_path);
    return NULL;
  }

  pid_t child = fork();
  if (child < 0) {
    perror("fork");
    close(output_fd);
    unlink(output_path);
    unlink(code_path);
    unlink(input_path);
    free(code_path);
    free(input_path);
    return NULL;
  }
  if (child == 0) {
    FILE *program_input = fopen(input_path, "rb");
    if (program_input == NULL ||
        dup2(fileno(program_input), STDIN_FILENO) < 0 ||
        dup2(output_fd, STDOUT_FILENO) < 0) {
      perror("interpreter setup");
      _exit(126);
    }
    fclose(program_input);
    close(output_fd);
    execlp(interpreter, interpreter, code_path, (char *)NULL);
    perror(interpreter);
    _exit(127);
  }

  close(output_fd);
  int status;
  while (waitpid(child, &status, 0) < 0) {
    if (errno != EINTR) {
      perror("waitpid");
      unlink(output_path);
      unlink(code_path);
      unlink(input_path);
      free(code_path);
      free(input_path);
      return NULL;
    }
  }
  *exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
  char *output = read_file(output_path, output_length);
  unlink(output_path);
  unlink(code_path);
  unlink(input_path);
  free(code_path);
  free(input_path);
  return output;
}

static bool command_available(const char *command) {
  if (strchr(command, '/') != NULL) {
    return access(command, X_OK) == 0;
  }
  const char *path = getenv("PATH");
  if (path == NULL) {
    return false;
  }
  char *copy = duplicate_string(path);
  char *save = NULL;
  for (char *directory = strtok_r(copy, ":", &save); directory != NULL;
       directory = strtok_r(NULL, ":", &save)) {
    char candidate[MAX_LINE];
    int length =
        snprintf(candidate, sizeof(candidate), "%s/%s", directory, command);
    if (length >= 0 && (size_t)length < sizeof(candidate) &&
        access(candidate, X_OK) == 0) {
      free(copy);
      return true;
    }
  }
  free(copy);
  return false;
}

static void print_escaped(const char *label, const char *text, size_t length) {
  printf("%s\"", label);
  for (size_t i = 0; i < length; i++) {
    unsigned char c = (unsigned char)text[i];
    if (c == '\n') {
      fputs("\\n", stdout);
    } else if (c == '\r') {
      fputs("\\r", stdout);
    } else if (c == '\t') {
      fputs("\\t", stdout);
    } else if (c == '\\' || c == '"') {
      putchar('\\');
      putchar(c);
    } else if (c < 32 || c > 126) {
      printf("\\x%02x", c);
    } else {
      putchar(c);
    }
  }
  puts("\"");
}

static bool contains_string(const string_list_t *list, const char *text) {
  for (size_t i = 0; i < list->count; i++) {
    if (strcmp(list->items[i], text) == 0) {
      return true;
    }
  }
  return false;
}

static bool validate_code(const char *code, char *reason, size_t reason_size) {
  if (strncmp(code, ".IFJcode26\n", strlen(".IFJcode26\n")) != 0) {
    snprintf(reason, reason_size, "missing IFJcode26 header");
    return false;
  }
  if (strstr(code, "CREATEFRAME\nCALL $function_") == NULL ||
      strstr(code, "EXIT int@0\n") == NULL) {
    snprintf(reason, reason_size, "missing main call or program exit");
    return false;
  }

  string_list_t labels = {0};
  string_list_t references = {0};
  char *copy = duplicate_string(code);
  char *save_line = NULL;
  bool has_function_label = false;

  for (char *line = strtok_r(copy, "\n", &save_line); line != NULL;
       line = strtok_r(NULL, "\n", &save_line)) {
    char instruction[MAX_LINE];
    char operand[MAX_LINE];
    if (sscanf(line, "%4095s %4095s", instruction, operand) != 2) {
      continue;
    }

    if (strcmp(instruction, "LABEL") == 0) {
      if (contains_string(&labels, operand)) {
        snprintf(reason, reason_size, "duplicate label %s", operand);
        free(copy);
        free_list(&labels);
        free_list(&references);
        return false;
      }
      list_append(&labels, operand);
      if (strncmp(operand, "$function_", strlen("$function_")) == 0) {
        has_function_label = true;
      }
    } else if (strcmp(instruction, "CALL") == 0 ||
               strncmp(instruction, "JUMP", strlen("JUMP")) == 0) {
      list_append(&references, operand);
    }
  }

  bool valid = has_function_label;
  if (!valid) {
    snprintf(reason, reason_size, "missing function label");
  }
  for (size_t i = 0; valid && i < references.count; i++) {
    if (!contains_string(&labels, references.items[i])) {
      snprintf(reason, reason_size, "unresolved label %s", references.items[i]);
      valid = false;
    }
  }

  free(copy);
  free_list(&labels);
  free_list(&references);
  return valid;
}

static bool run_runtime_test(const runtime_test_t *test, const char *compiler,
                             const char *interpreter,
                             bool interpreter_is_available, bool *was_skipped) {
  int compiler_status = -1;
  char *code = compile_source(compiler, test->source, &compiler_status);
  char reason[256] = "";
  bool code_valid = code != NULL && compiler_status == 0 &&
                    validate_code(code, reason, sizeof(reason));

  printf("\n[%s]\n", test->name);
  printf("  Expected compiler status: 0\n");
  if (code == NULL) {
    puts("  Actual compiler output: <could not run compiler>");
  } else {
    printf("  Actual compiler status: %d\n", compiler_status);
    if (!code_valid) {
      if (reason[0] == '\0') {
        snprintf(reason, sizeof(reason), "compiler exited with status %d",
                 compiler_status);
      }
      printf("  Actual generated code: invalid (%s)\n", reason);
    } else {
      puts("  Actual generated code: valid IFJcode26 structure");
    }
  }
  print_escaped("  Expected program stdout: ", test->expected_output,
                strlen(test->expected_output));
  printf("  Expected program exit: %d\n", test->expected_status);

  if (!code_valid) {
    puts("  Actual program stdout: <not run; code generation failed>");
    puts("  RESULT: FAIL");
    free(code);
    return false;
  }
  if (!interpreter_is_available) {
    puts("  Actual program stdout: <not run; ic26int unavailable>");
    puts("  Actual program exit: <not run>");
    puts("  RESULT: SKIP runtime comparison");
    *was_skipped = true;
    free(code);
    return true;
  }

  int interpreter_status = -1;
  size_t actual_length = 0;
  char *actual = run_interpreter(interpreter, code, test->input,
                                 &interpreter_status, &actual_length);
  if (actual == NULL) {
    puts("  Actual program stdout: <could not run interpreter>");
    puts("  RESULT: FAIL");
    free(code);
    return false;
  }
  print_escaped("  Actual program stdout: ", actual, actual_length);
  printf("  Actual program exit: %d\n", interpreter_status);

  bool output_matches =
      actual_length == strlen(test->expected_output) &&
      memcmp(actual, test->expected_output, actual_length) == 0;
  bool passed = output_matches && interpreter_status == test->expected_status;
  if (!output_matches) {
    puts("  MISMATCH: program stdout differs from expected output");
  }
  if (interpreter_status != test->expected_status) {
    printf("  MISMATCH: expected exit %d, got %d\n", test->expected_status,
           interpreter_status);
  }
  puts(passed ? "  RESULT: PASS" : "  RESULT: FAIL");
  free(actual);
  free(code);
  return passed;
}

int main(int argc, char **argv) {
  const char *compiler = "./ifj2026";
  const char *interpreter = getenv("IC26INT");
  if (interpreter == NULL) {
    interpreter = "ic26int";
  }
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      interpreter = argv[++i];
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Usage: %s [-i interpreter-path] [compiler-path]\n",
              argv[0]);
      return EXIT_FAILURE;
    } else {
      compiler = argv[i];
    }
  }
  bool interpreter_is_available = command_available(interpreter);
  if (!interpreter_is_available) {
    fprintf(stderr,
            "Warning: interpreter '%s' was not found; runtime comparisons "
            "will be skipped\n",
            interpreter);
  }

  string_list_t tests = {0};
  if (load_valid_tests(&tests) != 0) {
    free_list(&tests);
    return EXIT_FAILURE;
  }
  if (tests.count == 0) {
    fprintf(stderr, "No Scala tests found in %s\n", VALID_TEST_DIR);
    free_list(&tests);
    return EXIT_FAILURE;
  }

  size_t fixtures_passed = 0;
  size_t runtime_passed = 0;
  size_t runtime_failed = 0;
  size_t runtime_skipped = 0;
  for (size_t i = 0; i < tests.count; i++) {
    int exit_code = -1;
    char *code = compile_test(compiler, tests.items[i], &exit_code);
    char reason[256] = "";
    bool valid = code != NULL && exit_code == 0 &&
                 validate_code(code, reason, sizeof(reason));

    printf("\n[fixture %s]\n", tests.items[i]);
    puts("  Expected compiler output: IFJcode26 header, valid main call, "
         "defined CALL/JUMP labels");
    if (code == NULL) {
      puts("  Actual compiler output: <could not run compiler>");
    } else {
      printf("  Actual compiler exit: %d\n", exit_code);
      printf("  Actual compiler output: %s\n",
             valid               ? "valid IFJcode26 structure"
             : reason[0] != '\0' ? reason
                                 : "compiler failed");
    }
    if (valid) {
      puts("  RESULT: PASS");
      fixtures_passed++;
    } else {
      if (code != NULL && reason[0] == '\0') {
        snprintf(reason, sizeof(reason), "compiler exited with status %d",
                 exit_code);
      } else if (code == NULL && reason[0] == '\0') {
        snprintf(reason, sizeof(reason), "could not run compiler");
      }
      puts("  RESULT: FAIL");
      if (code != NULL && code[0] != '\0') {
        puts("  Actual generated code:");
        fputs(code, stdout);
        if (code[strlen(code) - 1] != '\n') {
          putchar('\n');
        }
      }
    }
    free(code);
  }

  size_t runtime_count = sizeof(RUNTIME_TESTS) / sizeof(RUNTIME_TESTS[0]);
  for (size_t i = 0; i < runtime_count; i++) {
    bool skipped = false;
    bool passed = run_runtime_test(&RUNTIME_TESTS[i], compiler, interpreter,
                                   interpreter_is_available, &skipped);
    if (skipped) {
      runtime_skipped++;
    } else if (passed) {
      runtime_passed++;
    } else {
      runtime_failed++;
    }
  }

  printf("\n=========== SUMMARY ===========\n");
  printf("Syntax fixture codegen: %zu/%zu passed\n", fixtures_passed,
         tests.count);
  printf("Embedded runtime tests: %zu passed, %zu failed, %zu skipped\n",
         runtime_passed, runtime_failed, runtime_skipped);
  free_list(&tests);
  return fixtures_passed == tests.count && runtime_failed == 0 ? EXIT_SUCCESS
                                                               : EXIT_FAILURE;
}

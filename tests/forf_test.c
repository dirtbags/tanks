#include "forf.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

static void check(char *program, enum forf_error_type error, long expected) {
  struct forf_env env = {0};
  struct forf_value data_values[32], command_values[32];
  struct forf_stack data, command;
  long memory_values[10] = {0};
  struct forf_memory memory;
  int ok;

  forf_stack_init(&data, data_values, 32);
  forf_stack_init(&command, command_values, 32);
  forf_memory_init(&memory, memory_values, 10);
  forf_env_init(&env, forf_base_lexical_env, &data, &command, &memory, NULL);
  if (forf_parse_string(&env, program) != 0) {
    fprintf(stderr, "parse failed: %s\n", program);
    exit(1);
  }
  ok = forf_eval(&env);
  if (env.error != error || ok != (error == forf_error_none)) {
    fprintf(stderr, "%s: expected error %d, got %d (ok=%d)\n", program,
            error, env.error, ok);
    exit(1);
  }
  if (error == forf_error_none &&
      (data.top != 1 || data.stack[0].type != forf_type_number ||
       data.stack[0].v.i != expected)) {
    fprintf(stderr, "%s: expected result %ld\n", program, expected);
    exit(1);
  }
}

static void binary(long a, long b, char *op, enum forf_error_type error,
                   long expected) {
  char program[100];
  snprintf(program, sizeof(program), "%ld %ld %s", a, b, op);
  check(program, error, expected);
}

static void unary(long a, char *op, enum forf_error_type error, long expected) {
  char program[100];
  snprintf(program, sizeof(program), "%ld %s", a, op);
  check(program, error, expected);
}

int main(void) {
  long bits = sizeof(long) * CHAR_BIT;

  binary(LONG_MAX, 0, "+", forf_error_none, LONG_MAX);
  binary(LONG_MAX, 1, "+", forf_error_overflow, 0);
  binary(LONG_MIN, -1, "+", forf_error_overflow, 0);
  binary(LONG_MIN, 0, "-", forf_error_none, LONG_MIN);
  binary(LONG_MIN, 1, "-", forf_error_overflow, 0);
  binary(LONG_MAX, -1, "-", forf_error_overflow, 0);
  binary(LONG_MIN, 1, "*", forf_error_none, LONG_MIN);
  binary(-7, -6, "*", forf_error_none, 42);
  binary(LONG_MAX, 2, "*", forf_error_overflow, 0);
  binary(LONG_MIN, -1, "*", forf_error_overflow, 0);

  binary(LONG_MAX >> 1, 1, "<<", forf_error_none, LONG_MAX - 1);
  binary(0, bits - 1, "<<", forf_error_none, 0);
  binary(1, bits - 1, "<<", forf_error_overflow, 0);
  binary(-1, 0, "<<", forf_error_overflow, 0);
  binary(1, -1, "<<", forf_error_overflow, 0);
  binary(1, bits, "<<", forf_error_overflow, 0);
  binary(LONG_MAX, bits - 1, ">>", forf_error_none, 0);
  binary(LONG_MIN, 0, ">>", forf_error_none, LONG_MIN);
  binary(LONG_MIN, bits - 1, ">>", forf_error_none, -1);
  binary(-3, 1, ">>", forf_error_none, -2);
  binary(1, -1, ">>", forf_error_overflow, 0);
  binary(1, bits, ">>", forf_error_overflow, 0);

  unary(-42, "abs", forf_error_none, 42);
  unary(LONG_MAX, "abs", forf_error_none, LONG_MAX);
  unary(LONG_MIN, "abs", forf_error_overflow, 0);
#if LONG_MAX > INT_MAX
  unary((long)INT_MAX + 1, "abs", forf_error_none, (long)INT_MAX + 1);
#endif
  binary(LONG_MIN, -1, "/", forf_error_divzero, 0);
  binary(LONG_MIN, -1, "%", forf_error_divzero, 0);
  check("+", forf_error_underflow, 0);
  check("abs", forf_error_underflow, 0);

  puts("Forf arithmetic tests passed");
  return 0;
}

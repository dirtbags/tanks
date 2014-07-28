/* forf: a crappy Forth implementation
 * Copyright (C) 2010 Adam Glasgall
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Notes
 * -------------------------------------------------------
 *
 * This is intended to be implemented as a library.  As such, it doesn't
 * use the libc memory allocation functions.  This may be a different
 * programming style than you're used to.
 *
 * There are two data types: numbers and stacks.  Because we can't
 * allocate memory, stacks are implemented with begin and end markers
 * and not new stack types.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "forf.h"
#include "dump.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

char *forf_error_str[] = {
  "None",
  "Runtime",
  "Parse",
  "Underflow",
  "Overflow",
  "Type",
  "No such procedure",
  "Divide by zero",
};

/*
 *
 * Memory manipulation
 *
 */
void
forf_memory_init(struct forf_memory *m,
                 long               *values,
                 size_t              size)
{
  m->mem  = values;
  m->size = size;
}


/*
 *
 * Stack manipulation
 *
 */

void
forf_stack_init(struct forf_stack *s,
                struct forf_value *values,
                size_t             size)
{
  s->stack = values;
  s->size = size;
  s->top = 0;
}

void
forf_stack_reset(struct forf_stack *s)
{
  s->top = 0;
}

size_t
forf_stack_len(struct forf_stack *s)
{
  return s->top;
}

int
forf_stack_push(struct forf_stack *s, struct forf_value *v)
{
  if (s->top == s->size) {
    return 0;
  }
  s->stack[(s->top)++] = *v;
  return 1;
}

int
forf_stack_pop(struct forf_stack *s, struct forf_value *v)
{
  if (0 == s->top) {
    return 0;
  }
  *v = s->stack[--(s->top)];
  return 1;
}

void
forf_stack_copy(struct forf_stack *dst, struct forf_stack *src)
{
  int top = min(dst->size, src->top);

  dst->top = top;
  memcpy(dst->stack, src->stack, sizeof(*dst->stack) * top);
}


void
forf_stack_reverse(struct forf_stack *s)
{
  struct forf_value val;
  size_t pos;

  for (pos = 0; pos < (s->top)/2; pos += 1) {
    size_t qos = s->top - pos - 1;

    val = s->stack[pos];
    s->stack[pos] = s->stack[qos];
    s->stack[qos] = val;
  }
}

long
forf_pop_num(struct forf_env *env)
{
  struct forf_value val;

  if (! forf_stack_pop(env->data, &val)) {
    env->error = forf_error_underflow;
    return 0;
  }
  if (forf_type_number != val.type) {
    forf_stack_push(env->data, &val);
    env->error = forf_error_type;
    return 0;
  }
  return val.v.i;
}

void
forf_push_num(struct forf_env *env, long i)
{
  struct forf_value val;

  val.type = forf_type_number;
  val.v.i = i;
  if (! forf_stack_push(env->data, &val)) {
    env->error = forf_error_overflow;
  }
}


/* Pop an entire stack
 *
 * DANGER WILL ROBINSON
 *
 * This returned stack points to values on the data stack.  You must be
 * finished with this stack before you push anything onto the data
 * stack, otherwise your returned stack will be corrupted.
 */
struct forf_stack
forf_pop_stack(struct forf_env *env)
{
  struct forf_stack s     = { 0, 0, NULL };
  struct forf_value val;
  size_t            depth = 1;

  if (! forf_stack_pop(env->data, &val)) {
    env->error = forf_error_underflow;
    return s;
  }
  if (forf_type_stack_end != val.type) {
    forf_stack_push(env->data, &val);
    env->error = forf_error_type;
    return s;
  }
  /* Duplicate just the stack onto s.  Begin with -1 to account for the
     end of list marker. */
  s.size = -1;
  while (depth) {
    s.size += 1;
    if (! forf_stack_pop(env->data, &val)) {
      /* You should never underflow here, there should at least be a
         stack begin marker. */
      env->error = forf_error_runtime;
      s.size = 0;
      return s;
    }
    switch (val.type) {
      case forf_type_stack_end:
        depth += 1;
        break;
      case forf_type_stack_begin:
        depth -= 1;
        break;
      default:
        break;
    }
  }
  s.top = s.size;
  s.stack = (env->data->stack) + (env->data->top + 1);
  return s;
}

/* Push an entire stack onto another stack.
 */
int
forf_push_stack(struct forf_stack *dst, struct forf_stack *src)
{
  struct forf_value val;

  while (forf_stack_pop(src, &val)) {
    if (! forf_stack_push(dst, &val)) {
      return 0;
    }
  }
  return 1;
}

/* Push an entire stack onto the command stack.
 *
 * This is meant to work with the return value from forf_pop_stack.
 */
int
forf_push_to_command_stack(struct forf_env *env, struct forf_stack *src)
{
  if (! forf_push_stack(env->command, src)) {
    env->error = forf_error_overflow;
    return 0;
  }
  return 1;
}

/* Move one value from src to dst.  Note that one value could mean a
 * whole substack, in which case dst gets the stack in reverse!  dst can
 * also be NULL, in which case a value is just discarded.
 *
 * Because of the reversing thing, it's important to make sure that the
 * data stack is either src or dst.  This way, the data stack will
 * always have "reversed" substacks, and everything else will have them
 * in the right order.
 */
int
forf_stack_move_value(struct forf_env *env,
                      struct forf_stack *dst,
                      struct forf_stack *src)
{
  struct forf_value val;
  size_t            depth = 0;

  do {
    /* Pop from src */
    if (! forf_stack_pop(env->command, &val)) {
      env->error = forf_error_underflow;
      return 0;
    }

    /* Push to dst (or discard if dst is NULL) */
    if (dst) {
      if (! forf_stack_push(env->data, &val)) {
        env->error = forf_error_overflow;
        return 0;
      }
    }

    /* Deal with it being a substack marker */
    switch (val.type) {
      case forf_type_stack_begin:
        depth += 1;
        break;
      case forf_type_stack_end:
        depth -= 1;
        break;
      default:
        break;
    }
  } while (depth > 0);

  return 1;

}


/*
 *
 * Procedures
 *
 */

#define unproc(name, op)                        \
  static void                                   \
  forf_proc_ ## name(struct forf_env *env)      \
  {                                             \
    long a = forf_pop_num(env);                 \
                                                \
    forf_push_num(env, op a);                   \
  }

unproc(inv, ~)
unproc(not, !)

#define binproc(name, op)                         \
  static void                                     \
  forf_proc_ ## name(struct forf_env *env)        \
  {                                               \
    long a = forf_pop_num(env);                   \
    long b = forf_pop_num(env);                   \
                                                  \
    forf_push_num(env, b op a);                   \
  }

binproc(add, +)
binproc(sub, -)
binproc(mul, *)
binproc(and, &)
binproc(or, |)
binproc(xor, ^)
binproc(lshift, <<)
binproc(rshift, >>)
binproc(gt, >)
binproc(ge, >=)
binproc(lt, <)
binproc(le, <=)
binproc(eq, ==)
binproc(ne, !=)

static void
forf_proc_div(struct forf_env *env)
{
  long a = forf_pop_num(env);
  long b = forf_pop_num(env);

  if (0 == a || (a == -1 && b == LONG_MIN)) {
    env->error = forf_error_divzero;
    return;
  }
  forf_push_num(env, b / a);
}

static void
forf_proc_mod(struct forf_env *env)
{
  long a = forf_pop_num(env);
  long b = forf_pop_num(env);

  if (0 == a || (a == -1 && b == LONG_MIN)) {
    env->error = forf_error_divzero;
    return;
  }
  forf_push_num(env, b % a);
}

static void
forf_proc_abs(struct forf_env *env)
{
  forf_push_num(env, abs(forf_pop_num(env)));
}

static void
forf_proc_dup(struct forf_env *env)
{
  long a = forf_pop_num(env);

  forf_push_num(env, a);
  forf_push_num(env, a);
}

static void
forf_proc_pop(struct forf_env *env)
{
  forf_pop_num(env);
}

static void
forf_proc_exch(struct forf_env *env)
{
  long a = forf_pop_num(env);
  long b = forf_pop_num(env);

  forf_push_num(env, a);
  forf_push_num(env, b);
}

static void
forf_proc_if(struct forf_env *env)
{
  struct forf_stack ifclause   = forf_pop_stack(env);
  long              cond       = forf_pop_num(env);

  if (cond) {
    forf_push_to_command_stack(env, &ifclause);
  }
}

static void
forf_proc_ifelse(struct forf_env *env)
{
  struct forf_stack elseclause = forf_pop_stack(env);
  struct forf_stack ifclause   = forf_pop_stack(env);
  long              cond       = forf_pop_num(env);

  if (cond) {
    forf_push_to_command_stack(env, &ifclause);
  } else {
    forf_push_to_command_stack(env, &elseclause);
  }
}

static void
forf_proc_memset(struct forf_env *env)
{
  long pos = forf_pop_num(env);
  long a   = forf_pop_num(env);

  if (pos >= env->memory->size) {
    env->error = forf_error_overflow;
    return;
  }

  env->memory->mem[pos] = a;
}

static void
forf_proc_memget(struct forf_env *env)
{
  long pos = forf_pop_num(env);

  if (pos >= env->memory->size) {
    env->error = forf_error_overflow;
    return;
  }

  forf_push_num(env, env->memory->mem[pos]);
}

/*
 *
 * Lexical environment
 *
 */
struct forf_lexical_env forf_base_lexical_env[] = {
  {"~", forf_proc_inv},
  {"!", forf_proc_not},
  {"+", forf_proc_add},
  {"-", forf_proc_sub},
  {"*", forf_proc_mul},
  {"/", forf_proc_div},
  {"%", forf_proc_mod},
  {"&", forf_proc_and},
  {"|", forf_proc_or},
  {"^", forf_proc_xor},
  {"<<", forf_proc_lshift},
  {">>", forf_proc_rshift},
  {">", forf_proc_gt},
  {">=", forf_proc_ge},
  {"<", forf_proc_lt},
  {"<=", forf_proc_le},
  {"=", forf_proc_eq},
  {"<>", forf_proc_ne},
  {"abs", forf_proc_abs},
  {"dup", forf_proc_dup},
  {"pop", forf_proc_pop},
  {"exch", forf_proc_exch},
  {"if", forf_proc_if},
  {"ifelse", forf_proc_ifelse},
  {"mset", forf_proc_memset},
  {"mget", forf_proc_memget},
  {NULL, NULL}
};

/** Extend a lexical environment */
int
forf_extend_lexical_env(struct forf_lexical_env *dest,
                        struct forf_lexical_env *src,
                        size_t size)
{
  int base, i;

  for (base = 0; dest[base].name; base += 1);
  for (i = 0; (base+i < size) && (src[i].name); i += 1) {
    dest[base+i] = src[i];
  }
  if (base + i == size) {
    /* Not enough room */
    return 0;
  }
  dest[base+i].name = NULL;
  dest[base+i].proc = NULL;
  return 1;
}


/*
 *
 * Parsing
 *
 */
static int
forf_push_token(struct forf_env *env, char *token, size_t tokenlen)
{
  long               i;
  char               s[MAX_TOKEN_LEN + 1];
  char              *endptr;
  struct forf_value  val;

  /* Zero-length token yields int:0 from strtol */

  /* NUL-terminate it */
  memcpy(s, token, tokenlen);
  s[tokenlen] = '\0';

  /* Try to make in an integer */
  i = strtol(s, &endptr, 0);
  if ('\0' == *endptr) {
    /* Was an int */
    val.type = forf_type_number;
    val.v.i = i;
  } else {
    /* If not an int, a procedure name */
    val.type = forf_type_proc;
    for (i = 0; NULL != env->lenv[i].name; i += 1) {
      if (0 == strcmp(s, env->lenv[i].name)) {
        val.v.p = env->lenv[i].proc;
        break;
      }
    }
    if (NULL == env->lenv[i].name) {
      env->error = forf_error_noproc;
      return 0;
    }
  }

  if (! forf_stack_push(env->command, &val)) {
    env->error = forf_error_overflow;
    return 0;
  }

  return 1;
}

/* Parse an input stream onto the command stack */
int
forf_parse_stream(struct forf_env *env,
                  forf_getch_func *getch,
                  void            *datum)
{
  int               running     = 1;
  long              pos         = 0;
  char              token[MAX_TOKEN_LEN];
  size_t            tokenlen    = 0;
  struct forf_value val;
  size_t            stack_depth = 0;
  int               comment     = 0;

#define _tokenize()                             \
  do {                                          \
    if (tokenlen) {                             \
      if (! forf_push_token(env, token, tokenlen)) return pos; \
      tokenlen = 0;                             \
    }                                           \
  } while (0)

  while (running) {
    int c;

    c = getch(datum);
    pos += 1;

    /* Handle comments */
    if (comment) {
      switch (c) {
        case EOF:
          env->error = forf_error_parse;
          return pos;
        case ')':
          comment = 0;
          break;
      }
      continue;
    }

    switch (c) {
      case EOF:
        running = 0;
        break;
      case '(':
        comment = 1;
        break;
      case ' ':
      case '\f':
      case '\n':
      case '\r':
      case '\t':
      case '\v':
        _tokenize();
        break;
      case '{':
        _tokenize();
        val.type = forf_type_stack_begin;
        if (! forf_stack_push(env->command, &val)) {
          env->error = forf_error_overflow;
          return pos;
        }
        stack_depth += 1;
        break;
      case '}':
        _tokenize();
        val.type = forf_type_stack_end;
        if (! forf_stack_push(env->command, &val)) {
          env->error = forf_error_overflow;
          return pos;
        }
        stack_depth -= 1;
        break;
      default:
        if (tokenlen < sizeof(token)) {
          token[tokenlen++] = c;
        }
        break;
    }
  }
  _tokenize();

  if (0 != stack_depth) {
    env->error = forf_error_parse;
    return pos;
  }

  // The first thing we read should be the first thing we do
  forf_stack_reverse(env->command);

  return 0;
}

struct forf_char_stream {
  char   *buf;
  size_t  len;
  size_t  pos;
};

static int
forf_string_getch(struct forf_char_stream *stream)
{
  if (stream->pos >= stream->len) {
    return EOF;
  }
  return stream->buf[stream->pos++];
}

int
forf_parse_buffer(struct forf_env *env,
                  char            *buf,
                  size_t           len)
{
  struct forf_char_stream stream;

  stream.buf = buf;
  stream.len = len;
  stream.pos = 0;

  return forf_parse_stream(env, (forf_getch_func *)forf_string_getch, &stream);
}

int
forf_parse_string(struct forf_env *env,
                  char            *str)
{
  return forf_parse_buffer(env, str, strlen(str));
}

int
forf_parse_file(struct forf_env *env,
                FILE            *f)
{
  return forf_parse_stream(env, (forf_getch_func *)fgetc, f);
}


/*
 *
 * Forf environment
 *
 */

void
forf_env_init(struct forf_env         *env,
              struct forf_lexical_env *lenv,
              struct forf_stack       *data,
              struct forf_stack       *cmd,
              struct forf_memory      *mem,
              void                    *udata)
{
  env->lenv    = lenv;
  env->data    = data;
  env->command = cmd;
  env->memory  = mem;
  env->udata   = udata;
}


int
forf_eval_once(struct forf_env *env)
{
  struct forf_value val;

  if (! forf_stack_pop(env->command, &val)) {
    env->error = forf_error_underflow;
    return 0;
  }
  switch (val.type) {
    case forf_type_number:
    case forf_type_stack_begin:
      // Push back on command stack, then move it
      forf_stack_push(env->command, &val);
      if (! forf_stack_move_value(env, env->data, env->command)) return 0;
      break;
    case forf_type_proc:
      (val.v.p)(env);
      break;
    default:
      env->error = forf_error_runtime;
      return 0;
  }
  return 1;
}

int
forf_eval(struct forf_env *env)
{
  int ret;

  env->error = forf_error_none;
  while (env->command->top) {
    ret = forf_eval_once(env);
    if ((! ret) || (env->error)) {
      return 0;
    }
  }
  return 1;
}

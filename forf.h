#ifndef __FORF_H__
#define __FORF_H__

#include <stdio.h>
#include <inttypes.h>

#define MAX_TOKEN_LEN 20
#define MAX_CMDSTACK 200

struct forf_env;

enum forf_value_type {
  forf_type_number,
  forf_type_proc,
  forf_type_stack_begin,
  forf_type_stack_end,
};

enum forf_error_type {
  forf_error_none,
  forf_error_runtime,
  forf_error_parse,
  forf_error_underflow,
  forf_error_overflow,
  forf_error_type,
  forf_error_noproc,
  forf_error_divzero,
};

extern char *forf_error_str[];

typedef void (forf_proc)(struct forf_env *);

struct forf_value {
  enum forf_value_type  type;
  union {
    forf_proc          *p;
    long                i;
  } v;
};

struct forf_stack {
  size_t             size;
  size_t             top;
  struct forf_value *stack;
};

struct forf_memory {
  size_t  size;
  long   *mem;
};

struct forf_lexical_env {
  char      *name;
  forf_proc *proc;
};

struct forf_env {
  enum forf_error_type     error;
  struct forf_lexical_env *lenv;
  struct forf_stack       *data;
  struct forf_stack       *command;
  struct forf_memory      *memory;
  void                    *udata;
};


/*
 *
 * Main entry points
 *
 */

/** Initialize a memory structure, given an array of longs */
void forf_memory_init(struct forf_memory *m,
                      long               *values,
                      size_t              size);

/** Initialize a stack, given an array of values */
void forf_stack_init(struct forf_stack *s,
                     struct forf_value *values,
                     size_t             size);

void forf_stack_reset(struct forf_stack *s);
void forf_stack_copy(struct forf_stack *dst, struct forf_stack *src);
int forf_stack_push(struct forf_stack *s, struct forf_value *v);
int forf_stack_pop(struct forf_stack *s, struct forf_value *v);

/** Pop a number off the data stack */
long forf_pop_num(struct forf_env *env);

/** Push a number onto the data stack */
void forf_push_num(struct forf_env *env, long i);

/** Pop a whole stack */
struct forf_stack forf_pop_stack(struct forf_env *env);


/** The base lexical environment */
extern struct forf_lexical_env forf_base_lexical_env[];

/** Extend a lexical environment */
int
forf_extend_lexical_env(struct forf_lexical_env *dest,
                        struct forf_lexical_env *src,
                        size_t                   size);

/** Initialize a forf runtime environment.
 *
 * data, cmd, and mem should have already been initialized
 */
void forf_env_init(struct forf_env         *env,
                   struct forf_lexical_env *lenv,
                   struct forf_stack       *data,
                   struct forf_stack       *cmd,
                   struct forf_memory      *mem,
                   void                    *udata);

/** The type of a getch function (used for parsing) */
typedef int (forf_getch_func)(void *);

/** Parse something by calling getch(datum)
 *
 * Returns the character at which an error was encountered, or
 * 0 for successful parse.
 */
int forf_parse_stream(struct forf_env *env,
                      forf_getch_func *getch,
                      void            *datum);

/** Parse a buffer */
int forf_parse_buffer(struct forf_env *env,
                      char            *buf,
                      size_t           len);

/** Parse a string */
int forf_parse_string(struct forf_env *env,
                      char            *str);

/** Parse a FILE * */
int forf_parse_file(struct forf_env *env,
                    FILE            *f);

/** Evaluate the topmost value on the command stack */
int forf_eval_once(struct forf_env *env);

/** Evaluate the entire command stack */
int forf_eval(struct forf_env *env);

#endif

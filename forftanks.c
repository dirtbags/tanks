#include "ctanks.h"
#include "dump.h"
#include "forf.h"
#include <errno.h>
#include <stdarg.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define LENV_SIZE 100

/* Defaults for everything an operator may override at run time.  The tank
 * physics defaults live in ctanks.h (TANK_DEFAULT_*). */
#define DEFAULT_MAX_TANKS 50
#define DEFAULT_ROUNDS 500
#define DEFAULT_SPACING 150
#define DEFAULT_DSTACK_SIZE 500
#define DEFAULT_CSTACK_SIZE 500
#define DEFAULT_MEMORY_SIZE 10

/* Run-time settings that are not tank physics.  Set once in main(), before
 * any tank is loaded. */
static struct {
  int max_tanks;
  int rounds;
  int spacing; /* initial distance between tanks */
  int seed;    /* 0 means "use the process ID" */
  int dstack_size;
  int cstack_size;
  int memory_size; /* number of registers */
  double turret_rot_deg; /* --max-turret-rot is given in degrees */
} cfg = {
    DEFAULT_MAX_TANKS,  DEFAULT_ROUNDS,     DEFAULT_SPACING,
    0,                  DEFAULT_DSTACK_SIZE, DEFAULT_CSTACK_SIZE,
    DEFAULT_MEMORY_SIZE, 45.0,
};

struct forftank {
  struct forf_env env;
  int error_pos;
  char color[8]; /* "#ff0088" */
  char name[50];
  char *path;
  unsigned int uid;
  time_t mtime;

  /* Stack and register storage is sized at run time (cfg.*_size) and
   * allocated, zeroed, by ft_tank_init(). */
  struct forf_stack _prog;
  struct forf_value *_progvals;
  struct forf_stack _cmd;
  struct forf_value *_cmdvals;
  struct forf_stack _data;
  struct forf_value *_datavals;
  struct forf_memory _mem;
  long *_memvals;
};

#ifndef NODEBUG
void forf_print_val(struct forf_value *val) {
  switch (val->type) {
  case forf_type_number:
    printf("%ld", val->v.i);
    break;
  case forf_type_proc:
    printf("[proc %p]", val->v.p);
    break;
  case forf_type_stack_begin:
    printf("{");
    break;
  case forf_type_stack_end:
    printf("}");
    break;
  }
}

void forf_print_stack(struct forf_stack *s) {
  size_t pos;

  for (pos = 0; pos < s->top; pos += 1) {
    forf_print_val(&(s->stack[pos]));
    printf(" ");
  }
}

void forf_dump_stack(struct forf_stack *s) {
  printf("Stack at %p: ", s);
  forf_print_stack(s);
  printf("\n");
}
#endif

/*
 *
 * Forf API
 *
 */

/** Has the turret recharged? */
void forf_tank_fire_ready(struct forf_env *env) {
  struct tank *tank = (struct tank *)env->udata;

  forf_push_num(env, tank_fire_ready(tank));
}

/** Fire! */
void forf_tank_fire(struct forf_env *env) {
  struct tank *tank = (struct tank *)env->udata;

  tank_fire(tank);
}

/** Set desired speed */
void forf_tank_set_speed(struct forf_env *env) {
  struct tank *tank = (struct tank *)env->udata;
  long right = forf_pop_num(env);
  long left = forf_pop_num(env);

  tank_set_speed(tank, left, right);
}

/** Get the current turret angle */
void forf_tank_get_turret(struct forf_env *env) {
  struct tank *tank = (struct tank *)env->udata;
  float angle = tank_get_turret(tank);

  forf_push_num(env, rad2deg(angle));
}

/** Set the desired turret angle */
void forf_tank_set_turret(struct forf_env *env) {
  struct tank *tank = (struct tank *)env->udata;
  long angle = forf_pop_num(env);

  tank_set_turret(tank, deg2rad(angle));
}

/** Is a sensor active? */
void forf_tank_get_sensor(struct forf_env *env) {
  struct tank *tank = (struct tank *)env->udata;
  long sensor_num = forf_pop_num(env);

  forf_push_num(env, tank_get_sensor(tank, sensor_num));
}

/** Set the LED state */
void forf_tank_set_led(struct forf_env *env) {
  struct tank *tank = (struct tank *)env->udata;
  long active = forf_pop_num(env);

  tank_set_led(tank, active);
}

/** Pick a random number */
void forf_proc_random(struct forf_env *env) {
  long max = forf_pop_num(env);

  if (max < 1) {
    forf_push_num(env, 0);
    return;
  }

  forf_push_num(env, rand() % max);
}

/* Tanks lexical environment */
struct forf_lexical_env tanks_lenv_addons[] = {
    {"fire-ready?", forf_tank_fire_ready},
    {"fire!", forf_tank_fire},
    {"set-speed!", forf_tank_set_speed},
    {"get-turret", forf_tank_get_turret},
    {"set-turret!", forf_tank_set_turret},
    {"sensor?", forf_tank_get_sensor},
    {"set-led!", forf_tank_set_led},
    {"random", forf_proc_random},
    {NULL, NULL}};

/*
 *
 * Filesystem stuff
 *
 */

int ft_read_file(char *ptr, size_t size, time_t *mtime, char *dir, char *fn) {
  char path[256];
  FILE *f = NULL;
  struct stat st;
  int ret;
  int success = 0;

  do {
    snprintf(path, sizeof(path), "%s/%s", dir, fn);
    f = fopen(path, "r");
    if (!f)
      break;

    if (-1 != fstat(fileno(f), &st)) {
      *mtime = max(*mtime, st.st_mtim.tv_sec);
    }

    ret = fread(ptr, 1, size - 1, f);
    ptr[ret] = '\0';
    if (!ret)
      break;

    success = 1;
  } while (0);

  if (f)
    fclose(f);
  if (!success) {
    return 0;
  }
  return 1;
}

void ft_bricked_tank(struct tank *tank, void *ignored) {
  /* Do nothing, the tank is comatose */
}

void ft_run_tank(struct tank *tank, struct forftank *ftank) {
  int ret;

  /* Copy program into command stack */
  forf_stack_copy(&ftank->_cmd, &ftank->_prog);
  forf_stack_reset(&ftank->_data);
  ret = forf_eval(&ftank->env);
  if (!ret) {
    fprintf(stderr, "Error in %s: %s\n", ftank->path,
            forf_error_str[ftank->env.error]);
  }
}

int ft_read_program(struct forftank *ftank, struct tank *tank,
                    time_t *mtime,
                    struct forf_lexical_env *lenv, char *path) {
  char progpath[256];
  struct stat st;
  FILE *f;

  /* Open program */
  snprintf(progpath, sizeof(progpath), "%s/program", path);
  f = fopen(progpath, "r");
  if (!f)
    return 0;

  if (-1 != fstat(fileno(f), &st)) {
    *mtime = max(*mtime, st.st_mtim.tv_sec);
  }

  /* Parse program */
  ftank->error_pos = forf_parse_file(&ftank->env, f);
  fclose(f);
  if (ftank->error_pos) {
    return 0;
  }

  /* Back up the program so we can run it over and over without
     needing to re-parse */
  forf_stack_copy(&ftank->_prog, &ftank->_cmd);

  tank_init(tank, (tank_run_func *)ft_run_tank, ftank);

  return 1;
}

static void *ft_calloc(size_t n, size_t size) {
  /* calloc zeroes, which also gives forf registers a defined initial value */
  void *p = calloc(n ? n : 1, size);

  if (!p) {
    fprintf(stderr, "Out of memory allocating %zu x %zu bytes.\n", n, size);
    exit(1);
  }
  return p;
}

void ft_tank_init(struct forftank *ftank, struct tank *tank,
                  struct forf_lexical_env *lenv) {
  ftank->_progvals = ft_calloc(cfg.cstack_size, sizeof(*ftank->_progvals));
  ftank->_cmdvals = ft_calloc(cfg.cstack_size, sizeof(*ftank->_cmdvals));
  ftank->_datavals = ft_calloc(cfg.dstack_size, sizeof(*ftank->_datavals));
  ftank->_memvals = ft_calloc(cfg.memory_size, sizeof(*ftank->_memvals));

  /* Set up forf environment */
  forf_stack_init(&ftank->_prog, ftank->_progvals, cfg.cstack_size);
  forf_stack_init(&ftank->_cmd, ftank->_cmdvals, cfg.cstack_size);
  forf_stack_init(&ftank->_data, ftank->_datavals, cfg.dstack_size);
  forf_memory_init(&ftank->_mem, ftank->_memvals, cfg.memory_size);
  forf_env_init(&ftank->env, lenv, &ftank->_data, &ftank->_cmd, &ftank->_mem,
                tank);
}

void ft_read_sensors(time_t *mtime, struct tank *tank, char *path) {
  for (int i = 0; i < TANK_MAX_SENSORS; i += 1) {
    int ret;
    char fn[10];
    char s[20];
    char *p = s;
    long range;
    long angle;
    long width;
    long turret;

    snprintf(fn, sizeof(fn), "sensor%d", i);
    ret = ft_read_file(s, sizeof(s), mtime, path, fn);
    if (!ret) {
      s[0] = 0;
    }
    range = strtol(p, &p, 0);
    angle = strtol(p, &p, 0);
    width = strtol(p, &p, 0);
    turret = strtol(p, &p, 0);

    range = max(0L, min(range, TANK_SENSOR_RANGE));
    tank->sensors[i].range = (int)range;
    tank->sensors[i].angle = deg2rad(angle % 360);
    tank->sensors[i].width = deg2rad(width % 360);
    tank->sensors[i].turret = (0 != turret);
  }

  /* Sensor files beyond the configured maximum are never read; say so rather
   * than silently dropping them. */
  for (int i = TANK_MAX_SENSORS; i < TANK_SENSORS_HARD_MAX; i += 1) {
    char fn[256];

    snprintf(fn, sizeof(fn), "%s/sensor%d", path, i);
    if (0 == access(fn, F_OK)) {
      fprintf(stderr,
              "warning: %s: ignoring sensor%d (only %d sensors enabled; see "
              "--max-sensors)\n",
              path, i, TANK_MAX_SENSORS);
    }
  }
}

// Crudely clean a string for json output
void clean(char *s) {
  for (; *s; s++) {
    switch (*s) {
    case '\0' ... '\037':
    case '"':
    case '\\':
      *s = '\0';
      return;
    }
  }
}

int ft_read_tank(struct forftank *ftank, struct tank *tank,
                 struct forf_lexical_env *lenv, char *path) {
  int ret;

  ftank->path = path;

  /* Store uid */
  {
    struct stat s;
    if (-1 == stat(path, &s)) {
      ftank->uid = 0;
    } else {
      ftank->uid = (unsigned int)s.st_ino;
    }
  }

  /* What is your name? */
  ret = ft_read_file(ftank->name, sizeof(ftank->name), &ftank->mtime, path, "name");
  if (!ret) {
    snprintf(ftank->name, sizeof(ftank->name), "i:%x", ftank->uid);
  }
  clean(ftank->name);

  /* What is your quest? */
  ft_tank_init(ftank, tank, lenv);
  ret = ft_read_program(ftank, tank, &ftank->mtime, lenv, path);
  if (ret) {
    ft_read_sensors(&ftank->mtime, tank, path);
  } else {
    /* Brick the tank */
    tank_init(tank, ft_bricked_tank, NULL);
  }

  /* What is your favorite color? */
  ret = ft_read_file(ftank->color, sizeof(ftank->color), &ftank->mtime, path, "color");
  if (!ret) {
    strncpy(ftank->color, "#808080", sizeof(ftank->color));
  }
  clean(ftank->color);

  return 1;
}

void print_header(FILE *f, struct tanks_game *game, int seed) {
  const struct tank_params *p = &tank_cfg;

  fprintf(f, "{\n");
  fprintf(f, "  \"seed\": %d,\n", seed);
  fprintf(f, "  \"field\": [%d,%d],\n", (int)game->size[0], (int)game->size[1]);

  /* The effective settings for this game, whether default or overridden.
   * Angles are radians, like everywhere else in the output. */
  fprintf(f, "  \"params\": {\n");
  fprintf(f, "    \"maxSensors\": %d,\n", p->max_sensors);
  fprintf(f, "    \"radius\": %.9g,\n", p->radius);
  fprintf(f, "    \"sensorRange\": %d,\n", p->sensor_range);
  fprintf(f, "    \"cannonRecharge\": %d,\n", p->cannon_recharge);
  fprintf(f, "    \"cannonRange\": %d,\n", p->cannon_range);
  fprintf(f, "    \"maxAccel\": %d,\n", p->max_accel);
  fprintf(f, "    \"maxTurretRot\": %.9g,\n", p->max_turret_rot);
  fprintf(f, "    \"topSpeed\": %.9g,\n", p->top_speed);
  fprintf(f, "    \"friction\": %.9g,\n", p->friction);
  fprintf(f, "    \"dataStackSize\": %d,\n", cfg.dstack_size);
  fprintf(f, "    \"commandStackSize\": %d,\n", cfg.cstack_size);
  fprintf(f, "    \"registers\": %d,\n", cfg.memory_size);
  fprintf(f, "    \"maxTanks\": %d,\n", cfg.max_tanks);
  fprintf(f, "    \"rounds\": %d,\n", cfg.rounds);
  fprintf(f, "    \"spacing\": %d\n", cfg.spacing);
  fprintf(f, "  },\n");
}

void print_rounds(FILE *f, struct tanks_game *game, struct tank *tanks,
                  int ntanks) {
  int alive;

  fprintf(f, "  \"rounds\": [\n");

  /* Run rounds */
  alive = ntanks;
  for (int i = 0; (alive > 1) && (i < cfg.rounds); i += 1) {
    if (i > 0) {
      fprintf(f, ",\n");
    }

    tanks_run_turn(game, tanks, ntanks);
    fprintf(f, "    [");
    alive = ntanks;
    for (int j = 0; j < ntanks; j += 1) {
      struct tank *t = &(tanks[j]);

      int k;
      int flags = 0;
      int sensors = 0;

      if (j > 0) {
        fprintf(f, ",");
      }

      for (k = 0; k < TANK_MAX_SENSORS; k += 1) {
        if (t->sensors[k].triggered) {
          sensors |= (1 << k);
        }
      }
      if (t->turret.firing) {
        flags |= 1;
      }
      if (t->led) {
        flags |= 2;
      }
      if (t->killer) {
        alive -= 1;
        flags |= 4;
      }
      fprintf(f, "[%d,%d,%.2f,%.2f,%d,%d]", (int)t->position[0],
              (int)(t->position[1]), t->angle, t->turret.current, flags,
              sensors);
    }
    fprintf(f, "]");
  }

  fprintf(f, "\n  ],\n");
}

void print_standings(FILE *f, struct forftank *ftanks, struct tank *tanks,
                     int ntanks) {

  fprintf(f, "  \"tanks\": [\n");
  for (int i = 0; i < ntanks; i += 1) {
    int killer = -1;
    int kills = 0;
    for (int j = 0; j < ntanks; j += 1) {
      if (tanks[i].killer == &(tanks[j])) {
        killer = j;
      }
      if (tanks[j].killer == &(tanks[i])) {
        kills += 1;
      }
    }

    if (i > 0) {
      fprintf(f, ",\n");
    }
    fprintf(f, "    {\n");
    fprintf(f, "      \"uid\": %d,\n", ftanks[i].uid);
    fprintf(f, "      \"mtime\": %ld,\n", ftanks[i].mtime);
    fprintf(f, "      \"color\": \"%s\",\n", ftanks[i].color);
    fprintf(f, "      \"name\": \"%s\",\n", ftanks[i].name);
    fprintf(f, "      \"death\": \"%s\",\n", tanks[i].cause_death);
    fprintf(f, "      \"killer\": %d,\n", killer);
    fprintf(f, "      \"kills\": %d,\n", kills);
    fprintf(f, "      \"errorPos\": %d,\n", ftanks[i].error_pos);
    fprintf(f, "      \"error\": \"%s\",\n",
            forf_error_str[ftanks[i].env.error]);
    fprintf(f, "      \"sensors\": [\n");
    for (int j = 0; j < TANK_MAX_SENSORS; j += 1) {
      struct sensor *s = &(tanks[i].sensors[j]);

      if (j > 0) {
        fprintf(f, ",\n");
      }

      if (!s->range) {
        fprintf(f, "        null");
      } else {
        fprintf(f,
                "        "
                "{\"range\":%d,\"angle\":%.2f,\"width\":%.2f,\"turret\":%s}",
                (int)(s->range), s->angle, s->width,
                s->turret ? "true" : "false");
      }
    }
    fprintf(f, "\n      ]");
    fprintf(f, "\n    }");
  }
  fprintf(f, "\n  ],\n");
}

void print_footer(FILE *f) {
  fprintf(f,
          "  \"\": null\n"); // sentry, so everything prior can end with a comma
  fprintf(f, "}\n");
}

/*
 *
 * Run-time options
 *
 * Every setting an operator may override is one row in opts[].  The same
 * table drives command-line parsing, environment-variable parsing, range
 * checking, and the --help text, so they cannot drift apart.
 *
 * Precedence: command-line option, then environment variable, then default.
 *
 */

enum opt_type { OPT_INT, OPT_REAL };

struct opt {
  const char *group; /* heading in --help */
  const char *name;  /* long option, without the leading -- */
  int shortc;        /* short option character, or 0 */
  const char *env;   /* environment variable */
  enum opt_type type;
  void *dest; /* int * or double *, per type */
  double min, max; /* inclusive limits */
  double def;      /* default, for --help only; the real default is in dest */
  const char *arg; /* argument name, for --help */
  const char *help;
  int seen; /* set once a value has been applied */
};

#define G_GAME "Game options"
#define G_TANK "Tank physics options"
#define G_FORF "Forf interpreter options (per tank)"

static struct opt opts[] = {
    {G_GAME, "seed", 's', "SEED", OPT_INT, &cfg.seed, INT_MIN, INT_MAX, 0, "N",
     "Random seed for tank placement and the forf random word.  0 means use "
     "the process ID.",
     0},
    {G_GAME, "max-tanks", 0, "TANK_MAX_TANKS", OPT_INT, &cfg.max_tanks, 2, 1000,
     DEFAULT_MAX_TANKS, "N",
     "Maximum number of tanks in the game.  Extra TANKDIRs are ignored.", 0},
    {G_GAME, "rounds", 0, "TANK_ROUNDS", OPT_INT, &cfg.rounds, 1, 100000,
     DEFAULT_ROUNDS, "N",
     "Maximum number of rounds (turns).  The game ends sooner if at most one "
     "tank is left alive.",
     0},
    {G_GAME, "spacing", 0, "TANK_SPACING", OPT_INT, &cfg.spacing, 1, 10000,
     DEFAULT_SPACING, "N",
     "Initial distance between tanks.  The playing field is a grid of "
     "spacing x spacing cells, one per tank.",
     0},

    {G_TANK, "max-sensors", 0, "TANK_MAX_SENSORS", OPT_INT,
     &tank_cfg.max_sensors, 1, TANK_SENSORS_HARD_MAX,
     TANK_DEFAULT_MAX_SENSORS, "N",
     "Number of sensors per tank (files sensor0 .. sensorN-1).", 0},
    {G_TANK, "radius", 0, "TANK_RADIUS", OPT_REAL, &tank_cfg.radius, 0.1, 500,
     TANK_DEFAULT_RADIUS, "R", "Tank radius.", 0},
    {G_TANK, "sensor-range", 0, "TANK_SENSOR_RANGE", OPT_INT,
     &tank_cfg.sensor_range, 1, 1000, TANK_DEFAULT_SENSOR_RANGE, "N",
     "Longest range a sensor may have.  Longer ranges in sensor files are "
     "clamped to this.",
     0},
    {G_TANK, "cannon-recharge", 0, "TANK_CANNON_RECHARGE", OPT_INT,
     &tank_cfg.cannon_recharge, 0, 1000, TANK_DEFAULT_CANNON_RECHARGE, "N",
     "Turns needed to recharge the cannon after firing.", 0},
    {G_TANK, "cannon-range", 0, "TANK_CANNON_RANGE", OPT_INT,
     &tank_cfg.cannon_range, 1, 1000, TANK_DEFAULT_CANNON_RANGE, "N",
     "Cannon range.  Traditionally half the sensor range, but independent of "
     "it: see NOTES.",
     0},
    {G_TANK, "max-accel", 0, "TANK_MAX_ACCEL", OPT_INT, &tank_cfg.max_accel, 1,
     200, TANK_DEFAULT_MAX_ACCEL, "N",
     "Most a tread's speed (-100..100) may change in one turn.", 0},
    {G_TANK, "max-turret-rot", 0, "TANK_MAX_TURRET_ROT", OPT_REAL,
     &cfg.turret_rot_deg, 0.1, 360, 45, "DEG",
     "Most the turret can rotate in one turn, in degrees (stock: 45, which "
     "is tau/8 radians).",
     0},
    {G_TANK, "top-speed", 0, "TANK_TOP_SPEED", OPT_REAL, &tank_cfg.top_speed,
     0.01, 1000, TANK_DEFAULT_TOP_SPEED, "V",
     "Distance a tank moves per turn with both treads at 100.", 0},
    {G_TANK, "friction", 0, "TANK_FRICTION", OPT_REAL, &tank_cfg.friction, 0, 1,
     TANK_DEFAULT_FRICTION, "F",
     "Speed penalty when the treads run at different speeds.", 0},

    {G_FORF, "dstack", 0, "FORF_DSTACK_SIZE", OPT_INT, &cfg.dstack_size, 1,
     16384, DEFAULT_DSTACK_SIZE, "N", "Data stack size, in values.", 0},
    {G_FORF, "cstack", 0, "FORF_CSTACK_SIZE", OPT_INT, &cfg.cstack_size, 1,
     16384, DEFAULT_CSTACK_SIZE, "N",
     "Command stack size, in values.  This also bounds program length.", 0},
    {G_FORF, "registers", 0, "FORF_REGISTERS", OPT_INT, &cfg.memory_size, 0,
     4096, DEFAULT_MEMORY_SIZE, "N",
     "Number of memory registers (all start at 0).", 0},
};

#define NOPTS ((int)(sizeof(opts) / sizeof(opts[0])))

static const char *prog = "forftanks";

/* Print text word-wrapped to a fixed width, indented. */
static void print_wrapped(FILE *f, const char *text, int indent, int width) {
  int col = 0;

  while (*text) {
    int len = (int)strcspn(text, " ");

    if (col && col + 1 + len > width) {
      fputc('\n', f);
      col = 0;
    }
    if (!col) {
      fprintf(f, "%*s", indent, "");
      col = indent;
    } else {
      fputc(' ', f);
      col += 1;
    }
    fwrite(text, 1, len, f);
    col += len;
    text += len;
    while (*text == ' ') {
      text += 1; /* collapses runs of spaces */
    }
  }
  fputc('\n', f);
}

static void usage(FILE *f) {
  const char *group = NULL;

  fprintf(f, "usage: %s [OPTIONS] [--] TANKDIR [TANKDIR...]\n", prog);
  fprintf(f,
          "\n"
          "Run one game of tanks between the tanks in the given directories,\n"
          "and write a JSON description of the game to standard output.\n"
          "\n"
          "Every setting below can be given as a command-line option or as an\n"
          "environment variable.  The command-line option wins; the\n"
          "environment variable is used if there is no option; otherwise the\n"
          "default applies.  Use -- before a TANKDIR that begins with a dash.\n");

  for (int i = 0; i < NOPTS; i += 1) {
    struct opt *o = &opts[i];
    char sopt[8] = "    ";

    if (o->group != group) {
      group = o->group;
      fprintf(f, "\n%s:\n", group);
    }
    if (o->shortc) {
      snprintf(sopt, sizeof(sopt), "-%c, ", o->shortc);
    }
    fprintf(f, "  %s--%s %s\n", sopt, o->name, o->arg);
    print_wrapped(f, o->help, 8, 78);
    fprintf(f, "        env: %s   default: %.10g   allowed: %.10g to %.10g\n",
            o->env, o->def, o->min, o->max);
  }

  fprintf(f, "\nOther options:\n");
  fprintf(f, "  -h, --help\n        Show this help and exit.\n");
  fprintf(f,
          "\n"
          "NOTES:\n"
          "  Cannon range has traditionally been defined as half the sensor\n"
          "  range.  It is now a separate setting that defaults to 50, so\n"
          "  changing --sensor-range does not change --cannon-range.  Set both\n"
          "  if you want the traditional ratio.\n"
          "\n"
          "  Settings that strain the movement or collision model (for\n"
          "  example a high top speed with a small radius) are accepted, but\n"
          "  a warning is printed on standard error.\n"
          "\n"
          "  The output includes a \"params\" object listing the settings in\n"
          "  effect for the game.\n");
}

static void opt_fail(const char *source, const char *fmt, ...)
    __attribute__((noreturn, format(printf, 2, 3)));

static void opt_fail(const char *source, const char *fmt, ...) {
  va_list ap;

  fprintf(stderr, "%s: %s: ", prog, source);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\nTry '%s --help' for more information.\n", prog);
  exit(1);
}

/* Parse val for option o and store it, or exit with a message naming source
 * (which tells the operator whether the bad value came from a command-line
 * option or from an environment variable). */
static void opt_apply(struct opt *o, const char *val, const char *source) {
  char *end;
  double v;

  if (!*val) {
    opt_fail(source, "empty value");
  }

  errno = 0;
  if (o->type == OPT_INT) {
    long l = strtol(val, &end, 10);

    if (end == val || *end || errno == ERANGE) {
      opt_fail(source, "\"%s\" is not a valid integer", val);
    }
    v = (double)l;
  } else {
    v = strtod(val, &end);

    if (end == val || *end || !isfinite(v)) {
      opt_fail(source, "\"%s\" is not a valid number", val);
    }
  }

  if (v < o->min || v > o->max) {
    opt_fail(source, "%s is out of range (allowed: %.10g to %.10g)", val,
             o->min, o->max);
  }

  if (o->type == OPT_INT) {
    *(int *)o->dest = (int)v;
  } else {
    *(double *)o->dest = v;
  }
  o->seen = 1;
}

static struct opt *opt_by_name(const char *name) {
  for (int i = 0; i < NOPTS; i += 1) {
    if (0 == strcmp(opts[i].name, name)) {
      return &opts[i];
    }
  }
  abort(); /* programming error */
}

/* Fill in settings from the command line, then the environment.  Returns the
 * index in argv of the first TANKDIR. */
static int parse_options(int argc, char *argv[]) {
  struct option longopts[NOPTS + 2];
  char source[64];
  int c;

  for (int i = 0; i < NOPTS; i += 1) {
    longopts[i].name = opts[i].name;
    longopts[i].has_arg = required_argument;
    longopts[i].flag = NULL;
    longopts[i].val = 1000 + i;
  }
  longopts[NOPTS].name = "help";
  longopts[NOPTS].has_arg = no_argument;
  longopts[NOPTS].flag = NULL;
  longopts[NOPTS].val = 'h';
  memset(&longopts[NOPTS + 1], 0, sizeof(longopts[0]));

  while ((c = getopt_long(argc, argv, "hs:", longopts, NULL)) != -1) {
    struct opt *o = NULL;

    if (c == 'h') {
      usage(stdout);
      exit(0);
    }
    if (c == '?') {
      /* getopt_long already said what was wrong */
      fprintf(stderr, "Try '%s --help' for more information.\n", prog);
      exit(1);
    }
    for (int i = 0; i < NOPTS; i += 1) {
      if (c == 1000 + i || (c != 0 && c == opts[i].shortc)) {
        o = &opts[i];
        break;
      }
    }
    if (!o) {
      abort(); /* programming error */
    }
    snprintf(source, sizeof(source), "option --%s", o->name);
    opt_apply(o, optarg, source);
  }

  /* Anything not set on the command line may come from the environment. */
  for (int i = 0; i < NOPTS; i += 1) {
    const char *e;

    if (opts[i].seen) {
      continue;
    }
    e = getenv(opts[i].env);
    if (e && *e) { /* an empty variable counts as unset */
      snprintf(source, sizeof(source), "environment variable %s", opts[i].env);
      opt_apply(&opts[i], e, source);
    }
  }

  return optind;
}

int main(int argc, char *argv[]) {
  struct tanks_game game;
  struct forftank *myftanks;
  struct tank *mytanks;
  struct forf_lexical_env lenv[LENV_SIZE];
  int *order;
  int ntanks = 0;
  int i;
  int seed;
  int first;

  prog = argv[0];
  first = parse_options(argc, argv);

  if (first >= argc) {
    fprintf(stderr, "%s: no tank directories given\n\n", prog);
    usage(stderr);
    return 1;
  }

  /* Settings given in different units get converted now */
  if (opt_by_name("max-turret-rot")->seen) {
    tank_cfg.max_turret_rot = deg2rad(cfg.turret_rot_deg);
  }
  tank_params_finalize(&tank_cfg);
  tank_params_check(&tank_cfg, cfg.spacing, stderr);

  lenv[0].name = NULL;
  lenv[0].proc = NULL;
  if ((!forf_extend_lexical_env(lenv, forf_base_lexical_env, LENV_SIZE)) ||
      (!forf_extend_lexical_env(lenv, tanks_lenv_addons, LENV_SIZE))) {
    fprintf(stderr, "Unable to initialize lexical environment.\n");
    return 1;
  }

  /* We only need slightly random numbers */
  seed = cfg.seed;
  if (!seed) {
    seed = getpid();
  }
  srand(seed);

  myftanks = ft_calloc(cfg.max_tanks, sizeof(*myftanks));
  mytanks = ft_calloc(cfg.max_tanks, sizeof(*mytanks));
  order = ft_calloc(cfg.max_tanks, sizeof(*order));

  if (argc - first > cfg.max_tanks) {
    fprintf(stderr,
            "warning: %d tank directories given but at most %d allowed (see "
            "--max-tanks); ignoring the last %d\n",
            argc - first, cfg.max_tanks, argc - first - cfg.max_tanks);
  }

  /* Every remaining argument is a tank directory */
  for (i = first; ntanks < cfg.max_tanks && i < argc; i += 1) {
    if (ft_read_tank(&myftanks[ntanks], &mytanks[ntanks], lenv, argv[i])) {
      ntanks += 1;
    }
  }

  /* Calculate the size of the game board */
  {
    int x, y;

    for (x = 1; x * x < ntanks; x += 1)
      ;
    y = ntanks / x;
    if (ntanks % x) {
      y += 1;
    }

    game.size[0] = x * cfg.spacing;
    game.size[1] = y * cfg.spacing;
  }

  /* Shuffle the order we place things on the game board */
  for (i = 0; i < ntanks; i += 1) {
    order[i] = i;
  }
  for (i = 0; i < ntanks; i += 1) {
    int j = rand() % ntanks;
    int n = order[j];

    order[j] = order[i];
    order[i] = n;
  }

  /* Position tanks */
  {
    int x = cfg.spacing / 2;
    int y = cfg.spacing / 2;

    for (i = 0; i < ntanks; i += 1) {
      mytanks[order[i]].position[0] = (float)x;
      mytanks[order[i]].position[1] = (float)y;
      mytanks[order[i]].angle = deg2rad(rand() % 360);
      mytanks[order[i]].turret.current = deg2rad(rand() % 360);
      mytanks[order[i]].turret.desired = mytanks[order[i]].turret.current;

      x += cfg.spacing;
      if (x > game.size[0]) {
        x %= (int)(game.size[0]);
        y += cfg.spacing;
      }
    }
  }

  print_header(stdout, &game, seed);
  print_rounds(stdout, &game, mytanks, ntanks);
  print_standings(stdout, myftanks, mytanks, ntanks);
  print_footer(stdout);

  return 0;
}

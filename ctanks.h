#pragma once

#include <stdio.h>

/* τ = 2π */
#define TAU 6.28318530717958647692
#define PI 3.14159265358979323846

/* In-game constants.
 *
 * These used to be compile-time #defines.  They are now run-time
 * parameters held in tank_cfg, so an operator can override them (see
 * forftanks.c for the command-line and environment handling).  The
 * TANK_* names below are kept as macros so the physics code reads the
 * same as before.
 *
 * The TANK_DEFAULT_* values are the stock game.  With no overrides the
 * simulation is bit-for-bit identical to the old compile-time version. */

/* Sensors are reported to the renderer as an int bitmask, so there is a
 * structural limit of 31.  struct tank is sized with this; the number
 * actually in use is tank_cfg.max_sensors. */
#define TANK_SENSORS_HARD_MAX 31

#define TANK_DEFAULT_MAX_SENSORS 10
#define TANK_DEFAULT_RADIUS 7.5
#define TANK_DEFAULT_SENSOR_RANGE 100
#define TANK_DEFAULT_CANNON_RECHARGE 20 /* Turns to recharge cannon */
/* Cannon range is traditionally TANK_SENSOR_RANGE / 2 (which is 50 with the
 * stock sensor range).  It is now an independent parameter that defaults
 * to 50: changing the sensor range does NOT change the cannon range. */
#define TANK_DEFAULT_CANNON_RANGE 50
#define TANK_DEFAULT_MAX_ACCEL 35
#define TANK_DEFAULT_MAX_TURRET_ROT (TAU / 8) /* radians */
#define TANK_DEFAULT_TOP_SPEED 7
#define TANK_DEFAULT_FRICTION 0.75

struct tank_params {
  int max_sensors;
  double radius;
  int sensor_range;
  int cannon_recharge;
  int cannon_range;
  int max_accel;
  double max_turret_rot; /* radians */
  double top_speed;
  double friction;

  /* Derived by tank_params_finalize(); do not set directly. */
  double collision_adj2;
  double sensor_adj2;
  double cannon_adj2;
};

/* The active parameters.  Statically initialised to the defaults. */
extern struct tank_params tank_cfg;

/* Recompute the derived fields after changing any parameter. */
void tank_params_finalize(struct tank_params *p);

/* Write a "warning: ..." line to out for each parameter combination that
 * strains the movement/collision model.  Never fatal; returns the number of
 * warnings.  spacing is the initial distance between tanks (<= 0 to skip
 * the check that involves it). */
int tank_params_check(const struct tank_params *p, double spacing, FILE *out);

#define TANK_MAX_SENSORS (tank_cfg.max_sensors)
#define TANK_RADIUS (tank_cfg.radius)
#define TANK_SENSOR_RANGE (tank_cfg.sensor_range)
#define TANK_CANNON_RECHARGE (tank_cfg.cannon_recharge)
#define TANK_CANNON_RANGE (tank_cfg.cannon_range)
#define TANK_MAX_ACCEL (tank_cfg.max_accel)
#define TANK_MAX_TURRET_ROT (tank_cfg.max_turret_rot)
#define TANK_TOP_SPEED (tank_cfg.top_speed)
#define TANK_FRICTION (tank_cfg.friction)

/* (tank radius + tank radius)^2 */
#define TANK_COLLISION_ADJ2 (tank_cfg.collision_adj2)

/* (Sensor range + tank radius)^2
 * If the distance^2 to the center of a tank <= TANK_SENSOR_ADJ2,
 * that tank is within sensor range. */
#define TANK_SENSOR_ADJ2 (tank_cfg.sensor_adj2)

/* (Cannon range + tank radius)^2 */
#define TANK_CANNON_ADJ2 (tank_cfg.cannon_adj2)

#ifndef rad2deg
#define rad2deg(r) ((int)(360 * (r) / TAU))
#define deg2rad(r) ((r * TAU) / 360)
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

struct tanks_game {
  float size[2]; /* dimensions of playing field */
};

struct tank;

struct sensor {
  float angle;
  float width;
  int range;
  int turret; /* Mounted to turret? */
  int triggered;
};

typedef void tank_run_func(struct tank *, void *);

struct tank {
  float position[2]; /* Current position on the board */
  float angle;       /* Current orientation */
  struct {
    float current[2]; /* Current tread speed */
    float desired[2]; /* Desired tread speed */
  } speed;
  struct {
    float current; /* Current turret angle */
    float desired; /* Desired turret angle */
    int firing;    /* True if firing this turn */
    int recharge;  /* Turns until gun is recharged */
  } turret;
  struct sensor sensors[TANK_SENSORS_HARD_MAX]; /* Sensor array */
  int led;                                      /* State of the LED */
  struct tank *killer;                          /* Killer, or NULL if alive */
  char *cause_death;                            /* Cause of death */

  tank_run_func *run; /* Function to run a tank */
  void *udata;        /* Argument to pass to run */
};

void tank_init(struct tank *tank, tank_run_func *run, void *udata);
void tanks_run_turn(struct tanks_game *game, struct tank *tanks, int ntanks);

/*
 *
 * Tanks API for scripts
 *
 */

/** Has the turret recharged? */
int tank_fire_ready(struct tank *tank);

/** Fire! */
void tank_fire(struct tank *tank);

/** Set desired speed */
void tank_set_speed(struct tank *tank, float left, float right);

/** Get the current turret angle */
float tank_get_turret(struct tank *tank);

/** Set the desired turret angle */
void tank_set_turret(struct tank *tank, float angle);

/** Is a sensor active? */
int tank_get_sensor(struct tank *tank, int sensor_num);

/** Set the LED state */
void tank_set_led(struct tank *tank, int active);

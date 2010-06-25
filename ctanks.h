#ifndef __CTANKS_H__
#define __CTANKS_H__

#include "brad.h"

/* Some useful constants */
#define TANK_MAX_SENSORS  10
#define TANK_RADIUS       ((bs_t)1920)
#define TANK_SENSOR_RANGE ((int)100)

/* (tank radius + tank radius)^2 */
#define TANK_COLLISION_ADJ2 \
  bs_to_int((TANK_RADIUS + TANK_RADIUS) * (TANK_RADIUS + TANK_RADIUS))

/* (Sensor range + tank radius)^2
 * If the distance^2 to the center of a tank <= TANK_SENSOR_ADJ2,
 * that tank is within sensor range. */
#define TANK_SENSOR_ADJ2 ((int)11556)

struct tanks_game {
  bs_t size[2];                 /* dimensions of playing field */
};

struct tank;

struct sensor {
  brad_t angle;
  brad_t width;
  int    range;
  int    range_adj2;            /* (range + TANK_RADIUS)^2 */
  int    turret;                /* Mounted to turret? */
  int    triggered;
};

typedef void tank_run_func(struct tank *, void *);

struct tank {
  bs_t           position[2];   /* Current position on the board */
  brad_t         angle;         /* Current orientation */
  struct {
    bs_t         current[2];    /* Current tread speed */
    bs_t         desired[2];    /* Desired tread speed */
  } speed;
  struct {
    brad_t       current;       /* Current turret angle */
    brad_t       desired;       /* Desired turret angle */
    int          firing;        /* True if firing this turn */
    int          recharge;      /* Turns until gun is recharged */
  } turret;
  struct sensor  sensor[MAX_SENSORS]; /* Sensor array */
  int            num_sensors;         /* Number of sensors */
  int            led;                 /* State of the LED */
  struct tank   *killer;              /* Killer, or NULL if alive */
  char          *cause_death;         /* Cause of death */

  tank_run_func *run;           /* Function to run a tank */
  void          *udata;         /* Argument to pass to run */
};

void tank_init(struct tank *tank, tank_run_func *run, void *udata);

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
void tank_set_speed(struct tank *tank, int left, int right);

/** Get the current turret angle */
int tank_get_turret(struct tank *tank);

/** Set the desired turret angle */
void tank_set_turret(struct tank *tank, int angle);

/** Is a sensor active? */
int tank_get_sensor(struct tank *tank, int sensor_num);

/** Set the LED state */
void tank_set_led(struct tank *tank, int active);

#endif /* __CTANKS_H__ */

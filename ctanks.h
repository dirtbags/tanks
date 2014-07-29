#ifndef __CTANKS_H__
#define __CTANKS_H__

/* τ = 2π */
#define TAU 6.28318530717958647692
#define PI  3.14159265358979323846

/* Some in-game constants */
#define TANK_MAX_SENSORS     10
#define TANK_RADIUS          7.5
#define TANK_SENSOR_RANGE    100
#define TANK_CANNON_RECHARGE 20 /* Turns to recharge cannon */
#define TANK_CANNON_RANGE    (TANK_SENSOR_RANGE / 2)
#define TANK_MAX_ACCEL       35
#define TANK_MAX_TURRET_ROT  (TAU/8)
#define TANK_TOP_SPEED       7
#define TANK_FRICTION 0.75

/* (tank radius + tank radius)^2 */
#define TANK_COLLISION_ADJ2 \
  ((TANK_RADIUS + TANK_RADIUS) * (TANK_RADIUS + TANK_RADIUS))

/* (Sensor range + tank radius)^2
 * If the distance^2 to the center of a tank <= TANK_SENSOR_ADJ2,
 * that tank is within sensor range. */
#define TANK_SENSOR_ADJ2 \
  ((TANK_SENSOR_RANGE + TANK_RADIUS) * (TANK_SENSOR_RANGE + TANK_RADIUS))

#define TANK_CANNON_ADJ2 \
  ((TANK_CANNON_RANGE + TANK_RADIUS) * (TANK_CANNON_RANGE + TANK_RADIUS))

#ifndef rad2deg
#define rad2deg(r) ((int)(360*(r)/TAU))
#define deg2rad(r) ((r*TAU)/360)
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

struct tanks_game {
  float size[2];                /* dimensions of playing field */
};

struct tank;

struct sensor {
  float angle;
  float width;
  int   range;
  int   turret;                 /* Mounted to turret? */
  int   triggered;
};

typedef void tank_run_func(struct tank *, void *);

struct tank {
  float          position[2];   /* Current position on the board */
  float          angle;         /* Current orientation */
  struct {
    float        current[2];    /* Current tread speed */
    float        desired[2];    /* Desired tread speed */
  } speed;
  struct {
    float        current;       /* Current turret angle */
    float        desired;       /* Desired turret angle */
    int          firing;        /* True if firing this turn */
    int          recharge;      /* Turns until gun is recharged */
  } turret;
  struct sensor  sensors[TANK_MAX_SENSORS]; /* Sensor array */
  int            led;           /* State of the LED */
  struct tank   *killer;        /* Killer, or NULL if alive */
  char          *cause_death;   /* Cause of death */

  tank_run_func *run;           /* Function to run a tank */
  void          *udata;         /* Argument to pass to run */
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


#endif /* __CTANKS_H__ */

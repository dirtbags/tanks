#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ctanks.h"

/* Debugging help */
#define DUMPf(fmt, args...) fprintf(stderr, "%s:%s:%d " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#define DUMP() DUMPf("")
#define DUMP_d(v) DUMPf("%s = %d", #v, v)
#define DUMP_x(v) DUMPf("%s = 0x%x", #v, v)
#define DUMP_s(v) DUMPf("%s = %s", #v, v)
#define DUMP_c(v) DUMPf("%s = %c", #v, v)
#define DUMP_f(v) DUMPf("%s = %f", #v, v)
#define DUMP_xy(v) DUMPf("%s = (%f, %f)", #v, v[0], v[1]);
#define DUMP_angle(v) DUMPf("%s = %.3fπ", #v, (v/PI));

#define sq(x) ((x) * (x))


void
tank_init(struct tank *tank, tank_run_func *run, void *udata)
{
  memset(tank, 0, sizeof(*tank));
  tank->run = run;
  tank->udata = udata;
}

int
tank_fire_ready(struct tank *tank)
{
  return (! tank->turret.recharge);
}

void
tank_fire(struct tank *tank)
{
  tank->turret.firing = tank_fire_ready(tank);
}

void
tank_set_speed(struct tank *tank, float left, float right)
{
  tank->speed.desired[0] = left;
  tank->speed.desired[1] = right;
}

float
tank_get_turret(struct tank *tank)
{
  return tank->turret.current;
}

void
tank_set_turret(struct tank *tank, float angle)
{
  tank->turret.desired = fmodf(angle, 2*PI);
}

int
tank_get_sensor(struct tank *tank, int sensor_num)
{
  if ((sensor_num < 0) || (sensor_num > TANK_MAX_SENSORS)) {
    return 0;
  }
  return tank->sensors[sensor_num].triggered;
}

void
tank_set_led(struct tank *tank, int active)
{
  tank->led = active;
}

static void
rotate_point(float angle, float point[2])
{
  float cos_, sin_;
  float new[2];

  cos_ = cosf(angle);
  sin_ = sinf(angle);

  new[0] = point[0]*cos_ - point[1]*sin_;
  new[1] = point[0]*sin_ + point[1]*cos_;

  point[0] = new[0];
  point[1] = new[1];
}


static void
tank_sensor_calc(struct tanks_game *game,
                 struct tank *this,
                 struct tank *that,
                 float dist2)
{
  float tpos[2];
  int   i;

  /* Translate other tank's position to make us the origin */
  for (i = 0; i < 2; i += 1) {
    tpos[i] = that->position[i] - this->position[i];
  }

  /* Did this shoot that?  Rotate point by turret degrees, and if |y| <
     TANK_RADIUS, we have a hit. */
  if ((this->turret.firing) && (dist2 <= TANK_CANNON_RANGE)) {
    int   theta = this->angle + this->turret.current;
    float rpos[2];

    rpos[0] = tpos[0];
    rpos[1] = tpos[1];
    rotate_point(-theta, rpos);
    if (fabsf(rpos[1]) < TANK_RADIUS) {
      that->killer = this;
      that->cause_death = "shot";
    }
  }

  /* Calculate sensors */
  for (i = 0; i < TANK_MAX_SENSORS; i += 1) {
    float theta;
    float rpos[2];
    float m_r, m_s;

    /* No need to re-check this sensor if it's already firing */
    if (this->sensors[i].triggered) {
      continue;
    }

    /* If the tank is out of range, don't bother */
    if (dist2 > sq(this->sensors[i].range + TANK_RADIUS)) {
      continue;
    }

    /* What is the angle of our sensor? */
    theta = this->angle + this->sensors[i].angle;
    if (this->sensors[i].turret) {
      theta += this->turret.current;
    }
    theta = fmodf(theta, 2*PI);

    /* Rotate tpos by theta */
    rpos[0] = tpos[0];
    rpos[1] = tpos[1];
    rotate_point(-theta, rpos);

    /* Sensor is symmetrical, we can consider only top quadrants */
    rpos[1] = fabsf(rpos[1]);

    /* Compute inverse slopes to tank and of our sensor */
    m_s = 1 / tanf(this->sensors[i].width / 2);
    m_r = rpos[0] / rpos[1];

    if (1 == i) {
      DUMP();
      DUMP_angle(this->angle);
      DUMP_angle(theta);
      DUMP_xy(tpos);
      DUMP_xy(rpos);
    }

    /* If our inverse slope is less than theirs, they're inside the arc */
    if (m_r >= m_s) {
      this->sensors[i].triggered = 1;
      continue;
    }

    /* Now check if the edge of the arc intersects the tank.  Do this
       just like with firing. */
    rotate_point(this->sensors[i].width / -2, rpos);
    if (fabsf(rpos[1]) < TANK_RADIUS) {
      this->sensors[i].triggered = 1;
    }
  }
}

void
do_shit_with(struct tanks_game *game,
             struct tank *this,
             struct tank *that)
{
  float vector[2];
  float dist2;                  /* distance squared */
  float tpos;                   /* Translated position */
  int   i;

  /* Don't bother if that is dead */
  /* XXX: If three tanks occupy the same space at the same time, only
     the first two will collide. */
  if ((this->killer) || (that->killer)) {
    return;
  }

  /* Establish shortest vector from center of this to center of that,
   * taking wrapping into account */
  for (i = 0; i < 2; i += 1) {
    float halfsize = game->size[i] / 2;

    vector[i] = halfsize - fabsf(that->position[i] - this->position[i] - halfsize);
  }

  /* Compute distance^2 for range comparisons */
  dist2 = sq(vector[0]) + sq(vector[1]);

  /* If they're not within sensor range, there's nothing to do. */
  if (dist2 > TANK_SENSOR_ADJ2) {
    return;
  }

  /* Did they collide? */
  if (dist2 < TANK_COLLISION_ADJ2) {
    this->killer = that;
    this->cause_death = "collision";

    that->killer = this;
    that->cause_death = "collision";

    return;
  }

  /* Figure out who's whomin' whom */
  tank_sensor_calc(game, this, that, dist2);
  tank_sensor_calc(game, that, this, dist2);
}

void
tanks_move_tank(struct tanks_game *game,
                struct tank *tank)
{
  int   i;
  float movement;
  float angle;

  /* Rotate the turret */
  {
    float rot_angle;              /* Quickest way there */

    /* Constrain rot_angle to between -PI and PI */
    rot_angle = tank->turret.desired - tank->turret.current;
    while (rot_angle < 0) {
      rot_angle += 2*PI;
    }
    rot_angle = fmodf(PI + rot_angle, 2*PI) - PI;

    rot_angle = min(TANK_MAX_TURRET_ROT, rot_angle);
    rot_angle = max(-TANK_MAX_TURRET_ROT, rot_angle);
    tank->turret.current = fmodf(tank->turret.current + rot_angle, 2*PI);
  }

  /* Fakey acceleration */
  for (i = 0; i < 2; i += 1) {
    if (tank->speed.current[i] == tank->speed.desired[i]) {
      /* Do nothing */
    } else if (tank->speed.current[i] < tank->speed.desired[i]) {
      tank->speed.current[i] = min(tank->speed.current[i] + TANK_MAX_ACCEL,
                                   tank->speed.desired[i]);
    } else {
      tank->speed.current[i] = max(tank->speed.current[i] - TANK_MAX_ACCEL,
                                   tank->speed.desired[i]);
    }
  }

  /* The simple case */
  if (tank->speed.current[0] == tank->speed.current[1]) {
    movement = tank->speed.current[0] * (TANK_TOP_SPEED / 100.0);
    angle    = 0;
  } else {
    /* pflarr's original comment:
     *
     *   The tank drives around in a circle of radius r, which is some
     *   offset on a line perpendicular to the tank.  The distance it
     *   travels around the circle varies with the speed of each tread,
     *   and is such that each side of the tank moves an equal angle
     *   around the circle.
     *
     * Sounds good to me.   pflarr's calculations here are fantastico,
     * there's nothing whatsoever to change. */
    float friction;
    float v[2];
    float So, Si;
    float r;
    float theta;
    int   dir;

    /* The first thing Paul's code does is find "friction", which seems
       to be a penalty for having the treads go in opposite directions.
       This probably plays hell with precisely-planned tanks, which I
       find very ha ha. */
    friction = .75 * (fabsf(tank->speed.current[0] - tank->speed.current[1]) / 200);
    v[0] = tank->speed.current[0] * (1 - friction) * (TANK_TOP_SPEED / 100.0);
    v[1] = tank->speed.current[1] * (1 - friction) * (TANK_TOP_SPEED / 100.0);

    /* Outside and inside speeds */
    So = max(v[0], v[1]);
    Si = min(v[0], v[1]);
    dir = (v[0] > v[1]) ? 1 : -1;

    /* Radius of circle to outside tread (use similar triangles) */
    r = So * (TANK_RADIUS * 2) / (So - Si);

    /* pflarr:

       The fraction of the circle traveled is equal to the speed
       of the outer tread over the circumference of the circle:
           Ft = So/(2*pi*r)
       The angle traveled is:
           theta = Ft * 2*pi
       This reduces to a simple
           theta = So/r
       We multiply it by dir to adjust for the direction of rotation
    */
    theta = So/r * dir;

    movement = r * tanf(theta);
    angle = theta;
  }

  /* Now move the tank */
  tank->angle = fmodf(tank->angle + angle + 2*PI, 2*PI);
  {
    float m[2];

    m[0] = cosf(tank->angle) * movement;
    m[1] = sinf(tank->angle) * movement;

    for (i = 0; i < 2; i += 1) {
      tank->position[i] = fmodf(tank->position[i] + m[i] + game->size[i],
                                game->size[i]);
    }
  }
}

void
tanks_run_turn(struct tanks_game *game, struct tank *tanks, int ntanks)
{
  int i, j;

  /* Run programs */
  for (i = 0; i < ntanks; i += 1) {
    if (! tanks[i].killer) {
      tanks[i].run(&tanks[i], &tanks[i].udata);
    }
  }

  /* Clear sensors */
  for (i = 0; i < ntanks; i += 1) {
    for (j = 0; j < TANK_MAX_SENSORS; j += 1) {
      tanks[i].sensors[j].triggered = 0;
    }
  }

  /* Move */
  for (i = 0; i < ntanks; i += 1) {
    if (! tanks[i].killer) {
      tanks_move_tank(game, &(tanks[i]));
    }
  }

  /* Sense and fire */
  for (i = 0; i < ntanks; i += 1) {
    for (j = i + 1; j < ntanks; j += 1) {
      do_shit_with(game, &(tanks[i]), &(tanks[j]));
    }
  }
}

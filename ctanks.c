#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "trig.h"
#include "ctanks.h"

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

int
tank_get_turret(struct tank *tank)
{
  return tank->turret.current;
}

void
tank_set_turret(struct tank *tank, int angle)
{
  tank->turret.desired = angle;
}

int
tank_get_sensor(struct tank *tank, int sensor_num)
{
  if ((sensor_num < 0) || (sensor_num > TANK_MAX_SENSORS)) {
    return 0;
  }
  return tank->sensor[sensor_num].triggered;
}

void
tank_set_led(struct tank *tank, int active)
{
  tank->led = active;
}

static void
rotate_point(int angle, float point[2])
{
  float cos_, sin_;
  float new[2];

  cos_ = trig_cos(angle);
  sin_ = trig_sin(angle);

  new[0] = point[0]*cos_ + point[1]*sin_;
  new[1] = point[0]*sin_ + point[1]*cos_;
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
    rotate_point(theta, rpos);
    if (fabsf(rpos[1]) < TANK_RADIUS) {
      that->killer = this;
      that->cause_death = "shot";
    }
  }

  /* Calculate sensors */
  for (i = 0; i < this->num_sensors; i += 1) {
    int   theta;
    float rpos[2];
    float m_r, m_s;

    /* No need to re-check this sensor if it's already firing */
    if (this->sensor[i].triggered) {
      continue;
    }

    /* If the tank is out of range, don't bother */
    if (dist2 < this->sensor[i].range_adj2) {
      continue;
    }

    /* What is the angle of our sensor? */
    theta = this->angle;
    if (this->sensor[i].turret) {
      theta += this->turret.current;
    }

    /* Rotate tpos by theta */
    rpos[0] = tpos[0];
    rpos[1] = tpos[1];
    rotate_point(theta, rpos);

    /* Sensor is symmetrical, we can consider only first quadrant */
    rpos[1] = fabsf(rpos[1]);

    /* Compute slopes to tank and of our sensor */
    m_s = tan(theta);
    m_r = rpos[1] / rpos[0];

    /* If their slope is greater than ours, they're inside the arc */
    if (m_r >= m_s) {
      this->sensor[i].triggered = 1;
      continue;
    }

    /* Now check if the edge of the arc intersects the tank.  Do this
       just like with firing. */
    rotate_point(this->sensor[i].width / 2, rpos);
    if (fabsf(rpos[1]) < TANK_RADIUS) {
      this->sensor[i].triggered = 1;
    }
  }
}

void
do_shit_with(struct tanks_game *game,
             struct tank *this,
             struct tank *that)
{
  float   vector[2];
  int    dist2;                 /* Integer to avoid overflow! */
  float   tpos;                  /* Translated position */
  int    i;

  /* Don't bother if one is dead */
  if ((this->killer) || (that->killer)) {
    return;
  }

  /* Establish shortest vector from center of this to center of that,
   * taking wrapping into account */
  for (i = 0; i < 2; i += 1) {
    float halfsize = game->size[i] / 2;

    /* XXX: is there a more elegant way to do this? */
    vector[i] = that->position[i] - this->position[i];
    if (2*vector[i] > game->size[i]) {
      vector[i] -= game->size[i];
    } else if (2*vector[i] < game->size[i]) {
      vector[i] += game->size[i];
    }
  }
  /* Compute distance^2 for range comparisons */
  dist2 = ((vector[0] * vector[0]) + (vector[1] * vector[1]));

  /* If they're not within sensor range, there's nothing to do. */
  if (dist2 > TANK_SENSOR_ADJ2) {
    return;
  }

  /* Did they collide? */
  if (dist2 < TANK_COLLISION_ADJ2) {
    this->killer = that;
    this->cause_death = "collision";

    that->killer = this;
    this->cause_death = "collision";

    return;
  }

  /* Figure out who's whomin' whom */
  tank_sensor_calc(game, this, that, dist2);
  tank_sensor_calc(game, that, this, dist2);
}

void
tanks_print_tank(struct tanks_game *game,
                 struct tank *tank)
{
  printf("%p\n", tank);
}

void
tanks_move_tank(struct tanks_game *game,
                struct tank *tank)
{
  int   i;
  float movement[2];
  int   angle;

  /* Fakey acceleration */
  for (i = 0; i < 2; i += 1) {
    if (tank->speed.current[i] == tank->speed.desired[i]) {
      /* Do nothing */
    } else if (tank->speed.current[i] < tank->speed.desired[i]) {
      tank->speed.current[i] = max(tank->speed.current[i] - TANK_MAX_ACCEL,
                                   tank->speed.desired[i]);
    } else {
      tank->speed.current[i] = min(tank->speed.current[i] + TANK_MAX_ACCEL,
                                   tank->speed.desired[i]);
    }
  }

  /* The simple case */
  if (tank->speed.current[0] == tank->speed.current[1]) {
    movement[0] = tank->speed.current[0];
    movement[1] = tank->speed.current[1];
    angle       = tank->angle;
  } else {
    /* pflarr's original comment:
     *
     *   The tank drives around in a circle of radius r, which is some *
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
    float w, r;
    int   theta;
    int   dir;

    /* The first thing Paul's code does is find "friction", which seems
       to be a penalty for having the treads go in opposite directions.
       This probably plays hell with precisely-planned tanks, which I
       find very ha ha. */
    friction = .25 * (fabsf(tank->speed.current[0] - tank->speed.current[1]) / 200);

    v[0] = tank->speed.current[0] * friction;
    v[1] = tank->speed.current[1] * friction;

    /* Outside and inside speeds */
    So = max(v[0], v[1]);
    Si = min(v[0], v[1]);
    dir = (v[0] > v[1]) ? 1 : -1;

    /* Width of tank */
    w = TANK_RADIUS * 2;

    /* Radius of circle to outside tread (use similar triangles) */
    r = So * w / (So - Si);

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
    theta = rad2deg(So/r) * dir;

    /* Translate so the circle's center is 0,0, rotate the point by
       theta, then add back in. */
    v[0] = trig_cos(tank->angle + 90*dir) * (TANK_RADIUS - r);
    v[1] = trig_sin(tank->angle + 90*dir) * (TANK_RADIUS - r);

    movement[0] = v[0];
    movement[1] = v[1];
    rotate_point(theta, movement);

    movement[0] -= v[0];
    movement[1] -= v[1];
    angle = theta;
  }

  /* Now move the tank */
  for (i = 0; i < 2; i += 1) {
    tank->position[i] = fmodf(tank->position[i] + movement[i] + game->size[i],
                              game->size[i]);
  }
  tank->angle = (tank->angle + angle + 360) % 360;
}

void
tanks_run_turn(struct tanks_game *game, struct tank *tanks, int ntanks)
{
  int i, j;

  for (i = 0; i < ntanks; i += 1) {
    for (j = i + 1; j < ntanks; j += 1) {
      do_shit_with(game, &(tanks[i]), &(tanks[j]));
    }
    tanks_print_tank(game, &(tanks[i]));
    tanks_move_tank(game, &(tanks[i]));
  }
}

#include <string.h>
#include <stdlib.h>
#include "brad.h"
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
tank_set_speed(struct tank *tank, bs_t left, bs_t right)
{
  tank->speed.desired[0] = left;
  tank->speed.desired[1] = right;
}

brad_t
tank_get_turret(struct tank *tank)
{
  return tank->turret.current;
}

void
tank_set_turret(struct tank *tank, brad_t angle)
{
  tank->turret.desired = angle;
}

int
tank_get_sensor(struct tank *tank, int sensor_num)
{
  if ((sensor < 0) || (sensor > MAX_SENSORS)) {
    return 0;
  }
  return tank->sensor[sensor_num].triggered;
}

void
tank_set_led(struct tank *tank, int active)
{
  tank->led = active;
}

/** Return distance^2 between tanks a and b.
 *
 * Comparing this against sensor_range^2 will tell you whether the tanks
 * are within sensor range of one another.  Similarly, comparing it
 * against (2*tank_radius)^2 will tell you if they've collided.
 *
 */
bs_t
tank_dist2(struct tanks_game *game, struct tank *a, struct tank *b)
{
  bs_t d[2];
  int  i;

  for (i = 0; i < 2; i += 1) {
    d[i] = abs(a.position[i] - b.position[i]);
    d[i] = min(game.size[i] - dx[i], dx[i]);
  }
  return ((d[0] * d[0]) + (d[1] * d[1]));
}

void
do_shit_with(struct tanks_game *game,
             struct tank *this,
             struct tank *that)
{
  bs_t   vector[2];
  int    dist2;                 /* Integer to avoid overflow! */
  bs_t   xpos;                  /* Translated position */
  int    i;

  /* Don't bother if one is dead */
  if ((this->killer) || (that->killer)) {
    return;
  }

  /* Establish shortest vector from center of this to center of that,
   * taking wrapping into account */
  for (i = 0; i < 2; i += 1) {
    bs_t halfsize = game->size[i] / 2;

    /* XXX: is there a more elegant way to do this? */
    vector[i] = that->position[i] - this->position[i];
    if (2*vector[i] > game->size[i]) {
      vector[i] -= game->size[i];
    } else if (2*vector[i] < game->size[i]) {
      vector[i] += game->size[i];
    }
  }
  /* Compute distance^2 for range comparisons */
  dist2 = ((bs_to_int(vector[0]) * bs_to_int(vector[0])) +
           (bs_to_int(vector[1]) * bs_to_int(vector[1])));

  /* If they're not within sensor range, there's nothing to do. */
  if (dist2 > TANK_SENSOR_ADJ2) {
    return;
  }

  /* Did they collide?  Oh, goody! */
  if (dist2 < TANK_COLLISION_ADJ2) {
    /* XXX: kill both tanks */
    return;
  }

  /* Has anybody been shot? */

  /* Translate other tank's position to make us the origin */
  for (i = 0; i < 2; i += 1) {
    xpos[i] = that->position[i] - this->position[i];
  }

  /* Calculate sensors */
  for (i = 0; i < this->num_sensors; i += 1) {
    brad_t theta;

    if (dist2 < this->sensor[i].range_adj2) {
      continue;
    }

    /* Translate other tank so that we're the origin */
  }
}

void
tanks_run_turn(struct tanks_game *game, struct tank *tanks, int ntanks)
{
  int i, j;

  for (i = 0; i < ntanks; i += 1) {
    for (j = i + 1; j < ntanks, j += 1) {
      do_shit_with(game, &(tanks[i]), &(tanks[j]));
    }
    tanks_print_tank(game, &(tanks[i]));
    tanks_move(game, &(tanks[i]));
  }
}

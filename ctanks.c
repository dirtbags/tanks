#include "ctanks.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Debugging help */
#define DUMPf(fmt, args...)                                                    \
  fprintf(stderr, "%s:%s:%d " fmt "\n", __FILE__, __FUNCTION__, __LINE__,      \
          ##args)
#define DUMP() DUMPf("")
#define DUMP_d(v) DUMPf("%s = %d", #v, v)
#define DUMP_x(v) DUMPf("%s = 0x%x", #v, v)
#define DUMP_s(v) DUMPf("%s = %s", #v, v)
#define DUMP_c(v) DUMPf("%s = %c", #v, v)
#define DUMP_f(v) DUMPf("%s = %f", #v, v)
#define DUMP_p(v) DUMPf("%s = %p", #v, v)
#define DUMP_xy(v) DUMPf("%s = (%f, %f)", #v, v[0], v[1]);
#define DUMP_angle(v) DUMPf("%s = %.3fτ", #v, (v / TAU));

#define sq(x) ((x) * (x))

/* Active game parameters; see ctanks.h.  The derived fields below must match
 * what tank_params_finalize() would compute from the defaults. */
struct tank_params tank_cfg = {
    TANK_DEFAULT_MAX_SENSORS,
    TANK_DEFAULT_RADIUS,
    TANK_DEFAULT_SENSOR_RANGE,
    TANK_DEFAULT_CANNON_RECHARGE,
    TANK_DEFAULT_CANNON_RANGE,
    TANK_DEFAULT_MAX_ACCEL,
    TANK_DEFAULT_MAX_TURRET_ROT,
    TANK_DEFAULT_TOP_SPEED,
    TANK_DEFAULT_FRICTION,
    (TANK_DEFAULT_RADIUS + TANK_DEFAULT_RADIUS) *
        (TANK_DEFAULT_RADIUS + TANK_DEFAULT_RADIUS),
    (TANK_DEFAULT_SENSOR_RANGE + TANK_DEFAULT_RADIUS) *
        (TANK_DEFAULT_SENSOR_RANGE + TANK_DEFAULT_RADIUS),
    (TANK_DEFAULT_CANNON_RANGE + TANK_DEFAULT_RADIUS) *
        (TANK_DEFAULT_CANNON_RANGE + TANK_DEFAULT_RADIUS),
};

void tank_params_finalize(struct tank_params *p) {
  p->collision_adj2 = (p->radius + p->radius) * (p->radius + p->radius);
  p->sensor_adj2 = (p->sensor_range + p->radius) * (p->sensor_range + p->radius);
  p->cannon_adj2 = (p->cannon_range + p->radius) * (p->cannon_range + p->radius);
}

/* Worst-case rotation (radians) a tank can make in one turn.  When the treads
 * differ, tanks_move_tank rotates by theta = (So - Si) / (2 * radius) and
 * then moves r * tan(theta).  tan() is singular at 90 degrees, and inflates
 * movement well before that.  The tread-speed difference after friction is
 * g(d) = d * (1 - F*d/200) * top_speed/100 for a raw difference d in
 * [0, 200]; g peaks at d = 100/F (or d = 200 when F <= 0.5). */
static double worst_turn(double radius, double top_speed, double friction) {
  double d = (friction > 0.5) ? (100.0 / friction) : 200.0;
  double g = d * (1.0 - friction * d / 200.0) * (top_speed / 100.0);

  return g / (2.0 * radius);
}

int tank_params_check(const struct tank_params *p, double spacing, FILE *out) {
  int n = 0;
  double theta = worst_turn(p->radius, p->top_speed, p->friction);
  double stock = worst_turn(TANK_DEFAULT_RADIUS, TANK_DEFAULT_TOP_SPEED,
                            TANK_DEFAULT_FRICTION);

  if (theta >= PI / 2) {
    fprintf(out,
            "warning: turning model: worst-case rotation is %.1f degrees per "
            "turn, at or past the 90-degree singularity of tan(); tanks may "
            "move backwards or get NaN positions (reduce top speed, increase "
            "radius, or raise friction)\n",
            theta * 180 / PI);
    n += 1;
  } else if (theta >= 0.8) {
    fprintf(out,
            "warning: turning model: worst-case rotation is %.1f degrees per "
            "turn (stock: %.1f); turning tanks move %.2fx farther than the "
            "arc they follow, and tan() blows up at 90 degrees\n",
            theta * 180 / PI, stock * 180 / PI, tan(theta) / theta);
    n += 1;
  }

  /* Collisions are only tested between end-of-turn positions, so two tanks
   * closing head-on faster than twice the collision diameter can swap
   * places without ever overlapping. */
  if (p->top_speed >= 2 * p->radius) {
    fprintf(out,
            "warning: top speed %g is at least twice the tank radius %g; "
            "tanks can pass through each other without a collision being "
            "detected\n",
            p->top_speed, p->radius);
    n += 1;
  }

  if (spacing > 0 && spacing < 2 * p->radius) {
    fprintf(out,
            "warning: initial spacing %g is less than the collision diameter "
            "%g; tanks will start out colliding\n",
            spacing, 2 * p->radius);
    n += 1;
  }

  return n;
}

void tank_init(struct tank *tank, tank_run_func *run, void *udata) {
  memset(tank, 0, sizeof(*tank));
  tank->run = run;
  tank->udata = udata;
}

int tank_fire_ready(struct tank *tank) { return (!tank->turret.recharge); }

void tank_fire(struct tank *tank) {
  tank->turret.firing = tank_fire_ready(tank);
}

void tank_set_speed(struct tank *tank, float left, float right) {
  tank->speed.desired[0] = min(max(left, -100), 100);
  tank->speed.desired[1] = min(max(right, -100), 100);
}

float tank_get_turret(struct tank *tank) { return tank->turret.current; }

void tank_set_turret(struct tank *tank, float angle) {
  tank->turret.desired = fmodf(angle, TAU);
}

int tank_get_sensor(struct tank *tank, int sensor_num) {
  if ((sensor_num < 0) || (sensor_num >= TANK_MAX_SENSORS)) {
    return 0;
  }
  return tank->sensors[sensor_num].triggered;
}

void tank_set_led(struct tank *tank, int active) { tank->led = active; }

static void rotate_point(float angle, float point[2]) {
  float cos_, sin_;
  float new[2];

  cos_ = cosf(angle);
  sin_ = sinf(angle);

  new[0] = point[0] * cos_ - point[1] * sin_;
  new[1] = point[0] * sin_ + point[1] * cos_;

  point[0] = new[0];
  point[1] = new[1];
}

static void tanks_fire_cannon(struct tanks_game *game, struct tank *this,
                              struct tank *that, float vector[2], float dist2) {
  float theta = this->angle + this->turret.current;
  float rpos[2];

  /* If someone's a crater, this is easy (unless we were just killed by the
     other one, in which case we have to check the other direction) */
  if ((this->killer && this->killer != that) || that->killer) {
    return;
  }

  /* Did they collide? */
  if ((!this->killer) && dist2 < TANK_COLLISION_ADJ2) {
    this->killer = that;
    this->cause_death = "collision";

    that->killer = this;
    that->cause_death = "collision";

    return;
  }

  /* No need to check if it's not even firing */
  if (!this->turret.firing) {
    return;
  }

  /* Also no need to check if it's outside cannon range */
  if (dist2 > TANK_CANNON_ADJ2) {
    return;
  }

  /* Did this shoot that?  Rotate point by turret degrees, and if |y| <
     TANK_RADIUS, we have a hit. */
  rpos[0] = vector[0];
  rpos[1] = vector[1];
  rotate_point(-theta, rpos);
  if ((rpos[0] > 0) && (fabsf(rpos[1]) < TANK_RADIUS)) {
    that->killer = this;
    that->cause_death = "shot";
  }
}

static void tanks_sensor_calc(struct tanks_game *game, struct tank *this,
                              struct tank *that, float vector[2], float dist2) {
  int i;

  /* If someone's a crater, this is easy */
  if (this->killer || that->killer) {
    return;
  }

  /* If they're not inside the max sensor, just skip it */
  if (dist2 > TANK_SENSOR_ADJ2) {
    return;
  }

  /* Calculate sensors */
  for (i = 0; i < TANK_MAX_SENSORS; i += 1) {
    float theta;
    float rpos[2];
    float m_r, m_s;

    if (0 == this->sensors[i].range) {
      /* Sensor doesn't exist */
      continue;
    }

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

    /* Rotate their position by theta */
    rpos[0] = vector[0];
    rpos[1] = vector[1];
    rotate_point(-theta, rpos);

    /* Sensor is symmetrical, we can consider only top quadrants */
    rpos[1] = fabsf(rpos[1]);

    /* Compute inverse slopes to tank and of our sensor */
    m_s = 1 / tanf(this->sensors[i].width / 2);
    m_r = rpos[0] / rpos[1];

    /* If our inverse slope is less than theirs, they're inside the arc */
    if (m_r >= m_s) {
      this->sensors[i].triggered = 1;
      continue;
    }

    /* Now check if the edge of the arc intersects the tank.  Do this
       just like with firing. */
    rotate_point(this->sensors[i].width / -2, rpos);
    if ((rpos[0] > 0) && (fabsf(rpos[1]) < TANK_RADIUS)) {
      this->sensors[i].triggered = 1;
    }
  }
}

void compute_vector(struct tanks_game *game, float vector[2], float *dist2,
                    struct tank *this, struct tank *that) {
  int i;

  /* Establish shortest vector from center of this to center of that,
   * taking wrapping into account */
  for (i = 0; i < 2; i += 1) {
    float halfsize = game->size[i] / 2;

    vector[i] = that->position[i] - this->position[i];
    if (vector[i] > halfsize) {
      vector[i] = vector[i] - game->size[i];
    } else if (vector[i] < -halfsize) {
      vector[i] = game->size[i] + vector[i];
    }
  }

  /* Compute distance^2 for range comparisons */
  *dist2 = sq(vector[0]) + sq(vector[1]);
}

void tanks_move_tank(struct tanks_game *game, struct tank *tank) {
  int i;
  float movement;
  float angle;
  int dir = 1;

  /* Rotate the turret */
  {
    float rot_angle; /* Quickest way there */

    /* Constrain rot_angle to between -PI and PI */
    rot_angle = tank->turret.desired - tank->turret.current;
    while (rot_angle < 0) {
      rot_angle += TAU;
    }
    rot_angle = fmodf(PI + rot_angle, TAU) - PI;

    rot_angle = min(TANK_MAX_TURRET_ROT, rot_angle);
    rot_angle = max(-TANK_MAX_TURRET_ROT, rot_angle);
    tank->turret.current = fmodf(tank->turret.current + rot_angle, TAU);
  }

  /* Fakey acceleration */
  for (i = 0; i < 2; i += 1) {
    if (tank->speed.current[i] == tank->speed.desired[i]) {
      /* Do nothing */
    } else if (tank->speed.current[i] < tank->speed.desired[i]) {
      tank->speed.current[i] =
          min(tank->speed.current[i] + TANK_MAX_ACCEL, tank->speed.desired[i]);
    } else {
      tank->speed.current[i] =
          max(tank->speed.current[i] - TANK_MAX_ACCEL, tank->speed.desired[i]);
    }
  }

  /* The simple case */
  if (tank->speed.current[0] == tank->speed.current[1]) {
    movement = tank->speed.current[0] * (TANK_TOP_SPEED / 100.0);
    angle = 0;
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

    /* The first thing Paul's code does is find "friction", which seems
       to be a penalty for having the treads go in opposite directions.
       This probably plays hell with precisely-planned tanks, which I
       find very ha ha. */
    friction = TANK_FRICTION *
               (fabsf(tank->speed.current[0] - tank->speed.current[1]) / 200);
    v[0] = tank->speed.current[0] * (1 - friction) * (TANK_TOP_SPEED / 100.0);
    v[1] = tank->speed.current[1] * (1 - friction) * (TANK_TOP_SPEED / 100.0);

    /* Outside and inside speeds */
    if (fabsf(v[0]) > abs(v[1])) {
      Si = v[1];
      So = v[0];
      dir = 1;
    } else {
      Si = v[0];
      So = v[1];
      dir = -1;
    }

    /* Radius of circle to outside tread (use similar triangles) */
    r = So * (TANK_RADIUS * 2) / (So - Si);

    /* pflarr:

       The fraction of the circle traveled is equal to the speed
       of the outer tread over the circumference of the circle:
           Ft = So/(tau*r)
       The angle traveled is:
           theta = Ft * tau
       This reduces to a simple
           theta = So/r
       We multiply it by dir to adjust for the direction of rotation
    */
    theta = So / r * dir;

    movement = r * tanf(theta);
    angle = theta;
  }

  /* Now move the tank */
  tank->angle = fmodf(tank->angle + angle + TAU, TAU);
  {
    float m[2];

    m[0] = cosf(tank->angle) * movement * dir;
    m[1] = sinf(tank->angle) * movement * dir;

    for (i = 0; i < 2; i += 1) {
      tank->position[i] =
          fmodf(tank->position[i] + m[i] + game->size[i], game->size[i]);
    }
  }
}

void tanks_run_turn(struct tanks_game *game, struct tank *tanks, int ntanks) {
  int i, j;
  float vector[2];
  float dist2; /* distance squared */

  /* It takes (at least) two to tank-o */
  if (ntanks < 2) {
    return;
  }

  /* Charge cannons and reset sensors */
  for (i = 0; i < ntanks; i += 1) {
    if (tanks[i].turret.firing) {
      tanks[i].turret.firing = 0;
      tanks[i].turret.recharge = TANK_CANNON_RECHARGE;
    }
    if (tanks[i].killer)
      continue;
    if (tanks[i].turret.recharge) {
      tanks[i].turret.recharge -= 1;
    }
    for (j = 0; j < TANK_MAX_SENSORS; j += 1) {
      tanks[i].sensors[j].triggered = 0;
    }
  }

  /* Move tanks */
  for (i = 0; i < ntanks; i += 1) {
    if (tanks[i].killer)
      continue;
    tanks_move_tank(game, &(tanks[i]));
  }

  /* Probe sensors */
  for (i = 0; i < ntanks; i += 1) {
    if (tanks[i].killer)
      continue;
    for (j = i + 1; j < ntanks; j += 1) {
      struct tank *this = &tanks[i];
      struct tank *that = &tanks[j];

      compute_vector(game, vector, &dist2, this, that);
      tanks_sensor_calc(game, this, that, vector, dist2);
      vector[0] = -vector[0];
      vector[1] = -vector[1];
      tanks_sensor_calc(game, that, this, vector, dist2);
    }
  }

  /* Run programs */
  for (i = 0; i < ntanks; i += 1) {
    if (tanks[i].killer)
      continue;
    tanks[i].run(&tanks[i], tanks[i].udata);
  }

  /* Fire cannons and check for crashes */
  for (i = 0; i < ntanks; i += 1) {
    if (tanks[i].killer)
      continue;
    for (j = i + 1; j < ntanks; j += 1) {
      struct tank *this = &tanks[i];
      struct tank *that = &tanks[j];

      compute_vector(game, vector, &dist2, this, that);
      tanks_fire_cannon(game, this, that, vector, dist2);
      vector[0] = -vector[0];
      vector[1] = -vector[1];
      tanks_fire_cannon(game, that, this, vector, dist2);
    }
  }
}

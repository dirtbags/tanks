#include <stdio.h>
#include <math.h>
#include "ctanks.h"

#define NTANKS 2

void
test_run(struct tank *tank, void *unused)
{
  tank_set_speed(tank, -60, -61);
  tank_set_turret(tank, tank->turret.desired + PI/15);
  if (tank->sensors[0].triggered) {
    tank_fire(tank);
  }
}

void
sitting_duck(struct tank *tank, void *unused)
{
  tank_set_turret(tank, tank->turret.desired + PI/15);
}

int
main(int argc, char *argv[])
{
  struct tanks_game game;
  struct tank       mytanks[NTANKS];
  int               i;

  game.size[0] = 600;
  game.size[1] = 200;

  printf("[\n");
  printf("[%d, %d, %d],\n",
         (int)game.size[0], (int)game.size[1], TANK_CANNON_RANGE);
  printf("[\n");
  for (i = 0; i < NTANKS; i += 1) {
    if (i == 1) {
      printf(" [\"#888888\",[");
      tank_init(&mytanks[i], sitting_duck, NULL);
    } else {
      int j;

      printf(" [\"#ff4444\",[");
      tank_init(&mytanks[i], test_run, NULL);

      mytanks[i].sensors[0].angle = 0;
      mytanks[i].sensors[0].width = PI/10;
      mytanks[i].sensors[0].range = 50;
      mytanks[i].sensors[0].turret = 1;

      mytanks[i].sensors[1].angle = 0*PI/2;
      mytanks[i].sensors[1].width = PI/3;
      mytanks[i].sensors[1].range = 100;
      mytanks[i].sensors[1].turret = 1;

      mytanks[i].sensors[2].angle = 1*PI/2;
      mytanks[i].sensors[2].width = PI/3;
      mytanks[i].sensors[2].range = 100;
      mytanks[i].sensors[2].turret = 1;

      mytanks[i].sensors[3].angle = 2*PI/2;
      mytanks[i].sensors[3].width = PI/3;
      mytanks[i].sensors[3].range = 100;
      mytanks[i].sensors[3].turret = 1;

      mytanks[i].sensors[4].angle = 3*PI/2;
      mytanks[i].sensors[4].width = PI/3;
      mytanks[i].sensors[4].range = 100;
      mytanks[i].sensors[4].turret = 1;

      mytanks[i].sensors[5].angle = 0;
      mytanks[i].sensors[5].width = PI*1.99;
      mytanks[i].sensors[5].range = 80;
      mytanks[i].sensors[5].turret = 0;

      for (j = 0; j < TANK_MAX_SENSORS; j += 1) {
        struct sensor *s = &(mytanks[i].sensors[j]);

        if (s->range) {
          printf("[%d, %.2f, %.2f, %d],",
                 (int)(s->range),
                 s->angle,
                 s->width,
                 s->turret);
        }
      }
    }
    mytanks[i].position[0] = (game.size[0] / NTANKS) * i + 50;
    mytanks[i].position[1] = 50;
    /* XXX: print sensors */
    printf("]],\n");
  }
  printf("],\n");
  printf("// Rounds\n");
  printf("[\n");

  for (i = 0; i < 100; i += 1) {
    int j;

    tanks_run_turn(&game, mytanks, NTANKS);
    printf("[\n");
    for (j = 0; j < NTANKS; j += 1) {
      struct tank *t = &(mytanks[j]);

      if (t->killer) {
        printf(" 0,\n");
      } else {
        int k;
        int flags   = 0;
        int sensors = 0;

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
        printf(" [%d,%d,%.2f,%.2f,%d,%d],\n",
               (int)(t->position[0]),
               (int)(t->position[1]),
               t->angle,
               t->turret.current,
               flags,
               sensors);
      }
    }
    printf("],\n");
  }
  printf("]]\n");
  return 0;
}

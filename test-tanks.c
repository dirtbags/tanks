#include <stdio.h>
#include <math.h>
#include "ctanks.h"

void
test_run(struct tank *tank, void *unused)
{
  tank_set_speed(tank, 30, 50);
  tank_set_turret(tank, fmod(tank->turret.desired + 0.2, 2*PI));
}

int
main(int argc, char *argv[])
{
  struct tank mytank;
  int i;

  tank_init(&mytank, test_run, NULL);
  printf("hi\n");
  for (i = 0; i < 4; i += 1) {
    printf("%f\n", mytank.turret.desired);
    mytank.run(&mytank, mytank.udata);
  }
  return 0;
}

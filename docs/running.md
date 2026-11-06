---
title: Running Tanks
---

Unfortunately, it's kind of a mess right now.
I know there are a few forks of this code,
and I would love it if someone proposed a merge to clearly illustrate how to run tanks.

When I spin up a new tanks game,
typically I run something like:

    while sleep 60; do ./run-tanks */; done

This assumes all your tanks directories are in the same place as `run-tanks`.



Included programs
-----------------

I tried to stick with the Unix philosophy of one program per task.  I
also tried to avoid doing any string processing in C.  The result is a
hodgepodge of C, Bourne shell, and awk, but at least each piece is
fairly simple to audit.


### run-tanks tank1 tank2 ...

Runs a single round, awards points with rank.awk, and creates a new
summary.html with summary.awk.  This is the main interface that you want
to run from cron or whatever.
      

### forftanks [OPTIONS] tank1 tank2 ...

A program to run a round of tanks and output a JSON description of the
game.  This is what tanks.js uses to render a game graphically.
The object printed contains:

| Key      | Contents |
|----------|----------|
| `seed`   | The random seed used (reuse it to replay the game). |
| `field`  | `[width, height]` of the playing field. |
| `params` | The settings in effect for this game (see below). |
| `rounds` | One entry per turn; each holds `[x, y, angle, turret, flags, sensors]` for every tank. |
| `tanks`  | Final standings: name, color, kills, cause of death, sensors, and so on. |

`forftanks -h` prints a summary of everything below.


#### Options and environment variables

Every setting can be given as a command-line option or an environment
variable.  The command-line option wins, then the environment variable,
then the default.  An empty environment variable counts as unset.  A value
that is not a number, or is outside the allowed range, stops the program
with a message that says which option or variable was at fault.

Options may appear before or after the tank directories.  Use `--` before a
tank directory whose name starts with a dash.

*Game*

| Option | Environment | Default | Allowed | Meaning |
|--------|-------------|---------|---------|---------|
| `-s`, `--seed N` | `SEED` | 0 | any 32-bit integer | Random seed.  0 means use the process ID. |
| `--max-tanks N` | `TANK_MAX_TANKS` | 50 | 2 to 1000 | Most tanks in a game; extra directories are ignored, with a warning. |
| `--rounds N` | `TANK_ROUNDS` | 500 | 1 to 100000 | Most turns in a game. |
| `--spacing N` | `TANK_SPACING` | 150 | 1 to 10000 | Initial distance between tanks (the field is one spacing-by-spacing cell per tank). |

*Tank physics*

| Option | Environment | Default | Allowed | Meaning |
|--------|-------------|---------|---------|---------|
| `--max-sensors N` | `TANK_MAX_SENSORS` | 10 | 1 to 31 | Sensors per tank (`sensor0` to `sensorN-1`). Sensor files past N are ignored, with a warning. |
| `--radius R` | `TANK_RADIUS` | 7.5 | 0.1 to 500 | Tank radius. |
| `--sensor-range N` | `TANK_SENSOR_RANGE` | 100 | 1 to 1000 | Longest sensor range; longer values in sensor files are clamped. |
| `--cannon-recharge N` | `TANK_CANNON_RECHARGE` | 20 | 0 to 1000 | Turns to recharge the cannon. |
| `--cannon-range N` | `TANK_CANNON_RANGE` | 50 | 1 to 1000 | Cannon range (see the note below). |
| `--max-accel N` | `TANK_MAX_ACCEL` | 35 | 1 to 200 | Most a tread speed (-100 to 100) can change per turn. |
| `--max-turret-rot DEG` | `TANK_MAX_TURRET_ROT` | 45 | 0.1 to 360 | Most the turret rotates per turn, in **degrees**. |
| `--top-speed V` | `TANK_TOP_SPEED` | 7 | 0.01 to 1000 | Distance moved per turn with both treads at 100. |
| `--friction F` | `TANK_FRICTION` | 0.75 | 0 to 1 | Speed penalty when the treads run at different speeds. |

*Forf interpreter (applies to each tank)*

| Option | Environment | Default | Allowed | Meaning |
|--------|-------------|---------|---------|---------|
| `--dstack N` | `FORF_DSTACK_SIZE` | 500 | 1 to 16384 | Data stack size, in values. |
| `--cstack N` | `FORF_CSTACK_SIZE` | 500 | 1 to 16384 | Command stack size, in values.  This also limits program length; a program that does not fit is not loaded and the tank is bricked. |
| `--registers N` | `FORF_REGISTERS` | 10 | 0 to 4096 | Number of memory registers.  All start at 0. |

With no overrides, a game is identical to what earlier versions produced for
the same seed and tanks.

**Cannon range.**  The cannon range has traditionally been half the sensor
range (50, with the default sensor range of 100).  It is now a separate
setting that defaults to 50, so changing `--sensor-range` does not change
the cannon range.  If you want the traditional ratio, set both.

**Warnings.**  Some combinations are accepted but strain the movement or
collision model, and `forftanks` prints a `warning:` line on standard error
for each.  The game still runs.  The cases are:

* A high top speed relative to the tank radius.  When the treads differ, a
  tank rotates by `(outer - inner speed) / (2 * radius)` and moves by `r *
  tan(rotation)`.  The warning appears when the worst-case rotation per turn
  reaches about 46 degrees (the stock model is about 18), because turning tanks
  then move noticeably farther than they should.  At 90 degrees `tan()` is
  singular and tanks may move backwards or get NaN positions.  Lowering
  friction, raising top speed, or shrinking the radius all push the rotation up.
* A top speed of at least twice the radius, where tanks closing head-on can
  pass through each other without a collision being detected (collisions are
  only checked at the end of each turn).
* A spacing smaller than twice the radius, where tanks start out colliding.

**The `params` object.**  `params` records the effective settings, so a
saved game describes itself.  Keys: `maxSensors`, `radius`, `sensorRange`,
`cannonRecharge`, `cannonRange`, `maxAccel`, `maxTurretRot` (radians, like
every other angle in the output), `topSpeed`, `friction`, `dataStackSize`,
`commandStackSize`, `registers`, `maxTanks`, `rounds`, and `spacing`.

**Not everything follows the overrides yet.**  The `tanksd` server accepts
uploads only for `sensor0` through `sensor9`, and the web designer and
renderer clamp sensor range to 100, so more sensors or longer ranges are
usable only from the command line for now.  `tanksd` passes its environment
to `forftanks`, so the environment variables above also apply to games it runs.

Examples:

    forftanks examples/*/                       # stock game
    forftanks --seed 42 --rounds 200 examples/*/
    TANK_CANNON_RECHARGE=10 forftanks mytanks/*/
    forftanks --sensor-range 150 --cannon-range 75 --max-sensors 12 a/ b/

If file descriptor 3 is open for writes, it also outputs the results of
the round to fd3.  


### rank.awk

Processes the fd3 output of forftanks to award points and output an
HTML results table.


### summary.awk tank1 tank2

Creates summary.html, linking to all rounds and showing overall
standing.


### designer.cgi

Accepts form input and writes a tank.





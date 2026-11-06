---
title: Running Tanks
---

Tanks is run by two programs.  `tanksd` is a web server written in Go that
does everything needed to host a game: it accepts tanks uploaded by players,
plays a round on a timer, stores the results, and serves the web interface
where players design their tanks and watch the rounds.  `forftanks` is a C
program that plays one game and prints what happened as JSON; `tanksd` runs
it for you, and you can also run it by hand.

This page covers both.  If you used the older shell and awk scripts, see
[Migrating from the old scripts](#migrating-from-the-old-scripts) at the end.


Quick start
-----------

    $ make                    # builds forftanks and tanksd (needs Go)
    $ mkdir rounds            # set up storage
    $ cp -r examples tanks    # install some tanks
    $ mkdir tanks/mytank      # provision your own tank
    $ ./tanksd

Then open <http://localhost:8080/>.  You can work on your tank with the
token `mytank`.


Programs
--------

### tanksd [OPTIONS]

Runs the whole game server from one process.  It must be started from a
directory containing `forftanks`, `www`, `tanks`, and `rounds`, unless you
point it elsewhere with the options below.  `tanksd -h` lists the options.

| Option | Default | Meaning |
|--------|---------|---------|
| `-listen ADDR` | `:8080` | Where to listen for HTTP connections. |
| `-round DURATION` | `1m` | Time between rounds, in Go duration syntax (`30s`, `5m`). |
| `-maxrounds N` | 90 | Number of rounds to keep. |
| `-maxsize BYTES` | 8000 | Largest file a player may upload. |
| `-locknames` | off | Reject uploads of the `name` file, so only the operator can name tanks. |
| `-tanks DIR` | `tanks` | Directory of tank state, one subdirectory per token (read-write). |
| `-rounds DIR` | `rounds` | Where finished rounds are stored (read-write). |
| `-www DIR` | `www` | Web content served at `/` (read-only). |
| `-forftanks PATH` | `./forftanks` | The `forftanks` executable to run each round. |

#### HTTP interface

| Request | What it does |
|---------|--------------|
| `GET /` | The web interface, from the `www` directory. |
| `GET /rounds/` | The stored rounds, plus `index.json`, a JSON array of round file names, oldest first. |
| `PUT /tanks/TOKEN/FILE` | Uploads one file of a tank. |
| `GET /tanks/TOKEN/` | Reads back the files of a tank. |

#### Tanks and tokens

Each subdirectory of the `tanks` directory is a token that can be used to
upload a tank.  The token is the directory name, which must be made of
letters, digits, underscores, and dashes.  Empty directories are fine: a
tank with no program is a brick and sits still.  Players cannot create
tokens, only operators can, by making directories.  The upload only works
if the directory already exists (and is not a symbolic link).

A tank is made of these files, and no others are accepted:

| File | Contents |
|------|----------|
| `program` | The Forf program. |
| `sensor0` to `sensor9` | One sensor each, as four numbers: `range angle width turret`.  Angles are in degrees, and `turret` is 1 if the sensor turns with the turret, else 0. |
| `name` | The tank's name. |
| `color` | A color such as `#ff0088`. |
| `author` | The player's name. |

The designer in the web interface uploads these for you, and can also copy a
shell script that does it.  To do it by hand:

    $ curl -X PUT -d 'Team Rocket' http://localhost:8080/tanks/mytank/name
    $ curl -X PUT -d '50 0 7 1'    http://localhost:8080/tanks/mytank/sensor0
    $ curl -X PUT --data-binary @program http://localhost:8080/tanks/mytank/program

An upload must state its length and be no larger than `-maxsize`, or it is
refused (`411` and `413`).  An unknown file name or token gets `404`, an
invalid token `400`, and an upload of `name` with `-locknames` set gets `403`.

`forftanks` uses the inode of each tank directory to identify the tank
across rounds, so to change somebody's token you should `mv` the directory.
If you `cp` it, or remove it and create a new one, the standings will treat
it as a different tank.

#### How rounds run

`tanksd` plays one round when it starts, and another every `-round`.  Each
time it lists the subdirectories of the `tanks` directory, runs `forftanks`
with all of them, and saves the JSON that `forftanks` prints as a new file in
the `rounds` directory.  The file is named for the time, as sixteen hex
digits of the Unix time followed by `.json`.  It then deletes the oldest
rounds beyond `-maxrounds` and rewrites `index.json`.  You can delete
individual rounds by hand; `tanksd` notices when it runs the next one.

Some things to know:

* At least two tank directories are needed to play a round.  With fewer,
  `tanksd` logs "Not enough tanks for a round" and waits for the next one,
  even though `forftanks` itself can run a single tank.
* No more than 50 tanks play in a round.  With more directories than that,
  the last ones, in name order, are left out of the round.
* If `forftanks` fails, `tanksd` logs the error and does not save a round.
* `tanksd` does not log the standard error of `forftanks` when it succeeds,
  so the warnings described below do not appear in the server log.  Errors
  in a tank's own program are not lost: they are in each tank's `error` and
  `errorPos` fields in the round, and the web interface shows them.  To see
  the warnings, run `forftanks` by hand on the same tank directories.

#### The web interface

The page in `www` loads `rounds/index.json` and the rounds it lists, and
refreshes every twelve seconds.  A selector chooses which round to watch (the
latest by default), with a table of that round's results: each tank's kills,
how it died, who killed it, and any program error.  A second table adds up
every stored round, sorted by kills, and hands out amusing titles such as
"ruthless" and "crashmaster".  There is no points system: the standings are
worked out in the browser from the stored rounds, so they cover the last
`-maxrounds` rounds and nothing older.  `round.html` shows only the most
recent round.

#### Game settings on the server

`tanksd` runs `forftanks` with no options, only the tank directories.  To
change game settings for a server, set the environment variables listed in the
`forftanks` section below when you start `tanksd`, which passes its
environment on:

    $ TANK_CANNON_RECHARGE=10 TANK_ROUNDS=300 ./tanksd

Do not set `SEED` for a server.  It would give every round the same seed, so
the tanks would start every round in the same arrangement.  Left unset,
each round gets a fresh seed.

The Docker image runs `./tanksd` with the examples installed as tanks.
`make image` builds and pushes it; set environment variables with
`docker run -e`.

### forftanks [OPTIONS] tank1 tank2 ...

A program to run a round of tanks and output a JSON description of the
game.  The web interface (`www/player.mjs`) renders it to show a game.
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
| `--max-tanks N` | `TANK_MAX_TANKS` | 50 | 1 to 1000 | Most tanks in a game; extra directories are ignored, with a warning. |
| `--rounds N` | `TANK_ROUNDS` | 500 | 1 to 100000 | Most turns in a game. |
| `--spacing N` | `TANK_SPACING` | 150 | 1 to 10000 | Initial distance between tanks (the field is one spacing-by-spacing cell per tank). |
| `--no-collision-credit` | `TANK_COLLISION_CREDIT` | credit on | flag takes no value; variable is exactly `true` or `false` | Collision scoring rule; see the note below. |

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

**Single-tank games.**  Giving `forftanks` one tank directory is allowed and
useful for smoke-testing a design: the tank's program runs every turn, and
the game lasts for the full number of rounds because nothing can end it.  The
tank always survives (`death` is `(null)`).  With nothing else on the field,
its sensors never trigger and it has nothing to shoot or hit, so test sensor
logic against a second tank, such as the stationary `easy-brick`.

**Collision scoring.**  When two tanks collide, both are destroyed.  By
default each is credited with a kill for destroying the other, as in earlier
versions.  With `--no-collision-credit`, both still die but neither gets a
kill, so a collision can never raise a tank's score.  The environment
variable `TANK_COLLISION_CREDIT` does the same when set to `false`; `true` is
the default rule.  The variable accepts exactly `true` or `false` (lowercase)
and anything else is an error.  If the command-line flag is given it wins and
the variable is not examined.  Either way the `killer` field in the standings
names the tank it collided with, and `death` is `collision`, so viewers and
scripts that look at those fields keep working; only `kills` changes.

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
`commandStackSize`, `registers`, `maxTanks`, `rounds`, `spacing`, and `collisionCredit` (true or false).

**Not everything follows the overrides yet.**  The `tanksd` server accepts
uploads only for `sensor0` through `sensor9`, and the web designer and
viewer clamp sensor range to 100, so more sensors or longer ranges are
usable only from the command line for now.  `tanksd` runs `forftanks`
without any options, but the environment variables above do apply to the
games it runs; see "Game settings on the server" above.

Examples:

    forftanks examples/*/                       # stock game
    forftanks --seed 42 --rounds 200 examples/*/
    TANK_CANNON_RECHARGE=10 forftanks mytanks/*/
    forftanks --sensor-range 150 --cannon-range 75 --max-sensors 12 a/ b/


Migrating from the old scripts
------------------------------

Earlier versions were a hodgepodge of C, shell, and awk, run from cron or a
loop like `while sleep 60; do ./run-tanks */; done`.  The scripts are gone,
and `tanksd` replaces all of them:

| Old | Now |
|-----|-----|
| `run-tanks`, and the cron job or sleep loop that ran it | `tanksd` plays a round every `-round` (one minute by default) as long as it is running. |
| The round results `forftanks` wrote to file descriptor 3 | There is no fd3.  `forftanks` prints one JSON document on standard output, including each tank's kills, killer, cause of death, and program error. |
| `rank.awk` (awarding points, writing a results table in HTML) | No points.  The web interface reads the JSON and sorts tanks by kills, for each round and in total. |
| `summary.awk` (the `summary.html` page linking every round) | `rounds/index.json`, plus the round selector and standings table in the web interface. |
| `designer.cgi` (a CGI program that wrote a tank from form input) | The designer form in the web interface, which uploads each file with `PUT /tanks/TOKEN/FILE`. |
| `round-*.html` and `next-round` files | JSON files in the `rounds` directory. |
| Tanks directories as arguments, all next to `run-tanks` | The `tanks` directory, with one subdirectory per token, which `tanksd` scans every round. |

If you have scripts of your own that read the old fd3 output or the old HTML
pages, they need to read the JSON from `rounds/` instead.  The format is
described under `forftanks` above.

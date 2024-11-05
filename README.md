Tanks
======

Dirtbags Tanks is a game in which you pit your coding abilities
against other hackers.  You write a program for your tank, set it out
on the battlefield, and watch how your program fares against tanks
written by other players.  Dirtbags Tanks is frequently a component of
[Dirtbags Capture The Flag](https://dirtbags.github.io/contest/).


Running it
============

    forftanks TANKDIR [TANKDIR...]
    
`forftanks` will run a round with every tank provided as an argument.
It outputs a JSON object describing the round.


Output fields
------

`seed`
: Seed used by the random number generator.
  You can specify your own seed with the environment variable `SEED.
  If the same seed is used with the same tanks,
  you will get the same output.

`field`
: Dimensions of the play field.

`tanks`
: Description of each tank.

`rounds`
: List of frames for each round.
  Each frame is a list of tank state for each tank.
  Tank state is described below.
  

Tank state
--------

Tank state is packed more tightly than most modern JSON APIs.
Tank state is a tuple of (x position, y position, orientation angle, turret angle, flags, sensor bits).

x position, y position
: Tank's position on the play field.

orientation angle
: Tank's orientation on the play field, in radians.

turret angle
: Turret angle, relative to the tank, in radians.

flags
: Logical or of 1 (firing), 2 (LED on), and 4 (dead)

sensor bits
: Bit field of sensor state (1 = triggered)



Documentation
============

* [Homepage](https://dirtbags.github.io/tanks/)
* [History](docs/history.md)
* [Running](docs/running.md)


Current Maintainer
=====

Neale Pickett <neale@woozle.org>

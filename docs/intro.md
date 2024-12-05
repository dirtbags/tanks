# Tanks Introduction 

<figure>
  <figcaption>"ChashTank" dominates this short round.</figcaption>
  <canvas id="shortround"></canvas>
  <script type="text/javascript">
    start("shortround", shortround);
  </script>
</figure>

Tanks is a game in which you pit your coding abilities against
other hackers.  You write a program for your tank, set it out on
the battlefield, and watch how it fares against other tanks while
running your program.

Each tank has a turret-mounted laser, two treads, up to ten
sensors, and a diagnostic LED.  Sensors are used to detect when
other tanks are inside a given arc.  In the examples on this page,
"triggered" sensors turn black.  Most tanks will take some action
if a sensor is triggered, such as changing speed of the treads,
turning the turret, or firing.

Tanks are programmed in Forf, a stack-based language similar to
PostScript.  Please read the [Forf manual](forf.md) to learn more
about forf, and the [Tanks procedure reference](procs.html) for a
description of Tanks extensions.

## Quick Start for the Impatient

<figure>
  <figcaption>"Crashmaster" pwns the lame default tank provided in this
  section.</figcaption>
  <canvas id="default"></canvas>
  <script type="text/javascript">
    start("default", default_);
  </script>
</figure>

To get started, head over to the designer for your game,
and use the following example tank.  This tank will
move around, turn the turret, and fire if there's something in
front of it.

    Sensor 0: 50 0 7 ☑
    Sensor 1: 30 0 90 ☐
    
    get-turret 12 + set-turret!         ( Rotate turret )
    37 40 set-speed!                    ( Go in circles )
    0 sensor? { fire! } if              ( Fire if turret sensor triggered )
    1 sensor? { -50 50 set-speed! } if  ( Turn if collision sensor triggered )

Obviously, this tank could be improved.  
Watch other tanks in your game to get ideas about how to improve yours.
Don't forget the [Forf manual](forf.md) and the
[Tank procedure reference](procs.html).

## Tank Specifications

<figure>
  <figcaption>"Ant Lion" nails "Rabbit With Gun".</figcaption>
  <canvas id="antlion"></canvas>
  <script type="text/javascript">
    start("antlion", antlion);
  </script>
</figure>

Your PF-255 autonomous tank is built to the exacting
specifications sent to our factory in New Khavistan.  All
distances are in meters, angles in degrees.


Tank size
: The targettable area of the tank—the part which can be hit by a cannon—is a circle about 7½ meters in radius.

Speed
: Each tread can have a speed between -100 and 100.  This is in
  percentage of total speed for that tread, where total speed is
  roughly 7 meters per turn.

Sensors
: Each sensor has a maximum range of 100 meters.  Of course, you
  don't have to use the full range.  Sensors may be attached to
  the turret (they turn along with the turret), or left fixed to
  the tank.

Turret
: Turret angle can be set between -359° and 359°, with 0° directly
  in front of the tank.  Be aware that it takes time for the
  turret to swing around: the turret can swing about 45° per turn.

Cannon range and recharging
: When the cannon is fired, it obliterates everything for 50
  meters in front of it.  It takes around 20 turns for your cannon
  to recharge after it's been fired, so only shoot when you feel
  like you're going to hit something.

Good luck blowing everybody up!

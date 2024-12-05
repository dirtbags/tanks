<!DOCTYPE html>
<html>
  <head>
    <title>Tanks Introduction</title>
    <meta charset="utf-8">
    <link rel="stylesheet" href="style.css" type="text/css">
    <script type="application/javascript" src="tanks.js"></script>
    <script type="application/javascript" src="figures.js"></script>
  </head>
  <body>
    <h1>Tanks Introduction</h1>

    <table class="figure">
      <caption>"ChashTank" dominates this short round.</caption>
      <tr>
        <td>
          <canvas id="shortround"></canvas>
          <script type="text/javascript">
            start("shortround", shortround);
          </script>
        </td>
      </tr>
    </table>

    <p>
      Tanks is a game in which you pit your coding abilities against
      other hackers.  You write a program for your tank, set it out on
      the battlefield, and watch how it fares against other tanks while
      running your program.
    </p>

    <p>
      Each tank has a turret-mounted laser, two treads, up to ten
      sensors, and a diagnostic LED.  Sensors are used to detect when
      other tanks are inside a given arc.  In the examples on this page,
      "triggered" sensors turn black.  Most tanks will take some action
      if a sensor is triggered, such as changing speed of the treads,
      turning the turret, or firing.
    </p>

    <p>
      Tanks are programmed in Forf, a stack-based language similar to
      PostScript.  Please read the <a href="forf.html">Forf manual</a>
      to learn more about forf, and the <a href="procs.html">Tanks
      procedure reference</a> for a description of Tanks extensions.
    </p>

    <h2>Quick Start for the Impatient</h2>

    <table class="figure left">
      <caption>"Crashmaster" pwns the lame default tank provided in this
      section.</caption>
      <tr>
        <td>
          <canvas id="default"></canvas>
          <script type="text/javascript">
            start("default", default_);
          </script>
        </td>
      </tr>
    </table>

    <p>
      To get started, head over to the <a href="debugger.html">Tank
      Debugger</a> and enter the following example tank.  This tank will
      move around, turn the turret, and fire if there's something in
      front of it.
    </p>

    <pre style="clear: both;">
Sensor 0: 50 0 7 ☑
Sensor 1: 30 0 90 ☐

get-turret 12 + set-turret!         ( Rotate turret )
37 40 set-speed!                    ( Go in circles )
0 sensor? { fire! } if              ( Fire if turret sensor triggered )
1 sensor? { -50 50 set-speed! } if  ( Turn if collision sensor triggered )
    </pre>

    <p>
      Obviously, this tank could be improved.  Studying the examples on
      this page should give you ideas about how to make a better tank.
      Don't forget the <a href="forf.html">Forf manual</a> and the
      <a href="procs.html">Tank procedure reference</a>.
    </p>

    <h2>Tank Specifications</h2>
    
    <table class="figure">
      <caption>"Ant Lion" nails "Rabbit With Gun".</caption>
      <tr>
        <td>
          <canvas id="antlion"></canvas>
          <script type="text/javascript">
            start("antlion", antlion);
          </script>
        </td>
      </tr>
    </table>

    <p>
      Your PF-255 autonomous tank is built to the exacting
      specifications sent to our factory in New Khavistan.  All
      distances are in meters, angles in degrees.
    </p>

    <dl style="clear: both;">
      <dt>Tank size</dt>
      <dd>
        The targettable area of the tank—the part which can be hit by a
        cannon—is a circle about 7½ meters in radius.
      </dd>

      <dt>Speed</dt>
      <dd>
        Each tread can have a speed between -100 and 100.  This is in
        percentage of total speed for that tread, where total speed is
        roughly 7 meters per turn.
      </dd>

      <dt>Sensors</dt>
      <dd>
        Each sensor has a maximum range of 100 meters.  Of course, you
        don't have to use the full range.  Sensors may be attached to
        the turret (they turn along with the turret), or left fixed to
        the tank.
      </dd>

      <dt>Turret</dt>
      <dd>
        Turret angle can be set between -359° and 359°, with 0° directly
        in front of the tank.  Be aware that it takes time for the
        turret to swing around: the turret can swing about 45° per turn.
      </dd>

      <dt>Cannon range and recharging</dt>
      <dd>
        When the cannon is fired, it obliterates everything for 50
        meters in front of it.  It takes around 20 turns for your cannon
        to recharge after it's been fired, so only shoot when you feel
        like you're going to hit something.
      </dd>
    </dl>

    Good luck blowing everybody up!
include(nav.html.inc)
</html>

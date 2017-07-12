---
title: Tanks
scripts:
 - assets/js/tanks.js
 - assets/js/figures.js
---

<table class="figure">
  <caption>"Ant Lion" nails "Rabbit With Gun".</caption>
  <tr><td>
    <canvas id="antlion"></canvas>
    <script type="application/javascript">
      start("antlion", antlion);
    </script>
  </td></tr>
</table>

Dirtbags Tanks is a game in which you pit your coding abilities
against other hackers.  You write a program for your tank, set it out
on the battlefield, and watch how your program fares against tanks
written by other players.  Dirtbags Tanks is frequently a component of
[Dirtbags Capture The Flag](/ctf/).

Each tank has a turret-mounted laser, two treads, up to ten sensors,
and a diagnostic LED.  Sensors are used to detect when other tanks are
inside a given arc.  In the examples on this page, "triggered" sensors
turn black.  Most tanks will take some action if a sensor is
triggered, such as changing speed of the treads, turning the turret,
or firing.

Tanks are programmed in Forf, a stack-based language similar to
PostScript.


Other Versions
--------------

Everybody loves tanks!
If you've made a port or done something fun with it,
please let me know and I'll link it here!

* [Class Lab](https://github.com/Arctem/nmt_python_labs/tree/master/labs/project) at New Mexico Tech, by Russell White


Author
------

Neale Pickett <neale@woozle.org>

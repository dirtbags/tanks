<!DOCTYPE html>
<html>
  <head>
    <title>Tanks Procedure Reference</title>
    <meta charset="utf-8">
    <link rel="stylesheet" href="style.css" type="text/css">
  </head>
  <body>
    <h1>Tanks Procedure Reference</h1>

    <p>
      Each tank's program is run once per turn.  The data and command
      stacks are reset at the beginning of each turn, but memory is not,
      so you can carry data over in memory registers if you want.  See
      the <a href="forf.html">Forf manual</a> for more information about
      the base language.
    </p>

    <p>
      For tank specifications (sensor range, maximum speeds, etc.), see
      the <a href="intro.html">introduction</a>.
    </p>


    <h2>Limits</h2>

    <p>
      Forf Tanks has a data stack size of 200, and a command stack size
      of 500.  This means your program cannot have more than 200 data
      items, or 500 instructions, including 2 instructions for each
      substack.
    </p>

    <p>
      Forf Tanks provides 10 memory registers (0-9) which persist across
      invocations of your tank's program.
    </p>


    <h2>Additional Procedures</h2>

    <dl>
      <dt>fire-ready?</dt>
      <dd>Returns 1 if the tank can fire, 0 if not.</dd>

      <dt>fire!</dt>
      <dd>Fires the cannon.</dd>

      <dt>l r set-speed!</dt>
      <dd>Sets the speed of the left and right treads (range: -100 to
      100).</dd>

      <dt>get-turret</dt>
      <dd>Returns the current angle of the turret.</dd>

      <dt>a set-turret!</dt>
      <dd>Set the turret to a degrees.</dd>

      <dt>n sensor?</dt>
      <dd>Returns 1 if sensor n is triggered, 0 if not.</dd>

      <dt>s set-led!</dt>
      <dd>Turns off the LED if s is 0, on for any other value.</dd>

      <dt>n random</dt>
      <dd>Returns a random number in the range [0, n).  That is, between
        0 and n-1, inclusive.</dd>
    </dl>

include(nav.html.inc)
  </body>
</html>

    

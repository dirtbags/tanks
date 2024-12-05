# Tanks Procedure Reference

Each tank's program is run once per turn.  The data and command
stacks are reset at the beginning of each turn, but memory is not,
so you can carry data over in memory registers if you want.  See
the [Forf manual](forf.md) for more information about
the base language.

For tank specifications (sensor range, maximum speeds, etc.), see
the [introduction](intro.html).

## Limits

Forf Tanks has a data stack size of 200, and a command stack size
of 500.  This means your program cannot have more than 200 data
items, or 500 instructions, including 2 instructions for each
substack.

Forf Tanks provides 10 memory registers (0-9) which persist across
invocations of your tank's program.


## Additional Procedures

fire-ready?
: Returns 1 if the tank can fire, 0 if not.

fire!
: Fires the cannon.

l r set-speed!
:  Sets the speed of the left and right treads (range: -100 to 100).

get-turret
: Returns the current angle of the turret, in degrees.

a set-turret!
: Begin moving the turret to a degrees.

n sensor?
: Returns 1 if sensor n is triggered, 0 if not.

s set-led!
: Turns off the LED if s is 0, on for any other value.

n random
: Returns a random number in the range [0, n).  That is, between 0 and n-1, inclusive.

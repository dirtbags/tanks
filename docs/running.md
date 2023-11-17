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
      

### forftanks tank1 tank2 ...

A program to run a round of tanks and output a JSON description of the
game.  This is what tanks.js uses to render a game graphically.
The object printed contains:

    [[game-width, game-height],
     [[tank1-color,
      [[sensor1range, sensor1angle, sensor1width, sensor1turret],
       ...]],
      ...],
     [[
      [tank1x, tank1y, tank1angle, tank1sensangle,
       tank1flags, tank1sensors],
      ...],
     ...]]

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



Problems
========

Nothing uploads
---------------

The user running the web server has to be able to write to the directory serving the CGI.
Yes, I realize this is a terrible design. Please feel encouraged to submit a merge request.


Only sensors upload
-------------------

The user running the web server has to be able to write to the directory serving the CGI.
Not just the directory with the tank token.
Yes, I realize this is a terrible design. Please feel encouraged to submit a merge request.


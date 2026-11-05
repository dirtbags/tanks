Tanks
======

Dirtbags Tanks is a game in which you pit your coding abilities
against other hackers.  You write a program for your tank, set it out
on the battlefield, and watch how your program fares against tanks
written by other players.


Running it
========

    $ make                    # build source code
    $ mkdir rounds            # set up storage
    $ cp -r examples tanks    # install some tanks
    $ mkdir tanks/mytank      # provision your own tank
    $ ./tanksd

At this point you can connect to http://localhost:8080/ and watch all the
built-in tanks destroy your motionless tank.
You can work on your tank with the token `mytank`.


Administering a server
==================

The `tanks` directory
----------------
The `tanks` directory has tank definitions.
Each subdirectory is a token that can be used to upload a tank.
Empty subdirectories are okay.

If you were running a 9-person event,
and had a Bash shell,
you could run something like this to set up `tanks` subdirectories:

    $ cd tanks
    $ for i in $(seq 9); do mkdir $(printf "%04x$04x" $RANDOM $RANDOM); done

`forftanks` uses the inode of each tank subdirectory
to uniquely identify the tank.
So if you want to change somebody's token,
you should `mv` the subdirectory.
If you `cp` it, or remove it and create a new one,
the scoreboard won't know it's the same tank.


Restricting tank names
----------------
By default, anyone with a token can set their tank's name
by uploading the `name` file.
To prevent that, start the server with `-locknames`:

    $ ./tanksd -locknames

Uploads to `name` are then rejected with `403 Forbidden`,
and all other files upload as usual.
Only someone with shell access can set a name,
by writing the file directly:

    $ echo 'Team Rocket' > tanks/<token>/name

Existing `name` files are left alone, so you can turn the flag on at any time.


The `rounds` directory
-----------------

The `rounds` directory has internal state,
containing an `index.json` file,
and a bunch of game files.
You can delete individual games if you need to, for some reason;
tanksd will notice your change when it runs the next game.


More Documentation
============

* [Homepage](https://dirtbags.net/tanks/)
* [History](docs/history.md)
* [Thanks](docs/thanks.md)


Current Maintainer
=====

Neale Pickett <neale@woozle.org>

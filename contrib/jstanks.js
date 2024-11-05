"use strict";
/*
 * jstanks: A forf/tanks implementation in javascript, based on the C version.
 * Copyright (C) 2014 Alyssa Milburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Disclaimer: I warned you all that I don't know javascript.
 *
 * TODO:
 *  - memory functions
 *  - peer at the arithmetic FIXMEs
 *  - overflow/underflow checks
 *  - do the substacks properly, not as a parse-time hack
 *  - type checking
 *    (one of those two should stop '{ dup 1 exch if } dup 1 exch if' from working)
 *
 *  - tests
 *  - catch/show exceptions
 *  - save/load state from cookie
 *  - stack visualisation
 *  - display live desired/current speed, turret angle, etc
 *  - show live sensor state in the Sensors box too?
 *  - scoreboard
 *  - apply simultaneous fire fixes and/or other upstream changes
 */

var TAU = 2 * Math.PI;

var mod = function(a, b) { return a % b; };
var sq = function(a) { return a * a; };

var rad2deg = function(r) { return Math.floor(360*(r)/TAU); };
var deg2rad = function(r) { return (r*TAU)/360; };

/* Some in-game constants */
var TANK_MAX_SENSORS = 10;
var TANK_RADIUS = 7.5;
var TANK_SENSOR_RANGE = 100;
var TANK_CANNON_RECHARGE = 20; /* Turns to recharge cannon */
var TANK_CANNON_RANGE = (TANK_SENSOR_RANGE / 2);
var TANK_MAX_ACCEL = 35;
var TANK_MAX_TURRET_ROT = (TAU/8);
var TANK_TOP_SPEED = 7;
var TANK_FRICTION = 0.75;

/* (tank radius + tank radius)^2 */
var TANK_COLLISION_ADJ2 = ((TANK_RADIUS + TANK_RADIUS) * (TANK_RADIUS + TANK_RADIUS));

/* (Sensor range + tank radius)^2
 * If the distance^2 to the center of a tank <= TANK_SENSOR_ADJ2,
 * that tank is within sensor range. */
var TANK_SENSOR_ADJ2 = ((TANK_SENSOR_RANGE + TANK_RADIUS) * (TANK_SENSOR_RANGE + TANK_RADIUS));

var TANK_CANNON_ADJ2 = ((TANK_CANNON_RANGE + TANK_RADIUS) * (TANK_CANNON_RANGE + TANK_RADIUS));

// initial game grid spacing
var SPACING = 150;

var MEMORY_SIZE = 10;

var Forf = function() {
    this.mem = new Object();
    this.builtins = new Object();

    this.builtins["debug!"] = function(myforf) { document.getElementById('debug').innerHTML = myforf.popData(); };
    var unfunc = function(func) {
        return function(myforf) {
            var a = myforf.popData();
            myforf.datastack.push(~~func(a)); // truncate, FIXME
        };
    };
    var binfunc = function(func) {
        return function(myforf) {
            var a = myforf.popData();
            var b = myforf.popData();
            myforf.datastack.push(~~func(b,a)); // truncate?, FIXME
        };
    };
    this.builtins["~"] = unfunc(function(a) { return ~a; });
    this.builtins["!"] = unfunc(function(a) { return !a; });
    this.builtins["+"] = binfunc(function(a, b) { return a+b; });
    this.builtins["-"] = binfunc(function(a, b) { return a-b; });
    this.builtins["/"] = binfunc(function(a, b) {
        if (b === 0) { throw "division by zero"; }
        return a/b;
    });
    this.builtins["%"] = binfunc(function(a, b) {
        if (b === 0) { throw "division by zero"; }
        return mod(a,b);
    });
    this.builtins["*"] = binfunc(function(a, b) { return a*b; });
    this.builtins["&"] = binfunc(function(a, b) { return a&b; });
    this.builtins["|"] = binfunc(function(a, b) { return a|b; });
    this.builtins["^"] = binfunc(function(a, b) { return a^b; });
    this.builtins["<<"] = binfunc(function(a, b) { return a<<b; });
    this.builtins[">>"] = binfunc(function(a, b) { return a>>b; });
    this.builtins[">"] = binfunc(function(a, b) { return a>b; });
    this.builtins[">="] = binfunc(function(a, b) { return a>=b; });
    this.builtins["<"] = binfunc(function(a, b) { return a<b; });
    this.builtins["<="] = binfunc(function(a, b) { return a<=b; });
    this.builtins["="] = binfunc(function(a, b) { return a===b; });
    this.builtins["<>"] = binfunc(function(a, b) { return a!==b; });
    this.builtins["abs"] = unfunc(function(a) { return Math.abs(a); });
    // FIXME: the three following functions can only manipulate numbers in cforf
    this.builtins["dup"] = function(myforf) {
        var val = myforf.popData();
        myforf.datastack.push(val);
        myforf.datastack.push(val);
    };
    this.builtins["pop"] = function(myforf) {
        myforf.popData();
    };
    this.builtins["exch"] = function(myforf) {
       var a = myforf.popData();
       var b = myforf.popData();
       myforf.datastack.push(a);
       myforf.datastack.push(b);
    };
    this.builtins["if"] = function(myforf) {
       var ifclause = myforf.popData();
       var cond = myforf.popData();
       if (cond) {
            // TODO: make sure ifclause is a list
            for (var i = 0; i < ifclause.length; i++) {
                myforf.cmdstack.push(ifclause[i]);
            }
        }
    };
    this.builtins["ifelse"] = function(myforf) {
        var elseclause = myforf.popData();
        var ifclause = myforf.popData();
        var cond = myforf.popData();
        if (!cond) {
            ifclause = elseclause;
        }
        // TODO: make sure ifclause is a list
        for (var i = 0; i < ifclause.length; i++) {
            myforf.cmdstack.push(ifclause[i]);
        }
    };
    this.builtins["mset"] = function(myforf) {
        var pos = myforf.popData();
        var a = myforf.popData();
        if (pos < 0 || pos >= MEMORY_SIZE) {
            throw "invalid memory location";
        }
        myforf.mem[pos] = a;
    };
    this.builtins["mget"] = function(myforf) {
        var pos = myforf.popData();
        if (pos < 0 || pos >= MEMORY_SIZE) {
            throw "invalid memory location";
        }
        myforf.datastack.push(myforf.mem[pos]);
    };
};

Forf.prototype.popData = function() {
    if (this.datastack.length === 0) {
        throw "tried to pop from empty stack";
    }
    return this.datastack.pop();
};

Forf.prototype.init = function(code) {
    this.code = code;
};

Forf.prototype.parse = function() {
    this.cmdstack = [];

    // 'parse' the input
    this.code = this.code.replace(/\([^)]*\)/g, "");
    var splitCode = this.code.split(/([{}])/).join(" ");
    var tokens = splitCode.split(/\s+/).filter(Boolean); // filter to deal with newlines etc
    // FIXME: this is a hack right now because ugh stacks
    var parseTokensAt = function(i, stack) {
        var val = tokens[i];
        if (val === "{") {
            var dststack = [];
            i = i + 1;
            while (i < tokens.length) {
                if (tokens[i] === "}") {
                    break;
                }
                i = parseTokensAt(i, dststack);
            }
            stack.push(dststack.reverse());
        } else {
            // replace numbers with actual numbers
            var n = parseInt(val);
            if (String(n) === val) {
                stack.push(n);
            } else {
                stack.push(val);
            }
        }
        return i + 1;
    };
    var i = 0;
    while (i < tokens.length) {
        i = parseTokensAt(i, this.cmdstack);
    }

    // The first thing we read should be the first thing we do.
    this.cmdstack = this.cmdstack.reverse();
};

Forf.prototype.run = function() {
    this.datastack = [];

    var running = true;
    while (running && this.cmdstack.length) {
        var val = this.cmdstack.pop();
        if (typeof(val) == "string") {
            var func = this.builtins[val];
            if (val in this.builtins) {
                func(this);
            } else {
                throw "no such function '" + val + "'";
            }
        } else {
            this.datastack.push(val);
        }
    }
};

var gameSize = [0, 0];

var ForfTank = function() {
    // http://www.paulirish.com/2009/random-hex-color-code-snippets/
    this.color = '#'+(4473924+Math.floor(Math.random()*12303291)).toString(16);

    this.sensors = [];
    this.position = [0, 0];
    this.angle = 0;
    this.speed = new Object;
    this.speed.desired = [0, 0];
    this.speed.current = [0, 0];
    this.turret = new Object;
    this.turret.current = 0;
    this.turret.desired = 0;
    this.turret.firing = 0;
    this.turret.recharge = 0;
    this.led = 0;
    this.killer = null;
    this.cause_death = "";

    this.builtins["fire-ready?"] = function(myforf) {
        myforf.datastack.push(myforf.fireReady());
    };
    this.builtins["fire!"] = function(myforf) {
        myforf.fire();
    };
    this.builtins["set-speed!"] = function(myforf) {
        var right = myforf.popData();
        var left = myforf.popData();
        myforf.setSpeed(left, right);
    };
    this.builtins["set-turret!"] = function(myforf) {
        var angle = myforf.popData();
        myforf.setTurret(deg2rad(angle));
    };
    this.builtins["get-turret"] = function(myforf) {
        var angle = myforf.getTurret();
        myforf.datastack.push(rad2deg(angle));
    };
    this.builtins["sensor?"] = function(myforf) {
        var sensor_num = myforf.popData();
        myforf.datastack.push(myforf.getSensor(sensor_num));
    };
    this.builtins["set-led!"] = function(myforf) {
        var active = myforf.popData();
        myforf.setLed(active);
    };
    this.builtins["random"] = function(myforf) {
        var max = myforf.popData();
        if (max < 1) {
             myforf.datastack.push(0);
             return;
        }
        myforf.datastack.push(Math.floor(Math.random() * max));
    };
};
ForfTank.prototype = new Forf();
ForfTank.prototype.constructor = ForfTank;

ForfTank.prototype.addSensor = function(range, angle, width, turret) {
    var sensor = new Object();
    sensor.range = range;
    sensor.angle = deg2rad(angle);
    sensor.width = deg2rad(width);
    sensor.turret = turret;
    this.sensors.push(sensor);
};

ForfTank.prototype.fireReady = function() {
    return !this.turret.recharge;
};

ForfTank.prototype.fire = function() {
    this.turret.firing = this.fireReady();
};

ForfTank.prototype.setSpeed = function(left, right) {
    this.speed.desired[0] = Math.min(Math.max(left, -100), 100);
    this.speed.desired[1] = Math.min(Math.max(right, -100), 100);
};

ForfTank.prototype.getTurret = function() {
    return this.turret.current;
};

ForfTank.prototype.setTurret = function(angle) {
    this.turret.desired = mod(angle, TAU);
};

ForfTank.prototype.getSensor = function(sensor_num) {
    if ((sensor_num < 0) || (sensor_num >= this.sensors.length)) {
        return 0;
    } else {
        return this.sensors[sensor_num].triggered;
    }
};

ForfTank.prototype.setLed = function(active) {
    this.led = active;
};

ForfTank.prototype.move = function() {
    var dir = 1;
    var movement;
    var angle;

    /* Rotate the turret */
    var rot_angle;              /* Quickest way there */

    /* Constrain rot_angle to between -PI and PI */
    rot_angle = this.turret.desired - this.turret.current;
    while (rot_angle < 0) {
        rot_angle += TAU;
    }
    rot_angle = mod(Math.PI + rot_angle, TAU) - Math.PI;

    rot_angle = Math.min(TANK_MAX_TURRET_ROT, rot_angle);
    rot_angle = Math.max(-TANK_MAX_TURRET_ROT, rot_angle);
    this.turret.current = mod(this.turret.current + rot_angle, TAU);

    /* Fakey acceleration */
    for (var i = 0; i < 2; i++) {
        if (this.speed.current[i] === this.speed.desired[i]) {
            /* Do nothing */
        } else if (this.speed.current[i] < this.speed.desired[i]) {
            this.speed.current[i] = Math.min(this.speed.current[i] + TANK_MAX_ACCEL,
                                             this.speed.desired[i]);
        } else {
            this.speed.current[i] = Math.max(this.speed.current[i] - TANK_MAX_ACCEL,
                                             this.speed.desired[i]);
        }
    }

    /* The simple case */
    if (this.speed.current[0] === this.speed.current[1]) {
        movement = this.speed.current[0] * (TANK_TOP_SPEED / 100.0);
        angle = 0;
    } else {
        /* pflarr's original comment:
         *
         *   The tank drives around in a circle of radius r, which is some
         *   offset on a line perpendicular to the tank.  The distance it
         *   travels around the circle varies with the speed of each tread,
         *   and is such that each side of the tank moves an equal angle
         *   around the circle.
         *
         * Sounds good to me.   pflarr's calculations here are fantastico,
         * there's nothing whatsoever to change. */

        /* The first thing Paul's code does is find "friction", which seems
           to be a penalty for having the treads go in opposite directions.
           This probably plays hell with precisely-planned tanks, which I
           find very ha ha. */
        var friction = TANK_FRICTION * (Math.abs(this.speed.current[0] - this.speed.current[1]) / 200);
        var v = [0, 0];
        v[0] = this.speed.current[0] * (1 - friction) * (TANK_TOP_SPEED / 100.0);
        v[1] = this.speed.current[1] * (1 - friction) * (TANK_TOP_SPEED / 100.0);

        var Si;
        var So;
        /* Outside and inside speeds */
        if (Math.abs(v[0]) > Math.abs(v[1])) {
            Si = v[1];
            So = v[0];
            dir = 1;
        } else {
            Si = v[0];
            So = v[1];
            dir = -1;
        }

        /* Radius of circle to outside tread (use similar triangles) */
        var r = So * (TANK_RADIUS * 2) / (So - Si);

        /* pflarr:

           The fraction of the circle traveled is equal to the speed
           of the outer tread over the circumference of the circle:
               Ft = So/(tau*r)
           The angle traveled is:
               theta = Ft * tau
           This reduces to a simple
               theta = So/r
           We multiply it by dir to adjust for the direction of rotation
        */
        var theta = So/r * dir;

        movement = r * Math.tan(theta);
        angle = theta;
    }

    /* Now move the tank */
    this.angle = mod(this.angle + angle + TAU, TAU);
    var m = [0, 0];

    m[0] = Math.cos(this.angle) * movement * dir;
    m[1] = Math.sin(this.angle) * movement * dir;

    for (var i = 0; i < 2; i++) {
        this.position[i] = mod(this.position[i] + m[i] + gameSize[i], gameSize[i]);
    }
};

var ftanks = [];
var tanks = [];
var interval = null;

var initTanks = function(tanks) {
    var ntanks = tanks.length;

    // Calculate the size of the game board.
    var x = 1;
    while (x * x < ntanks) {
        x = x + 1;
    }
    var y = Math.floor(ntanks / x);
    if (ntanks % x) {
        y = y + 1;
    }
    gameSize[0] = x * SPACING;
    gameSize[1] = y * SPACING;

    // Shuffle the order we place things on the game board.
    var order = [];
    for (var i = 0; i < ntanks; i++) {
        order.push(i);
    }
    for (var i = 0; i < ntanks; i++) {
        var j = Math.floor(Math.random() * ntanks);
        var n = order[j];
        order[j] = order[i];
        order[i] = n;
    }

    // Position tanks.
    x = SPACING / 2;
    y = SPACING / 2;
    for (var i = 0; i < ntanks; i++) {
        tanks[order[i]].position[0] = x;
        tanks[order[i]].position[1] = y;
        // TODO: Move to constructor?
        tanks[order[i]].angle = Math.random() * TAU;
        tanks[order[i]].turret.current = Math.random() * TAU;
        tanks[order[i]].turret.desired = tanks[order[i]].turret.current;

        x = x + SPACING;
        if (x > gameSize[0]) {
            x = x % gameSize[0];
            y = y + SPACING;
        }
    }
};

var rotate_point = function(angle, point) {
    var cos_ = Math.cos(angle);
    var sin_ = Math.sin(angle);

    var newp = [0, 0];
    newp[0] = point[0]*cos_ - point[1]*sin_;
    newp[1] = point[0]*sin_ + point[1]*cos_;

    point[0] = newp[0];
    point[1] = newp[1];
};

ForfTank.prototype.fireCannon = function(that, vector, dist2) {
    /* If someone's a crater, this is easy */
    if ((this.killer && this.killer !== that) || that.killer) {
        return;
    }

    /* Did they collide? */
    if ((!this.killer) && dist2 < TANK_COLLISION_ADJ2) {
        this.killer = that;
        this.cause_death = "collision";

        that.killer = this;
        that.cause_death = "collision";

        return;
    }

    /* No need to check if it's not even firing */
    if (! this.turret.firing) {
        return;
    }

    /* Also no need to check if it's outside cannon range */
    if (dist2 > TANK_CANNON_ADJ2) {
        return;
    }

    var theta = this.angle + this.turret.current;

    /* Did this shoot that?  Rotate point by turret degrees, and if |y| <
       TANK_RADIUS, we have a hit. */
    var rpos = [vector[0], vector[1]];
    rotate_point(-theta, rpos);
    if ((rpos[0] > 0) && (Math.abs(rpos[1]) < TANK_RADIUS)) {
        that.killer = this;
        that.cause_death = "shot";
    }
};

ForfTank.prototype.sensorCalc = function(that, vector, dist2) {
    /* If someone's a crater, this is easy */
    if (this.killer || that.killer) {
        return;
    }

    /* If they're not inside the max sensor, just skip it */
    if (dist2 > TANK_SENSOR_ADJ2) {
        return;
    }

    /* Calculate sensors */
    for (var i = 0; i < this.sensors.length; i++) {
        if (0 === this.sensors[i].range) {
            /* Sensor doesn't exist */
            continue;
        }

        /* No need to re-check this sensor if it's already firing */
        if (this.sensors[i].triggered) {
            continue;
        }

        /* If the tank is out of range, don't bother */
        if (dist2 > sq(this.sensors[i].range + TANK_RADIUS)) {
            continue;
        }

        /* What is the angle of our sensor? */
        var theta = this.angle + this.sensors[i].angle;
        if (this.sensors[i].turret) {
            theta += this.turret.current;
        }

        /* Rotate their position by theta */
        var rpos = [vector[0], vector[1]];
        rotate_point(-theta, rpos);

        /* Sensor is symmetrical, we can consider only top quadrants */
        rpos[1] = Math.abs(rpos[1]);

        /* Compute inverse slopes to tank and of our sensor */
        var m_s = 1 / Math.tan(this.sensors[i].width / 2);
        var m_r = rpos[0] / rpos[1];

        /* If our inverse slope is less than theirs, they're inside the arc */
        if (m_r >= m_s) {
            this.sensors[i].triggered = 1;
            continue;
        }

        /* Now check if the edge of the arc intersects the tank.  Do this
           just like with firing. */
        rotate_point(this.sensors[i].width / -2, rpos);
        if ((rpos[0] > 0) && (Math.abs(rpos[1]) < TANK_RADIUS)) {
            this.sensors[i].triggered = 1;
        }
    }
};

var compute_vector = function(vector, _this, that) {
    /* Establish shortest vector from center of this to center of that,
     * taking wrapping into account */
    for (var i = 0; i < 2; i += 1) {
        var halfsize = gameSize[i] / 2;

        vector[i] = that.position[i] - _this.position[i];
        if (vector[i] > halfsize) {
            vector[i] = vector[i] - gameSize[i];
        } else if (vector[i] < -halfsize) {
            vector[i] = gameSize[i] + vector[i];
        }
    }

    /* Compute distance^2 for range comparisons */
    return sq(vector[0]) + sq(vector[1]);
};

var updateTanks = function(tanks) {
    /* Charge cannons and reset sensors */
    for (var i = 0; i < tanks.length; i++) {
        if (tanks[i].turret.firing) {
            tanks[i].turret.firing = 0;
            tanks[i].turret.recharge = TANK_CANNON_RECHARGE;
        }
        if (tanks[i].killer) {
            continue;
        }
        if (tanks[i].turret.recharge) {
            tanks[i].turret.recharge -= 1;
        }
        for (var j = 0; j < tanks[i].sensors.length; j += 1) {
          tanks[i].sensors[j].triggered = 0;
       }
    }

    /* Move tanks */
    for (var i = 0; i < tanks.length; i++) {
        if (tanks[i].killer) {
            continue;
        }
        tanks[i].move();
    }

    /* Probe sensors */
    for (var i = 0; i < tanks.length; i++) {
        if (tanks[i].killer) {
            continue;
        }
        for (var j = i + 1; j < tanks.length; j += 1) {
            var _this = tanks[i];
            var that = tanks[j];

            var vector = [0, 0];
            var dist2 = compute_vector(vector, _this, that);
            _this.sensorCalc(that, vector, dist2);
            vector[0] = -vector[0];
            vector[1] = -vector[1];
            that.sensorCalc(_this, vector, dist2);
        }
    }

    /* Run programs */
    var errors = [];
    for (var i = 0; i < tanks.length; i++) {
        if (tanks[i].killer) {
            continue;
        }
        try {
            tanks[i].parse(tanks[i].code);
            tanks[i].run();
        } catch (e) {
            errors.push(e);
        }
    }
    if (errors.length) {
        if (interval) {
            clearInterval(interval);
        }

        document.getElementById('debug').innerHTML = "Error: " + errors.join();
        return;
    }

    /* Fire cannons and check for crashes */
    for (var i = 0; i < tanks.length; i++) {
        if (tanks[i].killer) {
            continue;
        }
        for (var j = i + 1; j < tanks.length; j += 1) {
            var _this = tanks[i];
            var that = tanks[j];

            var vector = [0, 0];
            var dist2 = compute_vector(vector, _this, that);
            _this.fireCannon(that, vector, dist2);
            vector[0] = -vector[0];
            vector[1] = -vector[1];
            that.fireCannon(_this, vector, dist2);
        }
    }
};

var addBerzerker = function() {
    var tank = new ForfTank();
    tank.init("2 random 0 = { 50 100 set-speed! } { 100 50 set-speed! } ifelse  4 random 0 = { 360 random  set-turret! } if  30 random 0 = { fire! } if");
    ftanks.push(tank);
};

var resetTanks = function() {
    if (interval) {
        clearInterval(interval);
    }

    document.getElementById('debug').innerHTML = "&nbsp;";

    tanks = [];
    ftanks = [];
    var tank;

    // add the user's tank
    tank = new ForfTank();
    tank.color = document.getElementsByName('color')[0].value;
    for (var i = 0; i < 10; i++) {
        var range = 1*document.getElementsByName('s'+i+'r')[0].value;
        var angle = (1*document.getElementsByName('s'+i+'a')[0].value) % 360;
        var width = (1*document.getElementsByName('s'+i+'w')[0].value) % 360;
        var turret = 1*document.getElementsByName('s'+i+'t')[0].checked;
        if (range) {
            tank.addSensor(range, angle, width, turret);
        }
    }
    var code = document.getElementById('program').value;
    tank.init(code);
    ftanks.push(tank);

    var n = 6 + Math.floor(Math.random()*3);
    for (var i = 0; i < n; i++) {
        addBerzerker();
    }

    initTanks(ftanks);

    var canvas = document.getElementById('battlefield');
    canvas.width = gameSize[0];
    canvas.height = gameSize[1];
    var ctx = canvas.getContext('2d');
    for (var i = 0; i < ftanks.length; i++) {
        var sensors = [];
        for (var j = 0; j < ftanks[i].sensors.length; j++) {
            var s = ftanks[i].sensors[j];
            var sensor = [s.range, s.angle, s.width, s.turret];
            sensors.push(sensor);
        }
        tank = new Tank(ctx, canvas.width, canvas.height, ftanks[i].color, sensors);
        tanks.push(tank);
    }

    function update() {
        updateTanks(ftanks);

        // clear
        canvas.width = canvas.width;

        var activeTanks = 0;
        for (var i = 0; i < ftanks.length; i++) {
            var flags = 0;
            if (ftanks[i].turret.firing) {
                flags |= 1;
            }
            if (ftanks[i].led) {
                flags |= 2;
            }
            if (ftanks[i].killer) {
                flags |= 4;
            } else {
                activeTanks++;
            }
            var sensor_state = 0;
            for (var j = 0; j < ftanks[i].sensors.length; j++) {
                if (ftanks[i].sensors[j].triggered) {
                    sensor_state |= (1 << j);
                }
            }
            tanks[i].set_state(ftanks[i].position[0], ftanks[i].position[1], ftanks[i].angle, ftanks[i].turret.current, flags, sensor_state);
        }

        if (activeTanks < 2) {
            // we're done
            clearInterval(interval);
            interval = null;
        }

        for (var i = 0; i < ftanks.length; i++) {
            tanks[i].draw_crater();
        }

        for (var i = 0; i < tanks.length; i++) {
            tanks[i].draw_wrap_sensors();
        }

        for (var i = 0; i < tanks.length; i++) {
            tanks[i].draw_tank();
        }
    }

    interval = setInterval(update, 100 /*66*/);
};


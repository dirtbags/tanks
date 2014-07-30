function dbg(o) {
    e = document.getElementById("debug");
    e.innerHTML = o;
}

function torgba(color, alpha) {
    var r = parseInt(color.substring(1,3), 16);
    var g = parseInt(color.substring(3,5), 16);
    var b = parseInt(color.substring(5,7), 16);

    return "rgba(" + r + "," + g + "," + b + "," + alpha + ")";
}

function Tank(ctx, width, height, color, sensors) {
    var craterStroke = torgba(color, 0.5);
    var craterFill = torgba(color, 0.2);
    var sensorStroke = torgba(color, 0.4);
    var maxlen = 0;

    this.x = 0;
    this.y = 0;
    this.rotation = 0;
    this.turret = 0;

    this.dead = 0;

    // Do all the yucky math up front
    this.sensors = new Array();
    for (i in sensors) {
        var s = sensors[i];

        if (! s) {
            this.sensors[i] = [0,0,0,0];
        } else {
            // r, angle, width, turret
            this.sensors[i] = new Array();
            this.sensors[i][0] = s[0];
            this.sensors[i][1] = s[1] - s[2]/2;
            this.sensors[i][2] = s[1] + s[2]/2;
            this.sensors[i][3] = s[3]?1:0;
            if (s[0] > maxlen) {
                maxlen = s[0];
            }
        }
    }

    // Set up our state, for later interleaved draw requests
    this.set_state = function(x, y, rotation, turret, flags, sensor_state) {
        this.x = x;
        this.y = y;
        this.rotation = rotation;
        this.turret = turret;
        if (flags & 1) {
            this.fire = 5;
        }
        this.led = flags & 2;
        this.dead = flags & 4;
        this.sensor_state = sensor_state;
    }

    this.draw_crater = function() {
        if (!this.dead) {
            return;
        }

        var points = 7;
        var angle = Math.PI / points;

        ctx.save();
        ctx.translate(this.x, this.y);
        ctx.rotate(this.rotation);

        if (this.fire == 5) {
            ctx.save();
            ctx.rotate(this.turret);
            // one frame of cannon fire
            this.draw_cannon();
            this.fire = 0;
            ctx.restore();
        }

        ctx.lineWidth = 2;
        ctx.strokeStyle = craterStroke;
        ctx.fillStyle = craterFill;
        ctx.beginPath();
        ctx.moveTo(12, 0);
        for (i = 0; i < points; i += 1) {
            ctx.rotate(angle);
            ctx.lineTo(6, 0);
            ctx.rotate(angle);
            ctx.lineTo(12, 0);
        }
        ctx.closePath()
        ctx.stroke();
        ctx.fill();

        ctx.restore();
    }

    this.draw_sensors = function() {
        if (this.dead) {
            return;
        }
        ctx.save();
        ctx.translate(this.x, this.y);
        ctx.rotate(this.rotation);

        ctx.lineWidth = 1;
        for (i in this.sensors) {
            var s = this.sensors[i];
            var adj = this.turret * s[3];

            if (this.sensor_state & (1 << i)) {
                // Sensor is triggered
                ctx.strokeStyle = "#000";
            } else {
                ctx.strokeStyle = sensorStroke;
            }
            ctx.beginPath();
            ctx.moveTo(0, 0);
            ctx.arc(0, 0, s[0], s[1] + adj, s[2] + adj, false);
            ctx.closePath();
            ctx.stroke();
        }

        ctx.restore();
    }

    this.draw_tank = function() {
        if (this.dead) {
            return;
        }
        ctx.save();
        ctx.translate(this.x, this.y);
        ctx.rotate(this.rotation);

        ctx.fillStyle = color;
        ctx.fillRect(-5, -4, 10, 8);
        ctx.fillStyle = "#777";
        ctx.fillRect(-7, -9, 15, 5);
        ctx.fillRect(-7,  4, 15, 5);
        ctx.rotate(this.turret);
        if (this.fire) {
            this.draw_cannon();
            this.fire -= 1;
        } else {
            if (this.led) {
                ctx.fillStyle = "#f00";
            } else {
                ctx.fillStyle = "#000";
            }
            ctx.fillRect(0, -1, 10, 2);
        }

        ctx.restore();
    }

    this.draw_cannon = function() {
        ctx.fillStyle = ("rgba(255,255,64," + this.fire/5 + ")");
        ctx.fillRect(0, -1, 45, 2);
    }

    this.draw_wrap_sensors = function() {
        var orig_x = this.x;
        var orig_y = this.y;
        for (x = this.x - width; x < width + maxlen; x += width) {
            for (y = this.y - height; y < height + maxlen; y += height) {
                if ((-maxlen < x) && (x < width + maxlen) &&
                    (-maxlen < y) && (y < height + maxlen)) {
                    this.x = x;
                    this.y = y;
                    this.draw_sensors();
                }
            }
        }
        this.x = orig_x;
        this.y = orig_y;
    }
}

var loop_id;
var updateFunc = null;
function togglePlayback() {
    if ($("#playing").prop("checked")) {
        loop_id = setInterval(updateFunc, 66);
    } else {
        clearInterval(loop_id);
        loop_id = null;
    }
    $("#pauselabel").toggleClass("ui-icon-play ui-icon-pause");
}

function start(id, game) {
    var canvas = document.getElementById(id);
    var ctx = canvas.getContext('2d');

    canvas.width = game[0][0];
    canvas.height = game[0][1];
    // game[2] is tank descriptions
    var turns = game[2];

    // Set up tanks
    var tanks = new Array();
    for (i in game[1]) {
        var desc = game[1][i];
        tanks[i] = new Tank(ctx, game[0][0], game[0][1], desc[0], desc[1]);
    }

    var frame = 0;
    var lastframe = 0;
    var fps = document.getElementById('fps');

    function update_fps() {
        fps.innerHTML = (frame - lastframe);
        lastframe = frame;
    }

    function drawFrame(idx) {
        canvas.width = canvas.width;
        turn = turns[idx];

        // Update and draw craters first
        for (i in turn) {
            t = turn[i];
            if (!t) {
                // old data, force-kill it
                tanks[i].fire = 0;
                tanks[i].dead = 5;
            } else {
                tanks[i].set_state(t[0], t[1], t[2], t[3], t[4], t[5]);
            }
            tanks[i].draw_crater();
        }
        // Then sensors
        for (i in turn) {
            tanks[i].draw_wrap_sensors();
        }
        // Then tanks
        for (i in turn) {
            tanks[i].draw_tank()
        }

        document.getElementById('frameid').innerHTML = idx;
    }

    function update() {
        var idx = frame % (turns.length + 20);
        var turn;

        frame += 1;
        if (idx >= turns.length) {
            return;
        }

        drawFrame(idx);

        $('#seekslider').slider('value', idx);
    }

    function seekToFrame(newidx) {
        var idx = frame % (turns.length + 20);
        if (idx !== newidx) {
            frame = newidx;
            drawFrame(newidx);
        }
        // make sure we're paused
        if ($("#playing").prop("checked")) {
            $("#playing").prop("checked", false);
            togglePlayback();
        }
    }

    updateFunc = update;
    loop_id = setInterval(update, 66);
    //loop_id = setInterval(update, 400);
    if (fps) {
        setInterval(update_fps, 1000);
    }

    if (id === "battlefield") {
        $("#game_box").append('<p><input type="checkbox" checked id="playing" onclick="togglePlayback();"><label for="playing"><span class="ui-icon ui-icon-pause" id="pauselabel"></class></label> <span id="frameid">0</span> <span id="seekslider" style="width: 75%; float: right;"></span></p>');
        $('#playing').button();
        var slider = $('#seekslider');
        slider.slider({ max: turns.length-1, slide: function(event, ui) { seekToFrame(ui.value); } });

        var spacing = 100 / turns.length;
        var deaths = [];
        for (i in turns[0]) {
            deaths.push(false);
        }
        var percent = 0;
        for (var f = 0; f < turns.length; f++) {
            var turn = turns[f];
            if (percent < (spacing * f)) {
                percent = spacing * f;
            }
            for (var i = 0; i < turn.length; i++) {
                if (deaths[i]) { continue; }
                if (!turn[i] || (turn[i][4] & 4)) {
                    deaths[i] = true;
                    // http://stackoverflow.com/questions/8648963/add-tick-marks-to-jquery-slider
                    $('<span class="ui-slider-tick-mark"></span>').css('left', percent +  '%').css('background-color', game[1][i][0]).appendTo(slider);
                    percent++;
                    break;
                }
            }
        }
    }
}


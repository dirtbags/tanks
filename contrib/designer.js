var canvas;
var ctx;
var tank;

function min(a, b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

function deg2rad(angle) {
    return angle*Math.PI/180;
}

function update() {
    var color = document.getElementsByName('color')[0].value;
    var sensors = new Array();

    for (i = 0; i < 10; i += 1) {
        var range = document.getElementsByName('s'+i+'r')[0].value;
        var angle = document.getElementsByName('s'+i+'a')[0].value;
        var width = document.getElementsByName('s'+i+'w')[0].value;
        var turret = document.getElementsByName('s'+i+'t')[0].checked;

        sensors[i] = [min(range, 100),
                      deg2rad(angle % 360),
                      deg2rad(width % 360),
                      turret];
    }

    tank = new Tank(ctx, 200, 200, color, sensors);
}

function design() {
    var frame = 0;
    var loop_id;

    canvas = document.getElementById('design');
    ctx = canvas.getContext('2d');

    canvas.width = 200;
    canvas.height = 200;

    tank = new Tank(ctx, 200, 200, "#c0c0c0", []);

    function update() {
        var turret = frame * Math.PI / 90;

        frame += 1;
        canvas.width = canvas.width;
        tank.set_state(100, 100, Math.PI * 1.5, turret, 0, 0);
        tank.draw_sensors();
        tank.draw_tank();
    }

    loop_id = setInterval(update, 66);
}

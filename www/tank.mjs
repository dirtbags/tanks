const craterPoints = 7
const craterAngle = Math.PI / craterPoints

export class Tank {
    constructor(ctx, color, sensors) {
        this.ctx = ctx
        this.color = color

        // Do all the yucky math up front
        this.maxlen = 0
        this.sensors = []
        for (let i in sensors) {
            let s = sensors[i]

            if (! s) {
                this.sensors[i] = [0,0,0,0]
            } else {
                let r = s.range
                // r, angle, width, turret
                this.sensors[i] = {
                    range: s.range,
                    beg: s.angle - s.width/2,
                    end: s.angle + s.width/2,
                    turret: s.turret,
                }
                if (s.range > this.maxlen) {
                    this.maxlen = s.range
                }
            }
        }

        this.set_state(0, 0, 0, 0, 0)
    }

    // Set up our state, for later interleaved draw requests
    set_state(x, y, rotation, turret, flags, sensor_state) {
        this.x = x
        this.y = y
        this.rotation = rotation
        this.turret = turret
        if (flags & 1) {
            this.fire = 5
        }
        this.led = flags & 2
        this.dead = flags & 4
        this.sensor_state = sensor_state
    }

    draw_crater() {
        if (!this.dead) {
            return
        }

        this.ctx.save()
        this.ctx.translate(this.x, this.y)
        this.ctx.rotate(this.rotation)

        if (this.fire == 5) {
            this.ctx.save()
            this.ctx.rotate(this.turret)
            // one frame of cannon fire
            this.draw_cannon()
            this.fire = 0
            this.ctx.restore()
        }

        this.ctx.lineWidth = 2
        this.ctx.strokeStyle = `rgb(from ${this.color} r g b / 50%)`
        this.ctx.fillStyle = `rgb(from ${this.color} r g b / 20%)`
        this.ctx.beginPath()
        this.ctx.moveTo(12, 0)
        for (let i = 0; i < craterPoints; i += 1) {
            this.ctx.rotate(craterAngle)
            this.ctx.lineTo(6, 0)
            this.ctx.rotate(craterAngle)
            this.ctx.lineTo(12, 0)
        }
        this.ctx.closePath()
        this.ctx.stroke()
        this.ctx.fill()

        this.ctx.restore()
    }

    draw_sensors() {
        if (this.dead) {
            return
        }
        this.ctx.save()
        this.ctx.translate(this.x, this.y)
        this.ctx.rotate(this.rotation)

        this.ctx.lineWidth = 1
        for (let i in this.sensors) {
            var s = this.sensors[i]

            this.ctx.save()
            if (s.turret) {
                this.ctx.rotate(this.turret)
            }
            if (this.sensor_state & (1 << i)) {
                // Sensor is triggered
                this.ctx.strokeStyle = "#000"
            } else {
                this.ctx.strokeStyle = `rgb(from ${this.color} r g b / 40%)`
            }
            this.ctx.beginPath()
            this.ctx.moveTo(0, 0)
            this.ctx.arc(0, 0, s.range, s.beg, s.end, false)
            this.ctx.closePath()
            this.ctx.stroke()
            this.ctx.restore()
        }

        this.ctx.restore()
    }

    draw_tank() {
        if (this.dead) {
            return
        }
        this.ctx.save()
        this.ctx.translate(this.x, this.y)
        this.ctx.rotate(this.rotation)

        this.ctx.fillStyle = this.color
        this.ctx.fillRect(-5, -4, 10, 8)
        this.ctx.fillStyle = "#777"
        this.ctx.fillRect(-7, -9, 15, 5)
        this.ctx.fillRect(-7,  4, 15, 5)
        this.ctx.rotate(this.turret)
        if (this.fire) {
            this.draw_cannon()
            this.fire -= 1
        } else {
            if (this.led) {
                this.ctx.fillStyle = "#f00"
            } else {
                this.ctx.fillStyle = "#000"
            }
            this.ctx.fillRect(0, -1, 10, 2)
        }

        this.ctx.restore()
    }

    draw_cannon() {
        this.ctx.fillStyle = ("hsl(0, 100%, 100%, " + this.fire/5 + ")")
        this.ctx.fillRect(0, -1, 45, 2)
    }

    draw_wrap_sensors() {
        let width = this.ctx.canvas.width
        let height = this.ctx.canvas.height
        let orig_x = this.x
        let orig_y = this.y
        for (let x = this.x - width; x < width + this.maxlen; x += width) {
            for (let y = this.y - height; y < height + this.maxlen; y += height) {
                if ((-this.maxlen < x) && (x < width + this.maxlen) &&
                    (-this.maxlen < y) && (y < height + this.maxlen)) {
                    this.x = x
                    this.y = y
                    this.draw_sensors()
                }
            }
        }
        this.x = orig_x
        this.y = orig_y
    }
}


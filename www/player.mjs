import {Tank} from "./tank.mjs"

const Millisecond = 1
const Second = 1000
const FPS = 12

export class Player {
    constructor(ctx) {
        this.ctx = ctx
    }

    load(game) {
        this.ctx.canvas.width = game.field[0]
        this.ctx.canvas.height = game.field[1]

        this.tanks = []
        for (let tankDef of game.tanks) {
            let tank = new Tank(this.ctx, tankDef.color, tankDef.sensors)
            this.tanks.push(tank)
        }

        this.rounds = game.rounds

        this.start()
    }

    start(frameno = 0) {
        if (!this.loop_id) {
            this.loop_id = setInterval(
                () => this.update(),
                Second / FPS,
            )
        }
        this.frameno = frameno
    }

    stop() {
        if (this.loop_id) {
            clearInterval(this.loop_id)
        }
    }

    
    update() {
        let frame = this.rounds[this.frameno]
        if (!frame) {
            this.stop()
            return
        }

        this.ctx.clearRect(0, 0, this.ctx.canvas.width, this.ctx.canvas.height)

        // Update and draw craters first
        for (let i in frame) {
            let tank = this.tanks[i]
            tank.set_state(...frame[i])
            tank.draw_crater()
        }
        // Then sensors
        for (let tank of this.tanks) {
            tank.draw_wrap_sensors()
        }
        // Then tanks
        for (let tank of this.tanks) {
            tank.draw_tank()
        }

        this.frameno += 1
    }
}

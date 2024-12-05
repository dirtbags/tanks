import {Player} from "./player.mjs"

const Millisecond = 1
const Second = 1000 * Millisecond
const Minute = 60 * Second

const GameTime = new Intl.DateTimeFormat(
    undefined,
    {
        dateStyle: "short",
        timeStyle: "short",
    },
)

function dateOfFn(fn) {
    let ts = parseInt(fn.replace(/\.json$/, ""), 16) * Second
    let when = new Date(ts)
    return GameTime.format(when)
}

class Replay {
    constructor(ctx, select, results, dateOutput, stats) {
        this.ctx = ctx
        this.select = select
        this.results = results
        this.dateOutput = dateOutput
        this.stats = stats
        this.player = new Player(ctx)
        this.games = {}

        select.addEventListener("change", () => this.reloadPlayer())
    }

    async refreshSingleGame(fn) {
        if (this.games[fn]) {
            return
        }
        let resp = await fetch(`rounds/${fn}`)
        let game = await resp.json()
        this.games[fn] = game
    }

    async refreshGames() {
        let resp = await fetch("rounds/index.json")
        let index = await resp.json()
        let promises = []
        for (let fn of index) {
            let p = this.refreshSingleGame(fn)
            promises.push(p)
        }
        for (let fn in this.games) {
            if (!index.includes(fn)) {
                delete this.games[fn]
            }
        }
        await Promise.all(promises)
    }

    fns() {
        return Object.keys(this.games).toSorted()
    }

    async refresh() {
        await this.refreshGames()

        let newGame = false
        let latest = this.select.firstElementChild
        for (let fn of this.fns()) {
            let game = this.games[fn]
            if (!game.element) {
                game.element = document.createElement("option")
                newGame = true
            }
            game.element.value = fn
            game.element.textContent = dateOfFn(fn)

            // This moves existing elements in Chrome.
            // But the MDN document doesn't specify what happens with existing elements.
            // So this may break somewhere.
            latest.after(game.element)
        }

        // Remove anything no longer in games
        for (let option of this.select.querySelectorAll("[value]")) {
            let fn = option.value
            if (!(fn in this.games)) {
                option.remove()
            }
        }

        if (newGame) {
            this.reloadStats()
        }

        if (newGame && (this.select.value == "latest")) {
            this.reloadPlayer()
        }
    }

    reloadStats() {
        let TankNames = {}
        let TankColors = {}
        let TotalGames = {}
        let TotalKills = {}
        let TotalDeaths = {}
        let MaxKills = 0
        let MaxDeaths = 0
        let ngames = 0
        for (let fn of this.fns()) {
            let game = this.games[fn]
            for (let tank of game.tanks) {
                TotalGames[tank.uid] = (TotalGames[tank.uid] ?? 0) + 1
                TotalKills[tank.uid] = (TotalKills[tank.uid] ?? 0) + tank.kills
                TotalDeaths[tank.uid] = (TotalDeaths[tank.uid] ?? 0) + (tank.killer == -1 ? 0 : 1)
                TankNames[tank.uid] = tank.name
                TankColors[tank.uid] = tank.color

                MaxKills = Math.max(MaxKills, TotalKills[tank.uid])
                MaxDeaths = Math.max(MaxDeaths, TotalDeaths[tank.uid])
            }
            ngames++
        }

        let tbody = this.stats.querySelector("tbody")
        tbody.replaceChildren()

        let byKills = Object.keys(TankNames)
        byKills.sort((a, b) => TotalKills[a] - TotalKills[b])
        byKills.reverse()

        for (let uid of byKills) {
            let tr = tbody.appendChild(document.createElement("tr"))

            let tdSwatch = tr.appendChild(document.createElement("td"))
            tdSwatch.class = "swatch"
            tdSwatch.style.backgroundColor = TankColors[uid]
            tdSwatch.textContent = "#"

            tr.appendChild(document.createElement("td")).textContent = TankNames[uid]
            tr.appendChild(document.createElement("td")).textContent = TotalGames[uid]
            tr.appendChild(document.createElement("td")).textContent = TotalKills[uid]
            tr.appendChild(document.createElement("td")).textContent = TotalDeaths[uid]

            let award = []
            if (TotalGames[uid] < 20) {
                award.push("noob")
            }
            if (TotalKills[uid] == MaxKills) {
                award.push("ruthless")
            }
            if (TotalDeaths[uid] == MaxDeaths) {
                award.push("punished")
            }
            if (TotalDeaths[uid] == 0) {
                award.push("invincible")
            }
            if (TotalDeaths[uid] == ngames) {
                award.push("wasted")
            }
            if (TotalGames[uid] > ngames) {
                award.push("overachiever")
            }
            if (TotalKills[uid] == TotalDeaths[uid]) {
                award.push("balanced")
            }
            tr.appendChild(document.createElement("td")).textContent = award.join(", ")
        }
    }

    async reloadPlayer() {
        let fn = this.select.value
        if (fn == "latest") {
            let fns = this.fns()
            fn = fns[fns.length - 1]
        }
        let game = this.games[fn]

        this.player.load(game)

        this.dateOutput.value = dateOfFn(fn)

        let tbody = this.results.querySelector("tbody")
        tbody.replaceChildren()

        let byKills = Object.keys(game.tanks)
        byKills.sort((a, b) => game.tanks[a].kills - game.tanks[b].kills)
        byKills.reverse()

        for (let i of byKills) {
            let tank = game.tanks[i]
            let tr = tbody.appendChild(document.createElement("tr"))

            let tdSwatch = tr.appendChild(document.createElement("td"))
            tdSwatch.class = "swatch"
            tdSwatch.style.backgroundColor = tank.color
            tdSwatch.textContent = "#"

            let tdName = tr.appendChild(document.createElement("td"))
            tdName.textContent = tank.name
            
            tr.appendChild(document.createElement("td")).textContent = tank.kills
            tr.appendChild(document.createElement("td")).textContent = tank.death
            tr.appendChild(document.createElement("td")).textContent = game.tanks[tank.killer]?.name
            tr.appendChild(document.createElement("td")).textContent = `${tank.error} @${tank.errorPos}`
        }
    }
}


export function init() {
    let replay = new Replay(
        document.querySelector("canvas#battlefield").getContext("2d"),
        document.querySelector("select#game"),
        document.querySelector("table#results"),
        document.querySelector("output#date"),
        document.querySelector("table#stats"),
    )
    replay.refresh()
    setInterval(() => replay.refresh(), Minute / 5)
}

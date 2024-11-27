import {Player} from "./player.mjs"

function results(round) {
    let tbody = document.querySelector("#results tbody")

    for (let tank of round.tanks) {
        let tr = tbody.appendChild(document.createElement("tr"))

        let tdSwatch = tr.appendChild(document.createElement("td"))
        tdSwatch.class = "swatch"
        tdSwatch.style.backgroundColor = tank.color
        tdSwatch.textContent = "#"

        let tdName = tr.appendChild(document.createElement("td"))
        tdName.textContent = tank.name
        
        tr.appendChild(document.createElement("td")).textContent = tank.kills
        tr.appendChild(document.createElement("td")).textContent = tank.death
        tr.appendChild(document.createElement("td")).textContent = round.tanks[tank.killer]?.name
        tr.appendChild(document.createElement("td")).textContent = `${tank.error} @${tank.errorPos}`
    }
}

async function init() {
    let canvas = document.querySelector("#battlefield")
    let ctx = canvas.getContext("2d")
    let player = new Player(ctx)

    let indexResp = await fetch("rounds/index.json")
    let index = await indexResp.json()

    let recentFn = index[index.length - 1]
    console.log(recentFn)

    let roundResp = await fetch(`rounds/${recentFn}`)
    let round = await roundResp.json()
    player.load(round)
    results(round)
}

init()

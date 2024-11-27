import {Tank} from "./tank.mjs"

Math.TAU = Math.PI * 2
const Millisecond = 1
const Second = 1000 * Millisecond
const Minute = 60 * Second
const TankRPM = 1
const TurretRPM = 4
const FPS = 12

function deg2rad(angle) {
    return angle*Math.TAU/360;
}

function update(ctx) {
    let color = document.querySelector("[name=color]").value
    let sensors = []

    for (let i = 0; i < 10; i += 1) {
        let range = document.querySelector(`[name=s${i}r]`).value
        let angle = document.querySelector(`[name=s${i}a]`).value
        let width = document.querySelector(`[name=s${i}w]`).value
        let turret = document.querySelector(`[name=s${i}t]`).checked
            
        sensors[i] = {
            range: Math.min(range, 100),
            angle: deg2rad(angle % 360),
            width: deg2rad(width % 360),
            turret: turret,
        }
    }

    let tankRevs = -TankRPM * (Date.now() / Minute)
    let turretRevs = TurretRPM * (Date.now() / Minute)
    let tank = new Tank(ctx, color, sensors);
    tank.set_state(
        100, 100,
        (tankRevs * Math.TAU) % Math.TAU,
        (turretRevs * Math.TAU) % Math.TAU,
        0,
        0,
    )
    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height)
    tank.draw_sensors()
    tank.draw_tank()
}

function formSubmit(event) {
    event.preventDefault()

    let formData = new FormData(event.target)
    for (let [k, v] of formData.entries()) {
        localStorage.setItem(k, v)
    }

    let files = {
        name: formData.get("name"),
        color: formData.get("color"),
        author: formData.get("author"),
        program: formData.get("program"),
    }
    for (let i = 0; i < 10; i++) {
        let r = formData.get(`s${i}r`) || 0
        let a = formData.get(`s${i}a`) || 0
        let w = formData.get(`s${i}w`) || 0
        let t = (formData.get(`s${i}t`) == "on") ? 1 : 0
        files[`sensor${i}`] = `${r} ${a} ${w} ${t}`
    }

    let id = formData.get("id")
    let apiURL = new URL(`tanks/${id}/`, location)

    // Fill slots
    for (let k in files) {
        let v = files[k]
        for (let e of document.querySelectorAll(`slot[name="${k}"]`)) {
            e.textContent = v
        }
    }
    for (let e of document.querySelectorAll("slot[name=apiurl]")) {
        e.textContent = apiURL
    }

    // Upload files
    let pending = 0
    let errors = 0
    let begin = performance.now()
    for (let k in files) {
        let url = new URL(k, apiURL)
        let opts = {
            method: "PUT",
            body: files[k],
        }
        pending += 1
        fetch(url, opts)
        .then(resp => {
            pending -= 1
            if (!resp.ok) {
                errors += 1
            }
            if (pending == 0) {
                let duration = (performance.now() - begin).toPrecision(2)
                let debug = document.querySelector("#debug")
                if (debug) {
                    let msg = `tank uploaded in ${duration}ms`
                    if (errors > 0) {
                        msg = msg + `; ${errors} errors`
                    }
                    debug.textContent = msg

                    setTimeout(() => debug.textContent = "", 2 * Second)
                }
            }
        })
    }
}

function init() {
    let canvas = document.querySelector("#design")
    let ctx = canvas.getContext("2d")
    canvas.width = 200
    canvas.height = 200

    let form = document.querySelector("form#upload")
    form.addEventListener("submit", formSubmit)

    let tbody = document.querySelector("#sensors tbody")
    for (let i = 0; i < 10; i++) {
        let tr = tbody.appendChild(document.createElement("tr"))

        tr.appendChild(document.createElement("td")).textContent = i

        let range = tr.appendChild(document.createElement("td")).appendChild(document.createElement("input"))
        range.name = `s${i}r`
        range.type = "number"
        range.min = 0
        range.max = 100
        range.value = 0

        let angle = tr.appendChild(document.createElement("td")).appendChild(document.createElement("input"))
        angle.name = `s${i}a`
        angle.type = "number"
        angle.min = -360
        angle.max = 360

        let width = tr.appendChild(document.createElement("td")).appendChild(document.createElement("input"))
        width.name = `s${i}w`
        width.type = "number"
        width.min = -360
        width.max = 360

        let turret = tr.appendChild(document.createElement("td")).appendChild(document.createElement("input"))
        turret.name = `s${i}t`
        turret.type = "checkbox"
    }

    // Load in previous values
    for (let e of form.querySelectorAll("[name]")) {
        let v = localStorage.getItem(e.name)
        if (v !== undefined) {
            e.checked = (v == "on")
            e.value = v
        }
    }

    update(ctx)
    setInterval(() => update(ctx), Second / FPS)
}

init()

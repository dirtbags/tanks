import assert from "node:assert/strict"
import {spawn} from "node:child_process"
import {once} from "node:events"
import {access, mkdtemp, rm, rmdir} from "node:fs/promises"
import {createServer} from "node:http"
import {tmpdir} from "node:os"
import {join} from "node:path"
import test from "node:test"
import {buildUploadScript} from "../www/upload-script.mjs"

test("the upload script treats file contents and URLs as data", async () => {
    let workdir = await mkdtemp(join(tmpdir(), "tanks-upload-test-"))
    let received = new Map()
    let server = createServer(async (req, res) => {
        let chunks = []
        for await (let chunk of req) {
            chunks.push(chunk)
        }
        received.set(req.url, {method: req.method, body: Buffer.concat(chunks)})
        res.end("written\n")
    })
    server.listen(0, "127.0.0.1")
    await once(server, "listening")

    try {
        let apiURL = new URL(`http://127.0.0.1:${server.address().port}/tanks/'$(uname)'/`)
        let files = {
            name: "'; touch field-injected; #",
            author: "quotes ' \" $() `uname` \\ and Unicode: café 🐢\n",
            color: "#abcdef",
            program: "(\nEOD\ntouch script-injected\n: <<'EOD'\n)\n0 0 set-speed!\n",
            sensor0: "100 0 90 1\u0000",
            sensor1: "",
        }
        let child = spawn("sh", [], {cwd: workdir, stdio: ["pipe", "ignore", "pipe"]})
        let stderr = ""
        child.stderr.setEncoding("utf8")
        child.stderr.on("data", chunk => stderr += chunk)
        child.stdin.end(buildUploadScript(apiURL, files))
        let [code, signal] = await once(child, "close")
        assert.equal(code, 0, `shell failed (${signal}): ${stderr}`)

        assert.equal(received.size, Object.keys(files).length)
        for (let [name, value] of Object.entries(files)) {
            let url = new URL(name, apiURL)
            let upload = received.get(url.pathname + url.search)
            assert.ok(upload, `missing upload for ${name}`)
            assert.equal(upload.method, "PUT")
            assert.deepEqual(upload.body, Buffer.from(value), `${name} was changed`)
        }
        for (let marker of ["field-injected", "script-injected"]) {
            await assert.rejects(access(join(workdir, marker)), {code: "ENOENT"})
        }
    } finally {
        await new Promise(resolve => server.close(resolve))
        for (let marker of ["field-injected", "script-injected"]) {
            await rm(join(workdir, marker), {force: true})
        }
        await rmdir(workdir)
    }
})

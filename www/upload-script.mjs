function shellQuote(value) {
    return "'" + String(value).replaceAll("'", "'\\''") + "'"
}

function utf8Base64(value) {
    let bytes = new TextEncoder().encode(value ?? "")
    let binary = ""
    for (let byte of bytes) {
        binary += String.fromCharCode(byte)
    }
    return btoa(binary)
}

export function buildUploadScript(apiURL, files) {
    let lines = ["#! /bin/sh", "set -eu", ""]
    for (let [name, value] of Object.entries(files)) {
        let data = shellQuote(utf8Base64(value))
        let url = shellQuote(new URL(name, apiURL))
        lines.push(`printf '%s' ${data} | base64 -d | curl --fail --silent --show-error -X PUT --data-binary @- -- ${url}`)
    }
    return lines.join("\n") + "\n"
}

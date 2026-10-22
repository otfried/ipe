"use strict";
// Copies the build products and Lua scripts from the Ipe source tree into
// this package.  Run `make IPEWASM=1` in src/ first.

const fs = require("node:fs");
const path = require("node:path");

const pkg = path.join(__dirname, "..");
const repo = path.join(pkg, "..", "..");
const tools = ["ipetoipe", "iperender", "ipeextract", "ipescript"];

function fail(msg) {
  console.error(`stage: ${msg}`);
  process.exit(1);
}

const wasmOut = path.join(pkg, "wasm");
fs.rmSync(wasmOut, { recursive: true, force: true });
fs.mkdirSync(wasmOut);
for (const t of tools) {
  for (const ext of [".js", ".wasm"]) {
    const src = path.join(repo, "emscripten", "bin", t + ext);
    if (!fs.existsSync(src)) fail(`${src} not found (build with IPEWASM=1)`);
    fs.copyFileSync(src, path.join(wasmOut, t + ext));
  }
}

const scriptsOut = path.join(pkg, "scripts");
fs.rmSync(scriptsOut, { recursive: true, force: true });
fs.mkdirSync(scriptsOut);
const scriptsSrc = path.join(repo, "scripts");
for (const f of fs.readdirSync(scriptsSrc)) {
  if (f.endsWith(".lua")) fs.copyFileSync(path.join(scriptsSrc, f), path.join(scriptsOut, f));
}

fs.copyFileSync(path.join(repo, "LICENSE.txt"), path.join(pkg, "LICENSE.txt"));
console.log(`staged ${tools.length} tools and scripts into ${pkg}`);

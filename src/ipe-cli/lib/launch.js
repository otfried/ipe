"use strict";
// Runs one of the Emscripten-compiled Ipe tools in the current process.

const path = require("node:path");
const os = require("node:os");

const root = path.join(__dirname, "..");

function launch(tool) {
  // ipescript looks for scripts like "onepage" in these folders; the system
  // folder of a normal Ipe installation is replaced by the bundled one.
  if (!process.env.IPESCRIPTS) {
    const dataHome =
      process.env.XDG_DATA_HOME || path.join(os.homedir(), ".local", "share");
    process.env.IPESCRIPTS = [
      ".",
      path.join(dataHome, "ipe", "scripts"),
      path.join(root, "scripts"),
    ].join(":");
  }
  // The Emscripten code starts running as soon as it is loaded and uses
  // process.argv and process.exitCode.
  require(path.join(root, "wasm", `${tool}.js`));
}

module.exports = launch;

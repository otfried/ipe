#!/usr/bin/env node
"use strict";
// ipetopptx: convert an Ipe document to PowerPoint, via `iperender -json`
// and the pptxgenjs-based writer that is also used by Ipe Web.

const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const { spawnSync } = require("node:child_process");

const conversions = ["all", "latex", "labels", "math", "none"];
const titleConversions = ["all", "latex", "none"];

function usage(status) {
  const out = status === 0 ? console.log : console.error;
  out(`Usage: ipetopptx [ -page <page> ] [ -markedview ]
                 [ -text-conversion <mode> ] [ -title-conversion <mode> ]
                 infile [ outfile ]
Ipetopptx converts an Ipe document to a PowerPoint presentation.
 -page             : export only this page (number or name; default all pages).
 -markedview       : export only marked views on marked pages.
 -text-conversion  : which text is exported as graphics instead of text:
                     ${conversions.join(", ")} (default latex).
 -title-conversion : which page titles are exported as graphics:
                     ${titleConversions.join(", ")} (default latex).
"latex" converts text that contains LaTeX commands or math, "labels" converts
labels and math, "math" only math, "none" keeps all text as text.
The default outfile is infile with extension .pptx.`);
  process.exit(status);
}

function parseArgs(argv) {
  const opts = { textConversion: "latex", titleConversion: "latex", files: [] };
  for (let i = 0; i < argv.length; ++i) {
    const a = argv[i];
    const value = () => {
      if (i + 1 >= argv.length) usage(1);
      return argv[++i];
    };
    if (a === "-help" || a === "-h" || a === "--help") usage(0);
    else if (a === "-page") opts.page = value();
    else if (a === "-markedview") opts.markedView = true;
    else if (a === "-text-conversion") opts.textConversion = value();
    else if (a === "-title-conversion") opts.titleConversion = value();
    else if (a.startsWith("-") && a.length > 1) usage(1);
    else opts.files.push(a);
  }
  if (opts.files.length < 1 || opts.files.length > 2) usage(1);
  if (!conversions.includes(opts.textConversion)) usage(1);
  if (!titleConversions.includes(opts.titleConversion)) usage(1);
  return opts;
}

function renderJson(opts, infile, jsonFile) {
  const args = [path.join(__dirname, "..", "wasm", "iperender.js"), "-json"];
  if (opts.page !== undefined) args.push("-page", opts.page);
  if (opts.markedView) args.push("-markedview");
  args.push(infile, jsonFile);
  const r = spawnSync(process.execPath, args, { stdio: ["ignore", "inherit", "inherit"] });
  if (r.error) throw r.error;
  // iperender does not always report a failure to write the file through its exit code
  if (r.status !== 0 || !fs.existsSync(jsonFile)) return false;
  return true;
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  const infile = opts.files[0];
  const outfile = opts.files[1] ?? infile.replace(/\.(ipe|xml)$/i, "") + ".pptx";
  if (!fs.existsSync(infile)) {
    console.error(`ipetopptx: cannot read ${infile}`);
    process.exit(1);
  }

  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), "ipetopptx-"));
  try {
    const jsonFile = path.join(tmpDir, "document.json");
    if (!renderJson(opts, path.resolve(infile), jsonFile)) {
      console.error("ipetopptx: iperender failed");
      return 1;
    }
    const doc = JSON.parse(fs.readFileSync(jsonFile, "utf8"));
    const { createPptx } = require("../lib/write-pptx.js");
    const data = await createPptx(doc, opts.textConversion, opts.titleConversion);
    fs.writeFileSync(outfile, data);
    return 0;
  } finally {
    fs.rmSync(tmpDir, { recursive: true, force: true });
  }
}

main().then((status) => {
  process.exitCode = status;
}, (err) => {
  console.error(`ipetopptx: ${err.message ?? err}`);
  process.exit(1);
});

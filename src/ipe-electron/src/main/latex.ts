import { execFile } from "node:child_process";
import * as fs from "node:fs";
import { promisify } from "node:util";

interface RunLatexResult {
	pdf: Uint8Array | null;
	log: string;
}

function runLatexInner(
	engine: string,
	texfile: string,
	callback: (err: unknown, result: RunLatexResult) => void,
) {
	fs.rmSync("/home/otfried/.ipe/latexrun/ipetemp.log", { force: true });
	fs.writeFileSync("/home/otfried/.ipe/latexrun/ipetemp.tex", texfile, "utf-8");
	execFile(
		engine,
		["ipetemp.tex"],
		{
			cwd: "/home/otfried/.ipe/latexrun",
		},
		(error, stdout, stderr) => {
			const log = fs.readFileSync(
				"/home/otfried/.ipe/latexrun/ipetemp.log",
				"utf-8",
			);
			const pdf = error
				? null
				: fs.readFileSync("/home/otfried/.ipe/latexrun/ipetemp.pdf");
			callback(null, { pdf, log });
		},
	);
}

export const runLatex = promisify(runLatexInner);

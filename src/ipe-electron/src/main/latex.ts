import { execFile } from "node:child_process";
import * as fs from "node:fs";
import { promisify } from "node:util";
import type { IpePathConfig } from "./pathconfig";

interface RunLatexResult {
	pdf: Uint8Array | null;
	log: string;
}

function runLatexInner(
	config: IpePathConfig,
	engine: string,
	texfile: string,
	callback: (err: unknown, result: RunLatexResult) => void,
) {
	fs.mkdirSync(config.latexdir, { recursive: true });
	fs.rmSync(`${config.latexdir}/ipetemp.log`, { force: true });
	fs.writeFileSync(`${config.latexdir}/ipetemp.tex`, texfile, "utf-8");
	execFile(
		config.latexpath === "" ? engine : `${config.latexpath}/${engine}`,
		["ipetemp.tex"],
		{
			cwd: config.latexdir,
		},
		(error, stdout, stderr) => {
			const log = fs.readFileSync(`${config.latexdir}/ipetemp.log`, "utf-8");
			const pdf = error
				? null
				: fs.readFileSync(`${config.latexdir}/ipetemp.pdf`);
			callback(null, { pdf, log });
		},
	);
}

export const runLatex = promisify(runLatexInner);

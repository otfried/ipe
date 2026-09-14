import { mkdir, readFile, writeFile } from "node:fs/promises";
import { join } from "node:path";
import type { IpePathConfig } from "./pathconfig";
import { spawn } from "node:child_process";

export interface RunLatexResult {
	pdf?: Uint8Array;
	log: string;
}

export async function runLatex(
	engine: string,
	source: string,
	paths: IpePathConfig,
): Promise<RunLatexResult> {
	await mkdir(paths.latexdir, { recursive: true });

	await writeFile(join(paths.latexdir, "ipetemp.tex"), source, "utf8");

	const result = await runProcess(
		engine,
		[
			"-interaction=nonstopmode",
			"-halt-on-error",
			"-jobname=ipetemp",
			"ipetemp.tex",
		],
		paths.latexdir,
	);

	const log = result.output;

	return {
		log,
		pdf:
			result.code === 0
				? new Uint8Array(await readFile(join(paths.latexdir, "ipetemp.pdf")))
				: undefined,
	};
}

function runProcess(
	executable: string,
	args: string[],
	cwd: string,
): Promise<{ code: number; output: string }> {
	return new Promise((resolve, reject) => {
		const child = spawn(executable, args, {
			cwd,
			stdio: ["ignore", "pipe", "pipe"],
			shell: false,
		});

		let output = "";

		child.stdout.setEncoding("utf8");
		child.stderr.setEncoding("utf8");
		child.stdout.on("data", (chunk: string) => {
			output += chunk;
		});
		child.stderr.on("data", (chunk: string) => {
			output += chunk;
		});

		child.once("error", reject);
		child.once("close", (code: number | null) => {
			resolve({ code: code ?? 1, output });
		});
	});
}

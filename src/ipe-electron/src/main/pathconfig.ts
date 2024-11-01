import * as fs from "node:fs";
import { env } from "node:process";

export class IpePathConfig {
	home: string;
	latexdir: string;
	latexpath: string;
	customization: string;
	recentFiles: string;
	styles: string[];
	ipelets: string[];
	customizationData: string | null;
	ipeletsData: { [fname: string]: string };

	constructor() {
		this.home = env.HOME ?? "/home/ipe";
		this.latexpath = env.IPELATEXPATH ?? "";
		const dataHome = env.XDG_DATA_HOME ?? `${this.home}/.local/share`;
		const configHome = env.XDG_CONFIG_HOME ?? `${this.home}/.config`;
		const cacheHome = env.XDG_CACHE_HOME ?? `${this.home}/.cache`;
		const stateHome = env.XDG_STATE_HOME ?? `${this.home}/.local/state`;
		this.latexdir = env.IPELATEXDIR ?? `${cacheHome}/ipe/latexrun`;
		this.customization = `${configHome}/ipe/customization.lua`;
		this.recentFiles = `${stateHome}/ipe/recent_files.lua`;

		if (env.IPELETPATH) {
			this.ipelets = env.IPELETPATH.split(":").map((s) =>
				s === "_" ? "/opt/ipe/ipelets" : s,
			);
		} else {
			this.ipelets = [`${dataHome}/ipe/ipelets`, "/opt/ipe/ipelets"];
		}

		if (env.IPESTYLES) {
			this.styles = env.IPESTYLES.split(":").map((s) =>
				s === "_" ? "/opt/ipe/styles" : s,
			);
		} else {
			this.styles = [`${dataHome}/ipe/styles`, "/opt/ipe/styles"];
		}

		this.customizationData = null;

		if (fs.lstatSync(this.customization, { throwIfNoEntry: false }))
			this.customizationData = fs.readFileSync(this.customization, "utf8");

		this.ipeletsData = {};
		for (const folder of this.ipelets) {
			if (folder === "/opt/ipe/ipelets") continue;
			if (fs.lstatSync(folder, { throwIfNoEntry: false })?.isDirectory()) {
				const files = fs.readdirSync(folder);
				for (const file of files) {
					if (!file.endsWith(".lua")) continue;
					const contents = fs.readFileSync(`${folder}/${file}`, "utf8");
					if (this.ipeletsData[file] == null) this.ipeletsData[file] = contents;
				}
			}
		}
	}

	watchFolders(): string[] {
		const result: string[] = [];
		for (const folder of this.styles) {
			if (folder === "/opt/ipe/styles") continue;
			if (fs.lstatSync(folder, { throwIfNoEntry: false })?.isDirectory()) {
				const files = fs.readdirSync(folder);
				for (const file of files) {
					result.push(`${folder}/${file}`);
				}
			}
		}
		return result;
	}
}

import * as fs from "node:fs";
import { env } from "node:process";

export class IpePathConfig {
	home: string;
	latexdir: string;
	latexpath: string;
	customization: string;
	styles: string[];
	ipelets: string[];
	customizationData: string | null;
	ipeletsData: { [fname: string]: string }[];

	constructor() {
		this.home = env.HOME ?? "/home/ipe";
		this.latexpath = env.IPELATEXPATH ?? "";
		const dataHome = env.XDG_DATA_HOME ?? `${this.home}/.local/share`;
		const configHome = env.XDG_CONFIG_HOME ?? `${this.home}/.config`;
		const cacheHome = env.XDG_CACHE_HOME ?? `${this.home}/.cache`;
		this.latexdir = env.IPELATEXDIR ?? `${cacheHome}/ipe`;
		this.customization = `${configHome}/ipe/customization.lua`;

		if (env.IPELETPATH) {
			this.ipelets = env.IPELETPATH.split(":").filter((s) => s !== "_");
		} else {
			this.ipelets = [`${dataHome}/ipe/ipelets`];
		}

		if (env.IPESTYLES) {
			this.styles = env.IPESTYLES.split(":").filter((s) => s !== "_");
		} else {
			this.styles = [`${dataHome}/ipe/styles`];
		}

		this.customizationData = null;

		if (fs.lstatSync(this.customization, { throwIfNoEntry: false }))
			this.customizationData = fs.readFileSync(this.customization, "utf8");

		this.ipeletsData = [];
		for (const folder of this.ipelets) {
			if (folder === "/opt/ipe/ipelets") continue;
			if (fs.lstatSync(folder, { throwIfNoEntry: false })?.isDirectory()) {
				const files = fs.readdirSync(folder);
				const ipelets1: { [fname: string]: string } = {};
				for (const file of files) {
					if (!file.endsWith(".lua")) continue;
					ipelets1[file] = fs.readFileSync(`${folder}/${file}`, "utf8");
				}
				this.ipeletsData.push(ipelets1);
			}
		}
	}
}

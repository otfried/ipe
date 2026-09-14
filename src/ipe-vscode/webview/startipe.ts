import "./ipe/index.css";
import { buildInfo } from "./gitversion";

declare global {
	interface Window {
		ipeBridge: any;
		ipeui: IpeUi;
	}
}

const vscode = acquireVsCodeApi();

import instantiateIpe from "./ipe/ipe.js";
import { IpeUi } from "./ipe/ipeui";

class IpeVSCodeBridge {
	constructor(readonly ipe: any) {
		window.addEventListener("message", (event) => {
			const message = event.data;
			switch (message.command) {
				case "startIpe":
					this.startIpe(message.content);
					break;
			}
		});
	}

	onAction(_action: string) {
		/* nada */
	}

	async setClipboard(_data: string) {}

	async getClipboard(_allowBitmap: boolean) {
		return null;
	}

	startIpe(content: string) {
		console.log("Starting Ipe from startIpe() method");
	/*
	const setup = await window.ipc.setup();
	for (const ipelet in setup.ipelets)
		ipe.FS.writeFile(`/opt/ipe/user-ipelets/${ipelet}`, setup.ipelets[ipelet]);
	if (setup.customizationData != null)
		ipe.FS.writeFile("/opt/ipe/customization.lua", setup.customizationData);
    */
   if (content !== "")
		this.ipe.FS.writeFile("/home/ipe/document.ipe", content);

	const env = [
		// `IPESTYLES=${setup.styles.join(":")}`,
		"IPELETPATH=/opt/ipe/customization.lua:/opt/ipe/user-ipelets:/opt/ipe/ipelets",
		"IPEJSLATEX=1",
		"IPEDEBUG=1",
		"IPELATEXDIR=/tmp/latexrun",
		// `HOME=${setup.home}`,
	];
	console.log("About to create IpeUi");
	const ipeui = new IpeUi(this.ipe, buildInfo, "vscode", env);
	ipeui.customizationFileName = "dummy"; // setup.customization as string;
	console.log("Starting Ipe");
	ipeui.startIpe(1920, 1024); // setup.screen.width, setup.screen.height);
	console.log("Ipe is running!");
	}
}

instantiateIpe({
	printErr: console.log.bind(console),
}).then(async (ipe: any) => {
	console.log("Ipe wasm code loaded");

	window.ipeBridge = new IpeVSCodeBridge(ipe);

	ipe.FS.mkdir("/opt/ipe/user-ipelets", 0o777);
	ipe.FS.mkdir("/tmp/pages", 0o777);
	ipe.FS.mkdir("/tmp/latexrun", 0o777);
	ipe.FS.mkdir("/tmp/latexrun/icons", 0o777);
	ipe.FS.mkdir("/home/ipe", 0o777);

	vscode.postMessage({ command: "ipeReady" });
});

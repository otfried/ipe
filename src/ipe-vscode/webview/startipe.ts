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

type IpeFormat = "ipe" | "pdf";

interface RunLatexResult {
	pdf?: Uint8Array;
	log: string;
}

function encodeBytes(data: Uint8Array): string {
	let binary = "";
	const chunkSize = 0x8000;

	for (let offset = 0; offset < data.length; offset += chunkSize) {
		binary += String.fromCharCode(...data.subarray(offset, offset + chunkSize));
	}

	return btoa(binary);
}

function decodeBytes(data: string): Uint8Array {
	const binary = atob(data);
	const result = new Uint8Array(binary.length);

	for (let i = 0; i < binary.length; ++i) {
		result[i] = binary.charCodeAt(i);
	}

	return result;
}

class IpeVSCodeBridge {
	_pendingRunLatex: ((result: RunLatexResult) => void) | null = null;
	_pendingGetClipboard: ((data: string) => void) | null = null;

	constructor(readonly ipe: any) {
		window.addEventListener("message", (event) => {
			const message = event.data;
			switch (message.command) {
				case "startIpe":
					this.startIpe(message.content, message.setup);
					break;
				case "serialize":
					this.handleSerialize(message.requestId, message.save, message.format);
					break;
				case "latexResult":
					this.handleLatexResult(message);
					break;
				case "getClipboardResult":
					this.handleGetClipboardResult(message);
					break;
				case "revert":
					this.handleRevert(message);
					break;
				case "undoRedo":
					this.handleUndoRedo(message);
					break;
			}
		});
	}

	async setClipboard(_data: string) {
		vscode.postMessage({
			command: "setClipboard",
			text: _data,
		});
	}

	manual(url: string) {
		vscode.postMessage({
			command: "manual",
			url,
		});
	}

	explain(msg: string, t: number) {
		vscode.postMessage({
			command: "explain",
			msg,
			t,
		});
	}

	startIpe(content: string, setup: any) {
		console.log("Starting Ipe from startIpe() method");
		for (const ipelet in setup.ipelets)
			this.ipe.FS.writeFile(
				`/opt/ipe/user-ipelets/${ipelet}`,
				setup.ipelets[ipelet],
			);
		if (setup.customizationData != null)
			this.ipe.FS.writeFile(
				"/opt/ipe/customization.lua",
				setup.customizationData,
			);
		if (content !== "")
			this.ipe.FS.writeFile("/home/ipe/document.ipe", decodeBytes(content));

		const env = [
			`IPESTYLES=${setup.styles.join(":")}`,
			// IPELETPATH is used on the virtual file system
			"IPELETPATH=/opt/ipe/customization.lua:/opt/ipe/user-ipelets:/opt/ipe/ipelets",
			"IPEJSLATEX=1",
			"IPEDEBUG=1",
			// IPELATEXDIR is used on the virtual file system
			"IPELATEXDIR=/tmp/latexrun",
			`HOME=${setup.home}`,
		];
		console.log("About to create IpeUi");
		const ipeui = new IpeUi(this.ipe, buildInfo, "vscode", env);
		ipeui.customizationFileName = setup.customization as string;
		ipeui.startIpe(setup.screen.width, setup.screen.height);
		console.log("Ipe is running!");
		vscode.postMessage({ command: "ipeRunning" });
	}

	private assertIpeUi(requestId: string): boolean {
		if (window.ipeui == null) {
			vscode.postMessage({
				command: "error",
				requestId,
				message: "Ipe UI is not initialized",
			});
			console.error("Ipe UI is not initialized");
			return false;
		}
		return true;
	}

	private handleSerialize(requestId: string, save: boolean, format: IpeFormat) {
		if (!this.assertIpeUi(requestId)) return;
		let content: Uint8Array;
		if (save) {
			window.ipeui.action("save");
			content = this.ipe.FS.readFile(`/home/ipe/document.${format}`);
		} else {
			window.ipeui.action("serialize");
			content = this.ipe.FS.readFile("/home/ipe/serialized.ipe");
		}
		vscode.postMessage({
			command: "response",
			requestId,
			content: encodeBytes(content),
		});
	}

	async runlatex(engine: string, texfile: string): Promise<RunLatexResult> {
		return new Promise<RunLatexResult>((resolve) => {
			this._pendingRunLatex = resolve;
			vscode.postMessage({
				command: "runLatex",
				engine,
				texfile,
			});
		});
	}

	async getClipboard(): Promise<string> {
		return new Promise<string>((resolve) => {
			this._pendingGetClipboard = resolve;
			vscode.postMessage({
				command: "getClipboard",
			});
		});
	}

	async fileDialog(options: any): Promise<string | null> {
		console.log("fileDialog called");
		vscode.postMessage({
			command: "alert",
			text: "This Ipe function is not yet implemented",
		});
		return null;
	}

	private handleUndoRedo(message: any) {
		if (!this.assertIpeUi(message.requestId)) return;
		const what = message.content.what;
		window.ipeui.action(what);
		vscode.postMessage({
			command: "response",
			requestId: message.requestId,
		});
	}

	private handleRevert(message: any) {
		const content = decodeBytes(message.content);
		this.ipe.FS.writeFile("/home/ipe/document.ipe", content);
		window.ipeui.action("revert");
		vscode.postMessage({
			command: "response",
			requestId: message.requestId,
		});
	}

	private handleLatexResult(message: any) {
		if (this._pendingRunLatex) {
			this._pendingRunLatex({
				pdf: decodeBytes(message.pdf),
				log: message.log,
			});
			this._pendingRunLatex = null;
		}
	}
	private handleGetClipboardResult(message: any) {
		if (this._pendingGetClipboard) {
			this._pendingGetClipboard(message.text);
			this._pendingGetClipboard = null;
		}
	}

	fireChange(label: string) {
		vscode.postMessage({
			command: "change",
			label,
		});
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

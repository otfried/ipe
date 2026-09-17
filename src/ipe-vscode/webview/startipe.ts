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
					this.startIpe(
						decodeBytes(message.content),
						message.setup,
						message.format,
					);
					break;
				case "serialize":
					this.handleSerialize(message.requestId, message.data.save, message.data.format);
					break;
				case "latexResult":
					this.handleLatexResult(decodeBytes(message.pdf), message.log);
					break;
				case "getClipboardResult":
					this.handleGetClipboardResult(message.text);
					break;
				case "revert":
					this.handleRevert(
						message.requestId,
						decodeBytes(message.data.content),
						message.data.format,
					);
					break;
				case "undoRedo":
					this.handleUndoRedo(message.requestId, message.data.what);
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

	startIpe(content: Uint8Array, setup: any, format: IpeFormat) {
		console.log("Starting Ipe");

		const ipeletPath: string[] = [];
		if (setup.customizationData != null) {
			this.ipe.FS.writeFile(
				"/opt/ipe/customization.lua",
				setup.customizationData,
			);
			ipeletPath.push("/opt/ipe/customization.lua");
		}
		let count = 1;
		for (const folder of setup.ipelets as { [fname: string]: string }[]) {
			this.ipe.FS.mkdir(`/opt/ipe/ipelets${count}`, 0o777);
			for (const ipelet in folder) {
				this.ipe.FS.writeFile(
					`/opt/ipe/ipelets${count}/${ipelet}`,
					folder[ipelet],
				);
			}
			ipeletPath.push(`/opt/ipe/ipelets${count}`);
			count++;
		}

		if (content.length !== 0)
			this.ipe.FS.writeFile(`/home/ipe/document.${format}`, content);

		// Environment on the virtual file system.
		// Webview remains unaware of the real file system, which is managed by
		// pathconfig.ts and extension.ts.
		const env = [
			"IPESTYLES=/opt/ipe/styles",
			`IPELETPATH=${ipeletPath.join(":")}:/opt/ipe/ipelets`,
			"IPEJSLATEX=1",
			"IPEDEBUG=1",
			"IPELATEXDIR=/tmp/latexrun",
			"HOME=/home/ipe",
		];
		console.log("Environment for Ipe:", env);
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
		console.log("Handling serialize request:", save, format);
		let content: Uint8Array;
		if (save) {
			window.ipeui.action(`vscode_save_${format}`);
			// TODO: for pdf this doesn't work, because vscode_save_pdf returns before 
			// the file has been saved (while Latex is running, control returns here!)
			console.log("Saved content to file system: ", `/home/ipe/document.${format}`);
			content = this.ipe.FS.readFile(`/home/ipe/document.${format}`);
			console.log("Read content from file system: ", `/home/ipe/document.${format}`);
		} else {
			window.ipeui.action("vscode_serialize");
			content = this.ipe.FS.readFile("/home/ipe/serialized.ipe");
		}
		console.log("Serialized content length:", save, format, content.length);
		vscode.postMessage({
			command: "response",
			requestId,
			content: encodeBytes(content),
		});
	}

	async runlatex(engine: string, texfile: string): Promise<RunLatexResult> {
		return new Promise<RunLatexResult>((resolve) => {
			this._pendingRunLatex = resolve;
			console.log(`Running LaTeX with engine: ${engine}, texfile: ${texfile}`);
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

	private handleUndoRedo(requestId: string, what: "undo" | "redo") {
		if (!this.assertIpeUi(requestId)) return;
		window.ipeui.action(what);
		vscode.postMessage({
			command: "response",
			requestId,
		});
	}

	private handleRevert(
		requestId: string,
		content: Uint8Array,
		format: IpeFormat,
	) {
		this.ipe.FS.writeFile(`/home/ipe/document.${format}`, content);
		window.ipeui.action("revert");
		vscode.postMessage({
			command: "response",
			requestId: requestId,
		});
	}

	private handleLatexResult(pdf: Uint8Array, log: string) {
		if (this._pendingRunLatex) {
			this._pendingRunLatex({
				pdf,
				log,
			});
			this._pendingRunLatex = null;
		}
	}
	private handleGetClipboardResult(text: string) {
		if (this._pendingGetClipboard) {
			this._pendingGetClipboard(text);
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

	ipe.FS.mkdir("/tmp/pages", 0o777);
	ipe.FS.mkdir("/tmp/latexrun", 0o777);
	ipe.FS.mkdir("/tmp/latexrun/icons", 0o777);
	ipe.FS.mkdir("/home/ipe", 0o777);

	vscode.postMessage({ command: "ipeReady" });
});

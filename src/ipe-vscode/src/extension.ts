import * as vscode from "vscode";

import { rootDocument } from "./root";
import { runLatex } from "./latex";
import { IpePathConfig } from "./pathconfig";

type IpeFormat = "ipe" | "pdf";

export function activate(context: vscode.ExtensionContext) {
	const options = {
		supportsMultipleEditorsPerDocument: false,
		webviewOptions: { retainContextWhenHidden: true },
	};
	// each view type needs its own provider instance: onDidChangeCustomDocument
	// is dispatched per registration, and a shared emitter would deliver every
	// edit to both registrations, crashing the one that didn't open the document
	for (const [viewType, format] of [
		["ipe", "ipe"],
		["ipe.pdf", "pdf"],
	] as const) {
		const provider = new IpeCustomEditorProvider(context.extensionUri, format);
		context.subscriptions.push(
			vscode.window.registerCustomEditorProvider(viewType, provider, options),
		);
	}
	context.subscriptions.push(
		vscode.commands.registerCommand(
			"ipe.keybinding",
			({ key }: { key: string }) => {
				/* ignore it */
			},
		),
	);
}

function getNonce() {
	let text = "";
	const possible =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	for (let i = 0; i < 32; i++) {
		text += possible.charAt(Math.floor(Math.random() * possible.length));
	}
	return text;
}

function escapeHtml(text: string): string {
    return text
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#39;");
}

function encodeBytes(data: Uint8Array): string {
	return Buffer.from(data).toString("base64");
}

function decodeBytes(data: string): Uint8Array {
	return new Uint8Array(Buffer.from(data, "base64"));
}

class IpeDocument implements vscode.CustomDocument {
	panel: IpePanel | undefined;

	constructor(
		public readonly uri: vscode.Uri,
		public readonly initialContent: Uint8Array,
		readonly format: IpeFormat,
	) {}

	dispose(): void {
		this.panel = undefined;
	}
}

class IpeCustomEditorProvider implements vscode.CustomEditorProvider {
	private readonly _onDidChangeCustomDocument = new vscode.EventEmitter<
		vscode.CustomDocumentEditEvent<IpeDocument>
	>();
	readonly onDidChangeCustomDocument = this._onDidChangeCustomDocument.event;

	constructor(
		private readonly extensionUri: vscode.Uri,
		private readonly format: IpeFormat,
	) {}

	async openCustomDocument(
		uri: vscode.Uri,
		context: vscode.CustomDocumentOpenContext,
	): Promise<IpeDocument> {
		const source = context.backupId ? vscode.Uri.parse(context.backupId) : uri;
		const format = context.backupId ? "ipe" : this.format;
		const content = await vscode.workspace.fs.readFile(source);
		console.log(
			`Opened document from ${source.fsPath}: ${content.length} bytes`,
		);
		return new IpeDocument(uri, content, format);
	}

	resolveCustomEditor(
		document: IpeDocument,
		webviewPanel: vscode.WebviewPanel,
	): void {
		webviewPanel.webview.options = {
			enableScripts: true, // Enable javascript in the webview
			// And restrict the webview to only loading content from our extension's `out/webview` directory.
			localResourceRoots: [
				vscode.Uri.joinPath(this.extensionUri, "out", "webview"),
			],
		};
		document.panel = new IpePanel(
			webviewPanel,
			this.extensionUri,
			document,
			(label) => {
				this._onDidChangeCustomDocument.fire({
					document,
					label,
					undo: () => document.panel?.undoRedo("undo"),
					redo: () => document.panel?.undoRedo("redo"),
				});
			},
		);
	}

	async saveCustomDocument(document: IpeDocument): Promise<void> {
		await this.saveIpeDocument(document, document.uri, document.format);
	}

	async saveCustomDocumentAs(
		document: IpeDocument,
		destination: vscode.Uri,
	): Promise<void> {
		// format might change during Save As
		const format =
			destination.fsPath.endsWith(".pdf") || destination.fsPath.endsWith(".PDF")
				? "pdf"
				: "ipe";
		await this.saveIpeDocument(document, destination, format);
	}

	async saveIpeDocument(
		document: IpeDocument,
		destination: vscode.Uri,
		format: IpeFormat,
	) {
		console.log(`Saving Ipe document to ${destination.fsPath} with format ${format}`);
		const content = await document.panel?.serialize(true, format);
		if (content === undefined) {
			throw new Error("Ipe editor is unavailable; cannot save the document.");
		}
		await vscode.workspace.fs.writeFile(destination, content);
		document.panel?.explain(`Saved document '${destination.fsPath}'`, 3000);
	}

	async revertCustomDocument(document: IpeDocument): Promise<void> {
		await document.panel?.revert();
	}

	async backupCustomDocument(
		document: IpeDocument,
		context: vscode.CustomDocumentBackupContext,
	): Promise<vscode.CustomDocumentBackup> {
		console.log(
			`Backing up document to ${context.destination.fsPath} (Ipe running: ${document.panel?.isIpeRunning()})`,
		);
		if (!document.panel?.isIpeRunning()) {
			await vscode.workspace.fs.writeFile(
				context.destination,
				document.initialContent,
			);
		} else {
			const content = await document.panel?.serialize(false, "ipe");
			if (content)
				await vscode.workspace.fs.writeFile(context.destination, content);
		}
		return {
			id: context.destination.toString(),
			delete: () => vscode.workspace.fs.delete(context.destination),
		};
	}
}

/**
 * Manages Ipe webview panels
 */
class IpePanel {
	public static readonly viewType = "ipe";
	public static readonly viewTypes = [IpePanel.viewType, "ipe.pdf"];

	private readonly _panel: vscode.WebviewPanel;
	private readonly _extensionUri: vscode.Uri;
	private _disposables: vscode.Disposable[] = [];
	private readonly _changeCallback: (label: string) => void;

	private readonly _document: IpeDocument;

	private readonly pending = new Map<
		string,
		{
			resolve: (content: any) => void;
			reject: (reason: Error) => void;
		}
	>();

	private lastExplain: vscode.Disposable | null = null;
	private readonly paths: IpePathConfig;
	private _ipeRunning: boolean = false;

	constructor(
		panel: vscode.WebviewPanel,
		extensionUri: vscode.Uri,
		document: IpeDocument,
		changeCallback: (label: string) => void,
	) {
		this._panel = panel;
		this._extensionUri = extensionUri;
		this._document = document;
		this.paths = new IpePathConfig();

		this._changeCallback = changeCallback;
		// Set the webview's initial html content
		this._update();

		// Listen for when the panel is disposed
		// This happens when the user closes the panel or when the panel is closed programmatically
		this._panel.onDidDispose(() => this.dispose(), null, this._disposables);

		// Handle messages from the webview
		this._panel.webview.onDidReceiveMessage(
			async (message) => {
				switch (message.command) {
					case "alert":
						vscode.window.showErrorMessage(message.text);
						return;
					case "response":
						this.handleResponse(message);
						return;
					case "error":
						this.handleError(message);
						return;
					case "manual":
						vscode.env.openExternal(vscode.Uri.parse(message.url));
						return;
					case "runLatex":
						await this.handleRunLatex(message.engine, message.texfile);
						return;
					case "ipeReady":
						this.startIpe();
						return;
					case "ipeRunning":
						this._ipeRunning = true;
						return;
					case "setClipboard":
						await vscode.env.clipboard.writeText(message.text);
						return;
					case "getClipboard":
						this.handleGetClipboard();
						return;
					case "explain":
						this.explain(message.msg, message.t);
						return;
					case "change":
						console.log(`change: ${message.label}`);
						this._changeCallback(message.label);
						return;
				}
			},
			null,
			this._disposables,
		);
	}

	// two-way communication with backend
	// when the response arrives, the callback will be invoked
	private async sendRequest(command: string, data?: any): Promise<any> {
		// TODO: if there is no reply within a certain time, reject the promise
		// generate a unique request ID
		const requestId = crypto.randomUUID();
		return new Promise((resolve, reject) => {
			this.pending.set(requestId, { resolve, reject });
			this._panel.webview
				.postMessage({
					command,
					requestId,
					data,
				})
				.then((delivered) => {
					if (!delivered) {
						this.pending.delete(requestId);
						reject(new Error("Ipe webview is no longer available."));
					}
				});
		});
	}

	private handleResponse(message: any) {
		const requestId = message.requestId;
		const request = this.pending.get(requestId);
		if (request?.resolve) {
			request.resolve(message.content);
			this.pending.delete(requestId);
		}
	}

	private handleError(message: any) {
		const requestId = message.requestId;
		const request = this.pending.get(requestId);
		if (request?.reject) {
			request.reject(new Error(message.content));
			this.pending.delete(requestId);
		}
	}

	public async serialize(
		save: boolean,
		format: IpeFormat,
	): Promise<Uint8Array> {
		const data = await this.sendRequest("serialize", { save, format });
		return decodeBytes(data);
	}

	public async revert(): Promise<void> {
		const content = await vscode.workspace.fs.readFile(this._document.uri);
		await this.sendRequest("revert", {
			content: encodeBytes(content),
			format: this._document.format,
		});
	}

	public async undoRedo(what: "undo" | "redo"): Promise<void> {
		await this.sendRequest("undoRedo", { what });
	}

	public dispose() {
		// Clean up our resources
		this._panel.dispose();

		while (this._disposables.length) {
			const x = this._disposables.pop();
			if (x) {
				x.dispose();
			}
		}
	}

	private startIpe() {
		let configuration = "<li>Style directories:<ul>";
		for (const dir of this.paths.styles) {
			configuration += `<li>${escapeHtml(dir)}</li>`;
		}
		configuration += "</ul><li>Ipelets:<ul>";
		for (const dir of this.paths.ipelets) {
			configuration += `<li>${escapeHtml(dir)}</li>`;
		}
		configuration += "</ul><li>Latex program path: ";
		configuration += escapeHtml(this.paths.latexpath);
		configuration += "</li><li>Latex directory: ";
		configuration += escapeHtml(this.paths.latexdir);
		configuration += "</li>";
		this._panel.webview.postMessage({
			command: "startIpe",
			content: encodeBytes(this._document.initialContent),
			format: this._document.format,
			setup: {
				screen: { width: 1920, height: 1080 },
				ipelets: this.paths.ipeletsData,
				customization: this.paths.customization,
				customizationData: this.paths.customizationData,
				configuration: configuration,
			},
		});
	}

	private _update() {
		// Local path to main script run in the webview (esbuild output, not the webview/ source)
		const scriptPathOnDisk = vscode.Uri.joinPath(
			this._extensionUri,
			"out",
			"webview",
			"startipe.js",
		);

		// And the uri we use to load this script in the webview
		const scriptUri = this._panel.webview.asWebviewUri(scriptPathOnDisk);

		// Local path to css styles (bundled by esbuild alongside startipe.js)
		const styleRootPath = vscode.Uri.joinPath(
			this._extensionUri,
			"out",
			"webview",
			"startipe.css",
		);

		// Uri to load styles into webview
		const stylesRootUri = this._panel.webview.asWebviewUri(styleRootPath);

		// Use a nonce to only allow specific scripts to be run
		const nonce = getNonce();

		// Use a content security policy to only allow loading images from https or from our extension directory,
		// and only allow scripts that have a specific nonce.
		// 'connect-src' is needed so the wasm binary can be fetched,
		// 'wasm-unsafe-eval' so it can be compiled,
		// 'unsafe-eval' for emscripten values (interpreting JS from string).

		this._panel.title = this._document.uri.path.split("/").pop() ?? "Ipe";

		this._panel.webview.html = `<!DOCTYPE html>
			<html lang="en">
			<head>
				<meta charset="UTF-8">
				<meta http-equiv="Content-Security-Policy" 
					content="default-src 'none'; style-src ${this._panel.webview.cspSource}; img-src ${this._panel.webview.cspSource} https: blob:; connect-src ${this._panel.webview.cspSource}; script-src 'nonce-${nonce}' 'wasm-unsafe-eval' 'unsafe-eval';">
				<meta name="viewport" content="width=device-width, initial-scale=1.0">
				<link href="${stylesRootUri}" rel="stylesheet">
				<title>Ipe</title>
			</head>
				<body>
			  	${rootDocument}
    			<script type="module" nonce="${nonce}" src="${scriptUri}"></script>
				</body>
			</html>`;
	}

	private async handleRunLatex(engine: string, texfile: string) {
		this.lastExplain?.dispose();
		this.lastExplain = vscode.window.setStatusBarMessage(
			`Running LaTeX with engine: ${engine}`,
		);
		const { log, pdf } = await runLatex(engine, texfile, this.paths);
		this._panel.webview.postMessage({
			command: "latexResult",
			log,
			pdf: pdf ? encodeBytes(pdf) : undefined,
		});
		this.lastExplain?.dispose();
		this.lastExplain = vscode.window.setStatusBarMessage(
			`Finished running LaTeX with engine: ${engine}`,
			2000,
		);
	}

	private async handleGetClipboard() {
		const text = await vscode.env.clipboard.readText();
		this._panel.webview.postMessage({
			command: "getClipboardResult",
			text,
		});
	}

	explain(msg: string, t: number) {
		this.lastExplain?.dispose();
		this.lastExplain = vscode.window.setStatusBarMessage(msg, t);
	}

	isIpeRunning(): boolean {
		return this._ipeRunning;
	}
}

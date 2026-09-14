import * as vscode from "vscode";

import { rootDocument } from "./root";

export function activate(context: vscode.ExtensionContext) {
	context.subscriptions.push(
		vscode.window.registerCustomEditorProvider(
			IpePanel.viewType,
			new IpeCustomEditorProvider(context.extensionUri),
			{
				webviewOptions: { retainContextWhenHidden: true },
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

class IpeDocument implements vscode.CustomDocument {
	constructor(public readonly uri: vscode.Uri) {}

	dispose(): void {
		// Release document-side resources.
	}
}

class IpeCustomEditorProvider implements vscode.CustomEditorProvider {
	readonly onDidChangeCustomDocument = new vscode.EventEmitter<{
		document: vscode.CustomDocument;
		content?: readonly vscode.CustomDocumentEditEvent[];
	}>().event;

	constructor(private readonly extensionUri: vscode.Uri) {}

	openCustomDocument(uri: vscode.Uri): IpeDocument {
		return new IpeDocument(uri);
	}

	resolveCustomEditor(
		document: IpeDocument,
		webviewPanel: vscode.WebviewPanel,
	): void {
		webviewPanel.webview.options = getWebviewOptions(this.extensionUri);
		new IpePanel(webviewPanel, this.extensionUri, document);
	}

	saveCustomDocument(document: IpeDocument): Thenable<void> {
		return Promise.resolve();
	}

	saveCustomDocumentAs(document: IpeDocument): Thenable<void> {
		return Promise.resolve();
	}

	revertCustomDocument(document: IpeDocument): Thenable<void> {
		return Promise.resolve();
	}

	backupCustomDocument(
		document: vscode.CustomDocument,
		context: vscode.CustomDocumentBackupContext,
	): Thenable<vscode.CustomDocumentBackup> {
		return Promise.resolve({
			id: context.destination.toString(),
			delete: () => undefined,
		});
	}
}

function getWebviewOptions(extensionUri: vscode.Uri): vscode.WebviewOptions {
	return {
		// Enable javascript in the webview
		enableScripts: true,

		// And restrict the webview to only loading content from our extension's `webview`/`out/webview` directories.
		localResourceRoots: [vscode.Uri.joinPath(extensionUri, "out", "webview")],
	};
}

/**
 * Manages Ipe webview panels
 */
class IpePanel {
	public static readonly viewType = "ipe";

	private readonly _panel: vscode.WebviewPanel;
	private readonly _extensionUri: vscode.Uri;
	private _disposables: vscode.Disposable[] = [];

	private readonly _document: IpeDocument;

	constructor(
		panel: vscode.WebviewPanel,
		extensionUri: vscode.Uri,
		document: IpeDocument,
	) {
		this._panel = panel;
		this._extensionUri = extensionUri;
		this._document = document;

		// Set the webview's initial html content
		this._update();

		// Listen for when the panel is disposed
		// This happens when the user closes the panel or when the panel is closed programmatically
		this._panel.onDidDispose(() => this.dispose(), null, this._disposables);

		// Update the content based on view changes
		this._panel.onDidChangeViewState(
			() => {
				if (this._panel.visible) {
					this._update();
				}
			},
			null,
			this._disposables,
		);

		// Handle messages from the webview
		this._panel.webview.onDidReceiveMessage(
			(message) => {
				switch (message.command) {
					case "alert":
						vscode.window.showErrorMessage(message.text);
						return;
					case "ipeReady": {
						vscode.window.showInformationMessage(
							"Ipe has started successfully!",
						);
						// pass file contents to webview
						const fileContents = vscode.workspace.fs.readFile(
							this._document.uri,
						);
						fileContents.then((content) => {
							this._panel.webview.postMessage({
								command: "startIpe",
								content: new TextDecoder().decode(content),
							});
						});
						return;
					}
				}
			},
			null,
			this._disposables,
		);
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
					content="default-src 'none'; style-src ${this._panel.webview.cspSource}; img-src ${this._panel.webview.cspSource} https:; connect-src ${this._panel.webview.cspSource}; script-src 'nonce-${nonce}' 'wasm-unsafe-eval' 'unsafe-eval';">
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
}

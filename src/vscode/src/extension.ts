import * as vscode from "vscode";

import { rootDocument } from "./root";

export function activate(context: vscode.ExtensionContext) {
	context.subscriptions.push(
		vscode.commands.registerCommand("ipe.start", () => {
			IpePanel.createOrShow(context.extensionUri);
		}),
	);

	context.subscriptions.push(
		vscode.commands.registerCommand("ipe.doRefactor", () => {
			if (IpePanel.currentPanel) {
				IpePanel.currentPanel.doRefactor();
			}
		}),
	);

	if (vscode.window.registerWebviewPanelSerializer) {
		// Make sure we register a serializer in activation event
		vscode.window.registerWebviewPanelSerializer(IpePanel.viewType, {
			async deserializeWebviewPanel(
				webviewPanel: vscode.WebviewPanel,
				state: unknown,
			) {
				console.log(`Got state: ${state}`);
				// Reset the webview options so we use latest uri for `localResourceRoots`.
				webviewPanel.webview.options = getWebviewOptions(context.extensionUri);
				IpePanel.revive(webviewPanel, context.extensionUri);
			},
		});
	}
}

function getWebviewOptions(extensionUri: vscode.Uri): vscode.WebviewOptions {
	return {
		// Enable javascript in the webview
		enableScripts: true,

		// And restrict the webview to only loading content from our extension's `webview`/`out/webview` directories.
		localResourceRoots: [
			vscode.Uri.joinPath(extensionUri, "webview"),
			vscode.Uri.joinPath(extensionUri, "out", "webview"),
		],
	};
}

/**
 * Manages Ipe webview panels
 */
class IpePanel {
	/**
	 * Track the currently panel. Only allow a single panel to exist at a time.
	 */
	public static currentPanel: IpePanel | undefined;

	public static readonly viewType = "ipe";

	private readonly _panel: vscode.WebviewPanel;
	private readonly _extensionUri: vscode.Uri;
	private _disposables: vscode.Disposable[] = [];

	public static createOrShow(extensionUri: vscode.Uri) {
		const column = vscode.window.activeTextEditor
			? vscode.window.activeTextEditor.viewColumn
			: undefined;

		// If we already have a panel, show it.
		if (IpePanel.currentPanel) {
			IpePanel.currentPanel._panel.reveal(column);
			return;
		}

		// Otherwise, create a new panel.
		const panel = vscode.window.createWebviewPanel(
			IpePanel.viewType,
			"Ipe",
			column || vscode.ViewColumn.One,
			getWebviewOptions(extensionUri),
		);

		IpePanel.currentPanel = new IpePanel(panel, extensionUri);
	}

	public static revive(panel: vscode.WebviewPanel, extensionUri: vscode.Uri) {
		IpePanel.currentPanel = new IpePanel(panel, extensionUri);
	}

	private constructor(panel: vscode.WebviewPanel, extensionUri: vscode.Uri) {
		this._panel = panel;
		this._extensionUri = extensionUri;

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
				}
			},
			null,
			this._disposables,
		);
	}

	public doRefactor() {
		// Send a message to the webview webview.
		// You can send any JSON serializable data.
		this._panel.webview.postMessage({ command: "refactor" });
	}

	public dispose() {
		IpePanel.currentPanel = undefined;

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
		const webview = this._panel.webview;
		this._panel.title = "Ipe";
		this._panel.webview.html = this._getHtmlForWebview(webview);
	}

	private _getHtmlForWebview(webview: vscode.Webview) {
		// Local path to main script run in the webview (esbuild output, not the webview/ source)
		const scriptPathOnDisk = vscode.Uri.joinPath(
			this._extensionUri,
			"out",
			"webview",
			"startipe.js",
		);

		// And the uri we use to load this script in the webview
		const scriptUri = webview.asWebviewUri(scriptPathOnDisk);

		// Local path to css styles (bundled by esbuild alongside startipe.js)
		const styleRootPath = vscode.Uri.joinPath(
			this._extensionUri,
			"out",
			"webview",
			"startipe.css",
		);

		// Uri to load styles into webview
		const stylesRootUri = webview.asWebviewUri(styleRootPath);

		// Use a nonce to only allow specific scripts to be run
		const nonce = getNonce();

		return `<!DOCTYPE html>
			<html lang="en">
			<head>
				<meta charset="UTF-8">
				<!--
					Use a content security policy to only allow loading images from https or from our extension directory,
					and only allow scripts that have a specific nonce.
					connect-src is needed so the wasm binary can be fetched, and 'wasm-unsafe-eval' so it can be compiled.
				-->
				<meta http-equiv="Content-Security-Policy" 
					content="default-src 'none'; style-src ${webview.cspSource}; img-src ${webview.cspSource} https:; connect-src ${webview.cspSource}; script-src 'nonce-${nonce}' 'wasm-unsafe-eval' 'unsafe-eval';">
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

function getNonce() {
	let text = "";
	const possible =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	for (let i = 0; i < 32; i++) {
		text += possible.charAt(Math.floor(Math.random() * possible.length));
	}
	return text;
}

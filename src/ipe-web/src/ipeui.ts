import Sortable, { type SortableEvent } from "sortablejs";
import {
	CANCEL,
	type DialogOptions,
	type ElementOptions,
	retrieveValues,
	setElement,
} from "./dialogs";
import { buildInfo } from "./gitversion";
import type { Ipe, ResumeResult } from "./ipejs";
import {
	type FileDialogOptions,
	type MessageBoxOptions,
	Modal,
	type PageSelectorItem,
} from "./modal";
import {
	type Color,
	type MainMenuItemOptions,
	type MainMenuItemType,
	type PopupItemOptions,
	PopupMenu,
} from "./popup-menu";
import { TouchDragZoom } from "./touch";
import { get, removeChildren } from "./util";

interface Action {
	name: string;
	label: string;
	shortcut: string | null;
}

interface Layer {
	name: string;
	text: string;
	checked: boolean;
	active: boolean;
	locked: boolean;
	snap: "normal" | "never" | "always";
}

function toRgb(rgb: Color): string {
	return `rgb(${255 * rgb.red}, ${255 * rgb.green}, ${255 * rgb.blue})`;
}

export class IpeUi {
	readonly ipe: Ipe;
	modal: Modal;
	readonly bottomCanvas = get("bottomCanvas") as HTMLCanvasElement;
	readonly topCanvas = get("topCanvas") as HTMLCanvasElement;
	readonly preloadCache: { [fname: string]: string } = {};
	readonly actions: { [name: string]: Action };
	readonly actionState: { [action: string]: boolean } = {
		toggle_notes: false,
		toggle_bookmarks: false,
	};
	readonly mainMenu: MainMenuItemOptions[];
	currentSubMenu: MainMenuItemOptions | null = null;
	popupMenu: PopupMenu;
	readonly selectorNames = [
		"stroke",
		"fill",
		"pen",
		"dashstyle",
		"textsize",
		"markshape",
		"symbolsize",
		"opacity",
		"gridsize",
		"anglesize",
	];
	readonly version: { year: number; version: string };
	filename: string | null;
	saveCallback: ((fn: string) => void) | null = null;
	readonly touch: TouchDragZoom;

	constructor(ipe: Ipe) {
		this.ipe = ipe;
		this.mainMenu = [];
		this.actions = {};
		window.ipeui = this;
		this.modal = new Modal(ipe, (result) => this.resume(result));
		this.version = this.ipe.Emval.toValue(this.ipe._ipeVersion());
		this.touch = new TouchDragZoom(this.ipe, this.topCanvas);

		this._calculateCanvasSize();
		this.ipe._initLib(
			this.ipe.Emval.toHandle(["HOME=/home/ipe", "IPEJSLATEX=1", "IPEDEBUG=1"]),
		);
		this.popupMenu = new PopupMenu();
		this.filename = null;

		document.addEventListener("keydown", (event) => {
			this._handleKeyEvent(event);
		});

		window.ipc?.onAction((action: string) => this.action(action));

		const lb = get("layerbox");
		Sortable.create(lb, {
			handle: ".drag-handle",
			onSort: (_evt: SortableEvent) => this._layersSorted(),
		});

		window.onclick = (event) => {
			if (event.target === this.modal.pane) this.modal.close(CANCEL);
			if (event.target === this.popupMenu.pane) {
				if (this.popupMenu.closeOne()) this.resume(null);
			}
		};
	}

	startIpe(width: number, height: number) {
		this.ipe._startIpe(
			width,
			height,
			window.devicePixelRatio,
			this.ipe.stringToNewUTF8("web"),
		);
		// these actions are not on the menu, need to create them first
		this.actions.stop = {
			name: "stop",
			label: "Stop",
			shortcut: null,
		};
		this.actions.shift_key = {
			name: "shift_key",
			label: "Press the Shift key",
			shortcut: null,
		};
		this.actions.context_menu = {
			name: "context_menu",
			label: "Show context menu",
			shortcut: null,
		};
		this.setCheckMark("coordinates|points");
		this.setCheckMark("scaling|1");
		this._setupCanvas();
		this._setupIcons();
		this._setupPathView();
	}

	resume(result: ResumeResult): void {
		this.ipe._resume(this.ipe.Emval.toHandle(result));
	}

	action(action: string) {
		if (action === "manual") {
			window.open("http://otfried.github.io/ipe", "_blank");
		} else if (action === "about") {
			this._aboutIpe();
		} else if (action === "preferences") {
			this._explainPreferences();
		} else if (action === "manage_files") {
			this.modal.fileManager();
		} else if (action === "shift_key") {
			this.setActionState("shift_key", !this.actionState.shift_key);
			this.ipe._canvasSetAdditionalModifiers(
				this.actionState.shift_key ? 0x100 : 0,
			);
		} else if (action === "context_menu") {
			this.setActionState("context_menu", !this.actionState.context_menu);
		} else if (action === "fullscreen") {
			if (!document.fullscreenElement) {
				document.body.requestFullscreen();
			} else if (document.exitFullscreen) {
				document.exitFullscreen();
			}
		} else {
			if (action in this.actionState) {
				this.setActionState(action, !this.actionState[action]);
			}
			if (action.startsWith("mode_")) {
				this._setButtonIcon(get("modeIndicator"), action);
				const el = get("toolbarMode");
				for (const child of el.children) {
					if (child.id === action) child.classList.add("selected");
					else child.classList.remove("selected");
				}
				this._setActionStateMark(action, true);
			} else if (action.includes("|")) {
				this.setCheckMark(action);
			}
			this.ipe._action(this.ipe.stringToNewUTF8(action));
		}
	}

	openFile(fn: string): void {
		this.ipe._openFile(this.ipe.stringToNewUTF8(fn));
	}

	addSaveCallback(callback: (fn: string) => void): void {
		this.saveCallback = callback;
	}

	private _calculateCanvasSize() {
		const dpr = window.devicePixelRatio;
		const stage = get("stage");
		const stageSize = stage.getBoundingClientRect();
		const w = stageSize.width;
		const h = stageSize.height;
		for (const htmlCanvas of [this.bottomCanvas, this.topCanvas]) {
			htmlCanvas.style.width = `${w}px`;
			htmlCanvas.style.height = `${h}px`;
			htmlCanvas.width = w * dpr;
			htmlCanvas.height = h * dpr;
		}
	}

	private _setupIcons() {
		for (const action in this.actions) {
			const el = document.getElementById(action);
			if (el == null) continue;
			if (action !== "context_menu") this._setButtonIcon(el, action);
			el.onclick = () => this.action(action);
			const a = this.actions[action];
			el.title = a.shortcut == null ? a.label : `${a.label} [${a.shortcut}]`;
		}

		this._setButtonIcon(get("modeIndicator"), "mode_select");
		this._setButtonIcon(get("stop"), "stop");
		this._setButtonIcon(get("abs-pen"), "pen");
		this._setButtonIcon(get("abs-textsize"), "mode_label");
		this._setButtonIcon(get("abs-symbolsize"), "mode_marks");

		for (const b of this.selectorNames) {
			const el1 = document.getElementById(`abs-${b}`);
			if (el1)
				el1.onclick = () =>
					this.ipe._absoluteButton(this.ipe.stringToNewUTF8(b));
			const el2 = get(b) as HTMLSelectElement;
			el2.onchange = () => {
				this.ipe._selector(
					this.ipe.stringToNewUTF8(b),
					this.ipe.stringToNewUTF8(el2.value),
				);
			};
		}

		const addListener = (id: string, b: string) => {
			const el = get(id) as HTMLInputElement;
			el.addEventListener("change", () => {
				this.actionState[b] = el.checked;
				this.ipe._absoluteButton(this.ipe.stringToNewUTF8(b));
			});
		};
		addListener("viewMarked", "viewmarked");
		addListener("pageMarked", "pagemarked");
		get("viewNumber").addEventListener("click", () => this.action("jump_view"));
		get("pageNumber").addEventListener("click", () => this.action("jump_page"));
	}

	private _setButtonIcon(el: HTMLElement, action: string) {
		const svg = this.ipe.FS.readFile(`/tmp/latexrun/icons/${action}.svg`, {
			encoding: "utf8",
		});
		el.innerHTML = svg;
	}

	private _setupCanvas() {
		this.topCanvas.addEventListener("pointerup", (event) => {
			console.log("up", event.pointerType, event);
			if (event.pointerType === "touch") {
				event.preventDefault();
				return;
			}
			this.ipe._canvasMouseButtonEvent(
				this.ipe.Emval.toHandle(event),
				event.buttons,
				false,
			);
		});
		this.topCanvas.addEventListener("pointerdown", (event) => {
			console.log("down", event.pointerType, event.buttons, event.tiltY, event);
			// ignore event on right mouse button, as it generates contextmenu
			if (event.pointerType === "touch" || event.buttons === 2) {
				event.preventDefault();
				return;
			}
			let buttons = event.buttons;
			if (
				buttons === 1 &&
				(this.actionState.context_menu || event.tiltY < -20)
			) {
				buttons = 2;
				this.setActionState("context_menu", false);
			}
			this.ipe._canvasMouseButtonEvent(
				this.ipe.Emval.toHandle(event),
				buttons,
				true,
			);
		});
		this.topCanvas.addEventListener("dblclick", (event) => {
			console.log("dblclick", event);
			this.ipe._canvasMouseButtonEvent(
				this.ipe.Emval.toHandle(event),
				0x81,
				true,
			);
		});
		this.topCanvas.addEventListener("pointermove", (event) => {
			if (event.pointerType === "touch") {
				event.preventDefault();
				return;
			}
			this.ipe._canvasMouseMoveEvent(this.ipe.Emval.toHandle(event));
		});
		this.topCanvas.addEventListener("wheel", (event) => {
			event.preventDefault();
			this.ipe._canvasWheelEvent(this.ipe.Emval.toHandle(event));
		});
		this.topCanvas.addEventListener("contextmenu", (event) => {
			event.preventDefault();
			// report as a press event on right mouse button
			this.ipe._canvasMouseButtonEvent(this.ipe.Emval.toHandle(event), 2, true);
		});

		// TODO: use window.matchMedia() to watch for changes in dpr
		let resizeTimeout: number | undefined = undefined;
		window.addEventListener("resize", () => {
			clearTimeout(resizeTimeout);
			resizeTimeout = setTimeout(() => {
				this._calculateCanvasSize();
				this.ipe._canvasUpdateSize();
				this.ipe._canvasUpdate();
			}, 250);
		});
	}

	private _setupPathView() {
		const pv = get("pathView");
		pv.addEventListener("pointerdown", (event) => {
			event.preventDefault();
			if (event.buttons === 1) {
				if (this.actionState.context_menu) {
					this.setActionState("context_menu", false);
					this.ipe._showPathStylePopup(event.clientX, event.clientY);
				} else this._pathViewButton(event);
			}
		});
		pv.addEventListener("contextmenu", (event) => {
			event.preventDefault();
			this.ipe._showPathStylePopup(event.clientX, event.clientY);
		});
		this.paintPathView();
	}

	private _pathViewButton(event: MouseEvent) {
		const x = event.offsetX;
		const w = get("pathView").getBoundingClientRect().width;
		if (x < (w * 3) / 10) {
			this.action("rarrow|toggle");
		} else if (x > (w * 4) / 10 && x < (w * 72) / 100) {
			this.action("farrow|toggle");
		} else if (x > (w * 78) / 100) {
			this.action("pathmode|next");
		}
	}

	private _convertAccelerator(s: string | null): string | undefined {
		if (s == null) return undefined;
		return s
			.replace("PgDown", "PageDown")
			.replace("PgUp", "PageUp")
			.replace("delete", "Delete");
	}

	private _layersSorted() {
		const lb = get("layerbox");
		const order: string[] = [];
		for (const child of lb.children) order.push(child.getAttribute("id")!);
		this.ipe._layerOrderChanged(this.ipe.Emval.toHandle(order));
	}

	private _handleKeyEvent(event: KeyboardEvent) {
		if (this.popupMenu.keyPressEvent(event)) {
			this.resume(null);
			return;
		}
		if (this.modal.keyPressEvent(event)) return;
		event.preventDefault();
		if (["Control", "Shift", "Alt", "Meta"].includes(event.key)) return;
		// offer to canvas first
		if (!this.ipe._canvasKeyPressEvent(this.ipe.Emval.toHandle(event))) {
			let ch = event.key;
			if (ch.length === 1 && "a" <= ch && ch <= "z") ch = ch.toUpperCase();
			let key = "";
			if (event.altKey) key += "Alt+";
			if (event.ctrlKey) key += "Ctrl+";
			if (event.metaKey) key += "Meta+";
			if (event.shiftKey) key += "Shift+";
			key += ch;
			const action = this._findActionFromShortcut(key);
			if (action) this.action(action);
		}
	}

	private _findActionFromShortcut(key: string): string | null {
		for (const rootItem of this.mainMenu) {
			for (const item of rootItem.submenu!) {
				if (item.accelerator === key) return item.id!;
			}
		}
		return null;
	}

	// ------------------------------------------------------------------------------------

	private _aboutIpe() {
		const yearLine = `<p>Copyright (c) 1993-${this.version.year} Otfried Cheong</p>`;
		const body =
			"<p>The extensible drawing editor Ipe creates figures in PDF format, " +
			"using LaTeX to format the text in the figures.</p>" +
			"<p>Ipe is released under the GNU Public License.</p>" +
			'<p>See the <a target="_blank" href="http://ipe.otfried.org">Ipe homepage</a>' +
			" for further information.</p>" +
			"<p>If you are an Ipe fan and want to show others, have a look at the " +
			'<a target="_blank" href="https://www.shirtee.com/en/store/ipe">Ipe T-shirts</a>.</p>' +
			"<h3>Platinum and gold sponsors</h3>" +
			"<ul><li>Hee-Kap Ahn</li>" +
			"<li>Günter Rote</li>" +
			"<li>SCALGO</li>" +
			"<li>Martin Ziegler</li></ul>" +
			"<p>If you enjoy Ipe, feel free to treat the author on a cup of coffee at " +
			'<a target="_blank" href="https://ko-fi.com/ipe7author">Ko-fi</a>.</p>' +
			"<p>You can also become a member of the exclusive community of " +
			'<a target="_blank" href="http://patreon.com/otfried">Ipe patrons</a>. ' +
			"For the price of a cup of coffee per month you can make a meaningful contribution " +
			"to the continuing development of Ipe.</p>";
		const build = `<div class="buildInfo">This Ipe is ${buildInfo}</div>`;
		this.modal.showBanner(
			`Ipe ${this.version.version} Web Edition`,
			`${yearLine}${body}${build}`,
		);
	}

	private _explainPreferences() {
		this.modal.showBanner(
			"Ipe preferences",
			"<p>Ipe preferences are changed by creating a Lua source file.</p>" +
				"<p>You can find the available options in " +
				'<a target="_blank" href="https://github.com/otfried/ipe/tree/master/src/ipe/lua/prefs.lua">' +
				"'prefs.lua'</a>, " +
				'<a target="_blank" href="https://github.com/otfried/ipe/tree/master/src/ipe/lua/shortcuts.lua">' +
				"'shortcuts.lua'</a>, and " +
				'<a target="_blank" href="https://github.com/otfried/ipe/tree/master/src/ipe/lua/mouse.lua">' +
				"'mouse.lua'</a>.</p>" +
				"<p>Create a new file 'customization.lua', place your changes in this file, " +
				"and upload it as an ipelet.</p><p>More details are in the " +
				'<a target="_blank" href="https://otfried.github.io/ipe/80_advanced.html#customizing-ipe">' +
				"Manual</a>.</p>",
		);
	}

	// ----------------- methods to be used by Ipe ---------------------------

	paintPathView() {
		const dpr = window.devicePixelRatio;
		const pv = get("pathView") as HTMLCanvasElement;
		const s = pv.getBoundingClientRect();
		const w = s.width;
		const h = s.height;
		pv.width = w * dpr;
		pv.height = h * dpr;
		this.ipe._paintPathView();
	}

	// ------------------------------------------------------------------------------------

	private async _mainMenuClicked(
		e: MouseEvent,
		submenu: MainMenuItemOptions[],
	) {
		this.popupMenu.openMainMenu(
			e.target as HTMLButtonElement,
			submenu,
			(action) => this.action(action),
		);
	}

	private _setActionStateMark(action: string, checked: boolean) {
		if (window.ipc) {
			window.ipc?.setMenuCheckmark(action, checked);
		} else {
			for (const rootItem of this.mainMenu) {
				for (const item of rootItem.submenu as MainMenuItemOptions[]) {
					if (item.id === action) {
						if (item.type === "radio") {
							for (const item1 of rootItem.submenu as MainMenuItemOptions[])
								item1.checked = false;
						}
						item.checked = checked;
						return;
					}
				}
			}
		}
	}

	setCheckMark(action: string) {
		const i = action.indexOf("|");
		const tag = action.substring(0, i + 1);
		for (const rootItem of this.mainMenu) {
			for (const item of rootItem.submenu as MainMenuItemOptions[]) {
				if (item.type === "submenu") {
					for (const item1 of item.submenu!) {
						if (item1.id?.startsWith(tag)) {
							item1.checked = item1.id === action;
						}
					}
				}
			}
		}
	}

	setupMenu() {
		if (window.ipc != null) {
			window.ipc.menu(this.mainMenu);
		} else {
			for (const m of this.mainMenu) {
				const tag = m.label!.replace("&", "").toLowerCase();
				const el = get(`menu-${tag}`) as HTMLButtonElement;
				el.addEventListener("click", async (e) =>
					this._mainMenuClicked(e, m.submenu!),
				);
			}
		}
	}

	buildMenu(
		type: MainMenuItemType | "rootmenu",
		id: number, // to refer to the main menu items 0 ..
		name: string, // internal name of the action or submenu
		label: string, // displayed in the menu
		shortcut: string, // the keyboard accelerator
	) {
		if (label?.includes("&&")) label = label.replace("&&", "&");
		else if (label?.includes("&")) label = label.replace("&", "");
		if (type === "rootmenu") {
			if (this.mainMenu.length !== id)
				throw new Error(`Error setting up menu ${name}`);
			this.mainMenu.push({
				type: "submenu",
				id: label.toLowerCase(),
				label,
				submenu: [],
			});
		} else if (type === "separator") {
			this.mainMenu[id].submenu!.push({
				type: "separator",
			});
		} else {
			const menu = id >= 0 ? this.mainMenu[id] : this.currentSubMenu!;
			// exclude these for the moment
			if (["new_window", "keyboard", "cloud_latex"].includes(name)) return;
			if (window.ipc === undefined && name === "close") {
				menu.submenu!.pop(); // remove separator
				return;
			}
			if (name === "submenu-recentfiles") {
				name = "manage_files";
				label = "Manage files";
				type = "normal";
			}
			menu.submenu!.push({
				label,
				id: name,
				accelerator: this._convertAccelerator(shortcut),
				type,
				submenu: [],
			});
			if (type === "submenu")
				this.currentSubMenu = menu.submenu![menu.submenu!.length - 1];
			else this.actions[name] = { name, label, shortcut };
			if (name === "save") {
				menu.submenu!.push({
					id: "download",
					label: "Download",
					type: "normal",
				});
				this.actions.download = {
					name: "download",
					label: "Download",
					shortcut: null,
				};
			}
		}
	}

	setSubmenu(
		name: string,
		action: string,
		type: MainMenuItemType,
		items: string[],
	) {
		const submenu: MainMenuItemOptions[] = [];
		for (const s of items) {
			submenu.push({
				label: s,
				id: `${action}${s}`,
				type,
			});
		}
		for (const rootItem of this.mainMenu) {
			for (const item of rootItem.submenu as MainMenuItemOptions[]) {
				if (item.id !== name) continue;
				item.submenu = submenu;
			}
		}
		if (window.ipc != null) {
			// TODO: call setup menu only once, at next event loop iteration
			this.setupMenu();
		}
	}

	async showPopupMenu(x: number, y: number, items: PopupItemOptions[]) {
		if (window.ipc != null) this.resume(await window.ipc.popupMenu(items));
		else this.popupMenu.openPopup(x, y, items, (result) => this.resume(result));
	}

	// ------------------------------------------------------------------------------------

	async preloadFile(fname: string, tmpname: string) {
		if (window.ipc == null)
			throw Error("preloadFile called in environment without file system");
		console.log("preloading", fname, tmpname);
		const data = await window.ipc.loadFile(fname);
		this.ipe.FS.writeFile(tmpname, data);
		this.preloadCache[fname] = tmpname;
		console.log("Preload cache: ", this.preloadCache);
		this.resume(null);
	}

	async persistFile(fname: string) {
		if (window.ipc) {
			const tmpname = this.preloadCache[fname];
			if (tmpname == null) throw new Error("Persisting non-existing file.");
			console.log("persisting", fname, tmpname);
			const data = this.ipe.FS.readFile(tmpname);
			await window.ipc.saveFile(fname, data);
			this.resume(true);
		} else if (this.saveCallback != null) {
			this.saveCallback(fname);
		}
	}

	// ------------------------------------------------------------------------------------

	async waitDialog(cmd: string, label: string) {
		this.modal.header.innerText = "Ipe: waiting";
		this.modal.body.innerText = label;
		this.modal.show();
		const [op, arg] = cmd.split(":");
		if (op === "runlatex") {
			const texfile = this.ipe.FS.readFile("/tmp/latexrun/ipetemp.tex", {
				encoding: "utf8",
			});
			// TODO: even in this case user may want to use online latex service
			if (window.ipc != null) {
				// in case we have access to a local latex installation
				const { log, pdf } = await window.ipc.runlatex(arg, texfile);
				this.ipe.FS.writeFile("/tmp/latexrun/ipetemp.log", log);
				if (pdf != null)
					this.ipe.FS.writeFile("/tmp/latexrun/ipetemp.pdf", pdf);
				this.modal.close(null);
			} else {
				const tarFile = this.ipe.Emval.toValue(
					this.ipe._createTarball(this.ipe.stringToNewUTF8(texfile)),
				);
				const tarBlob = new Blob([tarFile], {
					type: "application/octet-stream",
				});
				const host = import.meta.env.DEV
					? "http://localhost:5000"
					: `${window.location.protocol}//${window.location.hostname}`;
				const url = `${host}/data?target=ipetemp.tex&command=${arg}`;
				const form = new FormData();
				form.append("file", tarBlob, "latexTarball.tar");
				try {
					const response = await fetch(url, {
						method: "POST",
						headers: {
							"User-Agent": "Ipe 7.3.1",
						},
						body: form,
					});
					if (response.ok) {
						const pdf = new Uint8Array(await response.arrayBuffer());
						const log = `entering extended mode: using latexonline at ${host}\n`;
						this.ipe.FS.writeFile("/tmp/latexrun/ipetemp.pdf", pdf);
						this.ipe.FS.writeFile("/tmp/latexrun/ipetemp.log", log);
					} else {
						const log = await response.text();
						this.ipe.FS.writeFile("/tmp/latexrun/ipetemp.log", log);
					}
				} catch (err) {
					console.error("Latex online fetch failed: ", err);
				} finally {
					this.modal.close(null);
				}
			}
		} else throw new Error(`Unsupported operation: ${op}`);
	}

	async messageBox(options: MessageBoxOptions) {
		if (window.ipc != null) {
			// TODO: add option to use inline messagebox
			this.resume(await window.ipc.messageBox(options));
		} else {
			this.modal.messageBox(options);
		}
	}

	async showDialog(options: DialogOptions) {
		this.modal.showDialog(options);
	}

	dialogRetrieveValues(options: DialogOptions) {
		return retrieveValues(options);
	}

	dialogSetEnabled(name: string, enabled: boolean) {
		const el = document.getElementById(`dialog-element-${name}`);
		if (enabled) el?.removeAttribute("disabled");
		else el?.setAttribute("disabled", "disabled");
	}

	dialogSet(element: ElementOptions) {
		setElement(element);
	}

	async fileDialog(options: FileDialogOptions) {
		if (window.ipc != null) {
			this.resume(await window.ipc.fileDialog(options));
		} else {
			this.modal.fileDialog(options);
		}
	}

	download(): void {
		if (this.filename != null)
			this.modal.fileDownload("documents", this.filename);
	}

	// ------------------------------------------------------------------------------------

	async setClipboard(data: string) {
		if (window.ipc != null) {
			window.ipc.setClipboard(data);
		} else {
			navigator.clipboard.writeText(data);
		}
	}

	async getClipboard(allowBitmap: boolean) {
		if (window.ipc != null) {
			this.resume(await window.ipc.getClipboard(allowBitmap));
		} else {
			this.resume(await navigator.clipboard.readText());
		}
	}

	// ------------------------------------------------------------------------------------

	// TODO: macOS can use the actual filename and whether the file has been modified
	// (documentEdited and representedFilename on BrowserWindow)
	setTitle(_mod: boolean, caption: string, fn: string): void {
		document.title = caption;
		if (!fn.startsWith("/home/ipe/documents/")) {
			this.filename = null;
			window.history.replaceState({}, document.title, "/index.html");
		} else {
			const i = fn.lastIndexOf("/");
			if (i >= 0) fn = fn.substring(i + 1);
			this.filename = fn;
			window.history.replaceState({}, document.title, `/index.html?open=${fn}`);
		}
	}

	setToolVisible(tool: number, vis: boolean) {
		const panels = [
			get("propertiesPanel"),
			get("bookmarksPanel"),
			get("notesPanel"),
			get("layersPanel"),
		];
		panels[tool]!.style.display = vis ? "flex" : "none";
		const rightDock = get("rightDock");
		rightDock.style.display =
			panels[1].style.display === "none" && panels[2].style.display === "none"
				? "none"
				: "flex";
		// update canvas once browser has rerendered
		setTimeout(() => {
			this._calculateCanvasSize();
			this.ipe._canvasUpdateSize();
			this.ipe._canvasUpdate();
		}, 0);
	}

	resetCombos() {
		for (const sel of this.selectorNames) {
			const el = get(sel);
			if (el) removeChildren(el);
		}
	}

	addCombo(sel: number, s: string) {
		const el = get(this.selectorNames[sel]);
		if (el) {
			const option = document.createElement("option");
			option.innerText = s;
			el.appendChild(option);
		}
	}

	setComboCurrent(sel: number, idx: number) {
		(get(this.selectorNames[sel]) as HTMLSelectElement).selectedIndex = idx;
	}

	addComboColors(colors: { name: string; rgb: Color }[]) {
		const stroke = get("stroke") as HTMLSelectElement;
		const fill = get("fill") as HTMLSelectElement;
		const addOption = (el: HTMLSelectElement, name: string, color?: Color) => {
			const option = document.createElement("option");
			option.innerText = name;
			if (color) option.style.color = toRgb(color);
			el.appendChild(option);
		};
		addOption(stroke, "<absolute>");
		addOption(fill, "<absolute>");
		for (const item of colors) {
			addOption(stroke, item.name, item.rgb);
			addOption(fill, item.name, item.rgb);
		}
	}

	setButtonColor(sel: number, color: Color) {
		const el = get(`abs-${this.selectorNames[sel]}`);
		if (el) {
			const el1 = document.createElement("span");
			el1.style.backgroundColor = toRgb(color);
			el1.innerHTML = "&nbsp;&nbsp;&nbsp;&nbsp;";
			if (el.firstChild) el.removeChild(el.firstChild);
			el.appendChild(el1);
		}
	}

	setActionState(action: string, checked: boolean) {
		this.actionState[action] = checked;
		const el = document.getElementById(action);
		if (el) {
			if (checked) el.classList.add("selected");
			else el.classList.remove("selected");
		}
		this._setActionStateMark(action, checked);
	}

	setNumbers(vno: string, vm: boolean, pno: string, pm: boolean): void {
		if (vno === "" && pno === "") {
			get("pageNumberPanel").style.display = "none";
		} else {
			get("pageNumberPanel").style.display = "flex";
			get("viewNumber").innerText = vno;
			(get("viewMarked") as HTMLInputElement).checked = vm;
			get("pageNumber").innerText = pno;
			(get("pageMarked") as HTMLInputElement).checked = pm;
		}
	}

	setLayers(layers: Layer[]) {
		const lb = get("layerbox");
		removeChildren(lb);
		for (const layer of layers) {
			const div = document.createElement("div");
			div.setAttribute("id", layer.name);
			div.classList.add("layer");
			const cb = document.createElement("input");
			cb.setAttribute("type", "checkbox");
			if (layer.checked) cb.setAttribute("checked", "checked");
			div.appendChild(cb);
			const span = document.createElement("span");
			span.classList.add("layer-text");
			span.innerText = layer.text;
			div.appendChild(span);
			const dragHandle = document.createElement("span");
			dragHandle.innerText = "+";
			dragHandle.classList.add("drag-handle");
			div.appendChild(dragHandle);
			if (layer.active) div.classList.add("active");
			if (layer.locked) div.classList.add("locked");
			if (layer.snap === "always") div.classList.add("snap-always");
			else if (layer.snap === "never") div.classList.add("snap-never");
			div.addEventListener("contextmenu", (event) => {
				event.preventDefault();
				this.ipe._showLayerBoxPopup(
					this.ipe.stringToNewUTF8(layer.name),
					event.clientX,
					event.clientY,
				);
			});
			span.addEventListener("click", (event) => {
				event.preventDefault();
				if (this.actionState.context_menu) {
					this.setActionState("context_menu", false);
					this.ipe._showLayerBoxPopup(
						this.ipe.stringToNewUTF8(layer.name),
						event.clientX,
						event.clientY,
					);
				} else {
					this.ipe._layerAction(
						this.ipe.stringToNewUTF8("active"),
						this.ipe.stringToNewUTF8(layer.name),
					);
				}
			});
			cb.addEventListener("change", (_event) => {
				this.ipe._layerAction(
					this.ipe.stringToNewUTF8(cb.checked ? "selecton" : "selectoff"),
					this.ipe.stringToNewUTF8(layer.name),
				);
			});
			lb.appendChild(div);
		}
	}

	setBookmarks(bookmarks: string[]) {
		const bm = get("bookmarks");
		removeChildren(bm);
		for (let i = 0; i < bookmarks.length; ++i) {
			const div = document.createElement("div");
			div.setAttribute("id", `bookmark-${i}`);
			div.innerText = bookmarks[i].trim();
			div.classList.add("bookmark");
			if (bookmarks[i].startsWith(" ")) div.classList.add("subsection");
			div.addEventListener("click", (_e) => this.ipe._bookmarkSelected(i));
			bm.appendChild(div);
		}
	}

	selectPage(
		caption: string,
		items: PageSelectorItem[],
		sorter: boolean,
	): void {
		this.modal.pageSelector(caption, items, sorter, this.popupMenu);
	}

	setActionsEnabled(enabled: boolean): void {
		const enableMenu = (menu: string) => {
			(get(`menu-${menu}`) as HTMLButtonElement).disabled = !enabled;
		};
		enableMenu("file");
		enableMenu("edit");
		enableMenu("mode");
		enableMenu("properties");
		enableMenu("layers");
		enableMenu("views");
		enableMenu("pages");
		enableMenu("ipelets");

		const enablePanel = (s: string) => {
			const p = get(s);
			p.style.pointerEvents = enabled ? "auto" : "none";
		};
		enablePanel("propertiesPanel");
		enablePanel("layersPanel");
		enablePanel("bookmarksPanel");
	}
}

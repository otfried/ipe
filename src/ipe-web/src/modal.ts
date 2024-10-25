import {
	ACCEPT,
	CANCEL,
	type DialogOptions,
	type DialogResult,
	setupButtons,
	setupElements,
	setupMessageBoxButtons,
} from "./dialogs";
import type { DialogId, Ipe, ResumeResult } from "./ipejs";
import { get, removeChildren } from "./util";

import "./modal.css";
import Sortable from "sortablejs";
import type { PopupMenu, PopupMenuResults } from "./popup-menu";

type Directory = "documents" | "images" | "styles" | "ipelets";

interface FileType {
	dir: Directory;
	mimeType: string;
}

const knownFileTypes: { [ext: string]: FileType } = {
	pdf: { dir: "documents", mimeType: "application/pdf" },
	ipe: { dir: "documents", mimeType: "application/xml" },
	png: { dir: "images", mimeType: "image/png" },
	jpg: { dir: "images", mimeType: "image/jpeg" },
	eps: { dir: "images", mimeType: "application/octet-stream" },
	svg: { dir: "images", mimeType: "image/svg+xml" },
	isy: { dir: "styles", mimeType: "application/xml" },
	lua: { dir: "ipelets", mimeType: "text/plain" },
};

type ResumeCallback = (result: ResumeResult) => void;

export interface MessageBoxOptions {
	type: "none" | "warning" | "information" | "question" | "critical";
	text: string; // displayed as the heading
	details: string; // displayed in the message box
	buttons: number; // ipeui_common.h
}

export interface FileDialogOptions {
	type: "open" | "save";
	caption: string;
	filters: string[];
	dir: string;
	path: string;
	selected: number; // the selected filter
}

export interface PageSelectorItem {
	label: string;
	marked: boolean;
}

function dirFromOptions(options: FileDialogOptions): Directory {
	const f = options.filters[0];
	if (f.includes("*.ipe")) return "documents";
	if (f.includes("*.isy")) return "styles";
	if (f.includes("*.lua")) return "ipelets";
	return "images";
}

export class Modal {
	readonly ipe: Ipe;
	readonly pane: HTMLElement;
	readonly header: HTMLDivElement;
	readonly body: HTMLDivElement;
	readonly footer: HTMLDivElement;
	readonly file: HTMLDivElement;
	readonly file_manager: HTMLDivElement;
	readonly file_error: HTMLDivElement;
	inModal = false;
	inFileDialog = false;
	inPageSelector = false;
	dialogId: DialogId | null = null;
	ctrlEnterHandler: (() => void) | null = null;
	private readonly resumeCallback: ResumeCallback;

	constructor(ipe: Ipe, cb: ResumeCallback) {
		this.ipe = ipe;
		this.resumeCallback = cb;
		this.pane = get("modal-dialog");
		this.header = get("modal-header-content") as HTMLDivElement;
		this.body = get("modal-body") as HTMLDivElement;
		this.footer = get("modal-footer") as HTMLDivElement;
		this.file = get("modal-file") as HTMLDivElement;
		this.file_manager = get("modal-file-manager") as HTMLDivElement;
		this.file_error = get("filedialog-error") as HTMLDivElement;
		get("modal-close").addEventListener("click", () => this.close(CANCEL));
		get("upload-button").addEventListener("click", () =>
			this._fileUpload("upload"),
		);
		get("file-manager-upload-button").addEventListener("click", () =>
			this._fileUpload("file-manager-upload"),
		);
	}

	show() {
		this.pane.style.display = "block";
		this.inModal = true;
	}

	close(result: ResumeResult) {
		if (result === CANCEL && (this.inFileDialog || this.inPageSelector))
			result = null;
		this.ctrlEnterHandler = null;
		this.file_error.style.display = "none";
		this.body.style.display = "block";
		this.file.style.display = "none";
		this.pane.style.display = "none";
		this.file_manager.style.display = "none";
		this.inModal = false;
		this.inFileDialog = false;
		this.inPageSelector = false;
		this.dialogId = null;
		this.resumeCallback(result);
		setTimeout(() => {
			// give dialog a chance to retrieve values before we destroy the nodes
			removeChildren(this.body);
			removeChildren(this.footer);
		}, 0);
	}

	keyPressEvent(event: KeyboardEvent): boolean {
		if (this.inModal) {
			if (event.key === "Escape") {
				if (this.dialogId && this.ipe._dialogIgnoresEscapeKey(this.dialogId))
					return true;
				this.close(CANCEL);
			}
			if (event.key === "Enter" && event.ctrlKey && this.ctrlEnterHandler)
				this.ctrlEnterHandler();
			return true;
		}
		return false;
	}

	// ------------------------------------------------------------------------------------

	showBanner(header: string, body: string) {
		this.header.innerText = header;
		this.body.innerHTML = body;
		this.show();
	}

	messageBox(options: MessageBoxOptions) {
		this.header.innerText = options.text;
		this.body.innerText = options.details;
		setupMessageBoxButtons(
			this.footer,
			options.buttons,
			(result: DialogResult) => this.close(result),
		);
		this.show();
		this.ctrlEnterHandler = () => this.close(ACCEPT);
	}

	showDialog(options: DialogOptions) {
		this.dialogId = options.dialogId;
		this.header.innerText = options.caption;
		const focus = setupElements(this.ipe, this.body, options);
		setupButtons(this.footer, options, (result) => this.close(result));
		this.ctrlEnterHandler = () => this.close(ACCEPT);
		if (focus) setTimeout(() => focus.focus(), 0);
		this.show();
	}

	// ------------------------------------------------------------------------------------

	private _makeFileListing(
		div: HTMLDivElement,
		dir: Directory,
		manager: boolean,
		cb?: (fn: string) => void,
	) {
		removeChildren(div);
		const files = this.ipe.FS.readdir(`/home/ipe/${dir}`);
		let empty = true;
		for (const file of files) {
			if ([".", ".."].includes(file)) continue;
			const el = document.createElement("div");
			if (cb) {
				el.innerText = file;
				if (cb) el.addEventListener("click", () => cb(file));
			} else {
				const s1 = document.createElement("span");
				s1.innerText = file;
				el.appendChild(s1);
				if (manager) {
					const s2 = document.createElement("button");
					s2.innerHTML = "<u>⇩</u>";
					s2.classList.add("file-manager-action");
					s2.addEventListener("click", () => this.fileDownload(dir, file));
					const s3 = document.createElement("button");
					s3.innerText = "❌";
					s3.classList.add("file-manager-action");
					s3.addEventListener("click", () => this._fileDelete(dir, file));
					el.appendChild(s2);
					el.appendChild(s3);
				}
			}
			div.appendChild(el);
			empty = false;
		}
		if (empty) {
			const el = document.createElement("span");
			el.innerText = "No files";
			div.appendChild(el);
		}
	}

	async fileDialog(options: FileDialogOptions) {
		const dir = dirFromOptions(options);
		this.body.style.display = "none";
		this.file.style.display = "flex";
		const fileList = get("directory") as HTMLDivElement;
		const fileLabel = get("filedialog-label");
		const fileName = get("filename") as HTMLInputElement;
		const uploadPanel = get("filedialog-upload");
		if (options.type === "open") {
			this.header.innerText = "Open file";
			fileLabel.innerText = "Select a file";
			fileName.style.display = "none";
			uploadPanel.style.display = "block";
			this._makeFileListing(fileList, dir, false, (fn) => {
				this.close(`/home/ipe/${dir}/${fn}`);
			});
		} else {
			this.header.innerText = "Save file";
			fileLabel.innerText = "Enter a filename";
			fileName.style.display = "block";
			fileName.value = "";
			uploadPanel.style.display = "none";
			this._makeFileListing(fileList, dir, false);
			setupMessageBoxButtons(this.footer, 1, (result) =>
				this._acceptSave(dir, result),
			);
		}
		this.inFileDialog = true;
		this.show();
	}

	private _acceptSave(dir: Directory, result: DialogResult): void {
		if (result !== ACCEPT) this.close(null);
		const fileName = get("filename") as HTMLInputElement;
		const f = fileName.value;
		if (!f.includes("/")) {
			const j = f.lastIndexOf(".");
			if (j > 0 && knownFileTypes[f.substring(j + 1)] != null) {
				this.close(`/home/ipe/${dir}/${f}`);
				return;
			}
		}
		this.file_error.style.display = "block";
	}

	async fileManager() {
		this.header.innerText = "Manage your files";
		this.body.style.display = "none";
		this.file_manager.style.display = "flex";
		for (const d of ["documents", "images", "styles", "ipelets"])
			this._populateFileManager(d as Directory);
		this.show();
	}

	private _populateFileManager(dir: Directory) {
		const fileList = get(`directory-${dir}`) as HTMLDivElement;
		removeChildren(fileList);
		this._makeFileListing(fileList, dir, true);
	}

	private _fileDelete(dir: Directory, file: string) {
		this.ipe.FS.unlink(`/home/ipe/${dir}/${file}`);
		this._populateFileManager(dir);
	}

	async fileDownload(dir: Directory, file: string) {
		const i = file.lastIndexOf(".");
		let mt = "application/octet-stream";
		if (i >= 0 && knownFileTypes[file.substring(i + 1)])
			mt = knownFileTypes[file.substring(i + 1)].mimeType;
		const data = this.ipe.FS.readFile(`/home/ipe/${dir}/${file}`);
		const blob = new Blob([data.buffer], { type: mt });
		const objectUrl = URL.createObjectURL(blob);
		const link = document.getElementById(
			"file-manager-download-link",
		) as HTMLAnchorElement;
		link.setAttribute("href", objectUrl);
		link.setAttribute("download", file);
		link.click();
	}

	private async _fileUpload(id: string) {
		const fs = get(id) as HTMLInputElement;
		if (!fs.files?.length) return;
		const selectedFile = fs.files[0];
		const name = selectedFile.name;
		const i = name.lastIndexOf(".");
		if (i < 0 || knownFileTypes[name.substring(i + 1)] == null) {
			console.error("Unknown file type: ", name);
			return;
		}
		const ft = knownFileTypes[name.substring(i + 1)];
		const path = `/home/ipe/${ft.dir}/${name}`;
		const data = new Uint8Array(await selectedFile.arrayBuffer());
		this.ipe.FS.writeFile(path, data);
		if (id === "upload") {
			this.close(path);
		} else {
			this._populateFileManager(ft.dir);
		}
	}

	pageSelector(
		caption: string,
		items: PageSelectorItem[],
		sorter: boolean,
		popupMenu: PopupMenu,
	): void {
		this.inPageSelector = true;
		this.header.innerText = caption;
		const contents = document.createElement("div");
		contents.setAttribute("id", "contents");
		contents.classList.add("modal-pageselector");
		this.body.appendChild(contents);
		for (let i = 0; i < items.length; ++i) {
			const item = document.createElement("div");
			item.classList.add("modal-page");
			item.setAttribute("id", `select-${i}`);
			const image = document.createElement("img");
			image.classList.add("modal-pageimage");
			item.appendChild(image);
			const data = this.ipe.FS.readFile(`/tmp/pages/select-${i}.png`);
			const blob = new Blob([data.buffer], { type: "image/png" });
			image.setAttribute("src", URL.createObjectURL(blob));
			const label = document.createElement("div");
			label.classList.add("modal-pagelabel");
			label.innerText = items[i].label;
			item.appendChild(label);
			if (!sorter) {
				item.addEventListener("click", () => this.close(i + 1));
			} else {
				if (items[i].marked) item.classList.add("marked");
				item.addEventListener("contextmenu", (event) =>
					this._sorterContextMenu(event, item, popupMenu),
				);
			}
			contents.appendChild(item);
		}
		if (sorter) {
			Sortable.create(contents);
			setupMessageBoxButtons(this.footer, 1, (result) =>
				this._handlePageSorterAccept(result, items, contents),
			);
		}
		this.show();
	}

	private _sorterContextMenu(
		event: MouseEvent,
		item: HTMLDivElement,
		popupMenu: PopupMenu,
	): void {
		event.preventDefault();
		const menu = [{ name: "delete", label: "Delete" }];
		if (item.classList.contains("marked"))
			menu.push({ name: "unmark", label: "Unmark" });
		else menu.push({ name: "mark", label: "Mark" });
		popupMenu.openPopup(event.clientX, event.clientY, menu, (result) =>
			this._handleSorterPopupResult(result, item),
		);
	}

	private _handleSorterPopupResult(
		result: PopupMenuResults,
		item: HTMLDivElement,
	) {
		switch (result[0]) {
			case "delete":
				item.remove();
				break;
			case "mark":
				item.classList.add("marked");
				break;
			case "unmark":
				item.classList.remove("marked");
				break;
		}
	}

	private _handlePageSorterAccept(
		result: ResumeResult,
		items: PageSelectorItem[],
		contents: HTMLDivElement,
	): void {
		if (result !== 1) {
			this.close(null);
		} else {
			const pages = [];
			const marks = items.map((item) => item.marked);
			for (const child of contents.children) {
				const oldIdx = Number.parseInt(child.id.substring(7));
				marks[oldIdx] = child.classList.contains("marked");
				pages.push(oldIdx + 1);
			}
			this.close([pages, marks]);
		}
	}
}

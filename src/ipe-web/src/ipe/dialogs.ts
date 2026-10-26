import "@melloware/coloris/dist/coloris.css";
import Coloris from "@melloware/coloris";

import Sortable from "sortablejs";
import "./dialogs.css";
import type { DialogId, Ipe } from "./ipejs";
import type { PopupMenu } from "./popup-menu";
import { removeChildren } from "./util";

Coloris.init();
Coloris({ el: "#coloris" });
Coloris.close();

export const ACCEPT = 1;
export const REJECT = 0;
export const CANCEL = -1;
export type DialogResult = 0 | 1 | -1;

type DialogElement =
	| "input"
	| "combo"
	| "label"
	| "textedit"
	| "list"
	| "checkbox"
	| "button"
	| "image"
	| "sorter";

export interface ImageOptions {
	kind: "none" | "text" | "file" | "bitmap";
	text: string; // message ("text") or path in ipe's virtual FS ("file")
	zoom: number;
	width: number; // "bitmap" only
	height: number;
	pixels?: Uint8Array; // "bitmap" only: RGBA quadruples in row order, not premultiplied
}

// one item of a sorter element
export interface SorterItemOptions {
	hasMark: boolean;
	mark: boolean; // only used if hasMark
	label: string;
	image: ImageOptions;
}

export interface ElementOptions {
	name: string;
	type: DialogElement;
	text: string;
	flags: number;
	value: number;
	items?: string[];
	method: number | null;
	row: number;
	col: number;
	rowspan: number;
	colspan: number;
	width: number;
	height: number;
	image?: ImageOptions;
	sorter?: SorterItemOptions[];
}

interface ButtonOptions {
	name: string;
	flags: number;
}

export interface DialogOptions {
	dialogId: DialogId;
	caption: string;
	elements: ElementOptions[];
	buttons: ButtonOptions[];
	rowstretch: number[];
	colstretch: number[];
}

function updateListSelection(w: ElementOptions, idx: number): void {
	for (let i = 0; i < w.items!.length; ++i) {
		const el = document.getElementById(`dialog-element-${w.name}-item-${i}`);
		if (i === idx) el?.classList.add("selected");
		else el?.classList.remove("selected");
	}
}

// read the DOM, since the items of a list can be replaced after the dialog was built
function listItemElements(name: string): HTMLElement[] {
	const el = document.getElementById(`dialog-element-${name}`);
	return el
		? Array.from(el.querySelectorAll<HTMLElement>(".dialog-list-item"))
		: [];
}

function getListSelection(w: ElementOptions): number {
	const idx = listItemElements(w.name).findIndex((el) =>
		el.classList.contains("selected"),
	);
	return idx < 0 ? 0 : idx;
}

// Move the selection of the dialog's (first) list, as if the user had clicked.
// Returns false if there is no non-empty list.
function moveListSelection(options: DialogOptions, delta: number): boolean {
	const lw = options.elements.find((e) => e.type === "list");
	if (!lw) return false;
	const items = listItemElements(lw.name);
	if (items.length === 0) return false;
	const cur = getListSelection(lw);
	const next = Math.max(0, Math.min(items.length - 1, cur + delta));
	if (next !== cur) {
		for (const [i, el] of items.entries())
			el.classList.toggle("selected", i === next);
		items[next].scrollIntoView({ block: "nearest" });
		document
			.getElementById(`dialog-element-${lw.name}`)!
			.dispatchEvent(new Event("change"));
	}
	return true;
}

function setListItems(el1: HTMLDivElement, w: ElementOptions): void {
	removeChildren(el1);
	for (let i = 0; i < w.items!.length; ++i) {
		const el2 = document.createElement("span");
		el2.classList.add("dialog-list-item");
		el2.innerHTML = w.items![i];
		el2.id = `dialog-element-${w.name}-item-${i}`;
		if (i === w.value) el2.classList.add("selected");
		el2.addEventListener("click", () => {
			updateListSelection(w, i);
			el1.dispatchEvent(new Event("change"));
		});
		el1.appendChild(el2);
	}
}

// ------------------------------------------------------------------------------------
// The sorter element: icons with an optional label and checkbox.  The user can
// select items; unless the sorter is readonly, they can also delete, cut, insert,
// mark, and rearrange them (by dragging, all selected items move together).

const SORTER_ITEM = ".dialog-sorter-item";
const FLAG_READONLY = 0x010;

class DialogSorter {
	readonly el: HTMLDivElement;
	private readonly readOnly: boolean;
	private readonly popupMenu: PopupMenu;
	private readonly sortable: Sortable | null = null;
	private cut: HTMLElement[] = [];
	private anchor: HTMLElement | null = null;
	private collapseOnClick: HTMLElement | null = null;
	private dragGroup: HTMLElement[] = [];

	constructor(w: ElementOptions, popupMenu: PopupMenu) {
		this.readOnly = (w.flags & FLAG_READONLY) !== 0;
		this.popupMenu = popupMenu;
		const el = document.createElement("div");
		el.id = `dialog-element-${w.name}`;
		el.classList.add("dialog-sorter");
		this.el = el;
		el.addEventListener("pointerdown", (e) => this.pointerDown(e));
		el.addEventListener("click", (e) => this.click(e));
		if (!this.readOnly) {
			el.addEventListener("contextmenu", (e) => this.contextMenu(e));
			this.sortable = Sortable.create(el, {
				draggable: SORTER_ITEM,
				filter: "input",
				preventOnFilter: false,
				animation: 150,
				ghostClass: "dialog-sorter-ghost",
				onStart: (evt) => this.dragStart(evt.item),
				onEnd: (evt) => this.dragEnd(evt.item),
			});
		}
		this.setItems(w.sorter ?? []);
	}

	destroy(): void {
		this.sortable?.destroy();
	}

	// Replace the items.  The numbers of the items refer to the new items.
	setItems(items: SorterItemOptions[]): void {
		this.cut = [];
		this.anchor = null;
		removeChildren(this.el);
		let iconWidth = 32;
		let iconHeight = 32;
		for (const it of items) {
			if (it.image.kind !== "bitmap") continue;
			iconWidth = Math.max(iconWidth, it.image.width);
			iconHeight = Math.max(iconHeight, it.image.height);
		}
		// all items have the size of the largest icon (and room for a short label)
		this.el.style.setProperty(
			"--sorter-item-width",
			`${Math.max(iconWidth + 16, 90)}px`,
		);
		this.el.style.setProperty("--sorter-icon-height", `${iconHeight}px`);
		items.forEach((it, i) => {
			this.el.appendChild(this.makeItem(it, i + 1));
		});
	}

	// The numbers of the items in their current order, negated if marked.
	// For a readonly sorter, the numbers of the selected items.
	order(): number[] {
		const result: number[] = [];
		for (const item of this.items()) {
			const n = Number.parseInt(item.dataset.number ?? "0", 10);
			if (this.readOnly) {
				if (item.classList.contains("selected")) result.push(n);
			} else {
				const check = item.querySelector<HTMLInputElement>("input");
				result.push(check?.checked ? -n : n);
			}
		}
		return result;
	}

	private makeItem(it: SorterItemOptions, number: number): HTMLDivElement {
		const item = document.createElement("div");
		item.classList.add("dialog-sorter-item");
		item.dataset.number = `${number}`;
		const icon = document.createElement("div");
		icon.classList.add("dialog-sorter-icon");
		if (it.image.kind === "bitmap" && it.image.pixels != null)
			icon.appendChild(makeBitmapCanvas(it.image));
		item.appendChild(icon);
		const caption = document.createElement("div");
		caption.classList.add("dialog-sorter-caption");
		if (it.hasMark) {
			const check = document.createElement("input");
			check.type = "checkbox";
			check.checked = it.mark;
			// the marks of a readonly sorter are only displayed
			check.disabled = this.readOnly;
			caption.appendChild(check);
		}
		const label = document.createElement("span");
		label.innerText = it.label;
		caption.appendChild(label);
		item.appendChild(caption);
		return item;
	}

	private items(): HTMLElement[] {
		return Array.from(this.el.querySelectorAll<HTMLElement>(SORTER_ITEM));
	}

	private selected(): HTMLElement[] {
		return this.items().filter((i) => i.classList.contains("selected"));
	}

	private clearSelection(): void {
		for (const i of this.selected()) i.classList.remove("selected");
	}

	private selectOnly(item: HTMLElement): void {
		this.clearSelection();
		item.classList.add("selected");
		this.anchor = item;
	}

	private pointerDown(e: PointerEvent): void {
		if (e.button !== 0) return;
		const target = e.target as HTMLElement;
		this.collapseOnClick = null;
		if (target.closest("input")) return; // a checkbox
		const item = target.closest<HTMLElement>(SORTER_ITEM);
		if (!item) {
			// clicking on empty space clears the selection, but not clicking the scrollbar
			if (e.offsetX < this.el.clientWidth && e.offsetY < this.el.clientHeight)
				this.clearSelection();
			return;
		}
		if (e.ctrlKey || e.metaKey) {
			item.classList.toggle("selected");
			this.anchor = item;
		} else if (e.shiftKey && this.anchor?.isConnected) {
			const all = this.items();
			const a = all.indexOf(this.anchor);
			const b = all.indexOf(item);
			this.clearSelection();
			for (const i of all.slice(Math.min(a, b), Math.max(a, b) + 1))
				i.classList.add("selected");
		} else if (!item.classList.contains("selected")) {
			this.selectOnly(item);
		} else {
			// the item is part of the selection: this may be the start of a drag
			// of the whole selection, so only select it alone on mouse release
			this.collapseOnClick = item;
		}
	}

	private click(e: MouseEvent): void {
		const item = (e.target as HTMLElement).closest<HTMLElement>(SORTER_ITEM);
		if (item && item === this.collapseOnClick) this.selectOnly(item);
		this.collapseOnClick = null;
	}

	private dragStart(item: HTMLElement): void {
		this.collapseOnClick = null;
		this.dragGroup = item.classList.contains("selected")
			? this.selected()
			: [item];
		for (const i of this.dragGroup)
			if (i !== item) i.classList.add("dialog-sorter-dragged");
	}

	// Sortable has moved the dragged item; the other selected items follow it.
	private dragEnd(item: HTMLElement): void {
		const group = this.dragGroup;
		this.dragGroup = [];
		for (const i of group) i.classList.remove("dialog-sorter-dragged");
		const at = group.indexOf(item);
		if (at < 0) return;
		for (let i = at - 1; i >= 0; --i)
			this.el.insertBefore(group[i], group[i + 1]);
		for (let i = at + 1; i < group.length; ++i)
			this.el.insertBefore(group[i], group[i - 1].nextSibling);
	}

	private contextMenu(e: MouseEvent): void {
		e.preventDefault();
		const hit = (e.target as HTMLElement).closest<HTMLElement>(SORTER_ITEM);
		// the item under the pointer is selected, unless it was already
		if (hit && !hit.classList.contains("selected")) this.selectOnly(hit);
		const sel = this.selected();
		const checks = sel
			.map((i) => i.querySelector<HTMLInputElement>("input"))
			.filter((c): c is HTMLInputElement => c != null);
		const menu: { name: string; label: string }[] = [];
		if (sel.length > 0) {
			menu.push({ name: "delete", label: "Delete" });
			menu.push({ name: "cut", label: "Cut" });
			if (checks.some((c) => !c.checked))
				menu.push({ name: "mark", label: "Mark" });
			if (checks.some((c) => c.checked))
				menu.push({ name: "unmark", label: "Unmark" });
		}
		if (this.cut.length > 0) menu.push({ name: "insert", label: "Insert" });
		if (menu.length === 0) return;
		this.popupMenu.openPopup(e.clientX, e.clientY, menu, (result) => {
			if (result) this.menuAction(result[0], sel, checks, hit);
		});
	}

	private menuAction(
		name: string,
		sel: HTMLElement[],
		checks: HTMLInputElement[],
		hit: HTMLElement | null,
	): void {
		switch (name) {
			case "delete":
				for (const i of sel) i.remove();
				break;
			case "cut":
				this.cut = sel;
				for (const i of sel) {
					i.classList.remove("selected");
					i.remove();
				}
				break;
			case "insert": {
				// before the item that was clicked, or at the end
				const before = hit?.isConnected ? hit : null;
				this.clearSelection();
				for (const i of this.cut) {
					this.el.insertBefore(i, before);
					i.classList.add("selected");
				}
				this.cut = [];
				break;
			}
			case "mark":
			case "unmark":
				for (const c of checks) c.checked = name === "mark";
				break;
		}
	}
}

const sorters = new Map<HTMLElement, DialogSorter>();

// forget the sorters of dialogs that have been closed
function discardOldSorters(): void {
	for (const [el, s] of sorters) {
		if (el.isConnected) continue;
		s.destroy();
		sorters.delete(el);
	}
}

export function retrieveValues(options: DialogOptions) {
	const values: { [key: string]: string | number | boolean | number[] } = {};
	for (const w of options.elements) {
		const el = document.getElementById(`dialog-element-${w.name}`);
		switch (w.type) {
			case "checkbox":
				values[w.name] = (el as HTMLInputElement).checked;
				break;
			case "input":
				values[w.name] = (el as HTMLInputElement).value;
				break;
			case "textedit":
				values[w.name] = (el as HTMLTextAreaElement).value;
				break;
			case "combo":
				values[w.name] = Number.parseInt(
					(el as HTMLSelectElement).value.substring(5),
					10,
				);
				break;
			case "list":
				values[w.name] = getListSelection(w);
				break;
			case "sorter": {
				const sorter = el ? sorters.get(el) : undefined;
				if (sorter) values[w.name] = sorter.order();
				break;
			}
		}
	}
	return values;
}

export function setElement(ipe: Ipe, w: ElementOptions): void {
	const el = document.getElementById(`dialog-element-${w.name}`);
	console.log(`Setting element: ${w.name} of type ${w.type}`, el);
	if (el == null) return;
	switch (w.type) {
		case "image":
			drawImagePreview(ipe, el as HTMLCanvasElement, w.image);
			break;
		case "checkbox":
			(el as HTMLInputElement).checked = w.value !== 0;
			break;
		case "input":
			(el as HTMLInputElement).value = w.text;
			break;
		case "textedit":
			(el as HTMLTextAreaElement).value = w.text;
			break;
		case "combo":
			(el as HTMLSelectElement).selectedIndex = w.value;
			break;
		case "list":
			setListItems(el as HTMLDivElement, w);
			break;
		case "sorter":
			sorters.get(el)?.setItems(w.sorter ?? []);
			break;
		case "label":
			(el as HTMLSpanElement).innerText = w.text;
			break;
		default:
			console.error("Setting element not yet implemented: ", w);
			break;
	}
}

// a canvas showing a bitmap in memory (kind "bitmap")
function makeBitmapCanvas(image: ImageOptions): HTMLCanvasElement {
	const canvas = document.createElement("canvas");
	canvas.width = image.width;
	canvas.height = image.height;
	if (image.pixels != null) {
		const rgba = new Uint8ClampedArray(image.pixels);
		canvas
			.getContext("2d")
			?.putImageData(new ImageData(rgba, image.width, image.height), 0, 0);
	}
	return canvas;
}

const canvasImages = new WeakMap<HTMLCanvasElement, ImageOptions | undefined>();

// keep the canvas's backing-store resolution in sync with its rendered
// (grid-stretched) size, and redraw whenever that size changes
function observeImageCanvas(ipe: Ipe, canvas: HTMLCanvasElement): void {
	const observer = new ResizeObserver(() => {
		const rect = canvas.getBoundingClientRect();
		const width = Math.max(1, Math.round(rect.width));
		const height = Math.max(1, Math.round(rect.height));
		console.log(
			`Resizing canvas from ${canvas.width}x${canvas.height} to ${width}x${height}`,
		);
		if (canvas.width === width && canvas.height === height) return;
		canvas.width = width;
		canvas.height = height;
		drawImagePreview(ipe, canvas, canvasImages.get(canvas));
	});
	observer.observe(canvas);
}

function drawImagePreview(
	ipe: Ipe,
	canvas: HTMLCanvasElement,
	image: ImageOptions | undefined,
): void {
	const zoom = Math.max(0.1, Math.min(100, image?.zoom ?? 1));
	const ctx = canvas.getContext("2d");
	if (ctx == null) return;
	canvasImages.set(canvas, image);
	// identical specs can be redrawn at different canvas sizes (e.g. after a
	// resize), so the async image load below must not rely on spec equality
	// to detect staleness - use a fresh token for every call instead
	const token = `${Number(canvas.dataset.previewToken ?? "0") + 1}`;
	canvas.dataset.previewToken = token;
	const w = canvas.width;
	const h = canvas.height;
	ctx.clearRect(0, 0, w, h);
	ctx.fillStyle = "rgb(255,255,220)";
	ctx.fillRect(0, 0, w, h);
	ctx.strokeStyle = "rgb(160,160,130)";
	ctx.strokeRect(0.5, 0.5, w - 1, h - 1);
	const left = 18;
	const top = 16;
	const right = w - 18;
	const bottom = h - 16;
	const cx = (left + right) / 2;
	const cy = (top + bottom) / 2;
	ctx.lineCap = "round";
	ctx.lineJoin = "round";
	const showText = (text?: string) => {
		ctx.fillStyle = "rgb(90,90,90)";
		ctx.font = "12px sans-serif";
		ctx.textAlign = "center";
		ctx.textBaseline = "middle";
		ctx.fillText(text || "Preview unavailable", cx, cy);
	};
	const drawScaled = (source: CanvasImageSource, iw: number, ih: number) => {
		const logicalWidth = iw / zoom;
		const logicalHeight = ih / zoom;
		const scale = Math.min(
			1,
			(right - left) / logicalWidth,
			(bottom - top) / logicalHeight,
		);
		const width = logicalWidth * scale;
		const height = logicalHeight * scale;
		ctx.drawImage(source, cx - width / 2, cy - height / 2, width, height);
	};
	if (image?.kind === "bitmap" && image.pixels != null) {
		drawScaled(makeBitmapCanvas(image), image.width, image.height);
		return;
	}
	if (image?.kind === "file") {
		// `image.text` is a path in ipe's virtual FS, not a URL the browser can fetch directly
		let objectUrl: string;
		try {
			const data = ipe.FS.readFile(image.text);
			objectUrl = URL.createObjectURL(
				new Blob([data.buffer], { type: "image/png" }),
			);
		} catch {
			showText();
			return;
		}
		const img = new Image();
		img.addEventListener("load", () => {
			URL.revokeObjectURL(objectUrl);
			if (canvas.dataset.previewToken !== token) return;
			drawScaled(img, img.width, img.height);
		});
		img.addEventListener("error", () => {
			URL.revokeObjectURL(objectUrl);
			if (canvas.dataset.previewToken !== token) return;
			showText();
		});
		img.src = objectUrl;
		return;
	}
	showText(image?.kind === "text" ? image.text : undefined);
}

export function setupElements(
	ipe: Ipe,
	body: HTMLDivElement,
	options: DialogOptions,
	popupMenu: PopupMenu,
): HTMLElement | null {
	discardOldSorters();
	let focusElement: HTMLElement | null = null;
	const contents = document.createElement("div");
	contents.setAttribute("id", "contents");
	body.appendChild(contents);
	let colt = "";
	for (const cols of options.colstretch) {
		colt += `${cols + 1}fr `;
	}
	contents.style.gridTemplateColumns = colt;
	// non-stretched rows are fixed-size, but must be at least as tall
	// as any image or sorter placed in them (the only elements with a
	// meaningful pixel height)
	const rowMinHeight = options.rowstretch.map(() => 30);
	for (const w of options.elements) {
		if ((w.type !== "image" && w.type !== "sorter") || w.row < 0) continue;
		const perRow = Math.ceil(w.height / w.rowspan);
		for (let r = w.row; r < w.row + w.rowspan; ++r)
			rowMinHeight[r] = Math.max(rowMinHeight[r], perRow);
	}
	let rowt = "";
	options.rowstretch.forEach((rows, i) => {
		if (rows > 0) rowt += `${rows}fr `;
		else rowt += `${rowMinHeight[i]}px `;
	});
	contents.style.gridTemplateRows = rowt;
	contents.style.columnGap = "20px";
	contents.style.rowGap = "6px";
	for (const w of options.elements) {
		let el: HTMLElement | null = null;
		switch (w.type) {
			case "label":
				el = document.createElement("span");
				el.innerText = w.text.replace("&", "");
				el.id = `dialog-element-${w.name}`;
				break;
			case "checkbox": {
				const el1 = document.createElement("input");
				el1.type = "checkbox";
				if (w.value) el1.checked = true;
				el1.id = `dialog-element-${w.name}`;
				if (w.flags & 0x020) el1.disabled = true;
				el = document.createElement("label");
				el.innerText = w.text.replace("&", "");
				el.appendChild(el1);
				break;
			}
			case "input": {
				const el1 = document.createElement("input");
				el1.value = w.text;
				el1.id = `dialog-element-${w.name}`;
				if (w.flags & 0x020) el1.disabled = true;
				if (w.flags & 0x400) el1.setAttribute("data-coloris", "true");
				el1.addEventListener("keydown", (e) => {
					if (e.key !== "ArrowUp" && e.key !== "ArrowDown") return;
					if (moveListSelection(options, e.key === "ArrowDown" ? 1 : -1))
						e.preventDefault();
				});
				el = el1;
				break;
			}
			case "textedit": {
				const el1 = document.createElement("textarea");
				el1.value = w.text;
				el1.id = `dialog-element-${w.name}`;
				if (w.flags & 0x10) el1.readOnly = true;
				if (w.flags & 0x020) el1.disabled = true;
				el = el1;
				break;
			}
			case "button": {
				const el1 = document.createElement("button");
				el1.type = "button";
				el1.innerText = w.text.replace("&", "");
				if (w.flags & 0x020) el1.disabled = true;
				el = el1;
				break;
			}
			case "image": {
				const el1 = document.createElement("canvas");
				el1.id = `dialog-element-${w.name}`;
				el1.width = w.width;
				el1.height = w.height;
				drawImagePreview(ipe, el1, w.image);
				observeImageCanvas(ipe, el1);
				el = el1;
				break;
			}
			case "sorter": {
				const sorter = new DialogSorter(w, popupMenu);
				sorters.set(sorter.el, sorter);
				sorter.el.style.minWidth = `${w.width}px`;
				el = sorter.el;
				break;
			}
			case "list": {
				const el1 = document.createElement("div");
				el1.id = `dialog-element-${w.name}`;
				el1.classList.add("dialog-list");
				setListItems(el1, w);
				if (w.flags & 0x020) el1.classList.add("dialog-list-disabled");
				el = el1;
				break;
			}
			case "combo": {
				const el1 = document.createElement("select");
				el1.id = `dialog-element-${w.name}`;
				for (let i = 0; i < w.items!.length; ++i) {
					const el2 = document.createElement("option");
					el2.innerHTML = w.items![i];
					el2.value = `item-${i}`;
					el1.appendChild(el2);
				}
				el1.selectedIndex = w.value;
				if (w.flags & 0x020) el1.disabled = true;
				el = el1;
				break;
			}
			default:
				continue;
		}
		switch (w.type) {
			case "textedit":
			case "list":
			case "image":
			case "sorter":
				el.style.justifySelf = "stretch";
				el.style.alignSelf = "stretch";
				break;
			case "input":
			case "button":
			case "combo":
				el.style.justifySelf = "stretch";
				el.style.alignSelf = "center";
				break;
			default:
				el.style.justifySelf = "start";
				el.style.alignSelf = "center";
				break;
		}
		if (w.flags & 0x100) focusElement = el;
		if (w.method != null) {
			if (w.type === "button")
				el.addEventListener("click", () =>
					ipe._dialogCallLua(options.dialogId, w.method!),
				);
			// "input" fires on every user edit, "change" only on commit
			else
				el.addEventListener(w.type === "input" ? "input" : "change", () =>
					ipe._dialogCallLua(options.dialogId, w.method!),
				);
		}
		el.classList.add("dialog-element");
		el.style.gridRowStart = `${w.row + 1}`;
		el.style.gridRowEnd = `${w.row + w.rowspan + 1}`;
		el.style.gridColumnStart = `${w.col + 1}`;
		el.style.gridColumnEnd = `${w.col + w.colspan + 1}`;
		contents.appendChild(el);
	}
	return focusElement;
}

export function setupButtons(
	footer: HTMLDivElement,
	options: DialogOptions,
	cb: (results: DialogResult) => void,
) {
	const buttons = document.createElement("div");
	buttons.classList.add("dialog-buttons");
	for (const b of options.buttons) {
		const el = document.createElement("button");
		el.classList.add("dialog-button");
		el.type = "button";
		el.innerText = b.name.replace("&", "");
		if (b.flags & 0x008) el.onclick = () => cb(REJECT);
		else if (b.flags & 0x004) el.onclick = () => cb(ACCEPT);
		buttons.appendChild(el);
	}
	footer.appendChild(buttons);
}

const messageBoxButtonMap = [
	["Ok"],
	["Ok", "Cancel"],
	["Yes", "No", "Cancel"],
	["Discard", "Cancel"],
	["Save", "Discard", "Cancel"],
];

const messageBoxResultMap: { [key: string]: DialogResult } = {
	Ok: ACCEPT,
	Yes: ACCEPT,
	Save: ACCEPT,
	No: REJECT,
	Discard: REJECT,
	Cancel: CANCEL,
};

export function setupMessageBoxButtons(
	footer: HTMLDivElement,
	type: number,
	cb: (results: DialogResult) => void,
) {
	if (type === 0) return;
	const buttons = document.createElement("div")!;
	buttons.classList.add("dialog-buttons");
	for (const b of messageBoxButtonMap[type]) {
		const el = document.createElement("button");
		el.classList.add("dialog-button");
		el.type = "button";
		el.innerText = b;
		el.onclick = () => cb(messageBoxResultMap[b]);
		buttons.appendChild(el);
	}
	footer.appendChild(buttons);
}

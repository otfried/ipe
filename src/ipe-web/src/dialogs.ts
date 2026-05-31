import "@melloware/coloris/dist/coloris.css";
import Coloris from "@melloware/coloris";

import "./dialogs.css";
import type { DialogId, Ipe } from "./ipejs";
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
	| "image"
	| "button";

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

function getListSelection(w: ElementOptions): number {
	for (let i = 0; i < w.items!.length; ++i) {
		const el = document.getElementById(`dialog-element-${w.name}-item-${i}`);
		if (el?.classList.contains("selected")) return i;
	}
	return 0;
}

function setListItems(el1: HTMLDivElement, w: ElementOptions): void {
	removeChildren(el1);
	for (let i = 0; i < w.items!.length; ++i) {
		const el2 = document.createElement("span");
		el2.classList.add("dialog-list-item");
		el2.innerHTML = w.items![i];
		el2.id = `dialog-element-${w.name}-item-${i}`;
		if (i === w.value) el2.classList.add("selected");
		el2.addEventListener("click", () => updateListSelection(w, i));
		el1.appendChild(el2);
	}
}

export function retrieveValues(options: DialogOptions) {
	const values: { [key: string]: string | number | boolean } = {};
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
				);
				break;
			case "list":
				values[w.name] = getListSelection(w);
				break;
		}
	}
	return values;
}

export function setElement(w: ElementOptions): void {
	const el = document.getElementById(`dialog-element-${w.name}`);
	if (el == null) return;
	switch (w.type) {
		case "image":
			drawImagePreview(el as HTMLCanvasElement, w.text);
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
		default:
			console.error("Setting element not yet implemented: ", w);
			break;
	}
}

function previewNumber(value: string, fallback: number): number {
	const n = Number.parseFloat(value);
	return Number.isFinite(n) ? n : fallback;
}

function previewColor(value: string): string {
	if (value.startsWith("#")) return value;
	const rgb = value.trim().split(/\s+/).map(Number.parseFloat);
	if (rgb.length === 1) rgb[1] = rgb[2] = rgb[0];
	const [r = 0, g = 0, b = 0] = rgb;
	return `rgb(${Math.max(0, Math.min(255, Math.round(255 * r)))}, ${Math.max(0, Math.min(255, Math.round(255 * g)))}, ${Math.max(0, Math.min(255, Math.round(255 * b)))})`;
}

function previewNamedSize(value: string, fallback: number): number {
	const sizes: Record<string, number> = {
		"\\tiny": 8,
		"\\scriptsize": 9,
		"\\footnotesize": 10,
		"\\small": 12,
		"\\normalsize": 14,
		"\\large": 18,
		"\\Large": 22,
		"\\LARGE": 26,
		"\\huge": 30,
		"\\Huge": 36,
	};
	return sizes[value] ?? previewNumber(value, fallback);
}

function previewDashPattern(value: string): number[] {
	const m = value.match(/\[([^\]]+)\]/);
	if (!m) return [];
	return m[1]
		.trim()
		.split(/\s+/)
		.map(Number.parseFloat)
		.filter(Number.isFinite);
}

function drawImagePreview(canvas: HTMLCanvasElement, spec: string): void {
	const [kind, value = "", zoomText = "1"] = spec.split("|");
	const zoom = Math.max(0.1, Math.min(100, previewNumber(zoomText, 1)));
	const ctx = canvas.getContext("2d");
	if (ctx == null) return;
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
	if (kind === "color") {
		ctx.fillStyle = previewColor(value);
		ctx.fillRect(left + 8, top + 8, right - left - 16, bottom - top - 36);
		ctx.strokeStyle = "black";
		ctx.strokeRect(left + 8, top + 8, right - left - 16, bottom - top - 36);
		ctx.fillStyle = "black";
		ctx.font = "12px sans-serif";
		ctx.textAlign = "center";
		ctx.fillText(value, cx, bottom - 4);
	} else if (kind === "pen" || kind === "dashstyle") {
		ctx.strokeStyle = "rgb(20,40,160)";
		ctx.lineWidth =
			kind === "pen"
				? Math.max(0.1, previewNumber(value, 1) * zoom)
				: Math.max(0.1, 4 * zoom);
		if (kind === "dashstyle")
			ctx.setLineDash(previewDashPattern(value).map((x) => x * zoom));
		else ctx.setLineDash([]);
		ctx.beginPath();
		ctx.moveTo(left, cy);
		ctx.lineTo(right, cy);
		ctx.stroke();
		ctx.setLineDash([]);
	} else if (kind === "textsize") {
		ctx.fillStyle = "rgb(30,30,30)";
		ctx.font = `${Math.max(1, previewNamedSize(value, 18) * zoom)}px sans-serif`;
		ctx.textAlign = "center";
		ctx.textBaseline = "middle";
		ctx.fillText("Sample", cx, cy);
	} else if (kind === "symbolsize") {
		const s = Math.max(1, previewNumber(value, 3) * 3 * zoom);
		ctx.fillStyle = "rgb(230,80,70)";
		ctx.strokeStyle = "rgb(20,40,160)";
		ctx.lineWidth = 2;
		for (const x of [
			left + (right - left) * 0.25,
			cx,
			left + (right - left) * 0.75,
		]) {
			ctx.beginPath();
			ctx.arc(x, cy, s / 2, 0, 2 * Math.PI);
			ctx.fill();
			ctx.stroke();
		}
	} else if (kind === "arrowsize") {
		const s = Math.max(1, previewNumber(value, 7) * 2 * zoom);
		ctx.strokeStyle = "rgb(20,40,160)";
		ctx.fillStyle = "rgb(20,40,160)";
		ctx.lineWidth = Math.max(0.1, 4 * zoom);
		ctx.beginPath();
		ctx.moveTo(left, cy);
		ctx.lineTo(right - s, cy);
		ctx.stroke();
		ctx.beginPath();
		ctx.moveTo(right, cy);
		ctx.lineTo(right - s, cy - 0.45 * s);
		ctx.lineTo(right - s, cy + 0.45 * s);
		ctx.closePath();
		ctx.fill();
	} else if (kind === "opacity") {
		const op = Math.max(0, Math.min(1, previewNumber(value, 1)));
		ctx.fillStyle = "rgb(80,120,230)";
		ctx.fillRect(left + 12, top + 10, (right - left) * 0.45, bottom - top - 20);
		ctx.fillStyle = `rgba(230,70,50,${op})`;
		ctx.fillRect(cx - 12, top + 10, (right - left) * 0.45, bottom - top - 20);
	} else if (kind === "gridsize") {
		const step = Math.max(1, previewNumber(value, 8) * zoom);
		ctx.strokeStyle = "rgb(170,170,170)";
		ctx.lineWidth = 1;
		for (let x = left; x <= right; x += step) {
			ctx.beginPath();
			ctx.moveTo(x, top);
			ctx.lineTo(x, bottom);
			ctx.stroke();
		}
		for (let y = top; y <= bottom; y += step) {
			ctx.beginPath();
			ctx.moveTo(left, y);
			ctx.lineTo(right, y);
			ctx.stroke();
		}
	} else if (kind === "anglesize") {
		const radians = (previewNumber(value, 45) * Math.PI) / 180;
		const ox = left + 0.25 * (right - left);
		const oy = bottom - 12;
		const len = Math.min((right - left) * 0.65, (bottom - top) * 0.9);
		ctx.strokeStyle = "rgb(70,70,70)";
		ctx.lineWidth = 2;
		ctx.beginPath();
		ctx.moveTo(ox, oy);
		ctx.lineTo(right, oy);
		ctx.stroke();
		ctx.strokeStyle = "rgb(20,40,160)";
		ctx.lineWidth = 4;
		ctx.beginPath();
		ctx.moveTo(ox, oy);
		ctx.lineTo(ox + len * Math.cos(radians), oy - len * Math.sin(radians));
		ctx.stroke();
	}
}

export function setupElements(
	ipe: Ipe,
	body: HTMLDivElement,
	options: DialogOptions,
): HTMLElement | null {
	let focusElement: HTMLElement | null = null;
	const contents = document.createElement("div");
	contents.setAttribute("id", "contents");
	body.appendChild(contents);
	let colt = "";
	for (const cols of options.colstretch) {
		colt += `${cols + 1}fr `;
	}
	contents.style.gridTemplateColumns = colt;
	let rowt = "";
	for (const rows of options.rowstretch) {
		if (rows > 0) rowt += `${rows}fr `;
		else rowt += "30px ";
	}
	contents.style.gridTemplateRows = rowt;
	contents.style.columnGap = "20px";
	contents.style.rowGap = "10px";
	for (const w of options.elements) {
		let el: HTMLElement | null = null;
		switch (w.type) {
			case "label":
				el = document.createElement("span");
				el.innerText = w.text.replace("&", "");
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
				drawImagePreview(el1, w.text);
				el = el1;
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
				el.style.justifySelf = "stretch";
				el.style.alignSelf = "stretch";
				break;
			case "input":
			case "button":
			case "combo":
			case "image":
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
			else
				el.addEventListener("change", () =>
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

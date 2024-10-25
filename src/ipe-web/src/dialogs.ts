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

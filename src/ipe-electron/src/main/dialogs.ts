import { type BrowserWindow, dialog } from "electron";
import type { FileDialogOptions } from "./main/modal";

// For the moment, we use the ipe-web message box

/*
const messageBoxTypeMap: {
	[key: string]: "none" | "info" | "error" | "question" | "warning";
} = {
	critical: "error",
	information: "info",
	none: "none",
	question: "question",
	warning: "warning",
};

const messageBoxButtonMap = [
	["Ok"],
	["Ok", "Cancel"],
	["Yes", "No", "Cancel"],
	["Discard", "Cancel"],
	["Save", "Discard", "Cancel"],
];

const messageBoxResultMap: { [key: string]: number } = {
	Ok: 1,
	Yes: 1,
	Save: 1,
	No: 0,
	Discard: 0,
	Cancel: -1,
};

export async function messageBox(
	top: BrowserWindow,
	arg: MessageBoxOptions,
): Promise<number> {
	const response = await dialog.showMessageBox(top, {
		type: messageBoxTypeMap[arg.type],
		message: arg.text,
		detail: arg.details,
		buttons: messageBoxButtonMap[arg.buttons],
	});
	return messageBoxResultMap[
		messageBoxButtonMap[arg.buttons][response.response]
	];
}
*/

function convertFilters(filters: string[]) {
	const result = [];
	for (const s of filters) {
		const i = s.indexOf("(");
		const j = s.indexOf(")");
		const name = s.substring(0, i).trim();
		const exts = s.substring(i + 1, j).split(/\s+/);
		const extensions = exts.map((ext) => ext.replace("*.", ""));
		result.push({ name, extensions });
	}
	return result;
}

export async function fileDialog(
	top: BrowserWindow,
	options: FileDialogOptions,
): Promise<string | null> {
	console.log("File dialog: ", options);
	const filters = convertFilters(options.filters);
	if (options.type === "open") {
		const response = await dialog.showOpenDialog(top, {
			title: options.caption,
			defaultPath: options.path || "",
			filters,
			properties: ["openFile", "treatPackageAsDirectory"],
		});
		if (response.canceled) return null;
		return response.filePaths[0];
	} else {
		const response = await dialog.showSaveDialog(top, {
			title: options.caption,
			defaultPath: options.dir || "",
			filters,
			properties: ["createDirectory", "treatPackageAsDirectory"],
		});
		if (response.canceled) return null;
		return response.filePath;
	}
}

// For the moment, we use the ipe-web popup menu

/*
function popupMenuInner(
	top: BrowserWindow,
	items: PopupItemOptions[],
	callback: (err: unknown, result: PopupMenuResults) => void,
) {
	const template: MenuItemConstructorOptions[] = [];
	const cb = (action: string, current: string) => {
		callback(null, [action, current]);
	};
	for (const w of items) {
		const item: MenuItemConstructorOptions = {
			label: w.label,
		};
		if (w.submenu) {
			const submenu: MenuItemConstructorOptions[] = [];
			for (let i = 0; i < w.submenu.length; ++i) {
				const ww = w.submenu[i];
				submenu.push({
					label: ww.label,
					type: "normal",
					checked: ww.name === w.current,
					click: (_menuitem, _win, _event) => cb(w.name, ww.name),
				});
			}
			item.submenu = submenu;
		} else {
			item.click = (_menuitem, _win, _event) => cb(w.name, "");
		}
		template.push(item);
	}
	const menu = Menu.buildFromTemplate(template);
	menu.popup({ window: top, callback: () => callback(null, null) });
}

export const popupMenu = promisify(popupMenuInner);
*/

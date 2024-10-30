// import type { FileDialogOptions } from "../renderer/src/modal";
type FileDialogOptions = any;

const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("ipc", {
	setup: () => ipcRenderer.invoke("setup"),

	onAction: (cb: (action: string) => void) =>
		ipcRenderer.on("ipeAction", (event, action) => cb(action)),

	/*
	menu: (args: MenuItemConstructorOptions[]) =>
		ipcRenderer.invoke("menu", args),
	setMenuCheckmark: (action: string, checked: boolean) =>
		ipcRenderer.invoke("setMenuCheckmark", action, checked),
	*/

	runlatex: (engine: string, texfile: string) =>
		ipcRenderer.invoke("runlatex", engine, texfile),
	loadFile: (fname: string) => ipcRenderer.invoke("loadFile", fname),
	saveFile: (fname: string, data: string) =>
		ipcRenderer.invoke("saveFile", fname, data),
	setClipboard: (data: string) => ipcRenderer.invoke("setClipboard", data),
	getClipboard: (allowBitmap: boolean) =>
		ipcRenderer.invoke("getClipboard", allowBitmap),

	fileDialog: (options: FileDialogOptions) =>
		ipcRenderer.invoke("fileDialog", options),
	// messageBox: (arg: MessageBoxOptions) => ipcRenderer.invoke("messageBox", arg),
	// popupMenu: (arg: PopupItemOptions[]) => ipcRenderer.invoke("popupMenu", arg),
});

import * as fs from "node:fs";
import { join } from "node:path";
import { is } from "@electron-toolkit/utils";
import {
	BrowserWindow,
	Menu,
	app,
	clipboard,
	ipcMain,
	nativeImage,
	screen,
} from "electron";
import { fileDialog } from "./dialogs";
import { runLatex } from "./latex";
import appIcon from './icon_64x64.png?asset';

// For Windows only:
// Handle creating/removing shortcuts on Windows when installing/uninstalling.
// if (require("electron-squirrel-startup")) app.quit();

// -------------------------------------------------------------------------------

const ipeIcon = nativeImage.createFromPath(appIcon);

// -------------------------------------------------------------------------------

/* For the moment, we use the menu built into ipe-web.
 *
 * The Electron menu has some disadvantages:
 * - It disappears in fullscreen mode.
 * - You cannot modify it dynamically, one has to completely recreate it.
 * - The submenus with radio buttons are huge.
 */

/*
function setupMenu(template: MenuItemConstructorOptions[]) {
	for (const rootItem of template) {
		for (const item of rootItem.submenu as MenuItemConstructorOptions[]) {
			if (item.type !== "separator") {
				if (item.id === "close") {
					item.role = "close";
				} else if (item.id === "manual") {
					item.click = () => {
						shell.openExternal("http://otfried.github.io/ipe");
					};
				} else if (item.type === "submenu") {
					if (item.id.startsWith("submenu-")) {
						for (const subitem of item.submenu as MenuItemConstructorOptions[]) {
							subitem.click = (menuItem, win: BrowserWindow, event) =>
								win.webContents.send("ipeAction", subitem.id.substring(8));
						}
					}
				} else {
					item.click = (menuItem, win: BrowserWindow, event) =>
						win.webContents.send("ipeAction", menuItem.id);
				}
			}
		}
	}
	const menu = Menu.buildFromTemplate(template);
	Menu.setApplicationMenu(menu);
}

function setMenuCheckmark(action: string, checked: boolean) {
	const item = Menu.getApplicationMenu().getMenuItemById(action);
	item.checked = checked;
}
*/

// -------------------------------------------------------------------------------

const createWindow = () => {
	// Create the browser window.
	const mainWindow = new BrowserWindow({
		width: 1200,
		height: 800,
		show: false,
		webPreferences: {
			preload: join(__dirname, "../preload/index.js"),
		},
		icon: ipeIcon,
	});

	mainWindow.on("ready-to-show", () => {
		mainWindow.show();
	});

	// and load the index.html of the app.
	if (is.dev && process.env.ELECTRON_RENDERER_URL) {
		mainWindow.loadURL(process.env.ELECTRON_RENDERER_URL);
	} else {
		mainWindow.loadFile(join(__dirname, "../renderer/index.html"));
	}

	// Open the DevTools.
	if (is.dev) mainWindow.webContents.openDevTools();
	return mainWindow;
};

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
	ipcMain.handle("setup", () => {
		return {
			screen: screen.getPrimaryDisplay().workAreaSize,
			argv: process.argv,
		};
	});
	/*
	ipcMain.handle("menu", (event, template) => setupMenu(template));
	ipcMain.handle("setMenuCheckmark", (event, action, checked) =>
		setMenuCheckmark(action, checked),
	);
	*/

	Menu.setApplicationMenu(null);

	const mainWindow = createWindow();

	mainWindow.addListener("close", (event) => {
		// refuse to do it directly, let Lua handle it
		event.preventDefault();
		mainWindow.webContents.send("ipeAction", "close");
	});

	// show debug output from browser window also on stdout
	mainWindow.webContents.addListener(
		"console-message",
		(event, level, message, line, sourceId) => {
			console.log(">>> ", message);
		},
	);

	ipcMain.handle("runlatex", (event, engine, texfile) =>
		runLatex(engine, texfile),
	);
	ipcMain.handle("loadFile", (event, fname: string) => fs.readFileSync(fname));
	ipcMain.handle("saveFile", (event, fname: string, data: string) =>
		fs.writeFileSync(fname, data),
	);
	ipcMain.handle("setClipboard", (event, data: string) =>
		clipboard.writeText(data),
	);
	ipcMain.handle("getClipboard", (event, allowBitmap: boolean) => {
		const formats = clipboard.availableFormats();
		console.log("Available formats: ", formats);
		return clipboard.readText();
	});

	ipcMain.handle("fileDialog", (event, options) =>
		fileDialog(mainWindow, options),
	);
});

app.on("window-all-closed", () => {
	if (process.platform !== "darwin") app.quit();
});

app.on("activate", () => {
	if (BrowserWindow.getAllWindows().length === 0) createWindow();
});

// -------------------------------------------------------------------------------

import "./popup-menu.css";

export type MainMenuItemType =
	| "separator"
	| "submenu"
	| "checkbox"
	| "radio"
	| "normal";

export interface MainMenuItemOptions {
	type: MainMenuItemType;
	id?: string;
	label?: string;
	accelerator?: string;
	submenu?: MainMenuItemOptions[];
	checked?: boolean;
}

type MainMenuCallback = (action: string) => void;

export interface Color {
	red: number;
	green: number;
	blue: number;
}

export interface PopupSubitemOptions {
	name: string; // internal id
	label: string; // visible label
	color?: Color;
}

export interface PopupItemOptions {
	name: string;
	label: string;
	current?: string; // if there is submenu
	submenu?: PopupSubitemOptions[];
}

// action and current item in submenu
export type PopupMenuResults = [string, string];

type PopupCallback = (results: PopupMenuResults) => void;

export class PopupMenu {
	pane: HTMLDivElement;
	subpane: HTMLDivElement;
	private _menu: HTMLDivElement | null = null;
	private _submenu: HTMLDivElement | null = null;
	private _isOpen = false;
	private _inSubmenu = false;
	private _pinnedSubmenu = false;

	constructor() {
		const pane = document.getElementById("popup-menu-pane") as HTMLDivElement;
		const subpane = document.getElementById(
			"popup-submenu-pane",
		) as HTMLDivElement;
		if (pane == null || subpane == null)
			throw new Error("Element 'popup-(sub)menu-pane' has vanished");
		this.pane = pane;
		this.subpane = subpane;
		this.pane.addEventListener("contextmenu", (e) => e.preventDefault());
		this.subpane.addEventListener("pointerenter", () => {
			this._inSubmenu = true;
		});
		// this must be 'pointerleave', not 'pointerout', because it should not
		// trigger when moving into a submenu item
		this.subpane.addEventListener("pointerleave", () => {
			this._inSubmenu = false;
		});
	}

	keyPressEvent(event: KeyboardEvent): boolean {
		if (this._isOpen) {
			if (event.key === "Escape") {
				this.close();
				return true;
			}
		}
		return false;
	}

	// click outside the menu
	closeOne(): boolean {
		if (this._pinnedSubmenu) {
			this._closeSubmenu();
			return false;
		} else {
			this.close();
			return true;
		}
	}

	public close(): void {
		if (!this._isOpen) return;
		this._closeSubmenu();
		this._menu?.remove();
		this._menu = null;
		this.pane.style.display = "none";
		this._isOpen = false;
	}

	private _closeSubmenu(): void {
		this._submenu?.remove();
		this._submenu = null;
		this._inSubmenu = false;
		this._pinnedSubmenu = false;
		this.subpane.style.display = "none";
	}

	private _createSubmenu(): void {
		this._submenu?.remove();
		this._submenu = document.createElement("div");
		this._submenu.classList.add("popup-submenu");
	}

	private _createSubmenuItem(
		item: HTMLDivElement,
		label: string,
		enter: () => void,
	): void {
		item.classList.add("popup-menu-item", "popup-menu-submenu");
		const span = document.createElement("span");
		span.innerText = label.replace("&&", "&").replace("&&", "&");
		const arrow = document.createElement("span");
		arrow.classList.add("popup-menu-arrow");
		item.appendChild(span);
		item.appendChild(arrow);
		item.addEventListener("pointerenter", () => {
			if (!this._pinnedSubmenu) enter();
		});
		item.addEventListener("pointerleave", () => this._leaveSubmenuAnchor());
		item.addEventListener("click", () => {
			this._pinnedSubmenu = true;
			enter();
		});
	}

	private _leaveSubmenuAnchor(): void {
		if (this._inSubmenu || this._pinnedSubmenu) return;
		// remember the currently showing submenu
		const m = this._submenu;
		setTimeout(() => {
			// if the submenu is still showing and we are not in it, close it
			if (!this._inSubmenu && this._submenu === m) this._closeSubmenu();
		}, 320);
	}

	private _show(
		x: number,
		y1: number,
		y2: number,
		pane: HTMLDivElement,
		m: HTMLDivElement,
	) {
		m.style.left = `${x}px`;
		m.style.top = `${y1}px`;
		pane.appendChild(m);
		pane.style.display = "block";
		setTimeout(() => {
			// wait for browser to compute size of menu
			const h = m.offsetHeight;
			if (y1 + h > window.innerHeight) {
				if (h < y1) {
					m.style.top = `${y2 - h}px`;
				} else if (y1 < window.innerHeight / 2) {
					m.style.maxHeight = `${window.innerHeight - y1}px`;
				} else {
					m.style.top = "0px";
					m.style.maxHeight = `${y2}px`;
				}
			}
			this._isOpen = true;
		}, 0);
	}

	public openPopup(
		x: number,
		y: number,
		items: PopupItemOptions[],
		cb: PopupCallback,
	): void {
		if (this._isOpen) this.close();
		this._menu = document.createElement("div");
		this._menu.classList.add("popup-menu");
		for (const m of items) {
			const item = document.createElement("div");
			if (m.submenu == null) {
				item.innerText = m.label.replace("&&", "&");
				item.classList.add("popup-menu-item");
				item.addEventListener("click", () => {
					this.close();
					cb([m.name, ""]);
				});
			} else {
				this._createSubmenuItem(item, m.label!, () =>
					this._openPopupSubmenu(item, m, cb),
				);
			}
			this._menu.appendChild(item);
		}
		this._show(x, y, y, this.pane, this._menu);
	}

	public openMainMenu(
		button: HTMLButtonElement,
		items: MainMenuItemOptions[],
		cb: MainMenuCallback,
	): void {
		if (this._isOpen) this.close();
		this._menu = document.createElement("div");
		this._menu.classList.add("popup-menu");
		for (const m of items) {
			const item = document.createElement("div");
			if (["normal", "checkbox", "radio"].includes(m.type)) {
				const label = document.createElement("span");
				label.innerText = m.label!;
				label.classList.add("main-menu-label");
				item.appendChild(label);
				item.classList.add("main-menu-item");
				if (m.accelerator) {
					const s = document.createElement("span");
					s.innerText = m.accelerator;
					s.classList.add("main-menu-shortcut");
					item.appendChild(s);
				}
				item.addEventListener("click", () => {
					this.close();
					cb(m.id!);
				});
				if (m.checked) label.classList.add("checked");
			} else if (m.type === "separator") {
				item.classList.add("popup-menu-separator");
			} else if (m.type === "submenu") {
				item.classList.add("main-menu-item");
				this._createSubmenuItem(item, m.label!, () =>
					this._openMainSubmenu(item, m.submenu!, cb),
				);
			} else throw new Error(`Unknown menu item type '}${m.type}'`);
			this._menu.appendChild(item);
		}
		const r = button.getBoundingClientRect();
		this._show(r.left, r.bottom, r.bottom, this.pane, this._menu);
	}

	private _openMainSubmenu(
		handle: HTMLElement,
		items: MainMenuItemOptions[],
		cb: MainMenuCallback,
	) {
		this._createSubmenu();
		for (const m of items) {
			const item = document.createElement("div");
			if (["normal", "checkbox", "radio"].includes(m.type)) {
				item.innerText = m.label!;
				item.classList.add("popup-submenu-item");
				item.addEventListener("click", () => {
					this.close();
					cb(m.id!);
				});
				if (m.checked) item.classList.add("checked");
			} else throw new Error(`Unknown submenu item type '${m.type}'`);
			this._submenu!.appendChild(item);
		}
		const r = handle.getBoundingClientRect();
		this._show(r.right, r.top, r.bottom, this.subpane, this._submenu!);
	}

	private _openPopupSubmenu(
		handle: HTMLElement,
		entry: PopupItemOptions,
		cb: PopupCallback,
	) {
		this._createSubmenu();
		for (const m of entry.submenu!) {
			const item = document.createElement("div");
			item.classList.add("popup-submenu-item");
			item.innerText = m.label.replace("&&", "&");
			if (m.name === entry.current) item.classList.add("checked");
			item.addEventListener("click", () => {
				this.close();
				cb([entry.name, m.name]);
			});

			this._submenu!.appendChild(item);
		}
		const r = handle.getBoundingClientRect();
		this._show(r.right, r.top, r.bottom, this.subpane, this._submenu!);
	}
}

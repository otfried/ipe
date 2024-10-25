export function get(id: string): HTMLElement {
	const el = document.getElementById(id);
	if (el == null) throw new Error(`Element ${id} vanished`);
	return el;
}

export function removeChildren(el: HTMLElement) {
	while (el.firstChild) el.removeChild(el.firstChild);
}

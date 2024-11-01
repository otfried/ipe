import instantiateIpe from "./ipe.js";
import { IpeUi } from "./ipeui";
import "./index.css";

declare global {
	interface Window {
		ipc: any;
		ipeui: IpeUi;
	}
}

instantiateIpe({
	printErr: console.log.bind(console),
}).then(async (ipe) => {
	console.log("Ipe wasm code loaded");
	const ipeui = new IpeUi(ipe, [
		"HOME=/home/ipe",
		"IPEJSLATEX=1",
		"IPEDEBUG=1",
	]);

	ipe.FS.mkdir("/home/ipe", 0o777);
	ipe.FS.mkdir("/tmp/pages", 0o777);
	ipe.FS.mkdir("/tmp/latexrun", 0o777);
	ipe.FS.mkdir("/tmp/latexrun/icons", 0o777);
	ipe.FS.mount(ipe.IDBFS, { autoPersist: true }, "/home/ipe");
	ipe.FS.syncfs(true, (err: unknown) => {
		console.log("Synced files from browser's IDB: ", err);
		const create = (dir: string) => {
			const path = `/home/ipe/${dir}`;
			if (!ipe.FS.analyzePath(path).exists) {
				console.log(`Creating directory ${path}`);
				ipe.FS.mkdir(path, 0o777);
			}
		};
		create("documents");
		create("images");
		create("styles");
		create("ipelets");

		const params = new URLSearchParams(window.location.search);

		console.log("Starting Ipe");
		ipeui.startIpe(window.innerWidth, window.innerHeight);
		console.log("Ipe is running!");
		if (params.has("open")) {
			let fn = params.get("open")!;
			const i = fn?.lastIndexOf("/");
			if (i >= 0) fn = fn?.substring(i + 1);
			ipeui.openFile(`/home/ipe/documents/${fn}`);
		}
	});
});

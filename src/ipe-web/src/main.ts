import instantiateIpe from "./ipe/ipe.js";
import { IpeUi } from "./ipe/ipeui";
import "./ipe/index.css";
import { buildInfo } from "./gitversion";
import type { IpeDocument, TextConversion } from "./write-pptx";

declare global {
	interface Window {
		ipeBridge: any; // bridge between webview and the host environment
		ipeui: IpeUi; // used in WASM for calls into JS
	}
}

async function createSlides(
	jsonData: string,
	textConversion: string,
	titleConversion: string,
): Promise<Uint8Array> {
	const doc: IpeDocument = JSON.parse(jsonData);
	const { createPptx } = await import("./write-pptx");
	return await createPptx(
		doc,
		textConversion as TextConversion,
		titleConversion as TextConversion,
	);
}

instantiateIpe({
	printErr: console.log.bind(console),
}).then(async (ipe: any) => {
	console.log("Ipe wasm code loaded");
	const ipeui = new IpeUi(ipe, buildInfo, "web", [
		"HOME=/home/ipe",
		"IPELATEXDIR=/tmp/latexrun",
		"IPELETPATH=/home/ipe/ipelets:_",
		"IPESTYLES=/home/ipe/styles:_",
		"IPEJSLATEX=1",
		"IPEDEBUG=1",
	]);

	ipeui._createSlides = createSlides;

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
		create("presentations");
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

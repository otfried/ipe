import "./index.css";

import instantiateIpe from "./ipe.js";
import { IpeUi } from "./ipeui";

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
	const ipeui = new IpeUi(ipe);

	ipe.FS.mkdir("/home/ipe", 0o777);
	ipe.FS.mkdir("/tmp/pages", 0o777);
	ipe.FS.mkdir("/tmp/latexrun", 0o777);
	ipe.FS.mkdir("/tmp/latexrun/icons", 0o777);

	const setup = await window.ipc.setup();

	console.log("Starting Ipe");
	ipeui.startIpe(setup.screen.width, setup.screen.height);
	console.log("Ipe is running!");
});

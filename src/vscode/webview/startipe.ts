import "./ipe/index.css";

declare global {
	interface Window {
		ipc: any;
		ipeui: IpeUi;
	}
}

import instantiateIpe from "./ipe/ipe.js";
import { IpeUi } from "./ipe/ipeui";

instantiateIpe({
	printErr: console.log.bind(console),
}).then(async (ipe: any) => {
	console.log("Ipe wasm code loaded");

	ipe.FS.mkdir("/opt/ipe/user-ipelets", 0o777);
	ipe.FS.mkdir("/tmp/pages", 0o777);
	ipe.FS.mkdir("/tmp/latexrun", 0o777);
	ipe.FS.mkdir("/tmp/latexrun/icons", 0o777);

	/*
	const setup = await window.ipc.setup();
	for (const ipelet in setup.ipelets)
		ipe.FS.writeFile(`/opt/ipe/user-ipelets/${ipelet}`, setup.ipelets[ipelet]);
	if (setup.customizationData != null)
		ipe.FS.writeFile("/opt/ipe/customization.lua", setup.customizationData);
    */

	const env = [
		// `IPESTYLES=${setup.styles.join(":")}`,
		"IPELETPATH=/opt/ipe/customization.lua:/opt/ipe/user-ipelets:/opt/ipe/ipelets",
		"IPEJSLATEX=1",
		"IPEDEBUG=1",
		"IPEPRELOADER=1",
		"IPELATEXDIR=/tmp/latexrun",
		// `HOME=${setup.home}`,
	];
	console.log("About to create IpeUi");
	const ipeui = new IpeUi(ipe, env);
	ipeui.customizationFileName = "dummy"; // setup.customization as string;
	console.log("Starting Ipe");
	ipeui.startIpe(1920, 1024); // setup.screen.width, setup.screen.height);
	console.log("Ipe is running!");
});

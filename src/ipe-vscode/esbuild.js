// Bundles the webview TypeScript/CSS code in media/ into out/media/.
const esbuild = require("esbuild");

const watch = process.argv.includes("--watch");
const production = process.argv.includes("--production");

async function main() {
	const ctx = await esbuild.context({
		entryPoints: ["webview/startipe.ts"],
		bundle: true,
		outdir: "out/webview",
		platform: "browser",
		format: "esm",
		target: "es2022",
		sourcemap: !production,
		minify: production,
		// webview/ipe is a symlink into ../ipe-web/src/ipe; without this, esbuild
		// resolves node_modules from the real (target) path instead of from
		// ipe-vscode, so it can't find dependencies like sortablejs.
		preserveSymlinks: true,
		define: {
			"import.meta.env.DEV": JSON.stringify(!production),
		},
		loader: {
			".wasm": "file",
		},
	});

	if (watch) {
		await ctx.watch();
		console.log("[esbuild] watching webview/ for changes...");
	} else {
		await ctx.rebuild();
		await ctx.dispose();
	}
}

main().catch((err) => {
	console.error(err);
	process.exit(1);
});

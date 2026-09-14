// Ambient declarations for asset imports handled by the esbuild bundler.
declare module "*.css";

// import.meta.env.DEV is injected by esbuild's `define` option at build time.
interface ImportMetaEnv {
	readonly DEV: boolean;
}
interface ImportMeta {
	readonly env: ImportMetaEnv;
}

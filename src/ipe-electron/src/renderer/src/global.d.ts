// Ambient declarations for css imports
declare module "*.css";

// import.meta.env.DEV is injected by esbuild's `define` option at build time.
interface ImportMetaEnv {
	readonly DEV: boolean;
}
interface ImportMeta {
	readonly env: ImportMetaEnv;
}
import type { DialogResult } from "./dialogs";
import type { PopupMenuResults } from "./popup-menu";

type PageSorterResults = [number[], boolean[]];

export declare type ResumeResult =
	| DialogResult
	| PopupMenuResults
	| PageSorterResults
	| null
	| boolean
	| number
	| string;

// all these handles are actually pointers into WASM memory
type StringHandle = number;
type Handle = number;
export type DialogId = number;

declare class Emval {
	toHandle(val: any): Handle;
	toValue(handle: Handle): any;
}

export declare class Ipe {
	stringToNewUTF8(s: string): StringHandle;
	Emval: Emval;

	readonly FS: any;
	readonly Platform: any;

	_createCanvas(): void;
	_canvasUpdate(): void;
	_canvasUpdateSize(): void;
	_canvasMouseButtonEvent(event: Handle, button: number, press: boolean): void;
	_canvasMouseMoveEvent(event: Handle): void;
	_canvasWheelEvent(event: Handle): void;
	_canvasKeyPressEvent(event: Handle): boolean;
	_canvasSetAdditionalModifiers(modifiers: number): void;
	_canvasZoomPan(): Handle;
	_canvasSetZoomPan(x: number, y: number, zoom: number): void;

	_initLib(environment: Handle): void;
	_createTarball(texfile: StringHandle): Handle;
	_ipeVersion(): Handle;

	_startIpe(
		width: number,
		height: number,
		dpr: number,
		platform: StringHandle,
	): void;
	_resume(result: ResumeResult): void;
	_action(action: StringHandle): void;
	_openFile(fn: StringHandle): void;
	_absoluteButton(sel: StringHandle): void;
	_selector(b: StringHandle, v: StringHandle): void;
	_paintPathView(): void;
	_layerAction(name: StringHandle, layer: StringHandle): void;
	_showLayerBoxPopup(layer: StringHandle, x: number, y: number): void;
	_showPathStylePopup(x: number, y: number): void;
	_bookmarkSelected(row: number): void;
	_layerOrderChanged(order: Handle): void;

	_dialogIgnoresEscapeKey(dialogId: DialogId): boolean;
	_dialogCallLua(dialogId: DialogId, method: number): void;
}

declare function instantiateIpe(): (ipe: Ipe) => void;

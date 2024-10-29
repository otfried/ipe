import type { Ipe } from "./ipejs";

type Vector = [number, number];

function vec_add(a: Vector, b: Vector): Vector {
	return [a[0] + b[0], a[1] + b[1]];
}
function vec_sub(a: Vector, b: Vector): Vector {
	return [a[0] - b[0], a[1] - b[1]];
}
function vec_norm(v: Vector): number {
	return Math.sqrt(v[0] * v[0] + v[1] * v[1]);
}
function vec_scale(v: Vector, s: number): Vector {
	return [v[0] * s, v[1] * s];
}
function vec_dist(a: Vector, b: Vector): number {
	return vec_norm(vec_sub(a, b));
}

class Finger {
	identifier: number;
	pos: Vector;
	constructor(t: Touch) {
		this.identifier = t.identifier;
		this.pos = [t.clientX, t.clientY];
	}
}

export class TouchDragZoom {
	readonly target: HTMLElement;
	fingers: Finger[] = [];
	readonly ipe: Ipe;
	initialZoom = 1;
	initialPan: Vector = [0, 0];
	inGesture = false;

	constructor(ipe: Ipe, target: HTMLElement) {
		this.ipe = ipe;
		this.target = target;
		target.addEventListener("touchstart", (ev) => this._onTouchStart(ev));
		target.addEventListener("touchmove", (ev) => this._onTouchMove(ev));
		target.addEventListener("touchend", (ev) => this._onTouchEnd(ev));
		target.addEventListener("touchcancel", (ev) => this._onTouchEnd(ev));
	}

	private _onTouchStart(ev: TouchEvent): void {
		ev.preventDefault();
		if (ev.targetTouches.length === 2) {
			this.fingers = [
				new Finger(ev.targetTouches[0]),
				new Finger(ev.targetTouches[1]),
			];
			const panZoom = this.ipe.Emval.toValue(this.ipe._canvasZoomPan());
			this.initialPan = [panZoom[0], panZoom[1]];
			this.initialZoom = panZoom[2];
		}
	}

	private _onTouchMove(ev: TouchEvent): void {
		ev.preventDefault();
		if (ev.targetTouches.length !== 2) return;
		// Check if the two target touches are the same ones that started the 2-touch
		const f0 = new Finger(ev.targetTouches[0]);
		const f1 = new Finger(ev.targetTouches[1]);
		const findFinger = (f: Finger) => {
			for (let i = 0; i < this.fingers.length; ++i) {
				if (this.fingers[i].identifier === f.identifier) return i;
			}
			return -1;
		};
		const point0 = findFinger(f0);
		const point1 = findFinger(f1);
		if (point0 < 0 || point1 < 0) {
			this.inGesture = false;
			return;
		}
		const r = this.target.getBoundingClientRect();
		const s = [r.width, r.height] as Vector;
		const center = vec_scale(s, 0.5); // center of canvas in pixels
		const bl = [r.left, r.bottom] as Vector; // bottom left of canvas

		const v = (f: Finger) => [f.pos[0] - bl[0], bl[1] - f.pos[1]] as Vector;
		const p1 = v(f0);
		const p2 = v(f1);
		const q1 = v(this.fingers[point0]);
		const q2 = v(this.fingers[point1]);

		// wait for a bit of movement before we start a gesture;
		if (!this.inGesture && vec_dist(q1, p1) < 6 && vec_dist(q2, p2) < 6) return;
		this.inGesture = true;

		// anchor is the midpoint of two fingers at start, in user coordinates
		const anchor = vec_add(
			this.initialPan,
			vec_scale(
				vec_sub(vec_scale(vec_add(q1, q2), 0.5), center),
				1.0 / this.initialZoom,
			),
		);
		// change in zoom
		const ratio = vec_dist(p1, p2) / vec_dist(q1, q2);
		// the new zoom
		const nZoom = this.initialZoom * ratio;
		// screen coordinates where we want to move the anchor, relative to center
		const target = vec_sub(vec_scale(vec_add(p1, p2), 0.5), center);
		const nPan = vec_sub(anchor, vec_scale(target, 1.0 / nZoom));
		this.ipe._canvasSetZoomPan(nPan[0], nPan[1], nZoom);
		this.ipe._canvasUpdate();
	}

	private _onTouchEnd(ev: TouchEvent): void {
		if (ev.targetTouches.length === 0 && this.inGesture) this.inGesture = false;
	}
}

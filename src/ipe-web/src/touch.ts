// based on https://developer.mozilla.org/en-US/docs/Web/API/Touch_events/Multi-touch_interaction

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
function vec_dot(a: Vector, b: Vector): number {
	return a[0] * b[0] + a[1] * b[1];
}
function vec_dist(a: Vector, b: Vector): number {
	return vec_norm(vec_sub(a, b));
}

type Gesture = "none" | "drag" | "zoom";

export class TouchDragZoom {
	readonly target: HTMLElement;
	fingers: Touch[] = [];
	activeCount = 0;
	lastDistance = 0;
	ended = false;
	enabled = false;
	readonly ipe: Ipe;
	initialZoom = 1;
	initialPan: Vector = [0, 0];
	gesture: Gesture = "none";

	constructor(ipe: Ipe, target: HTMLElement) {
		this.ipe = ipe;
		this.target = target;
		target.addEventListener("touchstart", (ev) => this._onTouchStart(ev));
		target.addEventListener("touchmove", (ev) => this._onTouchMove(ev));
		target.addEventListener("touchend", (ev) => this._onTouchEnd(ev));
		target.addEventListener("touchcancel", (ev) => this._onTouchEnd(ev));
	}

	private _updateGesture(gesture: Gesture): void {
		if (gesture !== this.gesture) {
			this.gesture = gesture;
			console.log("New gesture: ", gesture);
		}
	}

	private _onTouchStart(ev: TouchEvent): void {
		ev.preventDefault();
		if (ev.targetTouches.length === 2) {
			for (let i = 0; i < ev.targetTouches.length; i++) {
				this.fingers.push(ev.targetTouches[i]);
			}
			const panZoom = this.ipe.Emval.toValue(this.ipe._canvasZoomPan());
			this.initialPan = [panZoom[0], panZoom[1]];
			this.initialZoom = panZoom[2];
		}
	}

	private _findFinger(t: Touch): number {
		for (let i = 0; i < this.fingers.length; ++i) {
			if (this.fingers[i].identifier === t.identifier) return i;
		}
		return -1;
	}

	private _onTouchMove(ev: TouchEvent): void {
		ev.preventDefault();
		if (ev.targetTouches.length === 2 && ev.changedTouches.length === 2) {
			// Check if the two target touches are the same ones that started
			// the 2-touch
			const point1 = this._findFinger(ev.targetTouches[0]);
			const point2 = this._findFinger(ev.targetTouches[1]);
			if (point1 < 0 || point2 < 0) return;

			const r = this.target.getBoundingClientRect();
			const s = [r.width, r.height] as Vector;
			const center = vec_scale(s, 0.5); // center of canvas in pixels
			const bl = [r.left, r.bottom] as Vector; // bottom left of canvas

			const v = (t: Touch) => [t.clientX - bl[0], bl[1] - t.clientY] as Vector;
			const p1 = v(ev.targetTouches[0]);
			const p2 = v(ev.targetTouches[1]);
			const q1 = v(this.fingers[point1]);
			const q2 = v(this.fingers[point2]);

			const d1 = vec_sub(p1, q1);
			const d2 = vec_sub(p2, q2);
			if (this.gesture === "none") {
				// need a little movement before we start a gesture;
				if (vec_norm(d1) < 10 && vec_norm(d2) < 10) return;
				if (vec_dot(d1, d2) > 50.0) this._updateGesture("drag");
				else this._updateGesture("zoom");
			}

			if (this.gesture === "drag") {
				const pan = vec_sub(
					this.initialPan,
					vec_scale(d1, 1 / this.initialZoom),
				);
				this.ipe._canvasSetZoomPan(pan[0], pan[1], this.initialZoom);
				this.ipe._canvasUpdate();
			} else {
				// midpoint of two fingers at start relative to center of canvas, in user coordinates
				const q = vec_scale(
					vec_sub(vec_scale(vec_add(q1, q2), 0.5), center),
					1.0 / this.initialZoom,
				);
				const origin = vec_add(q, this.initialPan); // finger midpoint in user coordinates
				// origin should be a fixpoint, so we need to adjust pan
				const ratio = vec_dist(p1, p2) / vec_dist(q1, q2);
				const nZoom = this.initialZoom * ratio;
				const nPan = vec_add(
					origin,
					vec_scale(vec_sub(this.initialPan, origin), 1.0 / ratio),
				);
				this.ipe._canvasSetZoomPan(nPan[0], nPan[1], nZoom);
				this.ipe._canvasUpdate();
			}
		}
	}

	private _onTouchEnd(ev: TouchEvent): void {
		ev.preventDefault();
		if (ev.targetTouches.length === 0 && this.gesture !== "none") {
			this.gesture = "none";
			console.log("Interaction has ended", ev);
		}
	}
}

// based on https://developer.mozilla.org/en-US/docs/Web/API/Touch_events/Multi-touch_interaction
export class TouchDragZoom {
	readonly target: HTMLElement;
	fingers: Touch[] = [];
	activeCount = 0;
	lastDistance = 0;
	ended = false;
	enabled = false;

	constructor(target: HTMLElement) {
		this.target = target;
		target.addEventListener("touchstart", (ev) => this._onTouchStart(ev));
		target.addEventListener("touchmove", (ev) => this._onTouchMove(ev));
		target.addEventListener("touchend", (ev) => this._onTouchEnd(ev));
		target.addEventListener("touchcancel", (ev) => this._onTouchEnd(ev));
	}

	private _onTouchStart(ev: TouchEvent): void {
		ev.preventDefault();
		if (ev.targetTouches.length === 2) {
			for (let i = 0; i < ev.targetTouches.length; i++) {
				this.fingers.push(ev.targetTouches[i]);
			}
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
			const point2 = this._findFinger(ev.targetTouches[0]);

			const dist = (t1: Touch, t2: Touch) => {
				const x1 = t1.clientX;
				const y1 = t1.clientY;
				const x2 = t2.clientX;
				const y2 = t2.clientY;
				return Math.sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
			};
			if (point1 >= 0 && point2 >= 0) {
				const diff1 = dist(this.fingers[point1], ev.targetTouches[0]);
				const diff2 = dist(this.fingers[point2], ev.targetTouches[1]);

				const PINCH_THRESHOLD = (ev.target! as HTMLElement).clientWidth / 10;
				if (diff1 >= PINCH_THRESHOLD && diff2 >= PINCH_THRESHOLD) {
					console.log("Drag interaction");
				}
			}
		}
	}

	private _onTouchEnd(ev: TouchEvent): void {
		ev.preventDefault();
		if (ev.targetTouches.length === 0) {
			console.log("Interaction has ended", ev);
		}
	}
}

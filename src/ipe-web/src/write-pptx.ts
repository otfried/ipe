import JSZip from "jszip";
import PptxGenJS from "pptxgenjs";

export type TextConversion = "all" | "latex" | "labels" | "math" | "none";

interface IpePoint {
	x: number;
	y: number;
	moveto?: boolean;
	x1?: number;
	y1?: number;
	x2?: number;
	y2?: number;
}

type IpePathElement = IpePoint | { close: true };

interface IpeObject {
	points?: IpePathElement[];
	fill?: PptxGenJS.ShapeFillProps;
	line?: PptxGenJS.ShapeLineProps;
}

interface IpeText {
	text: string;
	x: number;
	y: number;
	color: string;
	width: number;
	height: number; // above baseline
	depth: number; // below baseline
	minipage: boolean;
	verticalAlignment: "top" | "bottom" | "center" | "baseline";
	horizontalAlignment: "left" | "right" | "hcenter" | "center";
	size: number;
	style: string;
}

interface IpeImage {
	image: string;
	x: number;
	y: number;
	w: number;
	h: number;
	format: "jpeg" | "png";
}

interface IpePage {
	objects: (IpeObject | IpeText | IpeImage)[];
}

export interface IpeDocument {
	title?: string;
	author?: string;
	subject?: string;
	keywords?: string;
	created?: string;
	modified?: string;
	width: number;
	height: number;
	originX: number;
	originY: number;
	pages: IpePage[];
}

function pdfDateToW3cDate(date: string): string {
	const match =
		/^D:(\d{4})(\d{2})(\d{2})(\d{2})(\d{2})(\d{2})(Z|[+-]\d{2}'?\d{2}'?)?$/.exec(
			date,
		);
	if (!match) throw new Error(`Invalid PDF date: ${date}`);
	const [, year, month, day, hour, minute, second, zone] = match;
	let timezone = zone ?? "";
	if (timezone && timezone !== "Z") {
		timezone = timezone.replace(/'/g, "");
		timezone = `${timezone.slice(0, 3)}:${timezone.slice(3)}`;
	}
	return `${year}-${month}-${day}T${hour}:${minute}:${second}${timezone}`;
}

function escapeXml(text: string): string {
	return text.replace(
		/[&<>"']/g,
		(char) =>
			({
				"&": "&amp;",
				"<": "&lt;",
				">": "&gt;",
				'"': "&quot;",
				"'": "&apos;",
			})[char]!,
	);
}

async function updateCoreProperties(
	data: Uint8Array,
	metadata: IpeDocument,
): Promise<Uint8Array> {
	const zip = await JSZip.loadAsync(data);
	const coreFile = zip.file("docProps/core.xml");
	if (!coreFile) throw new Error("PPTX is missing docProps/core.xml");

	let xml = await coreFile.async("string");
	if (metadata.keywords !== undefined) {
		const keywords = `<cp:keywords>${escapeXml(metadata.keywords)}</cp:keywords>`;
		xml = xml.replace(
			/(<dc:creator>[\s\S]*?<\/dc:creator>)/,
			(creator) => `${creator}${keywords}`,
		);
	}
	for (const [field, date] of [
		["created", metadata.created],
		["modified", metadata.modified],
	] as const) {
		if (!date) continue;
		const value = escapeXml(pdfDateToW3cDate(date));
		const tag = field === "created" ? "dcterms:created" : "dcterms:modified";
		xml = xml.replace(
			new RegExp(`(<${tag}\\b[^>]*>)[\\s\\S]*?(</${tag}>)`),
			`$1${value}$2`,
		);
	}

	zip.file("docProps/core.xml", xml);
	return zip.generateAsync({ type: "uint8array" });
}

type ShapePoint = NonNullable<PptxGenJS.ShapeProps["points"]>[number];

function addIpeText(
	slide: PptxGenJS.Slide,
	t: IpeText,
	originX: number,
	originY: number,
): void {
	// (x, y) is the bottom-left corner of the bounding box
	const total = t.height + t.depth;
	const top = t.y + total;
	const unwrappedText =
		/^\\PageTitle\{([\s\S]*)\}$/.exec(t.text)?.[1] ?? t.text;
	const sourceText =
		t.style === "item" && !/^\\item\b/.test(unwrappedText)
			? `\\item ${unwrappedText}`
			: unwrappedText;
	const paragraphs: { text: string; bullet: boolean }[] = [];
	let paragraphText = "";
	let bulletNext = false;
	const flushParagraph = () => {
		const text = paragraphText.replace(/\n/g, " ").trim();
		if (text) paragraphs.push({ text, bullet: bulletNext });
		paragraphText = "";
		bulletNext = false;
	};
	for (const part of sourceText.split(/(\\\\|\\item\b|\\par\b|\n\s*\n)/)) {
		if (part === "\\item") {
			flushParagraph();
			bulletNext = true;
		} else if (part === "\\\\" || part === "\\par" || /^\n\s*\n$/.test(part)) {
			flushParagraph();
		} else {
			paragraphText += part;
		}
	}
	flushParagraph();

	const text: string | PptxGenJS.TextProps[] = paragraphs.some((p) => p.bullet)
		? paragraphs.map((p, index) => ({
				text: p.text,
				options: {
					...(p.bullet ? { bullet: true } : {}),
					breakLine: index < paragraphs.length - 1,
				},
			}))
		: paragraphs.map((p) => p.text).join("\n");

	slide.addText(text, {
		x: (originX + t.x) / 72,
		y: (originY - top - 0.2 * t.size) / 72,
		w: Math.max(t.width, 1) / 72,
		h: Math.max(total, 1) / 72,
		// Scale font size down slightly to better fit the bounding box
		fontSize: 0.95 * t.size,
		color: t.color,
		align:
			t.horizontalAlignment === "hcenter" ||
			(t.minipage && t.style === "center")
				? "center"
				: t.horizontalAlignment,
		valign: "top",
		margin: 0,
		wrap: t.minipage,
	});
}

function addIpeImage(
	slide: PptxGenJS.Slide,
	t: IpeImage,
	originX: number,
	originY: number,
): void {
	// (x, y) is the top-left corner of the image
	slide.addImage({
		data: `data:image/${t.format};base64,${t.image}`,
		x: (originX + t.x) / 72,
		y: (originY - t.y) / 72,
		w: Math.max(t.w, 1) / 72,
		h: Math.max(t.h, 1) / 72,
	});
}

function shouldConvert(
	obj: IpeText,
	textConversion: TextConversion,
	titleConversion: TextConversion,
): boolean {
	const titleText = /^\\PageTitle\{([\s\S]*)\}$/.exec(obj.text)?.[1];
	const latexText = (titleText ?? obj.text).replace(
		/\\\\|\\par\b|\\item\b/g,
		"",
	);
	if (obj.text.startsWith("\\PageTitle")) {
		if (titleConversion === "all") return true;
		return (
			titleConversion === "latex" &&
			(latexText.includes("$") || latexText.includes("\\"))
		);
	}
	if (textConversion === "all") return true;
	if (textConversion === "latex")
		return (
			obj.style === "math" ||
			latexText.includes("$") ||
			latexText.includes("\\")
		);
	if (obj.minipage) return false;
	return (
		textConversion === "labels" ||
		(textConversion === "math" && obj.style === "math")
	);
}

function addIpeObjects(
	slide: PptxGenJS.Slide,
	page: IpePage,
	textConversion: TextConversion,
	titleConversion: TextConversion,
	originX: number,
	originY: number,
): void {
	const tx = (x: number) => (originX + x) / 72;
	const ty = (y: number) => (originY - y) / 72;

	let inTextObject = false;
	for (const obj of page.objects) {
		if ("text" in obj) {
			if (!shouldConvert(obj, textConversion, titleConversion)) {
				addIpeText(slide, obj, originX, originY);
				inTextObject = true;
			}
			continue;
		}
		if ("endtext" in obj) {
			inTextObject = false;
			continue;
		}

		if (inTextObject) continue;

		if ("image" in obj) {
			addIpeImage(slide, obj, originX, originY);
			continue;
		}

		if (!obj.points || obj.points.length === 0) continue;

		// create custom path
		let xmin = Infinity,
			ymin = Infinity,
			xmax = -Infinity,
			ymax = -Infinity;
		const extend = (x: number, y: number) => {
			xmin = Math.min(xmin, tx(x));
			xmax = Math.max(xmax, tx(x));
			ymin = Math.min(ymin, ty(y));
			ymax = Math.max(ymax, ty(y));
		};
		for (const p of obj.points) {
			if ("close" in p) continue;
			extend(p.x, p.y);
			if (p.x1 !== undefined && p.y1 !== undefined) extend(p.x1, p.y1);
			if (p.x2 !== undefined && p.y2 !== undefined) extend(p.x2, p.y2);
		}
		if (xmin === Infinity) continue;

		const rx = (x: number) => tx(x) - xmin;
		const ry = (y: number) => ty(y) - ymin;
		const points: ShapePoint[] = obj.points.map((p): ShapePoint => {
			if ("close" in p) return { close: true };
			if (
				p.x1 !== undefined &&
				p.y1 !== undefined &&
				p.x2 !== undefined &&
				p.y2 !== undefined
			) {
				return {
					x: rx(p.x),
					y: ry(p.y),
					curve: {
						type: "cubic",
						x1: rx(p.x1),
						y1: ry(p.y1),
						x2: rx(p.x2),
						y2: ry(p.y2),
					},
				};
			}
			return { x: rx(p.x), y: ry(p.y), moveTo: p.moveto };
		});

		// Zero extent (e.g. horizontal line) would give a degenerate path coordinate system.
		const minExtent = 0.001;
		// "custGeom" (pptx.shapes.CUSTOM_GEOMETRY) is missing from pptxgenjs's typings.
		slide.addShape("custGeom" as PptxGenJS.SHAPE_NAME, {
			x: xmin,
			y: ymin,
			w: Math.max(xmax - xmin, minExtent),
			h: Math.max(ymax - ymin, minExtent),
			points,
			fill: obj.fill ?? { type: "none" },
			line: obj.line ?? { type: "none" },
		});
	}
}

export async function createPptx(
	doc: IpeDocument,
	textConversion: TextConversion,
	titleConversion: TextConversion,
): Promise<Uint8Array> {
	const { width, height, originX, originY } = doc;

	const pptx: PptxGenJS = new PptxGenJS();
	pptx.defineLayout({ name: "IPE", width: width / 72, height: height / 72 }); // pptxgenjs uses inches
	pptx.layout = "IPE";
	if (doc.title !== undefined) pptx.title = doc.title;
	if (doc.author !== undefined) pptx.author = doc.author;
	if (doc.subject !== undefined) pptx.subject = doc.subject;

	for (const page of doc.pages)
		addIpeObjects(
			pptx.addSlide(),
			page,
			textConversion,
			titleConversion,
			originX,
			originY,
		);

	const data = (await pptx.write({ outputType: "uint8array" })) as Uint8Array;
	return updateCoreProperties(data, doc);
}

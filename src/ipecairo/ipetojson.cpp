// --------------------------------------------------------------------
// Create a json document with Ipe drawing
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2026 Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipereveal.h"

#include "ipeutils.h"

#include "ipecairopainter.h"

#include <cairo-svg.h>
#include <cairo.h>

#include <cstring>

using namespace ipe;

// --------------------------------------------------------------------

class JsonPainter : public CairoPainter {
public:
    JsonPainter(Stream & ss, const Cascade * sheet, Fonts * fonts, cairo_t * cc);

protected:
    virtual void doNewPath();
    virtual void doMoveTo(const Vector & v);
    virtual void doLineTo(const Vector & v);
    virtual void doCurveTo(const Vector & v1, const Vector & v2, const Vector & v3);
    virtual void doClosePath();
    virtual void doDrawPath(TPathMode mode);
    virtual void doDrawText(const Text * text);
    virtual void doDrawBitmap(Bitmap bitmap);
    virtual void doDrawArc(const Arc & arc);

    virtual void drawGlyphs(std::vector<cairo_glyph_t> & glyphs);
    void emitPath(cairo_path_t * path, bool stroke, bool fill);
    virtual void opStrokeFill(bool close, bool fill, bool stroke, bool eofill);

private:
    Stream & iOut;
    bool iObjectsWritten;
    bool iPathElementWritten;
};

JsonPainter::JsonPainter(Stream & ss, const Cascade * sheet, Fonts * fonts, cairo_t * cc)
    : CairoPainter(sheet, fonts, cc, 1.0, true, true)
    , iOut{ss} {
    iObjectsWritten = false;
}

void JsonPainter::doNewPath() {
    if (iObjectsWritten) iOut << ",\n";
    iOut << "{ \"points\": [\n";
    iPathElementWritten = false;
}

void JsonPainter::doMoveTo(const Vector & v) {
    if (iPathElementWritten) iOut << ",\n";
    iOut << "{ \"x\": " << v.x << ", \"y\": " << v.y << ", \"moveto\": true}";
    iPathElementWritten = true;
}

void JsonPainter::doLineTo(const Vector & v) {
    if (iPathElementWritten) iOut << ",\n";
    iOut << "{ \"x\": " << v.x << ", \"y\": " << v.y << "}";
    iPathElementWritten = true;
}

void JsonPainter::doCurveTo(const Vector & v1, const Vector & v2, const Vector & v3) {
    if (iPathElementWritten) iOut << ",\n";
    iOut << "{ \"x\": " << v3.x << ", \"y\": " << v3.y << ", \"x1\": " << v1.x
	 << ", \"y1\": " << v1.y << ", \"x2\": " << v2.x << ", \"y2\": " << v2.y << "}";
    iPathElementWritten = true;
}

void JsonPainter::doClosePath() {
    if (iPathElementWritten) iOut << ",\n";
    iOut << "{ \"close\": true}";
    iPathElementWritten = true;
}

void JsonPainter::doDrawPath(TPathMode mode) {
    iOut << "\n]";
    if (mode >= EStrokedAndFilled) {
	Color fillColor = fill();
	int transparency = int(100 * (1.0 - opacity().toDouble()));
	int red = int(255 * fillColor.iRed.toDouble());
	int green = int(255 * fillColor.iGreen.toDouble());
	int blue = int(255 * fillColor.iBlue.toDouble());
	iOut << ",\n\"fill\": { \"color\": \"";
	iOut.putHexByte(red);
	iOut.putHexByte(green);
	iOut.putHexByte(blue);
	iOut << "\"";
	if (transparency != 0) iOut << ", \"transparency\": " << transparency;
	iOut << "}";
    }
    if (mode <= EStrokedAndFilled) {
	double lw = pen().toDouble();
	Color strokeColor = stroke();
	int transparency = int(100 * (1.0 - strokeOpacity().toDouble()));
	int red = int(255 * strokeColor.iRed.toDouble());
	int green = int(255 * strokeColor.iGreen.toDouble());
	int blue = int(255 * strokeColor.iBlue.toDouble());
	iOut << ",\n\"line\": { \"color\": \"";
	iOut.putHexByte(red);
	iOut.putHexByte(green);
	iOut.putHexByte(blue);
	iOut << "\"";
	if (transparency != 0) iOut << ", \"transparency\": " << transparency;
	String s = dashStyle();
	if (s == "[4] 0")
	    iOut << ", \"dashType\": \"dash\"";
	else if (s == "[1 3] 0")
	    iOut << ", \"dashType\": \"lgDash\"";
	else if (s == "[4 2 1 2] 0")
	    iOut << ", \"dashType\": \"dashDot\"";
	else if (s == "[4 2 1 2 1 2] 0")
	    iOut << ", \"dashType\": \"lgDashDotDot\"";
	iOut << ", \"width\": " << lw << "}";
    }
    iOut << "\n}";
    iObjectsWritten = true;
}

void JsonPainter::doDrawArc(const Arc & arc) { Painter::doDrawArc(arc); }

static const char * const horizontal_alignment_names[] = {"left", "right", "hcenter"};

static const char * const vertical_alignment_names[] = {"bottom", "baseline", "top",
							"vcenter"};

static const char * const latex_sizes[] = {"\\tiny",       "\\footnotesize", "\\small",
					   "\\normalsize", "\\large",        "\\Large",
					   "\\LARGE",      "\\huge",         "\\Huge"};

static const int latex_point_sizes[] = {5, 8, 9, 10, 12, 14, 17, 20, 25};

void JsonPainter::doDrawText(const Text * text) {
    if (iObjectsWritten) iOut << ",\n";
    iObjectsWritten = true;
    Vector pos = matrix() * Vector(0, 0);
    iOut << "{ \"text\": \"";
    iOut.putJsonString(text->text());
    iOut << "\", \"x\": " << pos.x << ", \"y\": " << pos.y;
    Color color = stroke();
    int transparency = int(100 * (1.0 - opacity().toDouble()));
    int red = int(255 * color.iRed.toDouble());
    int green = int(255 * color.iGreen.toDouble());
    int blue = int(255 * color.iBlue.toDouble());
    iOut << ", \"color\": \"";
    iOut.putHexByte(red);
    iOut.putHexByte(green);
    iOut.putHexByte(blue);
    iOut << "\"";
    if (transparency != 0) iOut << ", \"transparency\": " << transparency;
    iOut << ", \"width\": " << text->width();
    iOut << ", \"height\": " << text->height();
    iOut << ", \"depth\": " << text->depth();
    iOut << ", \"minipage\": " << (text->isMinipage() ? "true" : "false");
    iOut << ", \"verticalAlignment\": \""
	 << vertical_alignment_names[text->verticalAlignment()] << "\"";
    iOut << ", \"horizontalAlignment\": \""
	 << horizontal_alignment_names[text->horizontalAlignment()] << "\"";
    iOut << ", \"style\": \"" << text->style().string() << "\"";
    Attribute size = iCascade->find(ETextSize, text->size());
    double stretch = iCascade->find(ETextStretch, text->size()).number().toDouble();
    double fontSize = 10 * stretch;
    if (size.isNumber()) {
	fontSize = size.number().toDouble() * stretch;
    } else if (size.isString()) {
	String s = size.string();
	for (size_t i = 0; i < sizeof(latex_point_sizes) / sizeof(int); ++i) {
	    if (s == latex_sizes[i]) {
		fontSize = latex_point_sizes[i] * stretch;
		break;
	    }
	}
    }
    iOut << ", \"size\": " << fontSize << "}";
    CairoPainter::doDrawText(text);
    iOut << ",\n{\"endtext\": true}";
}

void JsonPainter::drawGlyphs(std::vector<cairo_glyph_t> & glyphs) {
    PdfState & ps = iPdfState.back();
    if (!ps.iFont) return;

    Matrix m = iTextMatrix
	       * Matrix(ps.iFontSize * ps.iHorizontalScaling, 0, 0, ps.iFontSize, 0,
			ps.iTextRise)
	       * Linear(1, 0, 0, -1);

    cairo_matrix_t matrix;
    cairoMatrix(matrix, m);

    if (ps.iFont->type() == FontType::Type3) return;

    cairo_save(iCairo);
    cairo_set_font_face(iCairo, ps.iFont->cairoFont());
    cairo_set_font_matrix(iCairo, &matrix);
    cairo_new_path(iCairo);
    cairo_glyph_path(iCairo, glyphs.data(), glyphs.size());
    // copy_path returns current user space; we want device space
    cairo_identity_matrix(iCairo);
    cairo_path_t * path = cairo_copy_path(iCairo);
    emitPath(path, false, true);
    cairo_path_destroy(path);
    cairo_new_path(iCairo);
    cairo_restore(iCairo);
}

void JsonPainter::emitPath(cairo_path_t * path, bool stroke, bool fill) {
    if (iObjectsWritten) iOut << ",\n";
    iObjectsWritten = true;
    iOut << "{ \"points\": [\n";
    for (int i = 0; i < path->num_data;) {
	const cairo_path_data_t * data = &path->data[i];
	const auto type = data->header.type;

	if (i > 0) iOut << ",";

	switch (type) {
	case CAIRO_PATH_MOVE_TO: {
	    const auto & p = data[1].point;
	    iOut << "{ \"x\": " << p.x << ", \"y\": " << p.y << ", \"moveto\": true}";
	    break;
	}
	case CAIRO_PATH_LINE_TO: {
	    const auto & p = data[1].point;
	    iOut << "{ \"x\": " << p.x << ", \"y\": " << p.y << "}";
	    break;
	}
	case CAIRO_PATH_CURVE_TO: {
	    const auto & v1 = data[1].point;
	    const auto & v2 = data[2].point;
	    const auto & v3 = data[3].point;
	    iOut << "{ \"x\": " << v3.x << ", \"y\": " << v3.y << ", \"x1\": " << v1.x
		 << ", \"y1\": " << v1.y << ", \"x2\": " << v2.x << ", \"y2\": " << v2.y
		 << "}";
	    break;
	}
	case CAIRO_PATH_CLOSE_PATH: iOut << "{ \"close\": true}"; break;
	}

	i += data->header.length;
    }
    iOut << "]";
    PdfState & ps = iPdfState.back();
    if (fill) {
	int transparency = int(100 * (1.0 - ps.iFillOpacity));
	int red = int(255 * ps.iFillRgb[0]);
	int green = int(255 * ps.iFillRgb[1]);
	int blue = int(255 * ps.iFillRgb[2]);
	iOut << ", \"fill\": { \"color\": \"";
	iOut.putHexByte(red);
	iOut.putHexByte(green);
	iOut.putHexByte(blue);
	iOut << "\"";
	if (transparency != 0) iOut << ", \"transparency\": " << transparency;
	iOut << "}";
    }
    if (stroke) {
	int transparency = int(100 * (1.0 - ps.iStrokeOpacity));
	int red = int(255 * ps.iStrokeRgb[0]);
	int green = int(255 * ps.iStrokeRgb[1]);
	int blue = int(255 * ps.iStrokeRgb[2]);
	iOut << ", \"line\": { \"color\": \"";
	iOut.putHexByte(red);
	iOut.putHexByte(green);
	iOut.putHexByte(blue);
	iOut << "\"";
	if (transparency != 0) iOut << ", \"transparency\": " << transparency;
	iOut << "}";
    }
    iOut << "}";
}

void JsonPainter::opStrokeFill(bool close, bool fill, bool stroke, bool eofill) {
    if (close) cairo_close_path(iCairo);
    cairo_save(iCairo);
    cairo_identity_matrix(iCairo);
    cairo_path_t * path = cairo_copy_path(iCairo);
    emitPath(path, stroke, fill);
    cairo_path_destroy(path);
    cairo_restore(iCairo);
}

static cairo_status_t stream_writer(void * closure, const unsigned char * data,
				    unsigned int length) {
    Stream * ss = (Stream *)closure;
    ss->putRaw((const char *)data, length);
    return CAIRO_STATUS_SUCCESS;
}

void JsonPainter::doDrawBitmap(Bitmap bitmap) {
    if (iObjectsWritten) iOut << ",\n";
    iObjectsWritten = true;
    Vector tl = matrix() * Vector(0.0, 1.0);
    Vector tr = matrix() * Vector(1.0, 1.0);
    Vector bl = matrix() * Vector(0.0, 0.0);
    iOut << "{\"image\": \"";
    Base64Stream b64(iOut, true);
    if (bitmap.isJpeg()) {
	Buffer buffer = bitmap.embed().first;
	const char * p = buffer.data();
	const char * fin = p + buffer.size();
	while (p != fin) b64.putChar(*p++);
    } else {
	cairo_surface_t * image = cairo_image_surface_create(
	    CAIRO_FORMAT_ARGB32, bitmap.width(), bitmap.height());
	memcpy(cairo_image_surface_get_data(image), bitmap.pixelData().data(),
	       4 * bitmap.width() * bitmap.height());
	cairo_surface_mark_dirty(image);
	cairo_surface_flush(image);
	cairo_surface_write_to_png_stream(image, &stream_writer, (void *)&b64);
    }
    b64.close();
    iOut << "\", \"x\": " << tl.x << ", \"y\": " << tl.y;
    iOut << ", \"w\": " << (tr - tl).len();
    iOut << ", \"h\": " << (bl - tl).len();
    iOut << ", \"format\": " << (bitmap.isJpeg() ? "\"jpeg\"" : "\"png\"");
    int transparency = int(100 * (1.0 - opacity().toDouble()));
    if (transparency != 0) iOut << ", \"transparency\": " << transparency;
    iOut << "}";
}

// --------------------------------------------------------------------

static cairo_status_t dummy_writer(void * /* closure */, const unsigned char * /* data */,
				   unsigned int /* length */) {
    // dummy
    return CAIRO_STATUS_SUCCESS;
}

void Reveal::createJsonView(Stream & out, int pageNo, int view, bool & sep) {
    if (sep) out << ",\n";
    sep = true;

    Attribute variant = iDoc->variant();
    const Page * page = iDoc->page(pageNo);

    out << "{\n\"page\": " << pageNo + 1 << ", \"view\": " << view + 1
	<< ", \"objects\": [\n";

    cairo_surface_t * surface =
	cairo_svg_surface_create_for_stream(&dummy_writer, nullptr, iWidth, iHeight);
    cairo_t * cc = cairo_create(surface);

    JsonPainter painter(out, iDoc->cascade(), iFonts.get(), cc);

    const auto viewMap = page->viewMap(view, iDoc->cascade());
    painter.setAttributeMap(&viewMap);
    std::vector<Matrix> layerMatrices = page->layerMatrices(view);
    painter.pushMatrix();

    Attribute bg = page->backgroundSymbol(iDoc->cascade());
    const Symbol * background = iDoc->cascade()->findSymbol(bg);
    if (background && page->findLayer("BACKGROUND") < 0) painter.drawSymbol(bg);

    const Text * title = page->titleText(variant);
    if (title) { title->draw(painter); }

    for (int i = 0; i < page->count(); ++i) {
	if (page->objectVisible(view, i)) {
	    auto obj = page->object(i);
	    if (!obj->displayInVariant(variant)) continue;
	    if (obj->type() == Object::EText) {}
	    painter.pushMatrix();
	    painter.transform(layerMatrices[page->layerOf(i)]);
	    obj->draw(painter);
	    painter.popMatrix();
	}
    }
    painter.popMatrix();
    out << "]\n"; // objects
    out << "}";   // page
    cairo_destroy(cc);
    cairo_surface_destroy(surface);
}

bool Reveal::createJson(const char * destination, bool onlyMarked) {
    std::FILE * file = Platform::fopen(destination, "wb");
    if (!file) return false;
    FileStream out(file);

    out << "{\n";
    if (!iDoc->properties().iTitle.empty()) {
	out << "\"title\": \"";
	out.putJsonString(iDoc->properties().iTitle);
	out << "\",\n";
    }
    if (!iDoc->properties().iAuthor.empty()) {
	out << "\"author\": \"";
	out.putJsonString(iDoc->properties().iAuthor);
	out << "\",\n";
    }
    if (!iDoc->properties().iKeywords.empty()) {
	out << "\"keywords\": \"";
	out.putJsonString(iDoc->properties().iKeywords);
	out << "\",\n";
    }
    if (!iDoc->properties().iSubject.empty()) {
	out << "\"subject\": \"";
	out.putJsonString(iDoc->properties().iSubject);
	out << "\",\n";
    }
    out << "\"created\": \"" << iDoc->properties().iCreated << "\",\n";
    out << "\"modified\": \"" << iDoc->properties().iModified << "\",\n";
    Vector offset = iLayout->paper().topLeft();

    out << "\"width\": " << iWidth << ",\n";
    out << "\"height\": " << iHeight << ",\n";
    out << "\"originX\": " << -offset.x << ",\n";
    out << "\"originY\": " << offset.y << ",\n";

    out << "\"pages\": [\n";

    bool sep = false;

    for (int pageNo = iFromPage; pageNo <= iToPage; ++pageNo) {
	const Page * page = iDoc->page(pageNo);
	int nViews = page->countViews();
	if (onlyMarked) {
	    if (!page->marked()) continue;
	    bool shown = false;
	    for (int view = 0; view < nViews; ++view) {
		if (page->markedView(view)) {
		    createJsonView(out, pageNo, view, sep);
		    shown = true;
		}
	    }
	    if (!shown) createJsonView(out, pageNo, nViews - 1, sep);
	} else {
	    for (int view = 0; view < nViews; ++view)
		createJsonView(out, pageNo, view, sep);
	}
    }
    out << "]\n"; // pages
    out << "}\n"; // document
    out.close();
    fclose(file);
    return true;
}

// --------------------------------------------------------------------

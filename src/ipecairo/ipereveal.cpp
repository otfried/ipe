// --------------------------------------------------------------------
// Create a reveal/gsap presentation
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

#include "ipecairopainter.h"

#include "ipegroup.h"

#include <cairo.h>
#include <cairo-svg.h>

#include <cstring>

using namespace ipe;

// --------------------------------------------------------------------

Reveal::Reveal(const Document * doc, int fromPage, int toPage)
    : iDoc{doc}
    , iFromPage{fromPage}
    , iToPage{toPage} {
    iLayout = iDoc->cascade()->findLayout();
    iWidth = int(iLayout->paper().width());
    iHeight = int(iLayout->paper().height());
    iFonts = std::make_unique<Fonts>(doc->resources());
}

static cairo_status_t stream_writer(void * closure, const unsigned char * data,
				    unsigned int length) {
    StringStream * ss = (StringStream *)closure;
    ss->putRaw((const char *)data, length);
    return CAIRO_STATUS_SUCCESS;
}

String Reveal::createSVG() {
    double tolerance = 0.1;

    String svg;
    StringStream svgWriter(svg);
    cairo_surface_t * surface = cairo_svg_surface_create_for_stream(
	&stream_writer, (void *)&svgWriter, iWidth, iHeight);

    cairo_t * cc = cairo_create(surface);
    // cairo_translate(cc, -offset.x, -offset.y);

    cairo_set_tolerance(cc, tolerance);

    CairoPainter painter(iDoc->cascade(), iFonts.get(), cc, 1.0, true, true);
    Attribute variant = iDoc->variant();

    auto flushCairo = [&]() {
	cairo_surface_flush(surface);
	cairo_show_page(cc);
    };

    for (int pageNo = iFromPage; pageNo <= iToPage; ++pageNo) {
	const Page * page = iDoc->page(pageNo);

	for (int view = 0; view < page->countViews(); ++view) {
	    const auto viewMap = page->viewMap(view, iDoc->cascade());
	    painter.setAttributeMap(&viewMap);
	    std::vector<Matrix> layerMatrices = page->layerMatrices(view);
	    painter.pushMatrix();

	    Attribute bg = page->backgroundSymbol(iDoc->cascade());
	    const Symbol * background = iDoc->cascade()->findSymbol(bg);
	    if (background && page->findLayer("BACKGROUND") < 0) {
		painter.drawSymbol(bg);
		flushCairo();
	    }

	    const Text * title = page->titleText(variant);
	    if (title) {
		title->draw(painter);
		flushCairo();
	    }

	    for (int i = 0; i < page->count(); ++i) {
		if (page->objectVisible(view, i)) {
		    auto obj = page->object(i);
		    if (!obj->displayInVariant(variant)) continue;
		    painter.pushMatrix();
		    painter.transform(layerMatrices[page->layerOf(i)]);
		    obj->draw(painter);
		    painter.popMatrix();
		    flushCairo();
		}
	    }
	    painter.popMatrix();
	}
    }

    cairo_destroy(cc);
    cairo_surface_destroy(surface);
    return svg;
}

bool Reveal::writeSlides(Stream & out, String svg) {
    std::string_view sv{svg.data(), size_t(svg.size())};

    size_t startDefs = sv.find("<defs>");
    size_t endDefs = sv.find("</defs>");
    size_t pageSet = sv.find("<pageSet>");
    size_t endPageSet = sv.find("</pageSet>");
    ipeDebug("startDefs = %d, endDefs = %d, endPageSet = %d", startDefs, endDefs, endPageSet);
    if (pageSet == std::string_view::npos || endPageSet == std::string_view::npos)
	return false;

    size_t p;
    auto skipSpace = [&]() {
	while (p < endPageSet && std::isspace(sv[p])) ++p;
    };

    if (startDefs != std::string_view::npos) {
	p = endDefs + 7;
	skipSpace();
	if (p != pageSet) return false;
    }
    p = pageSet + 9;
    skipSpace();

    struct SObject {
	size_t start, fin;
	bool path;
    };
    std::vector<SObject> objs;

    while (p != endPageSet) {
	if (strncmp(&sv[p], "<page>", 6)) return false;
	p += 6;
	skipSpace();
	size_t start = p;
	size_t finTag = sv.find("/>", start);
	size_t fin = sv.find("</page>", start);
	if (finTag == std::string_view::npos || fin == std::string_view::npos) return false;
	p = finTag + 2;
	skipSpace();
	bool isPath = p == fin && !strncmp(&sv[start], "<path ", 6);
	objs.emplace_back(SObject{start, fin, isPath});
	p = fin + 7;
	skipSpace();
    }

    out.putCString("<div style=\"display: none;\">\n<svg><defs id=\"ipe-definitions\">");
    if (startDefs != std::string_view::npos)
	out.putRaw(sv.data() + startDefs + 6, endDefs - startDefs - 6);

    size_t count = 0;
    auto handleObject = [&](int pageNo, int view, int objNo, const Object * obj) {
	if (count >= objs.size()) return false;
	SObject & o = objs[count++];
	if (o.start == o.fin) {
	    ipeDebug("Empty object: %d %d %d", pageNo, view, objNo);
	    return false;
	} else if (obj && o.path) {
	    out << "<path id=\"ipe-" << pageNo << "-" << objNo << "-" << view << "\"";
	    if (!obj->getCustom().isUndefined()) {
		out << " data-ipe-id=\"";
		out.putXmlString(obj->getCustom().string());
		out << "\"";
	    }
	    out.putRaw(&sv[o.start + 5], o.fin - o.start - 5);
	} else {
	    out << "<g id=\"ipe-" << pageNo << "-" << objNo << "-" << view << "\"";
	    if (obj && !obj->getCustom().isUndefined()) {
		out << " data-ipe-id=\"";
		out.putXmlString(obj->getCustom().string());
		out << "\"";
		Matrix m = obj->matrix();
		out << " data-ipe-matrix=\""
		    << m.a[0] << ","
		    << m.a[1] << ","
		    << m.a[2] << ","
		    << m.a[3] << ","
		    << m.a[4] << ","
		    << m.a[5] << "\"";
	    }
	    out << ">\n";
	    out.putRaw(&sv[o.start], o.fin - o.start);
	    out << "</g>\n";
	}
	return true;
    };

    Attribute variant = iDoc->variant();
    for (int pageNo = iFromPage; pageNo <= iToPage; ++pageNo) {
	const Page * page = iDoc->page(pageNo);
	for (int view = 0; view < page->countViews(); ++view) {
	    // TODO layer matrix in view for position computation?
	    Attribute bg = page->backgroundSymbol(iDoc->cascade());
	    const Symbol * background = iDoc->cascade()->findSymbol(bg);
	    if (background && page->findLayer("BACKGROUND") < 0
		&& !handleObject(pageNo, view, -2, nullptr))
		return false;

	    const Text * title = page->titleText(variant);
	    if (title && !handleObject(pageNo, view, -1, nullptr)) return false;

	    for (int i = 0; i < page->count(); ++i) {
		if (page->objectVisible(view, i)) {
		    auto obj = page->object(i);
		    if (!obj->displayInVariant(variant)) continue;
		    if (!handleObject(pageNo, view, i, obj)) return false;
		}
	    }
	}
    }

    out.putCString("</defs>\n</svg>\n</div>\n\n");

    out << "<div class=\"reveal\">\n<div class=\"slides\">\n\n";

    Vector offset = iLayout->paper().topLeft();

    auto showObject = [&](int pageNo, int view, int objNo) {
    	out << "<use href=\"#ipe-" << pageNo << "-" << objNo << "-" << view << "\" />\n";
    };

    for (int pageNo = iFromPage; pageNo <= iToPage; ++pageNo) {
	const Page * page = iDoc->page(pageNo);
	if (page->countViews() > 1)
	    out << "<section>\n";
	for (int view = 0; view < page->countViews(); ++view) {
	    out << "<section>\n"
		<< "<svg width=\"100%\" height=\"100%\" viewBox=\"0 0 "
		<< iWidth << " " << iHeight << "\">\n"
		<< "<g transform=\"matrix(1 0 0 -1 "
		<< -offset.x << " " << offset.y << ")\">\n";

	    Attribute bg = page->backgroundSymbol(iDoc->cascade());
	    const Symbol * background = iDoc->cascade()->findSymbol(bg);
	    if (background && page->findLayer("BACKGROUND") < 0)
		showObject(pageNo, view, -2);

	    const Text * title = page->titleText(variant);
	    if (title)
		showObject(pageNo, view, -1);

	    for (int i = 0; i < page->count(); ++i) {
		if (page->objectVisible(view, i)) {
		    auto obj = page->object(i);
		    if (!obj->displayInVariant(variant)) continue;
		    showObject(pageNo, view, i);
		}
	    }
	    out << "</g></svg>\n</section>\n";
	}
	if (page->countViews() > 1)
	    out << "</section>\n";
    }
    out << "\n</div>\n</div>\n";
    return true;
}

bool Reveal::createPresentation(const char * destination) {
    String svg = createSVG();

#ifdef IPEWASM
    String reveal = Platform::readFile("/opt/ipe/reveal/reveal.html");
#else
    String reveal = Platform::readFile("ipecairo/reveal.html");
#endif

    std::string_view sv{reveal.data(), size_t(reveal.size())};
    size_t titleIndex = sv.find("[TITLE]");
    size_t ipeIndex = sv.find("[IPE]");
    if (titleIndex == std::string_view::npos ||
	ipeIndex == std::string_view::npos)
	return false;

    ipeDebug("Template has %d bytes, inserting at %d", reveal.size(), ipeIndex);
    
    std::FILE * file = Platform::fopen(destination, "wb");
    if (!file) return false;
    FileStream out(file);

    out.putRaw(sv.data(), titleIndex);

    if (iDoc->properties().iTitle.empty())
	out << "Ipe Presentation";
    else
	out << iDoc->properties().iTitle;

    out.putRaw(sv.data() + titleIndex + 7, ipeIndex - titleIndex - 7);
    
    if (!writeSlides(out, svg))
	return false;

    out.putRaw(sv.data() + ipeIndex + 5, sv.size() - ipeIndex - 5);

    out.close();
    fclose(file);
    return true;
}

// --------------------------------------------------------------------

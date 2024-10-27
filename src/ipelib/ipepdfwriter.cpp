// --------------------------------------------------------------------
// Creating PDF output
// --------------------------------------------------------------------
/*

	This file is part of the extensible drawing editor Ipe.
	Copyright (c) 1993-2024 Otfried Cheong

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

#include "ipegroup.h"
#include "ipeimage.h"
#include "ipepainter.h"
#include "ipereference.h"
#include "ipetext.h"
#include "ipeutils.h"

#include "ipepdfparser.h"
#include "ipepdfwriter.h"
#include "iperesources.h"

using namespace ipe;

typedef std::vector<Bitmap>::const_iterator BmIter;

// --------------------------------------------------------------------

PdfPainter::PdfPainter(const Cascade * style, Stream & stream)
    : Painter(style)
    , iStream(stream) {
    State state;
    state.iStroke = Color(0, 0, 0);
    state.iFill = Color(0, 0, 0);
    state.iPen = Fixed(1);
    state.iDashStyle = "[]0";
    state.iLineCap = style->lineCap();
    state.iLineJoin = style->lineJoin();
    state.iOpacity = Fixed(1);
    state.iStrokeOpacity = Fixed(1);
    iStream << state.iLineCap - 1 << " J " << state.iLineJoin - 1 << " j\n";
    iActiveState.push_back(state);
}

void PdfPainter::doNewPath() { drawAttributes(); }

void PdfPainter::doMoveTo(const Vector & v) { iStream << v << " m\n"; }

void PdfPainter::doLineTo(const Vector & v) { iStream << v << " l\n"; }

void PdfPainter::doCurveTo(const Vector & v1, const Vector & v2, const Vector & v3) {
    iStream << v1 << " " << v2 << " " << v3 << " c\n";
}

void PdfPainter::doClosePath() { iStream << "h "; }

void PdfPainter::doAddClipPath() { iStream << "W* n "; }

void PdfPainter::doPush() {
    State state = iActiveState.back();
    iActiveState.push_back(state);
    iStream << "q ";
}

void PdfPainter::doPop() {
    iActiveState.pop_back();
    iStream << "Q\n";
}

void PdfPainter::drawColor(Stream & stream, Color color, const char * gray,
			   const char * rgb) {
    if (color.isGray())
	stream << color.iRed << " " << gray << "\n";
    else
	stream << color << " " << rgb << "\n";
}

void PdfPainter::drawAttributes() {
    State & s = iState.back();
    State & sa = iActiveState.back();
    if (s.iDashStyle != sa.iDashStyle) {
	sa.iDashStyle = s.iDashStyle;
	iStream << s.iDashStyle << " d\n";
    }
    if (s.iPen != sa.iPen) {
	sa.iPen = s.iPen;
	iStream << s.iPen << " w\n";
    }
    if (s.iLineCap != sa.iLineCap) {
	sa.iLineCap = s.iLineCap;
	iStream << s.iLineCap - 1 << " J\n";
    }
    if (s.iLineJoin != sa.iLineJoin) {
	sa.iLineJoin = s.iLineJoin;
	iStream << s.iLineJoin - 1 << " j\n";
    }
    if (s.iStroke != sa.iStroke) {
	sa.iStroke = s.iStroke;
	drawColor(iStream, s.iStroke, "G", "RG");
    }
    if (s.iFill != sa.iFill || !s.iTiling.isNormal()) {
	sa.iFill = s.iFill;
	if (!s.iTiling.isNormal()) {
	    iStream << "/PCS cs\n";
	    s.iFill.saveRGB(iStream);
	    iStream << " /Pat" << s.iTiling.index() << " scn\n";
	} else
	    drawColor(iStream, s.iFill, "g", "rg");
    }
    drawOpacity(true);
}

static const char * opacityName(Fixed alpha) {
    static char buf[12];
    sprintf(buf, "/alpha%03d", alpha.internal());
    return buf;
}

void PdfPainter::drawOpacity(bool withStroke) {
    State & s = iState.back();
    State & sa = iActiveState.back();
    if (s.iOpacity != sa.iOpacity) {
	sa.iOpacity = s.iOpacity;
	sa.iStrokeOpacity = s.iOpacity;
	iStream << opacityName(s.iOpacity) << " gs\n";
    }
    if (withStroke && s.iStrokeOpacity != sa.iStrokeOpacity) {
	iStream << opacityName(s.iStrokeOpacity) << "s gs\n";
    }
}

// If gradient fill is set, then StrokeAndFill does NOT stroke!
void PdfPainter::doDrawPath(TPathMode mode) {
    bool eofill = (fillRule() == EEvenOddRule);

    // if (!mode)
    // iStream << "n\n"; // no op path

    const Gradient * g = nullptr;
    Attribute grad = iState.back().iGradient;
    if (!grad.isNormal()) g = iCascade->findGradient(grad);

    if (g) {
	if (mode == EStrokedOnly)
	    iStream << "S\n";
	else
	    iStream << (eofill ? "q W* n " : "q W n ") << matrix() * g->iMatrix
		    << " cm /Grad" << grad.index() << " sh Q\n";
    } else {
	if (mode == EFilledOnly)
	    iStream << (eofill ? "f*\n" : "f\n");
	else if (mode == EStrokedOnly)
	    iStream << "S\n";
	else
	    iStream << (eofill ? "B*\n" : "B\n"); // fill and then stroke
    }
}

void PdfPainter::doDrawBitmap(Bitmap bitmap) {
    if (bitmap.objNum() < 0) return;
    drawOpacity(false);
    iStream << matrix() << " cm /Image" << bitmap.objNum() << " Do\n";
}

void PdfPainter::doDrawText(const Text * text) {
    const Text::XForm * xf = text->getXForm();
    if (!xf) return;
    drawOpacity(false);
    pushMatrix();
    transform(Matrix(xf->iStretch, 0, 0, xf->iStretch, 0, 0));
    translate(xf->iTranslation);
    iStream << matrix() << " cm ";
    iStream << "/" << xf->iName << " Do\n";
    popMatrix();
}

void PdfPainter::doDrawSymbol(Attribute symbol) {
    const Symbol * sym = iAttributeMap
			     ? cascade()->findSymbol(iAttributeMap->map(ESymbol, symbol))
			     : cascade()->findSymbol(symbol);
    if (!sym) return;
    if (sym->iXForm)
	iStream << "/Symbol" << symbol.index() << " Do\n";
    else
	sym->iObject->draw(*this);
}

// --------------------------------------------------------------------

/*! \class ipe::PdfWriter
  \brief Create PDF file.

  This class is responsible for the creation of a PDF file from the
  Ipe data. You have to create a PdfWriter first, providing a file
  that has been opened for (binary) writing and is empty.  Then call
  createPages() to embed the pages.  Optionally, call \c
  createXmlStream to embed a stream with the XML representation of the
  document. Finally, call \c createTrailer to complete the PDF
  document, and close the file.

  Some reserved PDF object numbers:

	- 0: Must be left empty (a PDF restriction).
	- 1:  XML stream.
	- 2: Parent of all pages objects.
	- 3: ExtGState resource from pdflatex
	- 4: Shading resource from pdflatex
	- 5: Pattern resource from pdflatex
	- 6: ColorSpace resource from pdflatex
*/

//! Create a PDF writer operating on this (open and empty) file.
PdfWriter::PdfWriter(TellStream & stream, const Document * doc,
		     const PdfResources * resources, uint32_t flags, int fromPage,
		     int toPage, int compression)
    : iStream(stream)
    , iDoc(doc)
    , iResources(resources)
    , iSaveFlags(flags)
    , iFromPage(fromPage)
    , iToPage(toPage) {
    iCompressLevel = compression;
    iObjNum = 7;        // 0 - 6 are reserved
    iXmlStreamNum = -1; // no XML stream yet
    iExtGState = -1;
    iPatternNum = -1;
    iBookmarks = -1;
    iDests = -1;

    if (iFromPage < 0 || iFromPage >= iDoc->countPages()) iFromPage = 0;
    if (iToPage < iFromPage || iToPage >= iDoc->countPages())
	iToPage = iDoc->countPages() - 1;

    // mark all bitmaps as not embedded
    BitmapFinder bm;
    iDoc->findBitmaps(bm);
    int id = -1;
    for (std::vector<Bitmap>::iterator it = bm.iBitmaps.begin(); it != bm.iBitmaps.end();
	 ++it) {
	it->setObjNum(id);
	--id;
    }

    iStream << "%PDF-1.5\n";

    // embed all fonts and other resources from Pdflatex
    embedResources();

    // embed resources from pdflatex
    embedLatexResource(3, "ExtGState");
    embedLatexResource(4, "Shading");
    embedLatexResource(5, "Pattern");
    embedLatexResource(6, "ColorSpace");

    // embed all extgstate objects
    AttributeSeq os;
    iDoc->cascade()->allNames(EOpacity, os);
    if (os.size() > 0) {
	iExtGState = startObject();
	iStream << "<<\n";
	for (const auto & obj : os) {
	    Attribute alpha = iDoc->cascade()->find(EOpacity, obj);
	    assert(alpha.isNumber());
	    iStream << opacityName(alpha.number()) << " << /CA " << alpha.number()
		    << " /ca " << alpha.number() << " >>\n";
	    iStream << opacityName(alpha.number()) << "s << /CA " << alpha.number()
		    << " >>\n";
	}
	iStream << ">> endobj\n";
    }

    // embed all gradients
    AttributeSeq gs;
    iDoc->cascade()->allNames(EGradient, gs);
    for (const auto & grad : gs) {
	const Gradient * g = iDoc->cascade()->findGradient(grad);
	int num = startObject();
	iStream << "<<\n"
		<< " /ShadingType " << int(g->iType) << "\n"
		<< " /ColorSpace /DeviceRGB\n";
	if (g->iType == Gradient::EAxial)
	    iStream << " /Coords [" << g->iV[0] << " " << g->iV[1] << "]\n";
	else
	    iStream << " /Coords [" << g->iV[0] << " " << g->iRadius[0] << " " << g->iV[1]
		    << " " << g->iRadius[1] << "]\n";

	iStream << " /Extend [" << (g->iExtend ? "true true]\n" : "false false]\n");

	if (g->iStops.size() == 2) {
	    iStream << " /Function << /FunctionType 2 /Domain [ 0 1 ] /N 1\n"
		    << "     /C0 [";
	    g->iStops[0].color.saveRGB(iStream);
	    iStream << "]\n" << "     /C1 [";
	    g->iStops[1].color.saveRGB(iStream);
	    iStream << "] >>\n";
	} else {
	    // need to stitch
	    iStream << " /Function <<\n"
		    << "  /FunctionType 3 /Domain [ 0 1 ]\n"
		    << "  /Bounds [";
	    int count = 0;
	    for (int i = 1; i < size(g->iStops) - 1; ++i) {
		if (g->iStops[i].offset > g->iStops[i - 1].offset) {
		    iStream << g->iStops[i].offset << " ";
		    ++count;
		}
	    }
	    iStream << "]\n  /Encode [";
	    for (int i = 0; i <= count; ++i) iStream << "0.0 1.0 ";
	    iStream << "]\n  /Functions [\n";
	    for (int i = 1; i < size(g->iStops); ++i) {
		if (g->iStops[i].offset > g->iStops[i - 1].offset) {
		    iStream << "   << /FunctionType 2 /Domain [ 0 1 ] /N 1 /C0 [";
		    g->iStops[i - 1].color.saveRGB(iStream);
		    iStream << "] /C1 [";
		    g->iStops[i].color.saveRGB(iStream);
		    iStream << "] >>\n";
		}
	    }
	    iStream << "] >>\n";
	}
	iStream << ">> endobj\n";
	iGradients[grad.index()] = num;
    }

    // embed all tilings
    AttributeSeq ts;
    std::map<int, int> patterns;
    iDoc->cascade()->allNames(ETiling, ts);
    if (ts.size() > 0) {
	for (const auto & tiling : ts) {
	    const Tiling * t = iDoc->cascade()->findTiling(tiling);
	    Linear m(t->iAngle);
	    int num = startObject();
	    iStream << "<<\n"
		    << "/Type /Pattern\n"
		    << "/PatternType 1\n" // tiling pattern
		    << "/PaintType 2\n"   // uncolored pattern
		    << "/TilingType 2\n"  // faster
		    << "/BBox [ 0 0 100 " << t->iStep << " ]\n"
		    << "/XStep 100\n"
		    << "/YStep " << t->iStep << "\n"
		    << "/Resources << >>\n"
		    << "/Matrix [" << m << " 0 0]\n";
	    String s;
	    StringStream ss(s);
	    ss << "0 0 100 " << t->iWidth << " re f\n";
	    createStream(s.data(), s.size(), false);
	    patterns[tiling.index()] = num;
	}

	// create pattern dictionary
	iPatternNum = startObject();
	iStream << "<<\n";
	for (const auto & pattern : ts) {
	    iStream << "/Pat" << pattern.index() << " " << patterns[pattern.index()]
		    << " 0 R\n";
	}
	iStream << ">> endobj\n";
    }

    // embed all symbols with xform attribute
    AttributeSeq sys;
    iDoc->cascade()->allNames(ESymbol, sys);
    if (sys.size()) {
	for (const auto & sysi : sys) {
	    const Symbol * sym = iDoc->cascade()->findSymbol(sysi);
	    if (sym->iXForm) {
		// compute bbox for object
		BBoxPainter bboxPainter(iDoc->cascade());
		sym->iObject->draw(bboxPainter);
		Rect bbox = bboxPainter.bbox();
		// embed all bitmaps it uses
		BitmapFinder bm;
		sym->iObject->accept(bm);
		embedBitmaps(bm);
		int num = startObject();
		iStream << "<<\n";
		iStream << "/Type /XObject\n";
		iStream << "/Subtype /Form\n";
		iStream << "/BBox [" << bbox << "]\n";
		createResources(bm);
		String s;
		StringStream ss(s);
		PdfPainter painter(iDoc->cascade(), ss);
		sym->iObject->draw(painter);
		createStream(s.data(), s.size(), false);
		iSymbols[sysi.index()] = num;
	    }
	}
    }
}

//! Destructor.
PdfWriter::~PdfWriter() {
    // nothing
}

/*! Write the beginning of the next object: "no 0 obj " and save
  information about file position. Default argument uses next unused
  object number.  Returns number of new object. */
int PdfWriter::startObject(int objnum) {
    if (objnum < 0) objnum = iObjNum++;
    iXref[objnum] = iStream.tell();
    iStream << objnum << " 0 obj ";
    return objnum;
}

bool PdfWriter::hasResource(String kind) const noexcept {
    return iResources && iResources->resourcesOfKind(kind);
}

/*! Write all PDF resources to the PDF file, and record their object
  numbers.  Embeds nothing if \c resources is nullptr, but must be
  called nevertheless. */
void PdfWriter::embedResources() {
    bool inflate = (iCompressLevel == 0);
    if (iResources) {
	const std::vector<int> & seq = iResources->embedSequence();
	for (auto & num : seq) {
	    const PdfObj * obj = iResources->object(num);
	    int embedNum = startObject();
	    if (iResources->isIpeXForm(num) && obj->dict())
		embedIpeXForm(obj->dict());
	    else
		obj->write(iStream, &iResourceNumber, inflate);
	    iStream << " endobj\n";
	    iResourceNumber[num] = embedNum;
	}
    }
}

void PdfWriter::embedIpeXForm(const PdfDict * d) {
    bool inflate = (iCompressLevel == 0) && d->deflated();
    iStream << "<<";
    for (int i = 0; i < d->count(); ++i) {
	String key = d->key(i);
	// skip /IpeId etc keys
	if (key.left(3) == "Ipe") continue;
	if ((inflate && key == "Filter") || key == "Length")
	    continue; // remove /FlateDecode filter and /Length
	iStream << "/" << key << " ";
	if (key == "Resources") {
	    const PdfObj * res = d->value(i);
	    if (res->ref()) res = iResources->object(res->ref()->value());
	    if (res->dict())
		embedXFormResource(res->dict());
	    else // should not happen!
		d->value(i)->write(iStream, &iResourceNumber);
	} else if (key == "BBox") {
	    const TextPadding * pad = iDoc->cascade()->findTextPadding();
	    std::vector<double> bbox;
	    d->getNumberArray("BBox", nullptr, bbox);
	    if (pad && bbox.size() == 4) {
		bbox[0] -= pad->iLeft;
		bbox[1] -= pad->iBottom;
		bbox[2] += pad->iRight;
		bbox[3] += pad->iTop;
	    }
	    iStream << "[";
	    for (auto it = bbox.begin(); it != bbox.end(); ++it) iStream << *it << " ";
	    iStream << "]";
	} else
	    d->value(i)->write(iStream, &iResourceNumber);
	iStream << " ";
    }
    Buffer stream = inflate ? d->inflate() : d->stream();
    if (stream.size() > 0) {
	iStream << "/Length " << stream.size() << ">>\nstream\n";
	for (int i = 0; i < stream.size(); ++i) iStream.putChar(stream[i]);
	iStream << "\nendstream";
    } else
	iStream << ">>";
}

void PdfWriter::embedXFormResource(const PdfDict * d) {
    iStream << "<<";
    for (int i = 0; i < d->count(); ++i) {
	String key = d->key(i);
	iStream << "/" << key << " ";
	if (key == "ColorSpace" || key == "Shading" || key == "Pattern"
	    || key == "ExtGState") {
	    ipeDebug("PDF Writer: Conflicting resource in XForm: %s", key.z());
	} else
	    d->value(i)->write(iStream, &iResourceNumber);
    }
    if (hasResource("ExtGState")) iStream << "/ExtGState 3 0 R\n";
    if (hasResource("Shading")) iStream << "/ColorSpace 4 0 R\n";
    if (hasResource("Pattern")) iStream << "/Pattern 5 0 R\n";
    if (hasResource("ColorSpace")) iStream << "/ColorSpace 6 0 R\n";
    iStream << ">>";
}

void PdfWriter::embedLatexResource(int num, String kind) {
    if (hasResource(kind)) {
	startObject(num);
	iStream << "<<\n";
	embedResource(kind);
	iStream << ">> endobj\n";
    }
}

void PdfWriter::embedResource(String kind) {
    if (!iResources) return;
    const PdfDict * d = iResources->resourcesOfKind(kind);
    if (!d) return;
    for (int i = 0; i < d->count(); ++i) {
	iStream << "/" << d->key(i) << " ";
	d->value(i)->write(iStream, &iResourceNumber);
	iStream << " ";
    }
}

//! Write a stream.
/*! Write a stream, either plain or compressed, depending on compress
  level.  Object must have been created with dictionary start having
  been written.
  If \a preCompressed is true, the data is already deflated.
*/
void PdfWriter::createStream(const char * data, int size, bool preCompressed) {
    if (preCompressed) {
	iStream << "/Length " << size << " /Filter /FlateDecode >>\nstream\n";
	iStream.putRaw(data, size);
	iStream << "\nendstream endobj\n";
	return;
    }

    if (iCompressLevel > 0) {
	int deflatedSize;
	Buffer deflated =
	    DeflateStream::deflate(data, size, deflatedSize, iCompressLevel);
	iStream << "/Length " << deflatedSize << " /Filter /FlateDecode >>\nstream\n";
	iStream.putRaw(deflated.data(), deflatedSize);
	iStream << "\nendstream endobj\n";
    } else {
	iStream << "/Length " << size << " >>\nstream\n";
	iStream.putRaw(data, size);
	iStream << "endstream endobj\n";
    }
}

// --------------------------------------------------------------------

void PdfWriter::embedBitmap(Bitmap bitmap) {
    int smaskNum = -1;
    auto embed = bitmap.getEmbedData();
    if (bitmap.hasAlpha() && embed.second.size() > 0) {
	smaskNum = startObject();
	iStream << "<<\n";
	iStream << "/Type /XObject\n";
	iStream << "/Subtype /Image\n";
	iStream << "/Width " << bitmap.width() << "\n";
	iStream << "/Height " << bitmap.height() << "\n";
	iStream << "/ColorSpace /DeviceGray\n";
	iStream << "/Filter /FlateDecode\n";
	iStream << "/BitsPerComponent 8\n";
	iStream << "/Length " << embed.second.size() << "\n>> stream\n";
	iStream.putRaw(embed.second.data(), embed.second.size());
	iStream << "\nendstream endobj\n";
    }
    int objnum = startObject();
    iStream << "<<\n";
    iStream << "/Type /XObject\n";
    iStream << "/Subtype /Image\n";
    iStream << "/Width " << bitmap.width() << "\n";
    iStream << "/Height " << bitmap.height() << "\n";
    if (bitmap.isGray())
	iStream << "/ColorSpace /DeviceGray\n";
    else
	iStream << "/ColorSpace /DeviceRGB\n";
    if (bitmap.isJpeg())
	iStream << "/Filter /DCTDecode\n";
    else
	iStream << "/Filter /FlateDecode\n";
    iStream << "/BitsPerComponent 8\n";
    if (smaskNum >= 0) {
	iStream << "/SMask " << smaskNum << " 0 R\n";
    } else if (bitmap.colorKey() >= 0) {
	int r = (bitmap.colorKey() >> 16) & 0xff;
	int g = (bitmap.colorKey() >> 8) & 0xff;
	int b = bitmap.colorKey() & 0xff;
	iStream << "/Mask [" << r << " " << r;
	if (!bitmap.isGray()) iStream << " " << g << " " << g << " " << b << " " << b;
	iStream << "]\n";
    }
    iStream << "/Length " << embed.first.size() << "\n>> stream\n";
    iStream.putRaw(embed.first.data(), embed.first.size());
    iStream << "\nendstream endobj\n";
    bitmap.setObjNum(objnum);
}

void PdfWriter::embedBitmaps(const BitmapFinder & bm) {
    for (BmIter it = bm.iBitmaps.begin(); it != bm.iBitmaps.end(); ++it) {
	BmIter it1 = std::find(iBitmaps.begin(), iBitmaps.end(), *it);
	if (it1 == iBitmaps.end()) {
	    // look again, more carefully
	    for (it1 = iBitmaps.begin(); it1 != iBitmaps.end() && !it1->equal(*it);
		 ++it1);
	    if (it1 == iBitmaps.end())
		embedBitmap(*it); // not yet embedded
	    else
		it->setObjNum(it1->objNum()); // identical Bitmap is embedded
	    iBitmaps.push_back(*it);
	}
    }
}

void PdfWriter::createResources(const BitmapFinder & bm) {
    // These are only the resources needed by Ipe drawing directly.
    // Resources used from inside text objects (e.g. by tikz)
    // are stored inside the XObject resources.
    // Font is only used inside XObject, no font resource needed on page
    // Resources:
    // ProcSet, ExtGState, ColorSpace, Pattern, Shading, XObject, Font
    // not used now: Properties
    // ProcSet
    iStream << "/Resources <<\n  /ProcSet [/PDF";
    if (iResources) iStream << "/Text";
    if (!bm.iBitmaps.empty()) iStream << "/ImageB/ImageC";
    iStream << "]\n";
    // Shading
    if (iGradients.size()) {
	iStream << "  /Shading <<";
	for (std::map<int, int>::const_iterator it = iGradients.begin();
	     it != iGradients.end(); ++it)
	    iStream << " /Grad" << it->first << " " << it->second << " 0 R";
	iStream << " >>\n";
    }
    // ExtGState
    if (iExtGState >= 0) iStream << "  /ExtGState " << iExtGState << " 0 R\n";
    // ColorSpace
    if (iPatternNum >= 0) {
	iStream << "  /ColorSpace << /PCS [/Pattern /DeviceRGB] ";
	iStream << ">>\n";
    }
    // Pattern
    if (iPatternNum >= 0) iStream << "  /Pattern " << iPatternNum << " 0 R\n";
    // XObject
    // TODO: Is "hasResource" here used correctly?
    // From Tikz xobject resources seem to go into the Ipe xform.
    if (!bm.iBitmaps.empty() || !iSymbols.empty() || hasResource("XObject")) {
	iStream << "  /XObject << ";
	for (BmIter it = bm.iBitmaps.begin(); it != bm.iBitmaps.end(); ++it) {
	    // mention each PDF object only once
	    BmIter it1;
	    for (it1 = bm.iBitmaps.begin(); it1 != it && it1->objNum() != it->objNum();
		 it1++);
	    if (it1 == it)
		iStream << "/Image" << it->objNum() << " " << it->objNum() << " 0 R ";
	}
	for (std::map<int, int>::const_iterator it = iSymbols.begin();
	     it != iSymbols.end(); ++it)
	    iStream << "/Symbol" << it->first << " " << it->second << " 0 R ";
	embedResource("XObject");
	iStream << ">>\n";
    }
    iStream << "  >>\n";
}

// --------------------------------------------------------------------

void PdfWriter::paintView(Stream & stream, int pno, int view) {
    const Page * page = iDoc->page(pno);
    PdfPainter painter(iDoc->cascade(), stream);
    const auto viewMap = page->viewMap(view, iDoc->cascade());
    painter.setAttributeMap(&viewMap);
    std::vector<Matrix> layerMatrices = page->layerMatrices(view);

    Attribute bg = page->backgroundSymbol(iDoc->cascade());
    const Symbol * background = iDoc->cascade()->findSymbol(bg);
    if (background && page->findLayer("BACKGROUND") < 0) painter.drawSymbol(bg);

    if (iDoc->properties().iNumberPages && iResources) {
	const Text * pn = iResources->pageNumber(pno, view);
	if (pn) pn->draw(painter);
    }

    const Text * title = page->titleText();
    if (title) title->draw(painter);

    for (int i = 0; i < page->count(); ++i) {
	if (page->objectVisible(view, i)) {
	    painter.pushMatrix();
	    painter.transform(layerMatrices[page->layerOf(i)]);
	    page->object(i)->draw(painter);
	    painter.popMatrix();
	}
    }
}

//! create contents and page stream for this page view.
void PdfWriter::createPageView(int pno, int view) {
    const Page * page = iDoc->page(pno);
    // Find bitmaps to embed
    BitmapFinder bm;
    Attribute bg = page->backgroundSymbol(iDoc->cascade());
    const Symbol * background = iDoc->cascade()->findSymbol(bg);
    if (background && page->findLayer("BACKGROUND") < 0) background->iObject->accept(bm);
    bm.scanPage(page);
    // ipeDebug("# of bitmaps: %d", bm.iBitmaps.size());
    embedBitmaps(bm);
    if (page->findLayer("NOPDF") >= 0) return;
    // create page stream
    String pagedata;
    StringStream sstream(pagedata);
    if (iCompressLevel > 0) {
	DeflateStream dfStream(sstream, iCompressLevel);
	paintView(dfStream, pno, view);
	dfStream.close();
    } else
	paintView(sstream, pno, view);

    int firstLink = -1;
    int lastLink = -1;
    for (int i = 0; i < page->count(); ++i) {
	const Group * g = page->object(i)->asGroup();
	if (g && page->objectVisible(view, i) && !g->url().empty()) {
	    lastLink = startObject();
	    if (firstLink < 0) firstLink = lastLink;
	    iStream << "<<\n"
		    << "/Type /Annot\n"
		    << "/Subtype /Link\n"
		    << "/H /N\n"
		    << "/Border [0 0 0]\n"
		    << "/Rect [" << page->bbox(i) << "]\n"
		    << "/A <</Type/Action/S";
	    String url = g->url();
	    if (url.left(6) == "named:") {
		iStream << "/Named/N/" << url.substr(6);
	    } else {
		if (url.left(7) == "launch:") {
		    url = url.substr(7);
		    iStream << "/Launch/F";
		} else if (url.left(5) == "goto:") {
		    url = url.substr(5);
		    iStream << "/GoTo/D";
		} else
		    iStream << "/URI/URI";
		writeString(url);
	    }
	    iStream << ">>\n>> endobj\n";
	}
    }
    int notesObj = -1;
    if (!page->notes().empty()
	&& (!(iSaveFlags & SaveFlag::Export) || (iSaveFlags & SaveFlag::KeepNotes))) {
	notesObj = startObject();
	iStream << "<<\n"
		<< "/Type /Annot\n"
		<< "/Subtype /Text\n"
		<< "/Rect [20 40 30 40]\n"
		<< "/F 4\n"
		<< "/Contents ";
	writeString(page->notes());
	iStream << "\n>> endobj\n";
    }

    int contentsobj = startObject();
    iStream << "<<\n";
    createStream(pagedata.data(), pagedata.size(), (iCompressLevel > 0));
    int pageobj = startObject();
    iStream << "<<\n";
    iStream << "/Type /Page\n";
    if (firstLink >= 0 || notesObj >= 0) {
	iStream << "/Annots [ ";
	if (firstLink >= 0) {
	    while (firstLink <= lastLink) iStream << firstLink++ << " 0 R ";
	}
	if (notesObj >= 0) iStream << notesObj << " 0 R";
	iStream << "]\n";
    }
    iStream << "/Contents " << contentsobj << " 0 R\n";
    // iStream << "/Rotate 0\n";
    createResources(bm);
    if (!page->effect(view).isNormal()) {
	const Effect * effect = iDoc->cascade()->findEffect(page->effect(view));
	if (effect) effect->pageDictionary(iStream);
    }
    const Layout * layout = iDoc->cascade()->findLayout();
    iStream << "/MediaBox [ " << layout->paper() << "]\n";

    int viewBBoxLayer = page->findLayer("VIEWBBOX");
    Rect bbox;
    if (viewBBoxLayer >= 0 && page->visible(view, viewBBoxLayer))
	bbox = page->viewBBox(iDoc->cascade(), view);
    else
	bbox = page->pageBBox(iDoc->cascade());
    if (layout->iCrop && !bbox.isEmpty()) iStream << "/CropBox [" << bbox << "]\n";
    if (!bbox.isEmpty()) iStream << "/ArtBox [" << bbox << "]\n";
    iStream << "/Parent 2 0 R\n";
    iStream << ">> endobj\n";
    iPageObjectNumbers.push_back({pno, view, pageobj});
}

//! Create all PDF pages.
void PdfWriter::createPages() {
    for (int page = iFromPage; page <= iToPage; ++page) {
	if ((iSaveFlags & SaveFlag::MarkedView) && !iDoc->page(page)->marked()) continue;
	int nViews = iDoc->page(page)->countViews();
	if (iSaveFlags & SaveFlag::MarkedView) {
	    bool shown = false;
	    for (int view = 0; view < nViews; ++view) {
		if (iDoc->page(page)->markedView(view)) {
		    createPageView(page, view);
		    shown = true;
		}
	    }
	    if (!shown) createPageView(page, nViews - 1);
	} else {
	    for (int view = 0; view < nViews; ++view) createPageView(page, view);
	}
    }
}

//! Create a stream containing the XML data.
void PdfWriter::createXmlStream(String xmldata, bool preCompressed) {
    iXmlStreamNum = startObject(1);
    iStream << "<<\n/Type /Ipe\n";
    createStream(xmldata.data(), xmldata.size(), preCompressed);
}

//! Write a PDF string object to the PDF stream.
void PdfWriter::writeString(String text) {
    // Check if it is all ASCII
    bool isAscii = true;
    for (int i = 0; isAscii && i < text.size(); ++i) {
	if (text[i] & 0x80) isAscii = false;
    }
    if (isAscii) {
	iStream << "(";
	for (int i = 0; i < text.size(); ++i) {
	    char ch = text[i];
	    switch (ch) {
	    case '(':
	    case ')':
	    case '\\':
		iStream << "\\";
		// fall through
	    default: iStream << ch; break;
	    }
	}
	iStream << ")";
    } else {
	char buf[5];
	iStream << "<FEFF";
	for (int i = 0; i < text.size();) {
	    sprintf(buf, "%04X", text.unicode(i));
	    iStream << buf;
	}
	iStream << ">";
    }
}

// --------------------------------------------------------------------

// returns object number of the first view of the requested page
int PdfWriter::pageObjectNumber(int page) {
    auto it = std::find_if(iPageObjectNumbers.begin(), iPageObjectNumbers.end(),
			   [=](const PON & pon) { return pon.page == page; });
    if (it != iPageObjectNumbers.end()) return it->objNum;
    ipeDebug("pageObjectNumber not found, this is a bug!");
    return 0;
}

struct Section {
    int iPage;
    int iObjNum;
    std::vector<int> iSubPages;
};

//! Create the bookmarks (PDF outline).
void PdfWriter::createBookmarks() {
    // first collect all information
    std::vector<Section> sections;
    for (int pg = iFromPage; pg <= iToPage; ++pg) {
	if ((iSaveFlags & SaveFlag::MarkedView) && !iDoc->page(pg)->marked()) continue;
	if (iDoc->page(pg)->findLayer("NOPDF") >= 0) continue;
	String s = iDoc->page(pg)->section(0);
	String ss = iDoc->page(pg)->section(1);
	if (!s.empty()) {
	    Section sec;
	    sec.iPage = pg;
	    sec.iObjNum = -1; // TODO: sensible value
	    sections.push_back(sec);
	}
	if (!sections.empty() && !ss.empty()) sections.back().iSubPages.push_back(pg);
    }
    if (sections.empty()) return;
    // reserve outline object
    iBookmarks = iObjNum++;
    // assign object numbers
    for (int s = 0; s < size(sections); ++s) {
	sections[s].iObjNum = iObjNum++;
	iObjNum += sections[s].iSubPages.size(); // leave space for subsections
    }
    // embed root
    startObject(iBookmarks);
    iStream << "<<\n/First " << sections[0].iObjNum << " 0 R\n"
	    << "/Count " << int(sections.size()) << "\n"
	    << "/Last " << sections.back().iObjNum << " 0 R\n>> endobj\n";
    for (int s = 0; s < size(sections); ++s) {
	int count = sections[s].iSubPages.size();
	int obj = sections[s].iObjNum;
	// embed section
	startObject(obj);
	iStream << "<<\n/Title ";
	writeString(iDoc->page(sections[s].iPage)->section(0));
	iStream << "\n/Parent " << iBookmarks << " 0 R\n"
		<< "/Dest [ " << pageObjectNumber(sections[s].iPage)
		<< " 0 R /XYZ null null null ]\n";
	if (s > 0) iStream << "/Prev " << sections[s - 1].iObjNum << " 0 R\n";
	if (s < size(sections) - 1)
	    iStream << "/Next " << sections[s + 1].iObjNum << " 0 R\n";
	if (count > 0)
	    iStream << "/Count " << -count << "\n"
		    << "/First " << (obj + 1) << " 0 R\n"
		    << "/Last " << (obj + count) << " 0 R\n";
	iStream << ">> endobj\n";
	// using ids obj + 1 .. obj + count for the subsections
	for (int ss = 0; ss < count; ++ss) {
	    int pageNo = sections[s].iSubPages[ss];
	    startObject(obj + ss + 1);
	    iStream << "<<\n/Title ";
	    writeString(iDoc->page(pageNo)->section(1));
	    iStream << "\n/Parent " << obj << " 0 R\n"
		    << "/Dest [ " << pageObjectNumber(pageNo)
		    << " 0 R /XYZ null null null ]\n";
	    if (ss > 0) iStream << "/Prev " << (obj + ss) << " 0 R\n";
	    if (ss < count - 1) iStream << "/Next " << (obj + ss + 2) << " 0 R\n";
	    iStream << ">> endobj\n";
	}
    }
}

//! Create the named destinations.
void PdfWriter::createNamedDests() {
    std::vector<std::pair<String, int>> dests;
    for (int pg = iFromPage; pg <= iToPage; ++pg) {
	if ((iSaveFlags & SaveFlag::MarkedView) && !iDoc->page(pg)->marked()) continue;
	if (iDoc->page(pg)->findLayer("NOPDF") >= 0) continue;
	String s = iDoc->page(pg)->section(0);
	if (!s.empty()) dests.push_back(std::make_pair(s, pageObjectNumber(pg)));
    }
    if (dests.empty()) return;
    std::sort(dests.begin(), dests.end());
    iDests = startObject();
    iStream << "<<\n/Limits [";
    writeString(dests.front().first);
    iStream << " ";
    writeString(dests.back().first);
    iStream << "]\n/Names [\n";
    for (const auto & dest : dests) {
	writeString(dest.first);
	iStream << " [" << dest.second << " 0 R /XYZ null null null]\n";
    }
    iStream << "]>> endobj\n";
}

// --------------------------------------------------------------------

//! Create the root objects and trailer of the PDF file.
void PdfWriter::createTrailer() {
    const Document::SProperties & props = iDoc->properties();
    // create /Pages
    startObject(2);
    iStream << "<<\n" << "/Type /Pages\n";
    iStream << "/Count " << int(iPageObjectNumbers.size()) << "\n";
    iStream << "/Kids [ ";
    for (auto pon : iPageObjectNumbers) iStream << pon.objNum << " 0 R ";
    iStream << "]\n>> endobj\n";
    // create Name dictionary
    int nameDict = -1;
    if (iDests >= 0) {
	nameDict = startObject();
	iStream << "<</Dests " << iDests << " 0 R>> endobj\n";
    }
    // create PieceInfo
    int pieceInfo = startObject();
    iStream << "<</Ipe<</Private 1 0 R/LastModified(" << props.iModified
	    << ")>> >> endobj\n";
    // create /Catalog
    int catalogobj = startObject();
    iStream << "<<\n/Type /Catalog\n/Pages 2 0 R\n"
	    << "/PieceInfo " << pieceInfo << " 0 R\n";
    if (props.iFullScreen) iStream << "/PageMode /FullScreen\n";
    if (iBookmarks >= 0) {
	if (!props.iFullScreen) iStream << "/PageMode /UseOutlines\n";
	iStream << "/Outlines " << iBookmarks << " 0 R\n";
    }
    if (nameDict >= 0) iStream << "/Names " << nameDict << " 0 R\n";
    if (iDoc->countTotalViews() > 1) {
	iStream << "/PageLabels << /Nums [ ";
	int count = 0;
	for (int page = 0; page < iDoc->countPages(); ++page) {
	    if (!(iSaveFlags & SaveFlag::MarkedView) || iDoc->page(page)->marked()) {
		int nviews = (iSaveFlags & SaveFlag::MarkedView)
				 ? iDoc->page(page)->countMarkedViews()
				 : iDoc->page(page)->countViews();
		if (nviews > 1) {
		    iStream << count << " <</S /D /P (" << (page + 1) << "-)>>";
		} else { // one view only!
		    iStream << count << " <</P (" << (page + 1) << ")>>";
		}
		count += nviews;
	    }
	}
	iStream << "] >>\n";
    }
    iStream << ">> endobj\n";
    // create /Info
    int infoobj = startObject();
    iStream << "<<\n";
    if (!props.iCreator.empty()) {
	iStream << "/Creator (" << props.iCreator << ")\n";
	iStream << "/Producer (" << props.iCreator << ")\n";
    }
    if (!props.iTitle.empty()) {
	iStream << "/Title ";
	writeString(props.iTitle);
	iStream << "\n";
    }
    if (!props.iAuthor.empty()) {
	iStream << "/Author ";
	writeString(props.iAuthor);
	iStream << "\n";
    }
    if (!props.iSubject.empty()) {
	iStream << "/Subject ";
	writeString(props.iSubject);
	iStream << "\n";
    }
    if (!props.iKeywords.empty()) {
	iStream << "/Keywords ";
	writeString(props.iKeywords);
	iStream << "\n";
    }
    iStream << "/CreationDate (" << props.iCreated << ")\n";
    iStream << "/ModDate (" << props.iModified << ")\n";
    iStream << ">> endobj\n";
    // create Xref
    long xrefpos = iStream.tell();
    iStream << "xref\n0 " << iObjNum << "\n";
    for (int obj = 0; obj < iObjNum; ++obj) {
	std::map<int, long>::const_iterator it = iXref.find(obj);
	char s[12];
	if (it == iXref.end()) {
	    std::sprintf(s, "%010d", obj);
	    iStream << s << " 00000 f \n"; // note the final space!
	} else {
	    std::sprintf(s, "%010ld", iXref[obj]);
	    iStream << s << " 00000 n \n"; // note the final space!
	}
    }
    iStream << "trailer\n<<\n";
    iStream << "/Size " << iObjNum << "\n";
    iStream << "/Root " << catalogobj << " 0 R\n";
    iStream << "/Info " << infoobj << " 0 R\n";
    iStream << ">>\nstartxref\n" << int(xrefpos) << "\n%%EOF\n";
}

// --------------------------------------------------------------------

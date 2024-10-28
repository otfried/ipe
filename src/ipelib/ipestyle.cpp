// --------------------------------------------------------------------
// Ipe style sheet
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

#include "ipestyle.h"
#include "ipeiml.h"
#include "ipeobject.h"
#include "ipepainter.h"
#include "ipeutils.h"

#include <errno.h>

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Symbol
  \ingroup attr
  \brief A symbol is a named object defined in an ipe::StyleSheet.
*/

//! Default constructor
Symbol::Symbol() {
    iObject = nullptr;
    iXForm = false;
    iTransformations = ETransformationsAffine;
}

//! Create symbol for \a object (takes ownership).
Symbol::Symbol(Object * object) {
    iObject = object;
    iXForm = false;
    iTransformations = ETransformationsAffine;
}

//! Copy constructor.
Symbol::Symbol(const Symbol & rhs) {
    iObject = rhs.iObject ? rhs.iObject->clone() : nullptr;
    iXForm = rhs.iXForm;
    iTransformations = rhs.iTransformations;
    iSnap = rhs.iSnap;
}

//! Assignment operator.
Symbol & Symbol::operator=(const Symbol & rhs) {
    if (this != &rhs) {
	delete iObject;
	iObject = rhs.iObject ? rhs.iObject->clone() : nullptr;
	iXForm = rhs.iXForm;
	iTransformations = rhs.iTransformations;
	iSnap = rhs.iSnap;
    }
    return *this;
}

//! Destructor.
Symbol::~Symbol() { delete iObject; }

// --------------------------------------------------------------------

/*! \class ipe::StyleSheet
  \ingroup doc
  \brief A style sheet maps symbolic names to absolute values.

  Ipe documents can use symbolic attributes, such as 'normal', 'fat',
  or 'thin' for line thickness, or 'red', 'navy', 'turquoise' for
  color.  The mapping to an absolute pen thickness or RGB value is
  performed by a StyleSheet.

  Style sheets are always included when the document is saved, so that
  an Ipe document is self-contained.

  The built-in standard style sheet is minimal, and only needed to
  provide sane fallbacks for all the "normal" settings.

*/

#define MASK 0x00ffffff
#define SHIFT 24
#define KINDMASK 0x7f000000

// --------------------------------------------------------------------

//! The default constructor creates an empty style sheet.
StyleSheet::StyleSheet() {
    iStandard = false;
    iTitleStyle.iDefined = false;
    iPageNumberStyle.iDefined = false;
    iTextPadding.iLeft = -1.0;
    iLineJoin = EDefaultJoin;
    iLineCap = EDefaultCap;
    iFillRule = EDefaultRule;
}

//! Set page layout.
void StyleSheet::setLayout(const Layout & layout) { iLayout = layout; }

//! Return page layout (or 0 if none defined).
const Layout * StyleSheet::layout() const {
    if (iLayout.isNull())
	return nullptr;
    else
	return &iLayout;
}

//! Return text object padding (for bbox computation).
const TextPadding * StyleSheet::textPadding() const {
    if (iTextPadding.iLeft < 0)
	return nullptr;
    else
	return &iTextPadding;
}

//! Set padding for text object bbox computation.
void StyleSheet::setTextPadding(const TextPadding & pad) { iTextPadding = pad; }

//! Set style of page titles.
void StyleSheet::setTitleStyle(const TitleStyle & ts) { iTitleStyle = ts; }

//! Return title style (or 0 if none defined).
const StyleSheet::TitleStyle * StyleSheet::titleStyle() const {
    if (iTitleStyle.iDefined)
	return &iTitleStyle;
    else
	return nullptr;
}

//! Set style of page numbering.
void StyleSheet::setPageNumberStyle(const PageNumberStyle & pns) {
    iPageNumberStyle = pns;
}

//! Return page number style.
const StyleSheet::PageNumberStyle * StyleSheet::pageNumberStyle() const {
    if (iPageNumberStyle.iDefined)
	return &iPageNumberStyle;
    else
	return nullptr;
}

//! Add gradient to this style sheet.
void StyleSheet::addGradient(Attribute name, const Gradient & s) {
    assert(name.isSymbolic());
    iGradients[name.index()] = s;
}

//! Find gradient in style sheet cascade.
const Gradient * StyleSheet::findGradient(Attribute sym) const {
    if (!sym.isSymbolic()) return nullptr;
    GradientMap::const_iterator it = iGradients.find(sym.index());
    if (it != iGradients.end())
	return &it->second;
    else
	return nullptr;
}

//! Add tiling to this style sheet.
void StyleSheet::addTiling(Attribute name, const Tiling & s) {
    assert(name.isSymbolic());
    iTilings[name.index()] = s;
}

//! Find tiling in style sheet cascade.
const Tiling * StyleSheet::findTiling(Attribute sym) const {
    if (!sym.isSymbolic()) return nullptr;
    TilingMap::const_iterator it = iTilings.find(sym.index());
    if (it != iTilings.end())
	return &it->second;
    else
	return nullptr;
}

void StyleSheet::addEffect(Attribute name, const Effect & e) {
    assert(name.isSymbolic());
    iEffects[name.index()] = e;
}

const Effect * StyleSheet::findEffect(Attribute sym) const {
    if (!sym.isSymbolic()) return nullptr;
    EffectMap::const_iterator it = iEffects.find(sym.index());
    if (it != iEffects.end())
	return &it->second;
    else
	return nullptr;
}

void StyleSheet::addPageStyle(Attribute name, const PageStyle & e) {
    assert(name.isSymbolic());
    iPageStyles[name.index()] = e;
}

const PageStyle * StyleSheet::findPageStyle(Attribute sym) const {
    if (!sym.isSymbolic()) return nullptr;
    PageStyleMap::const_iterator it = iPageStyles.find(sym.index());
    if (it != iPageStyles.end())
	return &it->second;
    else
	return nullptr;
}

// --------------------------------------------------------------------

//! Set line cap.
void StyleSheet::setLineCap(TLineCap s) { iLineCap = s; }

//! Set line join.
void StyleSheet::setLineJoin(TLineJoin s) { iLineJoin = s; }

//! Set fill rule.
void StyleSheet::setFillRule(TFillRule s) { iFillRule = s; }

// --------------------------------------------------------------------

//! Add a symbol object.
void StyleSheet::addSymbol(Attribute name, const Symbol & symbol) {
    assert(name.isSymbolic());
    iSymbols[name.index()] = symbol;
}

//! Find a symbol object with given name.
/*! If attr is not symbolic or if the symbol doesn't exist, returns 0. */
const Symbol * StyleSheet::findSymbol(Attribute attr) const {
    if (!attr.isSymbolic()) return nullptr;
    SymbolMap::const_iterator it = iSymbols.find(attr.index());
    if (it != iSymbols.end())
	return &it->second;
    else
	return nullptr;
}

// --------------------------------------------------------------------

//! Add an attribute.
/*! Does nothing if \a name is not symbolic. */
void StyleSheet::add(Kind kind, Attribute name, Attribute value) {
    if (!name.isSymbolic()) return;
    iMap[name.index() | (kind << SHIFT)] = value;
}

//! Find a symbolic attribute.
/*! If \a sym is not symbolic, returns \a sym itself.  If \a sym
  cannot be found, returns the "undefined" attribute.  In all other
  cases, the returned attribute is guaranteed to be absolute.
*/
Attribute StyleSheet::find(Kind kind, Attribute sym) const {
    if (!sym.isSymbolic()) return sym;
    Map::const_iterator it = iMap.find(sym.index() | (kind << SHIFT));
    if (it != iMap.end()) {
	return it->second;
    } else
	return Attribute::UNDEFINED();
}

//! Check whether symbolic attribute is defined.
/*! This method also works for ESymbol, EGradient, ETiling, and
  EEffect.

  Returns true if \a sym is not symbolic. */
bool StyleSheet::has(Kind kind, Attribute sym) const {
    if (!sym.isSymbolic()) return true;
    switch (kind) {
    case ESymbol: {
	SymbolMap::const_iterator it = iSymbols.find(sym.index());
	return (it != iSymbols.end());
    }
    case EGradient: {
	GradientMap::const_iterator it = iGradients.find(sym.index());
	return (it != iGradients.end());
    }
    case ETiling: {
	TilingMap::const_iterator it = iTilings.find(sym.index());
	return (it != iTilings.end());
    }
    case EEffect: {
	EffectMap::const_iterator it = iEffects.find(sym.index());
	return (it != iEffects.end());
    }
    default: {
	Map::const_iterator it = iMap.find(sym.index() | (kind << SHIFT));
	return (it != iMap.end());
    }
    }
}

//! Removes definition for a symbolic attribute from this stylesheet.
/*! This method also works for ESymbol, EGradient, ETiling, and
  EEffect.  It is okay if the symbolic attribute is not defined in the
  stylesheet, nothing happens in this case. */
void StyleSheet::remove(Kind kind, Attribute sym) {
    switch (kind) {
    case ETiling: iTilings.erase(sym.index()); break;
    case ESymbol: iSymbols.erase(sym.index()); break;
    case EGradient: iGradients.erase(sym.index()); break;
    case EEffect: iEffects.erase(sym.index()); break;
    default: iMap.erase(sym.index() | (kind << SHIFT)); break;
    };
}

// --------------------------------------------------------------------

//! Return all symbolic names in the style sheet cascade.
/*! Names are simply appended from top to bottom of the cascade.
  Names are inserted only once. */
void StyleSheet::allNames(Kind kind, AttributeSeq & seq) const {
    if (kind == ESymbol) {
	for (SymbolMap::const_iterator it = iSymbols.begin(); it != iSymbols.end();
	     ++it) {
	    Attribute attr(true, it->first);
	    if (std::find(seq.begin(), seq.end(), attr) == seq.end()) seq.push_back(attr);
	}
    } else if (kind == EGradient) {
	for (GradientMap::const_iterator it = iGradients.begin(); it != iGradients.end();
	     ++it) {
	    Attribute attr(true, it->first);
	    if (std::find(seq.begin(), seq.end(), attr) == seq.end()) seq.push_back(attr);
	}
    } else if (kind == ETiling) {
	for (TilingMap::const_iterator it = iTilings.begin(); it != iTilings.end();
	     ++it) {
	    Attribute attr(true, it->first);
	    if (std::find(seq.begin(), seq.end(), attr) == seq.end()) seq.push_back(attr);
	}
    } else if (kind == EEffect) {
	for (EffectMap::const_iterator it = iEffects.begin(); it != iEffects.end();
	     ++it) {
	    Attribute attr(true, it->first);
	    if (std::find(seq.begin(), seq.end(), attr) == seq.end()) seq.push_back(attr);
	}
    } else {
	uint32_t kindMatch = (kind << SHIFT);
	for (Map::const_iterator it = iMap.begin(); it != iMap.end(); ++it) {
	    if (uint32_t(it->first & KINDMASK) == kindMatch) {
		Attribute attr(true, (it->first & MASK));
		if (std::find(seq.begin(), seq.end(), attr) == seq.end())
		    seq.push_back(attr);
	    }
	}
    }
}

// --------------------------------------------------------------------

//! Save style sheet in XML format.
void StyleSheet::saveAsXml(Stream & stream, bool saveBitmaps) const {
    stream << "<ipestyle";
    if (!iName.empty()) stream << " name=\"" << iName << "\"";
    stream << ">\n";

    if (saveBitmaps) {
	BitmapFinder bm;
	for (SymbolMap::const_iterator it = iSymbols.begin(); it != iSymbols.end(); ++it)
	    it->second.iObject->accept(bm);
	if (!bm.iBitmaps.empty()) {
	    std::sort(bm.iBitmaps.begin(), bm.iBitmaps.end());
	    int id = 1;
	    Bitmap prev;
	    for (std::vector<Bitmap>::iterator it = bm.iBitmaps.begin();
		 it != bm.iBitmaps.end(); ++it) {
		if (!it->equal(prev)) {
		    it->saveAsXml(stream, id);
		    it->setObjNum(id);
		} else
		    it->setObjNum(prev.objNum()); // noop if prev == it
		prev = *it;
		++id;
	    }
	}
    }

    Repository * rep = Repository::get();

    for (SymbolMap::const_iterator it = iSymbols.begin(); it != iSymbols.end(); ++it) {
	stream << "<symbol name=\"" << rep->toString(it->first) << "\"";
	if (it->second.iTransformations == ETransformationsTranslations)
	    stream << " transformations=\"translations\"";
	else if (it->second.iTransformations == ETransformationsRigidMotions)
	    stream << " transformations=\"rigid\"";
	if (it->second.iXForm) stream << " xform=\"yes\"";
	if (it->second.iSnap.size() > 0) {
	    stream << " snap=\"";
	    String sep = "";
	    for (const Vector & pos : it->second.iSnap) {
		stream << sep << pos;
		sep = " ";
	    }
	    stream << "\"";
	}
	stream << ">\n";
	it->second.iObject->saveAsXml(stream, String());
	stream << "</symbol>\n";
    }

    std::vector<String> mappings;
    for (Map::const_iterator it = iMap.begin(); it != iMap.end(); ++it) {
	String mapping;
	StringStream out(mapping);
	int kind = (it->first >> SHIFT);
	int kind1 = (kind == ELabelStyle) ? ETextStyle : kind;
	out << "<" << kind_names[kind1] << " name=\"" << rep->toString(it->first & MASK)
	    << "\"";
	if (kind1 == ETextStyle) {
	    String s = it->second.string();
	    int i = s.find('\0');
	    if (kind == ELabelStyle) out << " type=\"label\"";
	    out << " begin=\"" << s.substr(0, i) << "\" end=\"" << s.substr(i + 1)
		<< "\"/>\n";
	} else
	    out << " value=\"" << it->second.string() << "\"/>\n";
	mappings.push_back(mapping);
    }
    // sort symbolic mapping table to make sure it is deterministic
    std::sort(mappings.begin(), mappings.end());
    for (auto & mapping : mappings) stream << mapping;
    if (!iPreamble.empty()) {
	stream << "<preamble>";
	stream.putXmlString(iPreamble);
	stream << "</preamble>\n";
    }
    if (!iLayout.isNull()) {
	stream << "<layout paper=\"" << iLayout.iPaperSize << "\" origin=\""
	       << iLayout.iOrigin << "\" frame=\"" << iLayout.iFrameSize;
	if (iLayout.iParagraphSkip > 0.0)
	    stream << "\" skip=\"" << iLayout.iParagraphSkip;
	if (!iLayout.iCrop) stream << "\" crop=\"no";
	stream << "\"/>\n";
    }
    if (iTextPadding.iLeft >= 0.0) {
	stream << "<textpad left=\"" << iTextPadding.iLeft << "\" right=\""
	       << iTextPadding.iRight << "\" top=\"" << iTextPadding.iTop
	       << "\" bottom=\"" << iTextPadding.iBottom << "\"/>\n";
    }
    if (iPageNumberStyle.iDefined) {
	stream << "<pagenumberstyle pos=\"" << iPageNumberStyle.iPos << "\""
	       << " color=\"" << iPageNumberStyle.iColor.string() << "\""
	       << " size=\"" << iPageNumberStyle.iSize.string() << "\"";
	Text::saveAlignment(stream, iPageNumberStyle.iHorizontalAlignment,
			    iPageNumberStyle.iVerticalAlignment);
	stream << ">" << iPageNumberStyle.iText << "</pagenumberstyle>\n";
    }
    if (iTitleStyle.iDefined) {
	stream << "<titlestyle pos=\"" << iTitleStyle.iPos << "\" size=\""
	       << iTitleStyle.iSize.string() << "\" color=\""
	       << iTitleStyle.iColor.string() << "\" ";
	Text::saveAlignment(stream, iTitleStyle.iHorizontalAlignment,
			    iTitleStyle.iVerticalAlignment);
	stream << "/>\n";
    }
    if (iLineCap != EDefaultCap || iLineJoin != EDefaultJoin
	|| iFillRule != EDefaultRule) {
	stream << "<pathstyle";
	if (iLineCap != EDefaultCap) stream << " cap=\"" << iLineCap - 1 << "\"";
	if (iLineJoin != EDefaultJoin) stream << " join=\"" << iLineJoin - 1 << "\"";
	if (iFillRule == EWindRule)
	    stream << " fillrule=\"wind\"";
	else if (iFillRule == EEvenOddRule)
	    stream << " fillrule=\"eofill\"";
	stream << "/>\n";
    }

    for (GradientMap::const_iterator it = iGradients.begin(); it != iGradients.end();
	 ++it) {
	stream << "<gradient name=\"" << rep->toString(it->first) << "\"";
	const Gradient & g = it->second;
	if (g.iType == Gradient::EAxial)
	    stream << " type=\"axial\" coords=\"" << g.iV[0] << " " << g.iV[1] << "\"";
	else
	    stream << " type=\"radial\" coords=\"" << g.iV[0] << " " << g.iRadius[0]
		   << " " << g.iV[1] << " " << g.iRadius[1] << "\"";
	if (g.iExtend) stream << " extend=\"yes\"";
	if (!g.iMatrix.isIdentity()) stream << " matrix=\"" << g.iMatrix << "\"";
	stream << ">\n";
	for (int i = 0; i < size(g.iStops); ++i)
	    stream << " <stop offset=\"" << g.iStops[i].offset << "\" color=\""
		   << g.iStops[i].color << "\"/>\n";
	stream << "</gradient>\n";
    }

    for (TilingMap::const_iterator it = iTilings.begin(); it != iTilings.end(); ++it) {
	const Tiling & t = it->second;
	stream << "<tiling name=\"" << rep->toString(it->first) << "\""
	       << " angle=\"" << t.iAngle.degrees() << "\""
	       << " step=\"" << t.iStep << "\""
	       << " width=\"" << t.iWidth << "\"/>\n";
    }

    for (EffectMap::const_iterator it = iEffects.begin(); it != iEffects.end(); ++it) {
	stream << "<effect name=\"" << rep->toString(it->first) << "\"";
	const Effect & e = it->second;
	if (e.iDuration != 0) stream << " duration=\"" << e.iDuration << "\"";
	if (e.iTransitionTime != 1)
	    stream << " transition=\"" << e.iTransitionTime << "\"";
	stream << " effect=\"" << int(e.iEffect) << "\"/>\n";
    }

    for (PageStyleMap::const_iterator it = iPageStyles.begin(); it != iPageStyles.end();
	 ++it) {
	stream << "<pagestyle name=\"" << rep->toString(it->first) << "\"";
	const PageStyle & ps = it->second;
	if (!ps.iBackground.isNormal())
	    stream << " background=\"" << ps.iBackground.string() << "\"";
	if (ps.iMapping.count() > 0) {
	    stream << ">\n";
	    ps.iMapping.saveAsXml(stream);
	    stream << "</pagestyle>\n";
	} else
	    stream << "/>\n";
    }

    stream << "</ipestyle>\n";
}

// --------------------------------------------------------------------

/*! \class ipe::Cascade
  \ingroup doc
  \brief A cascade of style sheets.

  The StyleSheets of a document cascade in the sense that a document
  can refer to several StyleSheets, which are arranged in a stack.  A
  lookup is done from top to bottom, and returns as soon as a match is
  found. Ipe always appends the built-in "standard" style sheet at the
  bottom of the cascade.
*/

//! Create an empty cascade.
/*! This does not add the standard style sheet. */
Cascade::Cascade() {
    // nothing
}

static void destruct_sheets(std::vector<StyleSheet *> & sheets) {
    for (int i = 0; i < size(sheets); ++i) {
	delete sheets[i];
	sheets[i] = nullptr;
    }
    sheets.clear();
}

//! Copy constructor.
Cascade::Cascade(const Cascade & rhs) {
    destruct_sheets(iSheets);
    for (int i = 0; i < rhs.count(); ++i)
	iSheets.push_back(new StyleSheet(*rhs.iSheets[i]));
}

//! Assignment operator.
Cascade & Cascade::operator=(const Cascade & rhs) {
    if (this != &rhs) {
	destruct_sheets(iSheets);
	for (int i = 0; i < rhs.count(); ++i)
	    iSheets.push_back(new StyleSheet(*rhs.iSheets[i]));
    }
    return *this;
}

//! Destructor.
Cascade::~Cascade() { destruct_sheets(iSheets); }

//! Insert a style sheet into the cascade.
/*! Takes ownership of \a sheet. */
void Cascade::insert(int index, StyleSheet * sheet) {
    iSheets.insert(iSheets.begin() + index, sheet);
}

//! Remove a style sheet from the cascade.
/*! The old sheet is deleted. */
void Cascade::remove(int index) { iSheets.erase(iSheets.begin() + index); }

void Cascade::saveAsXml(Stream & stream) const {
    for (int i = count() - 1; i >= 0; --i) {
	if (!iSheets[i]->isStandard()) iSheets[i]->saveAsXml(stream);
    }
}

bool Cascade::has(Kind kind, Attribute sym) const {
    for (int i = 0; i < count(); ++i) {
	if (iSheets[i]->has(kind, sym)) return true;
    }
    return false;
}

Attribute Cascade::find(Kind kind, Attribute sym) const {
    for (int i = 0; i < count(); ++i) {
	Attribute a = iSheets[i]->find(kind, sym);
	if (a != Attribute::UNDEFINED()) return a;
    }
    Attribute normal = Attribute::normal(kind);
    for (int i = 0; i < count(); ++i) {
	Attribute a = iSheets[i]->find(kind, normal);
	if (a != Attribute::UNDEFINED()) return a;
    }
    // this shouldn't happen
    return Attribute::UNDEFINED();
}

const Symbol * Cascade::findSymbol(Attribute sym) const {
    for (int i = 0; i < count(); ++i) {
	const Symbol * s = iSheets[i]->findSymbol(sym);
	if (s) return s;
    }
    return nullptr;
}

const Gradient * Cascade::findGradient(Attribute sym) const {
    for (int i = 0; i < count(); ++i) {
	const Gradient * s = iSheets[i]->findGradient(sym);
	if (s) return s;
    }
    return nullptr;
}

const Tiling * Cascade::findTiling(Attribute sym) const {
    for (int i = 0; i < count(); ++i) {
	const Tiling * s = iSheets[i]->findTiling(sym);
	if (s) return s;
    }
    return nullptr;
}

const Effect * Cascade::findEffect(Attribute sym) const {
    for (int i = 0; i < count(); ++i) {
	const Effect * s = iSheets[i]->findEffect(sym);
	if (s) return s;
    }
    return nullptr;
}

const PageStyle * Cascade::findPageStyle(Attribute sym) const {
    for (int i = 0; i < count(); ++i) {
	if (const PageStyle * s = iSheets[i]->findPageStyle(sym)) return s;
    }
    return nullptr;
}

//! Find page layout (such as text margins).
const Layout * Cascade::findLayout() const {
    for (int i = 0; i < count(); ++i) {
	const Layout * l = iSheets[i]->layout();
	if (l) return l;
    }
    // must never happen
    assert(false);
    return nullptr;
}

//! Find text padding (for text bbox computation).
const TextPadding * Cascade::findTextPadding() const {
    for (int i = 0; i < count(); ++i) {
	const TextPadding * t = iSheets[i]->textPadding();
	if (t) return t;
    }
    // must never happen
    assert(false);
    return nullptr;
}

//! Get style of page titles (or 0 if none defined).
const StyleSheet::TitleStyle * Cascade::findTitleStyle() const {
    for (int i = 0; i < count(); ++i) {
	const StyleSheet::TitleStyle * ts = iSheets[i]->titleStyle();
	if (ts) return ts;
    }
    return nullptr;
}

//! Return style of page numbering (or 0 if none defined).
const StyleSheet::PageNumberStyle * Cascade::findPageNumberStyle() const {
    for (int i = 0; i < count(); ++i) {
	const StyleSheet::PageNumberStyle * pns = iSheets[i]->pageNumberStyle();
	if (pns) return pns;
    }
    return nullptr;
}

//! Return total LaTeX preamble (of the whole cascade).
String Cascade::findPreamble() const {
    String s;
    for (int i = 0; i < count(); ++i) { s = iSheets[i]->preamble() + "\n" + s; }
    return s;
}

TLineCap Cascade::lineCap() const {
    for (int i = 0; i < count(); ++i) {
	if (iSheets[i]->lineCap() != EDefaultCap) return iSheets[i]->lineCap();
    }
    return EButtCap; // should not happen
}

TLineJoin Cascade::lineJoin() const {
    for (int i = 0; i < count(); ++i) {
	if (iSheets[i]->lineJoin() != EDefaultJoin) return iSheets[i]->lineJoin();
    }
    return ERoundJoin; // should not happen
}

TFillRule Cascade::fillRule() const {
    for (int i = 0; i < count(); ++i) {
	if (iSheets[i]->fillRule() != EDefaultRule) return iSheets[i]->fillRule();
    }
    return EEvenOddRule; // should not happen
}

void Cascade::allNames(Kind kind, AttributeSeq & seq) const {
    if (has(kind, Attribute::NORMAL())) seq.push_back(Attribute::NORMAL());
    for (int i = 0; i < count(); ++i) iSheets[i]->allNames(kind, seq);
}

//! Find stylesheet defining the attribute.
/*! This method goes through the cascade looking for a definition of
	the symbolic attribute \a sym.  It returns the index of the
	stylesheet defining the attribute, or -1 if the attribute is not
	defined.

	The method panics if \a sym is not symbolic.  It also works for
	ESymbol, EGradient, ETiling, and EEffect.
*/
int Cascade::findDefinition(Kind kind, Attribute sym) const {
    assert(sym.isSymbolic());
    for (int i = 0; i < count(); ++i) {
	if (iSheets[i]->has(kind, sym)) return i;
    }
    return -1;
}

// --------------------------------------------------------------------

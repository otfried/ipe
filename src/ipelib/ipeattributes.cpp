// -*- C++ -*-
// --------------------------------------------------------------------
// Ipe object attributes
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

#include "ipeattributes.h"

// --------------------------------------------------------------------

/*! \defgroup attr Ipe Attributes
  \brief Attributes for Ipe objects.

  Ipe objects have attributes such as color, line width, dash pattern,
  etc.  Most attributes can be symbolic (the need to be looked up in a
  style sheet before rendering) or absolute.

  The Color class represents absolute values of colors. The class
  Attribute encapsulates all attributes that can be either symbolic or
  absolute.

  The Lua bindings for attributes are described
  \ref luaobj "here".
*/

// --------------------------------------------------------------------

namespace ipe {
const char * const kind_names[] = {
    "pen",         "symbolsize", "arrowsize",  "color",    "dashstyle", "textsize",
    "textstretch", "textstyle",  "labelstyle", "gridsize", "anglesize", "opacity",
    "tiling",      "symbol",     "gradient",   "effect",   nullptr};

const char * const property_names[] = {"pen",
				       "symbolsize",
				       "farrow",
				       "rarrow",
				       "farrowsize",
				       "rarrowsize",
				       "farrowshape",
				       "rarrowshape",
				       "stroke",
				       "fill",
				       "markshape",
				       "pathmode",
				       "dashstyle",
				       "textsize",
				       "textstyle",
				       "labelstyle",
				       "opacity",
				       "strokeopacity",
				       "tiling",
				       "gradient",
				       "horizontalalignment",
				       "verticalalignment",
				       "linejoin",
				       "linecap",
				       "fillrule",
				       "pinned",
				       "transformations",
				       "transformabletext",
				       "splinetype",
				       "minipage",
				       "width",
				       "decoration",
				       nullptr};
} // namespace ipe

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::Color
  \ingroup attr
  \brief An absolute RGB color.
*/

//! Construct a color.
/*! Arguments \a red, \a green, \a blue range from 0 to 1000. */
Color::Color(int red, int green, int blue) {
    iRed = Fixed::fromInternal(red);
    iGreen = Fixed::fromInternal(green);
    iBlue = Fixed::fromInternal(blue);
}

//! Save to stream.
void Color::save(Stream & stream) const {
    if (isGray())
	stream << iRed;
    else
	stream << iRed << " " << iGreen << " " << iBlue;
}

//! Save to stream in long format.
/*! The RGB components are saved separately even for gray colors. */
void Color::saveRGB(Stream & stream) const {
    stream << iRed << " " << iGreen << " " << iBlue;
}

//! is it an absolute gray value?
bool Color::isGray() const { return (iRed == iGreen && iRed == iBlue); }

bool Color::operator==(const Color & rhs) const {
    return (iRed == rhs.iRed) && (iGreen == rhs.iGreen) && (iBlue == rhs.iBlue);
}

// --------------------------------------------------------------------

/*! \class ipe::Effect
  \ingroup attr
  \brief Effect that Acrobat Reader will show on page change.

  Acrobat Reader and other PDF viewers can show a special effect when
  a new page of the document is shown.  This class describes such an
  effect.
*/

//! Construct default effect.
Effect::Effect() {
    iEffect = ENormal;
    iDuration = 0;
    iTransitionTime = 1;
}

//! Write part of page dictionary.
/*! Write part of page dictionary indicating effect,
  including the two keys /Dur and /Trans. */
void Effect::pageDictionary(Stream & stream) const {
    if (iDuration > 0) stream << "/Dur " << iDuration << "\n";
    if (iEffect != ENormal) {
	stream << "/Trans << /D " << iTransitionTime << " /S ";
	switch (iEffect) {
	case ESplitHI: stream << "/Split /Dm /H /M /I"; break;
	case ESplitHO: stream << "/Split /Dm /H /M /O"; break;
	case ESplitVI: stream << "/Split /Dm /V /M /I"; break;
	case ESplitVO: stream << "/Split /Dm /V /M /O"; break;
	case EBlindsH: stream << "/Blinds /Dm /H"; break;
	case EBlindsV: stream << "/Blinds /Dm /V"; break;
	case EBoxI: stream << "/Box /M /I"; break;
	case EBoxO: stream << "/Box /M /O"; break;
	case EWipeLR: stream << "/Wipe /Di 0"; break;
	case EWipeBT: stream << "/Wipe /Di 90"; break;
	case EWipeRL: stream << "/Wipe /Di 180"; break;
	case EWipeTB: stream << "/Wipe /Di 270"; break;
	case EDissolve: stream << "/Dissolve"; break;
	case EGlitterLR: stream << "/Glitter /Di 0"; break;
	case EGlitterTB: stream << "/Glitter /Di 270"; break;
	case EGlitterD: stream << "/Glitter /Di 315"; break;
	case EFlyILR: stream << "/Fly /M /I /Di 0"; break;
	case EFlyOLR: stream << "/Fly /M /O /Di 0"; break;
	case EFlyITB: stream << "/Fly /M /I /Di 270"; break;
	case EFlyOTB: stream << "/Fly /M /O /Di 270"; break;
	case EPushLR: stream << "/Push /Di 0"; break;
	case EPushTB: stream << "/Push /Di 270"; break;
	case ECoverLR: stream << "/Cover /Di 0"; break;
	case ECoverLB: stream << "/Cover /Di 270"; break;
	case EUncoverLR: stream << "/Uncover /Di 0"; break;
	case EUncoverTB: stream << "/Uncover /Di 270"; break;
	case EFade: stream << "/Fade"; break;
	case ENormal: break; // to satisfy compiler
	}
	stream << " >>\n";
    }
}

// --------------------------------------------------------------------

/*! \class ipe::Repository
  \ingroup attr
  \brief Repository of strings.

  Ipe documents can use symbolic attributes, such as 'normal', 'fat',
  or 'thin' for line thickness, or 'red', 'navy', 'turquoise' for
  color, as well as absolute attributes such as "[3 1] 0" for a dash
  pattern.  To avoid storing these common strings hundreds of times,
  Repository keeps a repository of strings. Inside an Object, strings
  are replaced by indices into the repository.

  The Repository is a singleton object.  It is created the first time
  it is used. You obtain access to the repository using get().
*/

// pointer to singleton object
Repository * Repository::singleton = nullptr;

//! Constructor.
Repository::Repository() {
    // put certain strings at index 0 ..
    iStrings.push_back("normal");
    iStrings.push_back("undefined");
    iStrings.push_back("Background");
    iStrings.push_back("sym-stroke");
    iStrings.push_back("sym-fill");
    iStrings.push_back("sym-pen");
    iStrings.push_back("arrow/normal(spx)");
    iStrings.push_back("opaque");
    iStrings.push_back("arrow/arc(spx)");
    iStrings.push_back("arrow/farc(spx)");
    iStrings.push_back("arrow/ptarc(spx)");
    iStrings.push_back("arrow/fptarc(spx)");
}

//! Get pointer to singleton Repository.
Repository * Repository::get() {
    if (!singleton) { singleton = new Repository(); }
    return singleton;
}

//! Return string with given index.
String Repository::toString(int index) const { return iStrings[index]; }

//! Return index of given string.
/*! The string is added to the repository if it doesn't exist yet. */
int Repository::toIndex(String str) {
    assert(!str.empty());
    std::vector<String>::const_iterator it =
	std::find(iStrings.begin(), iStrings.end(), str);
    if (it != iStrings.end()) return (it - iStrings.begin());
    iStrings.push_back(str);
    return iStrings.size() - 1;
}

//! Destroy repository object.
void Repository::cleanup() {
    delete singleton;
    singleton = nullptr;
}

// --------------------------------------------------------------------

/*! \class ipe::Attribute
  \ingroup attr
  \brief An attribute of an Ipe Object.

  An attribute is either an absolute value or a symbolic name that has
  to be looked up in a StyleSheet.

  All string values are replaced by indices into a Repository (that
  applies both to symbolic names and to absolute values that are
  strings).  All other values are stored directly inside the
  attribute, either as a Fixed or a Color.

  There are five different kinds of Attribute objects:

  - if isSymbolic() is true, index() returns the index into the
	repository, and string() returns the symbolic name.
  - if isColor() is true, color() returns an absolute RGB color.
  - if isNumeric() is true, number() returns an absolute scalar value.
  - if isEnum() is true, the attribute represents an enumeration value.
  - otherwise, isString() is true, and index() returns the index into
	the repository (for a string expressing the absolute value of the
	attribute), and string() returns the string itself.
*/

/*
  if (n & 0xc000.0000) == 0x0000.0000 -> color in 0x3fff.ffff
  if (n & 0xc000.0000) == 0x4000.0000 -> fixed in 0x3fff.ffff
  if (n & 0xe000.0000) == 0x8000.0000 -> symbolic string in 0x1fffffff
  if (n & 0xe000.0000) == 0xc000.0000 -> absolute string in 0x1fffffff
  if (n & 0xe000.0000) == 0xe000.0000 -> enum in 0x1fff.ffff
 */

//! Create  an attribute with absolute color.
Attribute::Attribute(Color color) {
    iName = (color.iRed.internal() << 20) + (color.iGreen.internal() << 10)
	    + (color.iBlue.internal());
}

//! Create an attribute with string value.
Attribute::Attribute(bool symbolic, int index) {
    iName = index | (symbolic ? ESymbolic : EAbsolute);
}

//! Create an absolute numeric attribute.
Attribute::Attribute(Fixed value) { iName = EFixed | value.internal(); }

//! Create an attribute with string value.
Attribute::Attribute(bool symbolic, String name) {
    int index = Repository::get()->toIndex(name);
    iName = index | (symbolic ? ESymbolic : EAbsolute);
}

static const char * const enumeration_name[] = {
    "false",        "true",                                 // bool
    "left",         "right",         "hcenter",             // halign
    "bottom",       "baseline",      "top",      "vcenter", // valign
    "normal",       "miter",         "round",    "bevel",   // linejoin
    "normal",       "butt",          "round",    "square",  // linecap
    "normal",       "wind",          "evenodd",             // fillrule
    "none",         "horizontal",    "vertical", "fixed",   // pinned
    "translations", "rigid",         "affine",              // transformations
    "stroked",      "strokedfilled", "filled",              // pathmode
    "bspline",      "cardinal",      "spiro",               // splinetype
};

//! Return string representing the attribute.
String Attribute::string() const {
    if (isSymbolic() || isString()) return Repository::get()->toString(index());

    String str;
    StringStream stream(str);
    if (isNumber()) {
	stream << number();
    } else if (isColor()) {
	stream << color();
    } else {
	// is enumeration value
	stream << enumeration_name[index()];
    }
    return str;
}

//! Return value of absolute numeric attribute.
Fixed Attribute::number() const {
    assert(isNumber());
    return Fixed::fromInternal(iName & EFixedMask);
}

//! Return absolute color.
Color Attribute::color() const {
    assert(isColor());
    Color col;
    col.iRed = Fixed::fromInternal(iName >> 20);
    col.iGreen = Fixed::fromInternal((iName >> 10) & 0x3ff);
    col.iBlue = Fixed::fromInternal(iName & 0x3ff);
    return col;
}

//! Construct a color from a string.
/*! If only a single number is given, this is a gray value, otherwise
	red, green, and blue components must be given. */
Color::Color(String str) {
    Lex st(str);
    st >> iRed >> iGreen;
    if (st.eos())
	iGreen = iBlue = iRed;
    else
	st >> iBlue;
}

//! Is it a symbolic arrow name of the form "arrow/mid-*"?
bool Attribute::isMidArrow() const {
    return isSymbolic() && string().hasPrefix("arrow/mid-");
}

//! Make a color attribute.
/*! If the string starts with a letter, make a symbolic attribute.
  Otherwise, it's either a single gray value (0.0 to 1.0), or the
  three red, green, and blue components, separated by spaces.
  If it's an empty string, return \a deflt.
*/
Attribute Attribute::makeColor(String str, Attribute deflt) {
    if (str.empty())
	return deflt;
    else if (('a' <= str[0] && str[0] <= 'z') || ('A' <= str[0] && str[0] <= 'Z'))
	return Attribute(true, str);
    else
	return Attribute(Color(str));
}

//! Make a scalar attribute.
/*! If \a str is empty, simply return \a deflt.
  If \a str starts with a letter, make a symbolic attribute.
  Otherwise, must be a number. */
Attribute Attribute::makeScalar(String str, Attribute deflt) {
    if (str.empty()) {
	return deflt;
    } else if (('a' <= str[0] && str[0] <= 'z') || ('A' <= str[0] && str[0] <= 'Z')) {
	return Attribute(true, str);
    } else {
	return Attribute(Lex(str).getFixed());
    }
}

//! Construct dash style attribute from string.
/*! Strings starting with '[' create an absolute dash style.  The
  empty string is equivalent to 'normal'.  Any other string creates a
  symbolic dash style.
*/
Attribute Attribute::makeDashStyle(String str) {
    if (str.empty())
	return Attribute::NORMAL();
    else if (str[0] == '[')
	return Attribute(false, str);
    else
	return Attribute(true, str);
}

//! Construct text size attribute from string.
/*! String starting with digit creates a numeric absolute value,
  string starting with letter creates symbolic text size, anything
  else creates absolute (string) text size.  The empty string is
  treated like "normal".
*/
Attribute Attribute::makeTextSize(String str) {
    if (str.empty())
	return Attribute::NORMAL();
    else if ('0' <= str[0] && str[0] <= '9')
	return Attribute(Lex(str).getFixed());
    else if (('a' <= str[0] && str[0] <= 'z') || ('A' <= str[0] && str[0] <= 'Z'))
	return Attribute(true, str);
    else
	return Attribute(false, str);
}

//! Return a standard value for attribute of \a kind.
/*! The value is used if the stylesheet doesn't define a symbolic
	attribute used in the document. */
Attribute Attribute::normal(Kind kind) {
    switch (kind) {
    case EPen:
    case EArrowSize:
    case ESymbolSize:
    case ETextSize:
    case ETextStyle:
    case EDashStyle:
    default: return Attribute::NORMAL();
    case ETextStretch:
    case EOpacity: return Attribute::ONE();
    case EColor: return Attribute::BLACK();
    case EGridSize: return Attribute(Fixed(8));
    case EAngleSize: return Attribute(Fixed(45));
    }
}

// --------------------------------------------------------------------

//! Map the given symbolic attribute sym.  Returns sym if there is no mapping.
Attribute AttributeMap::map(Kind kind, Attribute sym) const {
    for (const auto & m : iMap) {
	if (m.kind == kind and m.from == sym) return m.to;
    }
    return sym;
}

//! Save map in XML format
void AttributeMap::saveAsXml(Stream & stream) const {
    for (int k = 0; k < size(iMap); ++k) {
	const AttributeMapping & mapping = iMap[k];
	stream << "<map kind=\"" << kind_names[mapping.kind] << "\" from=\""
	       << mapping.from.string() << "\" to=\"" << mapping.to.string() << "\" />\n";
    }
}

//! Add a mapping
void AttributeMap::add(const AttributeMapping & map) { iMap.push_back(map); }

// --------------------------------------------------------------------

/*! \class ipe::AllAttributes
  \ingroup attr
  \brief Collection of all object attributes.
*/

//! Constructor sets default values.
AllAttributes::AllAttributes() {
    iStroke = iFill = Attribute::BLACK();
    iPathMode = EStrokedOnly;
    iDashStyle = Attribute::NORMAL();
    iPen = iFArrowSize = iRArrowSize = Attribute::NORMAL();
    iFArrowShape = iRArrowShape = Attribute::ARROW_NORMAL();
    iFArrow = iRArrow = false;
    iSymbolSize = iTextSize = iTextStyle = iLabelStyle = Attribute::NORMAL();
    iTransformableText = false;
    iVerticalAlignment = EAlignBaseline;
    iHorizontalAlignment = EAlignLeft;
    iPinned = ENoPin;
    iSplineType = EBSpline;
    iTransformations = ETransformationsAffine;
    iLineJoin = EDefaultJoin;
    iLineCap = EDefaultCap;
    iFillRule = EDefaultRule;
    iOpacity = Attribute::OPAQUE();
    iStrokeOpacity = Attribute::OPAQUE();
    iTiling = Attribute::NORMAL();
    iGradient = Attribute::NORMAL();
    iMarkShape = Attribute::NORMAL();
}

// --------------------------------------------------------------------

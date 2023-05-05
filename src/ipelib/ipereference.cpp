// --------------------------------------------------------------------
// The reference object.
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2023 Otfried Cheong

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

#include "ipereference.h"
#include "ipestyle.h"
#include "ipepainter.h"

using namespace ipe;

/*! \class ipe::Reference
  \ingroup obj
  \brief The reference object.

  A Reference uses a symbol, that is, an object defined in an Ipe
  StyleSheet. The object is defined as a named symbol in the style
  sheet, and can be reused arbitrarily often in the document.  This
  can, for instance, be used for backgrounds on multi-page documents.

  It is admissible to refer to an undefined object (that is, the
  current style sheet cascade does not define a symbol with the given
  name).  Nothing will be drawn in this case.

  The Reference has a stroke, fill, and pen attribute.  When drawing a
  symbol, these attributes are made available to the symbol through
  the names "sym-stroke", "sym-fill", and "sym-pen". These are not
  defined by the style sheet, but resolved by the Painter when the
  symbol sets its attributes.

  Note that it is not possible to determine \e whether a symbol is
  filled from the Reference object.

  The size attribute is of type ESymbolSize, and indicates a
  magnification factor applied to the symbol.  This magnification is
  applied after the untransformation indicated in the Reference and in
  the Symbol has been performed, so that symbols are magnified even if
  they specify ETransformationsTranslations.

  The size is meant for symbols such as marks, that can be shown in
  different sizes.  Another application of symbols is for backgrounds
  and logos.  Their size should not be changed when the user changes
  the symbolsize for the entire page.  For such symbols, the size
  attribute of the Reference should be set to the absolute value zero.
  This means that no magnification is applied to the object, and it
  also \e stops setAttribute() from modifying the size.  (The size can
  still be changed using setSize(), but this is not available from
  Lua.)
*/

//! Create a reference to the named object in stylesheet.
Reference::Reference(const AllAttributes &attr, Attribute name, Vector pos)
  : Object()
{
  assert(name.isSymbolic());
  iName = name;
  iPos = pos;
  iPen = Attribute::NORMAL();
  iSize = Attribute::ONE();
  iStroke = Attribute::BLACK();
  iFill = Attribute::WHITE();
  iFlags = flagsFromName(name.string());
  if (iFlags & EHasPen)
    iPen = attr.iPen;
  if (iFlags & EHasSize)
    iSize = attr.iSymbolSize;
  if (iFlags & EHasStroke)
    iStroke = attr.iStroke;
  if (iFlags & EHasFill)
    iFill = attr.iFill;
}

//! Create from XML stream.
Reference::Reference(const XmlAttributes &attr, String /* data */)
  : Object(attr)
{
  iName = Attribute(true, attr["name"]);
  String str;
  if (attr.has("pos", str)) {
    Lex st(str);
    st >> iPos.x >> iPos.y;
  } else
    iPos = Vector::ZERO;
  iPen = Attribute::makeScalar(attr["pen"], Attribute::NORMAL());
  iSize = Attribute::makeScalar(attr["size"], Attribute::ONE());
  iStroke = Attribute::makeColor(attr["stroke"], Attribute::BLACK());
  iFill = Attribute::makeColor(attr["fill"], Attribute::WHITE());
  iFlags = flagsFromName(iName.string());
}

//! Clone object
Object *Reference::clone() const
{
  return new Reference(*this);
}

//! Return pointer to this object.
Reference *Reference::asReference()
{
  return this;
}

Object::Type Reference::type() const
{
  return EReference;
}

//! Call visitReference of visitor.
void Reference::accept(Visitor &visitor) const
{
  visitor.visitReference(this);
}

//! Save in XML format.
void Reference::saveAsXml(Stream &stream, String layer) const
{
  stream << "<use";
  saveAttributesAsXml(stream, layer);
  stream << " name=\"" << iName.string() << "\"";
  if (iPos != Vector::ZERO)
    stream << " pos=\"" << iPos << "\"";
  if ((iFlags & EHasPen) && !iPen.isNormal())
    stream << " pen=\"" << iPen.string() << "\"";
  if ((iFlags & EHasSize) && iSize != Attribute::ONE())
    stream << " size=\"" << iSize.string() << "\"";
  if ((iFlags & EHasStroke) && iStroke != Attribute::BLACK())
    stream << " stroke=\"" << iStroke.string() << "\"";
  if ((iFlags & EHasFill) && iFill != Attribute::WHITE())
    stream << " fill=\"" << iFill.string() << "\"";
  stream << "/>\n";
}

//! Draw reference.
/*! If the symbolic attribute is not defined in the current style sheet,
  nothing is drawn at all. */
void Reference::draw(Painter &painter) const
{
  const Symbol *symbol = painter.cascade()->findSymbol(iName);
  if (symbol) {
    iSnap = symbol->iSnap;  // cache snap point information
    Attribute si = painter.lookup(ESymbolSize, iSize);
    double s = si.number().toDouble();
    painter.pushMatrix();
    painter.transform(matrix());
    painter.translate(iPos);
    painter.untransform(transformations());
    painter.untransform(symbol->iTransformations);
    if (iFlags & EHasSize) {
      Matrix m(s, 0, 0, s, 0, 0);
      painter.transform(m);
    }
    painter.push();
    if (iFlags & EHasStroke)
      painter.setSymStroke(iStroke);
    if (iFlags & EHasFill)
      painter.setSymFill(iFill);
    if (iFlags & EHasPen)
      painter.setSymPen(iPen);
    painter.drawSymbol(iName);
    painter.pop();
    painter.popMatrix();
  }
}

void Reference::drawSimple(Painter &painter) const
{
  painter.pushMatrix();
  painter.transform(matrix());
  painter.translate(iPos);
  if (iSnap.size() > 0) {
    const Symbol *symbol = painter.cascade()->findSymbol(iName);
    if (symbol) {
      painter.untransform(symbol->iTransformations);
      if (iFlags & EHasSize) {
	Attribute si = painter.cascade()->find(ESymbolSize, iSize);
	double s = si.number().toDouble();
	Matrix m(s, 0, 0, s, 0, 0);
	painter.transform(m);
      }
      painter.push();
      symbol->iObject->drawSimple(painter);
      painter.pop();
      painter.popMatrix();
      return;
    }
  }
  painter.untransform(ETransformationsTranslations);
  const int size = 10;
  painter.newPath();
  painter.moveTo(Vector(-size, 0));
  painter.lineTo(Vector(size, 0));
  painter.moveTo(Vector(0, -size));
  painter.lineTo(Vector(0, size));
  painter.drawPath(EStrokedOnly);
  painter.popMatrix();
}

/*! \copydoc Object::addToBBox

  This only adds the position (or the snap positions) to the \a box. */
void Reference::addToBBox(Rect &box, const Matrix &m, bool cp) const
{
  if (iSnap.size() > 0) {
    for (const Vector & pos : iSnap)
      box.addPoint((m * matrix()) * (iPos + pos));
  } else
    box.addPoint((m * matrix()) * iPos);
}

void Reference::checkStyle(const Cascade *sheet, AttributeSeq &seq) const
{
  const Symbol *symbol = sheet->findSymbol(iName);
  if (!symbol) {
    if (std::find(seq.begin(), seq.end(), iName) == seq.end())
      seq.push_back(iName);
  } else {
    iSnap = symbol->iSnap;  // cache snap positions
  }
  if (iFlags & EHasStroke)
    checkSymbol(EColor, iStroke, sheet, seq);
  if (iFlags & EHasFill)
    checkSymbol(EColor, iFill, sheet, seq);
  if (iFlags & EHasPen)
    checkSymbol(EPen, iPen, sheet, seq);
  if (iFlags & EHasSize)
    checkSymbol(ESymbolSize, iSize, sheet, seq);
}

double Reference::distance(const Vector &v, const Matrix &m, double bound) const
{
  if (iSnap.size() > 0) {
    double d = bound;
    for (const Vector & snapPos : iSnap) {
      double d1 = (v - (m * (matrix() * (iPos + snapPos)))).len();
      if (d1 < d)
	d = d1;
    }
    return d;
  } else
    return (v - (m * (matrix() * iPos))).len();
}

void Reference::snapVtx(const Vector &mouse, const Matrix &m,
			Vector &pos, double &bound) const
{
  if (iSnap.size() > 0) {
    for (const Vector & snapPos : iSnap)
      (m * (matrix() * (iPos + snapPos))).snap(mouse, pos, bound);
  } else
    (m * (matrix() * iPos)).snap(mouse, pos, bound);
}

void Reference::snapBnd(const Vector &, const Matrix &,
			Vector &, double &) const
{
  // nothing
}

//! Set name of symbol referenced.
void Reference::setName(Attribute name)
{
  iName = name;
  iFlags = flagsFromName(name.string());
}

//! Set pen.
void Reference::setPen(Attribute pen)
{
  iPen = pen;
}

//! Set stroke color.
void Reference::setStroke(Attribute color)
{
  iStroke = color;
}

//! Set fill color.
void Reference::setFill(Attribute color)
{
  iFill = color;
}

//! Set size (magnification) of symbol.
void Reference::setSize(Attribute size)
{
  iSize = size;
}

//! \copydoc Object::setAttribute
bool Reference::setAttribute(Property prop, Attribute value)
{
  switch (prop) {
  case EPropPen:
    if ((iFlags & EHasPen) && value != pen()) {
      setPen(value);
      return true;
    }
    break;
  case EPropStrokeColor:
    if ((iFlags & EHasStroke) && value != stroke()) {
      setStroke(value);
      return true;
    }
    break;
  case EPropFillColor:
    if ((iFlags & EHasFill) && value != fill()) {
      setFill(value);
      return true;
    }
    break;
  case EPropSymbolSize:
    if ((iFlags & EHasSize) && value != size()) {
      setSize(value);
      return true;
    }
    break;
  case EPropMarkShape:
    if ((iFlags & EIsMark) && value != name()) {
      setName(value);
      return true;
    }
    break;
  default:
    return Object::setAttribute(prop, value);
  }
  return false;
}

Attribute Reference::getAttribute(Property prop) const noexcept
{
  switch (prop) {
  case EPropPen:
    if (iFlags & EHasPen)
      return pen();
    break;
  case EPropStrokeColor:
    if (iFlags & EHasStroke)
      return stroke();
    break;
  case EPropFillColor:
    if (iFlags & EHasFill)
      return fill();
    break;
  case EPropSymbolSize:
    if (iFlags & EHasSize)
      return size();
    break;
  case EPropMarkShape:
    if (iFlags & EIsMark)
      return name();
    break;
  default:
    break;
  }
  return Object::getAttribute(prop);
}

// --------------------------------------------------------------------

uint32_t Reference::flagsFromName(String name)
{
  uint32_t flags = 0;
  if (name.left(5) == "mark/")
    flags |= EIsMark;
  if (name.left(6) == "arrow/")
    flags |= EIsArrow;
  int i = name.rfind('(');
  if (i < 0 || name[name.size() - 1] != ')')
    return flags;
  String letters = name.substr(i+1, name.size() - i - 2);
  if (letters.find('x') >= 0)
    flags |= EHasSize;
  if (letters.find('s') >= 0)
    flags |= EHasStroke;
  if (letters.find('f') >= 0)
    flags |= EHasFill;
  if (letters.find('p') >= 0)
    flags |= EHasPen;
  return flags;
}

// --------------------------------------------------------------------


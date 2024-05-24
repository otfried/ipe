// --------------------------------------------------------------------
// The Text object.
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

#include "ipetext.h"
#include "ipepainter.h"

using namespace ipe;

/*! \class ipe::Text
  \ingroup obj
  \brief The text object.

  The text object stores a Latex source representation, which needs to
  be translated into PDF by Pdflatex before it can be saved as PDF.

  There are two types of text objects: labels and minipages.  The
  textType() method tells you which, or use isMinipage().

  The dimensions of a text object are given by width(), height(), and
  depth().  They are recomputed by Ipe when running LaTeX, with the
  exception of the width for minipage objects (whose width is fixed).
  Before Latex has been run, the dimensions are not reliable.

  The position of the reference point relative to the text box is
  given by verticalAlignment() and horizontalAlignment().

  The text() must be a legal LaTeX fragment that can be interpreted by
  LaTeX inside \c \\hbox, possibly using the macros or packages
  defined in the preamble.
*/

//! Construct an empty internal text object.
Text::Text() : Object()
{
  iXForm = nullptr;
  iPos = Vector::ZERO;
  iType = TextType(0);
  iStroke = Attribute::BLACK();
  iOpacity = Attribute::OPAQUE();
  iStyle = Attribute::NORMAL();
  iWidth = 10.0;
  iHeight = 10.0;
  iDepth = 0.0;
  iVerticalAlignment = EAlignBottom;
  iHorizontalAlignment = EAlignLeft;
}

//! Create text object.
Text::Text(const AllAttributes &attr, String data,
	   const Vector &pos, TextType type, double width)
  : Object(attr)
{
  iXForm = nullptr;
  iText = data;
  iStroke = attr.iStroke;
  iOpacity = attr.iOpacity;
  iSize = attr.iTextSize;
  iPos = pos;
  iType = type;
  iWidth = width;
  iHeight = 10.0;
  iDepth = 0.0;
  if (iType == ELabel) {
    iStyle = attr.iLabelStyle;
    iVerticalAlignment = attr.iVerticalAlignment;
  } else {
    iStyle = attr.iTextStyle;
    iVerticalAlignment = EAlignTop;
  }
  iHorizontalAlignment = attr.iHorizontalAlignment;
  if (!attr.iTransformableText)
    // override setting
    iTransformations =  ETransformationsTranslations;
}

//! Copy constructor.
Text::Text(const Text &rhs)
  : Object(rhs)
{
  iPos = rhs.iPos;
  iText = rhs.iText;
  iStroke = rhs.iStroke;
  iOpacity = rhs.iOpacity;
  iSize = rhs.iSize;
  iStyle = rhs.iStyle;
  iWidth = rhs.iWidth;
  iHeight = rhs.iHeight;
  iDepth = rhs.iDepth;
  iType = rhs.iType;
  iVerticalAlignment = rhs.iVerticalAlignment;
  iHorizontalAlignment = rhs.iHorizontalAlignment;
  iXForm = rhs.iXForm;
  if (iXForm) iXForm->iRefCount++;
}

//! Destructor.
Text::~Text()
{
  if (iXForm && --iXForm->iRefCount == 0)
    delete iXForm;
}

// --------------------------------------------------------------------

//! Create from XML stream.
Text::Text(const XmlAttributes &attr, String data)
  : Object(attr)
{
  iXForm = nullptr;
  iText = data;

  iStroke = Attribute::makeColor(attr["stroke"], Attribute::BLACK());

  Lex st(attr["pos"]);
  st >> iPos.x >> iPos.y;

  iSize = Attribute::makeTextSize(attr["size"]);

  String str;
  iType = ELabel;
  iWidth = 10.0;
  if (attr.has("type", str)) {
    if (str == "minipage")
      iType = EMinipage;
  } else if (attr.has("width", str))
    iType = EMinipage;  // no type attribute

  if (attr.has("width", str))
    iWidth = Lex(str).getDouble();

  iHeight = 10.0;
  if (attr.has("height", str))
    iHeight = Lex(str).getDouble();

  iDepth = 0.0;
  if (attr.has("depth", str))
    iDepth = Lex(str).getDouble();

  iVerticalAlignment =
    makeVAlign(attr["valign"], isMinipage() ? EAlignTop : EAlignBottom);
  iHorizontalAlignment = makeHAlign(attr["halign"], EAlignLeft);

  if (attr.has("style", str) && str != "normal")
    iStyle = Attribute(true, str);
  else
    iStyle = Attribute::NORMAL();

  if (attr.has("opacity", str))
    iOpacity = Attribute(true, str);
  else
    iOpacity = Attribute::OPAQUE();

  if (iType == ELabel && iStyle == Attribute::NORMAL() &&
      iText.size() >= 3 && iText[0] == '$' && iText[iText.size()-1] == '$') {
    for (int i = 1; i < iText.size() - 1; ++i)  // check if no other $ in text
      if (iText[i] == '$')
	return;
    iStyle = Attribute(true, "math");
    iText = iText.substr(1, iText.size() - 2);
  }
}

// --------------------------------------------------------------------

//! Clone object
Object *Text::clone() const
{
  return new Text(*this);
}

//! Return pointer to this object.
Text *Text::asText()
{
  return this;
}

Object::Type Text::type() const
{
  return EText;
}

// --------------------------------------------------------------------

//! Return vertical alignment indicated by a name, or else default.
TVerticalAlignment Text::makeVAlign(String str, TVerticalAlignment def)
{
  if (str == "top")
    return EAlignTop;
  else if (str == "bottom")
    return EAlignBottom;
  else if (str == "baseline")
    return EAlignBaseline;
  else if (str == "center")
    return EAlignVCenter;
  else
    return def;
}

//! Return horizontal alignment indicated by a name, or else default.
THorizontalAlignment Text::makeHAlign(String str, THorizontalAlignment def)
{
  if (str == "left")
    return EAlignLeft;
  else if (str == "right")
    return EAlignRight;
  else if (str == "center")
    return EAlignHCenter;
  else
    return def;
}

//! Call visitText of visitor.
void Text::accept(Visitor &visitor) const
{
  visitor.visitText(this);
}

//! Return type of text object.
Text::TextType Text::textType() const
{
  if (iType == 0) // internal for title
    return ELabel;
  return iType;
}

//! Save object to XML stream.
void Text::saveAsXml(Stream &stream, String layer) const
{
  stream << "<text";
  saveAttributesAsXml(stream, layer);
  stream << " pos=\"" << iPos.x << " " << iPos.y << "\"";
  stream << " stroke=\"" << iStroke.string() << "\"";
  switch (iType) {
  case ELabel:
    stream << " type=\"label\"";
    break;
  case EMinipage:
    stream << " type=\"minipage\"";
    break;
  }
  if (iXForm || isMinipage())
    stream << " width=\"" << iWidth << "\"";
  if (iXForm)
    stream << " height=\"" << iHeight << "\""
	   << " depth=\"" << iDepth << "\"";
  saveAlignment(stream, iHorizontalAlignment, iVerticalAlignment);
  if (iSize != Attribute::NORMAL())
    stream << " size=\"" << iSize.string() << "\"";
  if (iStyle != Attribute::NORMAL())
    stream << " style=\"" << iStyle.string() << "\"";
  if (iOpacity != Attribute::OPAQUE())
    stream << " opacity=\"" << iOpacity.string() << "\"";
  stream << ">";
  stream.putXmlString(iText);
  stream << "</text>\n";
}

void Text::saveAlignment(Stream &stream, THorizontalAlignment h,
			 TVerticalAlignment v)
{
  switch (h) {
  case EAlignLeft:
    break;
  case EAlignHCenter:
    stream << " halign=\"center\"";
    break;
  case EAlignRight:
    stream << " halign=\"right\"";
    break;
  }
  switch (v) {
  case EAlignTop:
    stream << " valign=\"top\"";
    break;
  case EAlignBottom:
    stream << " valign=\"bottom\"";
    break;
  case EAlignBaseline:
    stream << " valign=\"baseline\"";
    break;
  case EAlignVCenter:
    stream << " valign=\"center\"";
    break;
  }
}

//! Save text as PDF.
void Text::draw(Painter &painter) const
{
  painter.push();
  painter.pushMatrix();
  painter.transform(matrix());
  painter.translate(iPos);
  painter.untransform(transformations());
  painter.setStroke(iStroke);
  painter.setOpacity(iOpacity);
  // Adjust alignment: make lower left corner of text box the origin
  painter.translate(-align());
  painter.drawText(this);
  painter.popMatrix();
  painter.pop();
}

void Text::drawSimple(Painter &painter) const
{
  painter.pushMatrix();
  painter.transform(matrix());
  painter.translate(iPos);
  painter.untransform(transformations());
  painter.newPath();
  double wid = iWidth;
  double ht = totalHeight();
  Vector offset = -align();
  painter.moveTo(offset + Vector(0, 0));
  painter.lineTo(offset + Vector(wid, 0));
  painter.lineTo(offset + Vector(wid, ht));
  painter.lineTo(offset + Vector(0, ht));
  painter.closePath();
  painter.drawPath(EStrokedOnly);
  painter.popMatrix();
}

double Text::distance(const Vector &v, const Matrix &m,
		      double bound) const
{
  Vector u[5];
  quadrilateral(m, u);
  u[4] = u[0];

  double d = bound;
  double d1;
  for (int i = 0; i < 4; ++i) {
    if ((d1 = Segment(u[i], u[i+1]).distance(v, d)) < d)
      d = d1;
  }
  return d1;
}

void Text::addToBBox(Rect &box, const Matrix &m, bool) const
{
  Vector v[4];
  quadrilateral(m, v);

  for (int i = 0; i < 4; ++i)
    box.addPoint(v[i]);
}

void Text::snapCtl(const Vector &mouse, const Matrix &m,
		   Vector &pos, double &bound) const
{
  (m * (matrix() * iPos)).snap(mouse, pos, bound);
  Vector v[4];
  quadrilateral(m, v);
  for (int i = 0; i < 4; ++i)
    v[i].snap(mouse, pos, bound);
}

// --------------------------------------------------------------------

//! Set stroke color
void Text::setStroke(Attribute stroke)
{
  iStroke = stroke;
}

//! Set opacity of the object.
void Text::setOpacity(Attribute opaq)
{
  iOpacity = opaq;
}

//! Set width of paragraph.
/*! This invalidates (and destroys) the XForm.
  The function panics if object is not a (true) minipage. */
void Text::setWidth(double width)
{
  assert(textType() == EMinipage);
  iWidth = width;
  setXForm(nullptr);
}

//! Set font size of text.
/*! This invalidates (and destroys) the XForm. */
void Text::setSize(Attribute size)
{
  iSize = size;
  setXForm(nullptr);
}

//! Set Latex style of text.
/*! This invalidates (and destroys) the XForm. */
void Text::setStyle(Attribute style)
{
  iStyle = style;
  setXForm(nullptr);
}

//! Sets the text of the text object.
/*! This invalidates (and destroys) the XForm. */
void Text::setText(String text)
{
  iText = text;
  setXForm(nullptr);
}

//! Change type.
/*! This invalidates (and destroys) the XForm. */
void Text::setTextType(TextType type)
{
  if (type != iType) {
    iType = type;
    iStyle = Attribute::NORMAL();
    setXForm(nullptr);
  }
}

//! Change horizontal alignment (text moves with respect to reference point).
void Text::setHorizontalAlignment(THorizontalAlignment align)
{
  iHorizontalAlignment = align;
}

//! Change vertical alignment (text moves with respect to reference point).
void Text::setVerticalAlignment(TVerticalAlignment align)
{
  iVerticalAlignment = align;
}

bool Text::setAttribute(Property prop, Attribute value)
{
  switch (prop) {
  case EPropStrokeColor:
    if (value != stroke()) {
      setStroke(value);
      return true;
    }
    break;
  case EPropTextSize:
    if (value != size()) {
      setSize(value);
      return true;
    }
    break;
  case EPropTextStyle:
  case EPropLabelStyle:
    if ((isMinipage() != (prop == EPropTextStyle)) || value == style())
      return false;
    setStyle(value);
    return true;
  case EPropOpacity:
    if (value != opacity()) {
      setOpacity(value);
      return true;
    }
    break;
  case EPropHorizontalAlignment:
    assert(value.isEnum());
    if (value.horizontalAlignment() != horizontalAlignment()) {
      iHorizontalAlignment = THorizontalAlignment(value.horizontalAlignment());
      return true;
    }
    break;
  case EPropVerticalAlignment:
    assert(value.isEnum());
    if (value.verticalAlignment() != verticalAlignment()) {
      iVerticalAlignment = TVerticalAlignment(value.verticalAlignment());
      return true;
    }
    break;
  case EPropMinipage:
    assert(value.isEnum());
    if (value.boolean() != isMinipage()) {
      iType = value.boolean() ? EMinipage : ELabel;
      iStyle = Attribute::NORMAL();
      return true;
    }
    break;
  case EPropWidth:
    assert(value.isNumber());
    if (value.number().toDouble() != width()) {
      setWidth(value.number().toDouble());
      return true;
    }
    break;
  case EPropTransformableText:
    assert(value.isEnum());
    if (value.boolean() && transformations() != ETransformationsAffine) {
      setTransformations(ETransformationsAffine);
      return true;
    } else if (!value.boolean() &&
	       transformations() != ETransformationsTranslations) {
      setTransformations(ETransformationsTranslations);
      return true;
    }
    break;
  default:
    return Object::setAttribute(prop, value);
  }
  return false;
}

Attribute Text::getAttribute(Property prop) const noexcept
{
  switch (prop) {
  case EPropStrokeColor:
    return stroke();
  case EPropTextSize:
    return size();
  case EPropTextStyle:
  case EPropLabelStyle:
    return style();
  case EPropOpacity:
    return opacity();
  case EPropHorizontalAlignment:
    return Attribute(horizontalAlignment());
  case EPropVerticalAlignment:
    return Attribute(verticalAlignment());
  case EPropMinipage:
    return Attribute::Boolean(isMinipage());
  case EPropWidth:
    return Attribute(Fixed::fromDouble(width()));
  default:
    return Object::getAttribute(prop);
  }
}

// --------------------------------------------------------------------

//! Check symbolic size attribute.
void Text::checkStyle(const Cascade *sheet, AttributeSeq &seq) const
{
  checkSymbol(EColor, iStroke, sheet, seq);
  checkSymbol(ETextSize, iSize, sheet, seq);
  checkSymbol((iType == ELabel ? ELabelStyle : ETextStyle), iStyle, sheet, seq);
  checkSymbol(EOpacity, iOpacity, sheet, seq);
}

//! Return quadrilateral including the text.
/*! This is the bounding box, correctly transformed by matrix(),
  taking into consideration whether the object is transformable.
 */
void Text::quadrilateral(const Matrix &m, Vector v[4]) const
{
  double wid = iWidth;
  double ht = totalHeight();
  Vector offset = -align();
  v[0] = offset + Vector(0, 0);
  v[1] = offset + Vector(wid, 0);
  v[2] = offset + Vector(wid, ht);
  v[3] = offset + Vector(0, ht);

  Matrix m1 = m * matrix() * Matrix(iPos);

  if (iTransformations == ETransformationsTranslations) {
    m1 = Matrix(m1.translation());
  } else if (iTransformations == ETransformationsRigidMotions) {
    Angle alpha = Vector(m1.a[0], m1.a[1]).angle();
    // ensure that (1,0) is rotated into this orientation
    m1 = Matrix(m1.translation()) * Linear(alpha);
  }

  for (int i = 0; i < 4; ++i)
    v[i] = m1 * v[i];
}

//! Update the PDF code for this object.
void Text::setXForm(XForm *xform) const
{
  if (iXForm && --iXForm->iRefCount == 0)
    delete iXForm;
  iXForm = xform;
  if (iXForm) {
    ++iXForm->iRefCount;
    iDepth = iXForm->iStretch * iXForm->iDepth / 100.0;
    iHeight = iXForm->iStretch * iXForm->iBBox.height() - iDepth;
    if (!isMinipage())
      iWidth = iXForm->iStretch * iXForm->iBBox.width();
  }
}

//! Return position of reference point in text box coordinate system.
/*! Assume a coordinate system where the text box has corners (0,0)
  and (Width(), TotalHeight()).  This function returns the coordinates
  of the reference point in this coordinate system. */
Vector Text::align() const
{
  Vector align(0.0, 0.0);
  switch (verticalAlignment()) {
  case EAlignTop:
    align.y = totalHeight();
    break;
  case EAlignBottom:
    break;
  case EAlignVCenter:
    align.y = 0.5 * totalHeight();
    break;
  case EAlignBaseline:
    align.y = depth();
    break;
  }
  switch (horizontalAlignment()) {
  case EAlignLeft:
    break;
  case EAlignRight:
    align.x = width();
    break;
  case EAlignHCenter:
    align.x = 0.5 * width();
    break;
  }
  return align;
}

// --------------------------------------------------------------------

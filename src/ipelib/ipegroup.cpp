// --------------------------------------------------------------------
// The group object
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
#include "ipepainter.h"
#include "ipetext.h"
#include "ipeshape.h"

using namespace ipe;

/*! \class ipe::Group
  \ingroup obj
  \brief The group object.

  Ipe objects can be grouped together, and the resulting composite can
  be used like any Ipe object.

  This is an application of the "Composite" pattern.
*/

//! Create empty group (objects can be added later).
Group::Group() : Object()
{
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iPinned = ENoPin;
  iDecoration = Attribute::NORMAL();
}

//! Create empty group with these attributes (objects can be added later).
Group::Group(const XmlAttributes &attr)
  : Object(attr)
{
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iPinned = ENoPin;
  String str;
  if (attr.has("clip", str)) {
    Shape clip;
    if (clip.load(str) && clip.countSubPaths() > 0)
      iClip = clip;
  }
  iUrl = attr["url"];
  iDecoration = attr.has("decoration", str) ?
    Attribute(true, str) : Attribute::NORMAL();
}

//! Copy constructor. Constant time --- components are not copied!
Group::Group(const Group &rhs)
  : Object(rhs)
{
  iImp = rhs.iImp;
  iImp->iRefCount++;
  iClip = rhs.iClip;
  iUrl = rhs.iUrl;
  iDecoration = rhs.iDecoration;
}

//! Destructor.
Group::~Group()
{
  if (iImp->iRefCount == 1) {
    for (List::iterator it = iImp->iObjects.begin();
	 it != iImp->iObjects.end(); ++it) {
      delete *it;
      *it = nullptr;
    }
    delete iImp;
  } else
    iImp->iRefCount--;
}

//! Assignment operator (constant-time).
Group &Group::operator=(const Group &rhs)
{
  if (this != &rhs) {
    if (iImp->iRefCount == 1)
      delete iImp;
    else
      iImp->iRefCount--;
    iImp = rhs.iImp;
    iImp->iRefCount++;
    iClip = rhs.iClip;
    iUrl = rhs.iUrl;
    iDecoration = rhs.iDecoration;
    Object::operator=(rhs);
  }
  return *this;
}

//! Clone a group object (constant-time).
Object *Group::clone() const
{
  return new Group(*this);
}

//! Return pointer to this object.
Group *Group::asGroup()
{
  return this;
}

//! Return pointer to this object.
const Group *Group::asGroup() const
{
  return this;
}

Object::Type Group::type() const
{
  return EGroup;
}

//! Add an object.
/*! Takes ownership of the object.
  This will panic if the object shares its implementation!
  The method is only useful right after construction of the group. */
void Group::push_back(Object *obj)
{
  assert(iImp->iRefCount == 1);
  iImp->iObjects.push_back(obj);
  iImp->iPinned = TPinned(iImp->iPinned | obj->pinned());
}

//! Save all the components, one by one, in XML format.
void Group::saveComponentsAsXml(Stream &stream) const
{
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->saveAsXml(stream, String());
}

//! Call visitGroup of visitor.
void Group::accept(Visitor &visitor) const
{
  visitor.visitGroup(this);
}

void Group::saveAsXml(Stream &stream, String layer) const
{
  stream << "<group";
  saveAttributesAsXml(stream, layer);
  if (iClip.countSubPaths()) {
    stream << " clip=\"";
    iClip.save(stream);
    stream << "\"";
  }
  if (!iUrl.empty()) {
    stream << " url=\"";
    stream.putXmlString(iUrl);
    stream << "\"";
  }
  if (!iDecoration.isNormal())
    stream << " decoration=\"" << iDecoration.string() << "\"";
  stream << ">\n";
  saveComponentsAsXml(stream);
  stream << "</group>\n";
}

// --------------------------------------------------------------------

class DecorationPainter : public Painter {
public:
  DecorationPainter(Painter &painter, const Vector &center,
		    double dx, double dy);
protected:
  virtual void doPush();
  virtual void doPop();
  virtual void doNewPath();
  virtual void doMoveTo(const Vector &v);
  virtual void doLineTo(const Vector &v);
  virtual void doCurveTo(const Vector &v1, const Vector &v2,
			 const Vector &v3);
  virtual void doClosePath();
  virtual void doDrawPath(TPathMode mode);
  Vector adapt(const Vector &v);
private:
  Painter &iPainter;
  Vector iCenter;
  double iX, iY;
};

DecorationPainter::DecorationPainter(Painter &painter, const Vector &center,
				     double dx, double dy) :
  Painter(painter.cascade()), iPainter(painter), iCenter(center),
  iX(dx), iY(dy)
{
  // nothing
}

Vector DecorationPainter::adapt(const Vector &v)
{
  Vector r;
  r.x = (v.x < iCenter.x) ? v.x - iX : v.x + iX;
  r.y = (v.y < iCenter.y) ? v.y - iY : v.y + iY;
  return r;
}

void DecorationPainter::doPush()
{
  iPainter.push();
}

void DecorationPainter::doPop()
{
  iPainter.pop();
}

void DecorationPainter::doNewPath()
{
  iPainter.setState(iState.back());
  iPainter.newPath();
}

void DecorationPainter::doMoveTo(const Vector &v)
{
  iPainter.moveTo(adapt(v));
}

void DecorationPainter::doLineTo(const Vector &v)
{
  iPainter.lineTo(adapt(v));
}

void DecorationPainter::doCurveTo(const Vector &v1, const Vector &v2,
				  const Vector &v3)
{
  iPainter.curveTo(adapt(v1), adapt(v2), adapt(v3));
}

void DecorationPainter::doClosePath()
{
  iPainter.closePath();
}

void DecorationPainter::doDrawPath(TPathMode mode)
{
  iPainter.drawPath(mode);
}

// --------------------------------------------------------------------

void Group::draw(Painter &painter) const
{
  if (!iDecoration.isNormal()) {
    painter.pushMatrix();
    auto m = painter.matrix();
    painter.untransform(ETransformationsTranslations);
    Rect r;
    addToBBox(r, m, false);
    double dx = 0.5 * (r.width() - 200.0);
    double dy = 0.5 * (r.height() - 100.0);
    DecorationPainter dp(painter, r.center(), dx, dy);
    dp.translate(r.center() - Vector(200.0, 150.0));
    dp.drawSymbol(iDecoration);
    painter.popMatrix();
  }
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  if (iClip.countSubPaths()) {
    painter.push();
    painter.newPath();
    iClip.draw(painter);
    painter.addClipPath();
  }
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->draw(painter);
  if (iClip.countSubPaths())
    painter.pop();
  painter.popMatrix();
}

void Group::drawSimple(Painter &painter) const
{
  painter.pushMatrix();
  painter.transform(matrix());
  painter.untransform(transformations());
  if (iClip.countSubPaths()) {
    painter.push();
    painter.newPath();
    iClip.draw(painter);
    painter.addClipPath();
  }
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->drawSimple(painter);
  if (iClip.countSubPaths())
    painter.pop();
  painter.popMatrix();
}

void Group::addToBBox(Rect &box, const Matrix &m, bool cp) const
{
  Matrix m1 = m * matrix();
  Rect tbox;
  for (const_iterator it = begin(); it != end(); ++it) {
    (*it)->addToBBox(tbox, m1, cp);
  }
  // now clip to clipping path
  if (iClip.countSubPaths()) {
    Rect cbox;
    iClip.addToBBox(cbox, m1, false);
    tbox.clipTo(cbox);
  }
  box.addRect(tbox);
}

//! Return total pinning status of group and its elements.
TPinned Group::pinned() const
{
  return TPinned(Object::pinned() | iImp->iPinned);
}

double Group::distance(const Vector &v, const Matrix &m, double bound) const
{
  double d = bound;
  double d1;
  Matrix m1 = m * matrix();
  for (const_iterator it = begin(); it != end(); ++it) {
    if ((d1 = (*it)->distance(v, m1, d)) < d)
      d = d1;
  }
  return d;
}

void Group::snapVtx(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound) const
{
  Matrix m1 = m * matrix();
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->snapVtx(mouse, m1, pos, bound);
}

void Group::snapCtl(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound) const
{
  Matrix m1 = m * matrix();
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->snapCtl(mouse, m1, pos, bound);
}

void Group::snapBnd(const Vector &mouse, const Matrix &m,
		    Vector &pos, double &bound) const
{
  Matrix m1 = m * matrix();
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->snapBnd(mouse, m1, pos, bound);
}

void Group::checkStyle(const Cascade *sheet,
		       AttributeSeq &seq) const
{
  for (const_iterator it = begin(); it != end(); ++it)
    (*it)->checkStyle(sheet, seq);
}

//! Set clip path for this group.
/*! Any previously set clip path is deleted. */
void Group::setClip(const Shape &clip)
{
  iClip = clip;
}

//! Set link destination to use this group as a hyperlink.
void Group::setUrl(String url)
{
  iUrl = url;
}

//! Create private implementation.
void Group::detach()
{
  Imp *old = iImp;
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iPinned = old->iPinned;
  for (const_iterator it = old->iObjects.begin();
       it != old->iObjects.end(); ++it)
    iImp->iObjects.push_back((*it)->clone());

}

Attribute Group::getAttribute(Property prop) const noexcept
{
  if (prop == EPropDecoration)
    return iDecoration;
  else
    return Object::getAttribute(prop);
}

//! Set attribute on all children.
bool Group::setAttribute(Property prop, Attribute value)
{
  if (prop == EPropPinned || prop == EPropTransformations)
    return Object::setAttribute(prop, value);
  if (prop == EPropDecoration) {
    auto old = iDecoration;
    iDecoration = value;
    return (old != iDecoration);
  }
  // all others are handled by elements themselves
  detach();
  bool result = false;
  for (List::iterator it = iImp->iObjects.begin();
       it != iImp->iObjects.end(); ++it)
    result |= (*it)->setAttribute(prop, value);
  return result;
}

// --------------------------------------------------------------------

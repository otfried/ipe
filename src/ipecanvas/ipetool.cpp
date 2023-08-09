// --------------------------------------------------------------------
// ipe::Tool
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

#include "ipecanvas.h"
#include "ipetool.h"

using namespace ipe;

#define EPS 1e-12

// --------------------------------------------------------------------

/*! \class ipe::PanTool
  \ingroup canvas
  \brief A tool for panning the canvas.
*/

PanTool::PanTool(CanvasBase *canvas, const Page *page, int view)
  : Tool(canvas), iPage(page), iView(view)
{
  iPan = Vector::ZERO;
  iMouseDown = iCanvas->unsnappedPos();
  iCanvas->setCursor(CanvasBase::EHandCursor);
}

void PanTool::draw(Painter &painter) const
{
  painter.translate(iPan);
  painter.setStroke(Attribute(Color(0, 0, 1000)));

  painter.newPath();
  const Layout *l = iCanvas->cascade()->findLayout();
  painter.rect(Rect(-l->iOrigin, -l->iOrigin + l->iPaperSize));
  painter.drawPath(EStrokedOnly);

  for (int i = 0; i < iPage->count(); ++i) {
    if (iPage->objectVisible(iView, i))
      iPage->object(i)->drawSimple(painter);
  }
}

void PanTool::mouseButton(int button, bool press)
{
  if (!press) {
    Vector dpan = iCanvas->unsnappedPos() - iMouseDown;
    iCanvas->setPan(iCanvas->pan() - dpan);
  }
  iCanvas->finishTool();
}

void PanTool::mouseMove()
{
  iPan = iCanvas->unsnappedPos() - iMouseDown;
  iCanvas->updateTool();
}

// --------------------------------------------------------------------

/*! \class ipe::SelectTool
  \ingroup canvas
  \brief A tool for selecting objects.
*/

class SelectCompare {
public:
  int operator()(const SelectTool::SObj &lhs,
		 const SelectTool::SObj &rhs) const
  {
    return (lhs.distance < rhs.distance);
  }
};

//! Constructor starts selection.
SelectTool::SelectTool(CanvasBase *canvas, Page *page, int view,
		       double selectDistance, bool nonDestructive)
  : Tool(canvas)
{
  iPage = page;
  iView = view;
  iNonDestructive = nonDestructive;
  iSelectDistance = selectDistance;

  // coordinates in user space
  Vector v = iCanvas->unsnappedPos();

  iMouseDown = v;
  iDragging = false;

  double bound = iSelectDistance / iCanvas->zoom();

  // Collect objects close enough
  double d;
  for (int i = iPage->count() - 1; i >= 0; --i) {
    if (iPage->objectVisible(iView, i) &&
	!iPage->isLocked(iPage->layerOf(i))) {
      if ((d = iPage->distance(i, v, bound)) < bound) {
	SObj obj;
	obj.index = i;
	obj.distance = d;
	iObjs.push_back(obj);
      }
    }
  }
  iCur = 0;
  std::stable_sort(iObjs.begin(), iObjs.end(), SelectCompare());
  iCanvas->setCursor(CanvasBase::ECrossCursor);
}

void SelectTool::draw(Painter &painter) const
{
  if (iDragging) {
    Rect r(iMouseDown, iCorner);
    painter.setStroke(Attribute(Color(1000, 0, 1000)));
    painter.newPath();
    painter.rect(r);
    painter.drawPath(EStrokedOnly);
  } else {
    painter.setStroke(Attribute(Color(1000, 0, 1000)));
    painter.newPath();
    double d = iSelectDistance / iCanvas->zoom();
    painter.drawArc(Arc(Matrix(d, 0, 0, d, iMouseDown.x, iMouseDown.y)));
    painter.closePath();
    painter.drawPath(EStrokedOnly);

    if (iObjs.size() > 0) {
      // display current object
      painter.setStroke(Attribute(Color(1000, 0, 0)));
      iPage->object(iObjs[iCur].index)->drawSimple(painter);
    }
  }
}

void SelectTool::mouseButton(int button, bool press)
{
  // ipeDebug("SelectTool::mouseButton(%d, %d)", button, press);
  if (press) {
    iCanvas->finishTool();
    return;
  }

  bool changed = false;
  if (iDragging) {
    Rect r(iMouseDown, iCorner);
    // dragging from right to left behaves differently
    bool alternate = (iCorner.x < iMouseDown.x);
    if (iNonDestructive) {
      // xor selection status of objects in range
      // last object that is not selected is made primary selection
      int new_primary = -1;
      for (int i = 0; i < iPage->count(); ++i) {
	Matrix id;
	if (iPage->objectVisible(iView, i) &&
	    !iPage->isLocked(iPage->layerOf(i))) {
	  Rect s;
	  iPage->object(i)->addToBBox(s, id, false);
	  if (alternate ? r.intersects(s) : r.contains(s)) {
	    changed = true;
	    // in range
	    if (iPage->select(i))
	      iPage->setSelect(i, ENotSelected);
	    else {
	      new_primary = i;
	      iPage->setSelect(i, ESecondarySelected);
	    }
	  }
	}
      }
      if (new_primary >= 0) {
	// deselect old primary, select new primary
	int old_primary = iPage->primarySelection();
	if (old_primary >= 0)
	  iPage->setSelect(old_primary, ESecondarySelected);
	iPage->setSelect(new_primary, EPrimarySelected);
      } else
	iPage->ensurePrimarySelection();
    } else {
      // deselect all objects outside range,
      // secondary select all objects in range,
      for (int i = 0; i < iPage->count(); ++i) {
	iPage->setSelect(i, ENotSelected);
	Matrix id;
	if (iPage->objectVisible(iView, i) &&
	    !iPage->isLocked(iPage->layerOf(i))) {
	  Rect s;
	  iPage->object(i)->addToBBox(s, id, false);
	  if (alternate ? r.intersects(s) : r.contains(s))
	    iPage->setSelect(i, ESecondarySelected);
	}
      }
      changed = true;  // XXX not accurate, but not used now anyway
      iPage->ensurePrimarySelection();
    }
  } else if (iObjs.size() > 0) {
    int index = iObjs[iCur].index;
    if (iNonDestructive) {
      if (!iPage->select(index)) {
	// selecting unselected object as primary
	int old = iPage->primarySelection();
	if (old >= 0) iPage->setSelect(old, ESecondarySelected);
	iPage->setSelect(index, EPrimarySelected);
      } else
	// deselecting selected object
	iPage->setSelect(index, ENotSelected);
      changed = true;
      iPage->ensurePrimarySelection();
    } else {
      // destructive: unselect all
      for (int i = 0; i < iPage->count(); ++i) {
	if (i != index && iPage->select(i)) {
	  changed = true;
	  iPage->setSelect(i, ENotSelected);
	}
      }
      if (iPage->select(index) != EPrimarySelected)
	changed = true;
      iPage->setSelect(index, EPrimarySelected);
    }
  } else {
    // no object in range, deselect all
    if (!iNonDestructive) {
      changed = iPage->hasSelection();
      iPage->deselectAll();
    }
  }
  iCanvas->finishTool();
  // not using right now
  (void) changed;
}

void SelectTool::mouseMove()
{
  iCorner = iCanvas->unsnappedPos();
  if ((iCorner - iMouseDown).sqLen() > 9.0)
    iDragging = true;
  iCanvas->updateTool();
}

bool SelectTool::key(String text, int modifiers)
{
  if (!iDragging && text == " " && iObjs.size() > 0) {
    iCur++;
    if (iCur >= int(iObjs.size()))
      iCur = 0;
    iCanvas->updateTool();
    return true;
  } else if (text == "\027") {
    iCanvas->finishTool();
    return true;
  } else
    return false;
}

// --------------------------------------------------------------------

/*! \class ipe::TransformTool
  \ingroup canvas
  \brief A tool for transforming the selected objects on the canvas.

  Supports moving, rotating, scaling, stretching, and shearing.
*/

//! Constructor starts transformation.
/*! After constructing a TransformTool, you must call isValid() to
    ensure that the transformation can be performed.

    A transformation can fail because the selection contains pinned
    objects, or because the initial mouse position is identical to the
    transformation origin. */
TransformTool::TransformTool(CanvasBase *canvas, Page *page, int view,
			     TType type, bool withShift)
  : Tool(canvas)
{
  iPage = page;
  iView = view;
  iType = type;
  iWithShift = withShift;
  iMouseDown = iCanvas->pos();
  if (iType == ETranslate)
    iCanvas->setAutoOrigin(iMouseDown);
  iOnlyHorizontal = false;
  iOnlyVertical = false;
  iValid = true;

  // check if objects are pinned
  TPinned pin = ENoPin;
  for (int i = 0; i < page->count(); ++i) {
    if (page->select(i))
      pin = TPinned(pin | page->object(i)->pinned());
  }

  // rotating, scaling, stretching, shearing are not allowed on pinned objects
  if (pin == EFixedPin || (pin && iType != ETranslate)) {
    iValid = false;
    return;
  }

  if (pin) {
    if (pin == EVerticalPin)
      iOnlyHorizontal = true;
    else
      iOnlyVertical = true;
    iWithShift = false;  // ignore this and follow pinning restriction
  }

  // compute origin
  const Snap &sd = iCanvas->snap();
  if (sd.iWithAxes) {
    iOrigin = sd.iOrigin;
    iDir = sd.iDir;
  } else {
    iDir = 0.0;
    // find bounding box of selected objects
    Rect bbox;
    for (int i = 0; i < iPage->count(); ++i) {
      if (iPage->select(i))
	bbox.addRect(iPage->bbox(i));
    }
    iOrigin = 0.5 * (bbox.bottomLeft() + bbox.topRight());
    if (iType == EStretch || iType == EScale || iType == EShear) {
      if (iMouseDown.x > iOrigin.x)
	iOrigin.x = bbox.bottomLeft().x;
      else
	iOrigin.x = bbox.topRight().x;
      if (iMouseDown.y > iOrigin.y)
	iOrigin.y = bbox.bottomLeft().y;
      else
	iOrigin.y = bbox.topRight().y;
    }
  }

  if (iType == EShear && abs((Linear(-iDir) * (iMouseDown - iOrigin)).y) < 0.1)
    iValid = false;
  else if (iType != ETranslate && iMouseDown == iOrigin)
    iValid = false;
  else
    iCanvas->setCursor(CanvasBase::EHandCursor);
}

//! Check that the transformation can be performed.
bool TransformTool::isValid() const
{
  return iValid;
}

void TransformTool::draw(Painter &painter) const
{
  painter.setStroke(Attribute(Color(0, 600, 0)));
  painter.transform(iTransform);
  for (int i = 0; i < iPage->count(); ++i) {
    if (iPage->select(i))
      iPage->object(i)->drawSimple(painter);
  }
}

// compute iTransform
void TransformTool::compute(const Vector &v1)
{
  Vector u0 = iMouseDown - iOrigin;
  Vector u1 = v1 - iOrigin;

  switch (iType) {
  case ETranslate: {
    Vector d = v1 - iMouseDown;
    if (iOnlyHorizontal || (iWithShift && ipe::abs(d.x) > ipe::abs(d.y)))
      d.y = 0.0;
    else if (iOnlyVertical || iWithShift)
      d.x = 0.0;
    iTransform = Matrix(d);
    break; }
  case ERotate:
    iTransform = (Matrix(iOrigin) *
		  Linear(Angle(u1.angle() - u0.angle())) *
		  Matrix(-iOrigin));
    break;
  case EScale: {
    double factor = sqrt(u1.sqLen() / u0.sqLen());
    iTransform = (Matrix(iOrigin) *
		  Linear(factor, 0, 0, factor) *
		  Matrix(-iOrigin));
    break; }
  case EStretch: {
    auto rot = Linear(-iDir);
    auto v0 = rot * u0;
    auto v1 = rot * u1;
    double xfactor = (abs(u0.x) < EPS) ? 1.0 : v1.x / v0.x;
    double yfactor = (abs(u0.y) < EPS) ? 1.0 : v1.y / v0.y;
    Matrix m = (Matrix(iOrigin) * Linear(iDir) *
		Linear(xfactor, 0, 0, yfactor) * rot *
		Matrix(-iOrigin));
    if (std::fabs(m.determinant()) > 0.0001)
      iTransform = m;
    break; }
  case EShear: {
    auto rot = Linear(-iDir);
    auto v0 = rot * u0;
    auto v1 = rot * u1;
    double s = (v1.x - v0.x) / v0.y;
    Matrix m = (Matrix(iOrigin) * Linear(iDir) *
		Linear(1.0, 0, s, 1.0) * rot *
		Matrix(-iOrigin));
    if (std::fabs(m.determinant()) > 0.0001)
      iTransform = m;
    break; }
  }
}

void TransformTool::mouseButton(int button, bool press)
{
  if (!press) {
    compute(iCanvas->pos());
    report();
  }
  iCanvas->finishTool();
}

//! Report the final transformation chosen.
/*! The implementation in TransformTool does nothing.
  Derived classes should reimplement report(). */
void TransformTool::report()
{
  // nothing yet
}

void TransformTool::mouseMove()
{
  compute(iCanvas->pos());
  iCanvas->updateTool();
}

// --------------------------------------------------------------------

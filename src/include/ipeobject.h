// -*- C++ -*-
// --------------------------------------------------------------------
// The Ipe object type.
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

#ifndef IPEOBJ_H
#define IPEOBJ_H

#include "ipeattributes.h"
#include "ipexml.h"

// --------------------------------------------------------------------

namespace ipe {

  class Visitor;
  class Painter;
  class Group;
  class Text;
  class Path;
  class Image;
  class Reference;
  class StyleSheet;
  class Cascade;

  // --------------------------------------------------------------------

  class Object {
  public:
    enum Type { EGroup, EPath, EText, EImage, EReference };

    virtual ~Object() = 0;

    //! Calls visitXXX method of the visitor.
    virtual void accept(Visitor &visitor) const = 0;

    //! Make a copy of this object (constant-time).
    virtual Object *clone() const = 0;

    virtual Group *asGroup();
    virtual const Group *asGroup() const;
    virtual Text *asText();
    virtual Path *asPath();
    virtual Image *asImage();
    virtual Reference *asReference();

    virtual Type type() const = 0;

    virtual TPinned pinned() const;
    void setPinned(TPinned pin);

    //! Return allowed transformations of the object.
    inline TTransformations transformations() const { return iTransformations; }
    void setTransformations(TTransformations trans);

    virtual void setMatrix(const Matrix &matrix);
    //! Return transformation matrix.
    inline const Matrix &matrix() const { return iMatrix; }

    virtual bool setAttribute(Property prop, Attribute value);
    virtual Attribute getAttribute(Property prop) const noexcept;

    void setCustom(Attribute value);
    Attribute getCustom() const noexcept;

    //! Save the object in XML format.
    virtual void saveAsXml(Stream &stream, String layer) const = 0;

    //! Draw the object.
    virtual void draw(Painter &painter) const = 0;

    //! Draw simple version for selecting and transforming.
    virtual void drawSimple(Painter &painter) const = 0;

    /*! Return distance of transformed object to point \a v.
      If larger than \a bound, can just return \a bound. */
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const = 0;

    //! Extend \a box to include the object transformed by \a m.
    /*! For objects in a page, don't call this directly.  The Page
      caches the bounding box of each object, so it is far more
      efficient to call Page::bbox.

      Control points that lie outside the visual object are included
      if \a cp is true.

      If called with an empty box and \a cp == \c false, the result of
      this function is a tight bounding box for the object, with a
      little leeway in case the boundary is determined by a spline (it
      has to be approximated to perform this operation).
    */
    virtual void addToBBox(Rect &box, const Matrix &m, bool cp)
      const = 0;

    virtual void snapVtx(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;
    virtual void snapCtl(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;
    virtual void snapBnd(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;

    virtual void checkStyle(const Cascade *sheet, AttributeSeq &seq) const;


  protected:
    explicit Object();
    explicit Object(const AllAttributes &attr);
    Object(const Object &rhs);

    explicit Object(const XmlAttributes &attr);

    void saveAttributesAsXml(Stream &stream, String layer) const;
    static void checkSymbol(Kind kind, Attribute attr,
			    const Cascade *sheet, AttributeSeq &seq);

  protected:
    Matrix iMatrix;
    Attribute iCustom;
    TPinned iPinned : 8;
    TTransformations iTransformations : 8;
  };

  // --------------------------------------------------------------------

  class Visitor {
  public:
    virtual ~Visitor();
    virtual void visitGroup(const Group *obj);
    virtual void visitPath(const Path *obj);
    virtual void visitText(const Text *obj);
    virtual void visitImage(const Image *obj);
    virtual void visitReference(const Reference *obj);
  };

} // namespace

// --------------------------------------------------------------------
#endif

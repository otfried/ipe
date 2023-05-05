// -*- C++ -*-
// --------------------------------------------------------------------
// The path object.
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

#ifndef IPEPATH_H
#define IPEPATH_H

#include "ipeobject.h"
#include "ipeshape.h"

// --------------------------------------------------------------------

namespace ipe {

  class Path : public Object {
  public:
    explicit Path(const AllAttributes &attr, const Shape &shape,
		  bool withArrows = false);

    static Path *create(const XmlAttributes &attr, String data);

    virtual Object *clone() const override;

    virtual Path *asPath() override;

    virtual Type type() const override;

    void setPathMode(TPathMode pm);
    void setStroke(Attribute stroke);
    void setFill(Attribute fill);
    void setPen(Attribute pen);
    void setDashStyle(Attribute dash);
    void setLineCap(TLineCap s);
    void setLineJoin(TLineJoin s);
    void setFillRule(TFillRule s);
    void setOpacity(Attribute opaq);
    void setStrokeOpacity(Attribute opaq);
    void setTiling(Attribute a);
    void setGradient(Attribute a);

    //! Return opacity of the opject.
    inline Attribute opacity() const { return iOpacity; }
    //! Return stroke opacity of the opject.
    inline Attribute strokeOpacity() const { return iStrokeOpacity; }
    //! Return tiling pattern.
    inline Attribute tiling() const { return iTiling; }
    //! Return gradient fill.
    inline Attribute gradient() const { return iGradient; }
    inline TPathMode pathMode() const;
    inline Attribute stroke() const;
    inline Attribute fill() const;
    inline Attribute pen() const;
    inline Attribute dashStyle() const;
    inline TLineCap lineCap() const;
    inline TLineJoin lineJoin() const;
    inline TFillRule fillRule() const;

    virtual void saveAsXml(Stream &stream, String layer) const override;
    virtual void draw(Painter &painter) const override;
    virtual void drawSimple(Painter &painter) const override;

    virtual void accept(Visitor &visitor) const override;

    virtual void addToBBox(Rect &box, const Matrix &m, bool cp) const override;
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const override;
    virtual void snapVtx(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const override;
    virtual void snapCtl(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const override;
    virtual void snapBnd(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const override;

    virtual void checkStyle(const Cascade *sheet,
			    AttributeSeq &seq) const override;

    virtual void setMatrix(const Matrix &matrix) override;

    virtual bool setAttribute(Property prop, Attribute value) override;
    virtual Attribute getAttribute(Property prop) const noexcept override;

    inline bool arrow() const;
    inline bool rArrow() const;
    inline Attribute arrowShape() const;
    inline Attribute rArrowShape() const;
    inline Attribute arrowSize() const;
    inline Attribute rArrowSize() const;

    void setArrow(bool arrow, Attribute shape, Attribute size);
    void setRarrow(bool arrow, Attribute shape, Attribute size);

    static void drawArrow(Painter &painter, Vector pos, Angle alpha,
			  Attribute shape, Attribute size, double radius);

    //! Return shape of the path object.
    const Shape &shape() const { return iShape; }
    void setShape(const Shape &shape);

  private:
    explicit Path(const XmlAttributes &attr);
    void init(const AllAttributes &attr, bool withArrows);
    void makeArrowData();

  private:
    TPathMode iPathMode : 2;
    int iHasFArrow : 1;
    int iHasRArrow : 1;
    TLineJoin iLineJoin : 3;
    TLineCap iLineCap : 3;
    TFillRule iFillRule : 2;
    int iFArrowOk : 1;
    int iRArrowOk : 1;
    int iFArrowArc : 1;
    int iRArrowArc : 1;
    int iMArrowOk : 1;
    int iFArrowIsM : 1;
    int iRArrowIsM : 1;

    Attribute iStroke;
    Attribute iFill;
    Attribute iDashStyle;
    Attribute iPen;
    Attribute iOpacity;
    Attribute iStrokeOpacity;
    Attribute iTiling;
    Attribute iGradient;

    Attribute iFArrowShape;
    Attribute iRArrowShape;
    Attribute iFArrowSize;
    Attribute iRArrowSize;

    Vector iFArrowPos;
    Angle  iFArrowDir;
    Vector iRArrowPos;
    Angle  iRArrowDir;
    Vector iMArrowPos;
    Angle  iMArrowDir;

    Shape iShape;
  };

  // --------------------------------------------------------------------

  //! Is the object stroked and filled?
  inline TPathMode Path::pathMode() const
  {
    return iPathMode;
  }

  //! Return stroke color.
  inline Attribute Path::stroke() const
  {
    return iStroke;
  }

  //! Return object fill color.
  inline Attribute Path::fill() const
  {
    return iFill;
  }

  //! Return object pen.
  inline Attribute Path::pen() const
  {
    return iPen;
  }

  //! Return object line style.
  inline Attribute Path::dashStyle() const
  {
    return iDashStyle;
  }

  //! Return line cap style.
  inline TLineCap Path::lineCap() const
  {
    return TLineCap(iLineCap);
  }

  //! Return line join style.
  inline TLineJoin Path::lineJoin() const
  {
    return TLineJoin(iLineJoin);
  }

  //! Return fill rule.
  inline TFillRule Path::fillRule() const
  {
    return TFillRule(iFillRule);
  }

  //! Does object have an arrow?
  inline bool Path::arrow() const
  {
    return iHasFArrow;
  }

  //! Does object have a reverse arrow?
  inline bool Path::rArrow() const
  {
    return iHasRArrow;
  }

  //! Return shape of arrow.
  inline Attribute Path::arrowShape() const
  {
    return iFArrowShape;
  }

  //! Return shape of reverse arrow.
  inline Attribute Path::rArrowShape() const
  {
    return iRArrowShape;
  }

  //! Return size of arrow.
  inline Attribute Path::arrowSize() const
  {
    return iFArrowSize;
  }

  //! Return size of reverse arrow.
  inline Attribute Path::rArrowSize() const
  {
    return iRArrowSize;
  }

} // namespace

// --------------------------------------------------------------------
#endif


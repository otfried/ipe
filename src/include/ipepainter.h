// -*- C++ -*-
// --------------------------------------------------------------------
// Painter abstraction
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

#ifndef IPEPAINTER_H
#define IPEPAINTER_H

#include <list>

#include "ipeattributes.h"
#include "ipestyle.h"
#include "ipebitmap.h"

// --------------------------------------------------------------------

namespace ipe {

  class Painter {
  public:
    struct State {
      Color iStroke;
      Color iFill;
      Fixed iPen;
      String iDashStyle;
      TLineCap iLineCap;
      TLineJoin iLineJoin;
      TFillRule iFillRule;
      Color iSymStroke;
      Color iSymFill;
      Fixed iSymPen;
      Fixed iOpacity;
      Fixed iStrokeOpacity;
      Attribute iTiling;
      Attribute iGradient;
    };

  public:
    Painter(const Cascade *style);
    virtual ~Painter();

    void setAttributeMap(const AttributeMap *map);
    Attribute lookup(Kind kind, Attribute sym) const;

    void transform(const Matrix &m);
    void untransform(TTransformations trans);
    void translate(const Vector &v);
    void push();
    void pop();
    void pushMatrix();
    void popMatrix();

    void newPath();
    void moveTo(const Vector &v);
    void lineTo(const Vector &v);
    void curveTo(const Vector &v1, const Vector &v2, const Vector &v3);
    inline void curveTo(const Bezier &bezier);
    void rect(const Rect &re);
    void drawEllipse();
    void drawArc(const Arc &arc);
    void closePath();
    void drawPath(TPathMode mode);
    void drawBitmap(Bitmap bitmap);
    void drawText(const Text *text);
    void drawSymbol(Attribute symbol);
    void addClipPath();

    void setStroke(Attribute color);
    void setFill(Attribute color);
    void setPen(Attribute pen);
    void setDashStyle(Attribute dash);
    void setLineCap(TLineCap cap);
    void setLineJoin(TLineJoin join);
    void setFillRule(TFillRule rule);
    void setSymStroke(Attribute color);
    void setSymFill(Attribute color);
    void setSymPen(Attribute wid);
    void setOpacity(Attribute opaq);
    void setStrokeOpacity(Attribute opaq);
    void setTiling(Attribute til);
    void setGradient(Attribute grad);

    //! Return style sheet cascade.
    inline const Cascade *cascade() const { return iCascade; }
    //! Return current stroke color.
    inline Color stroke() const { return iState.back().iStroke; }
    //! Return current fill color.
    inline Color fill() const { return iState.back().iFill; }
    //! Return current transformation matrix.
    inline const Matrix &matrix() const { return iMatrix.back(); }
    //! Return current pen.
    inline Fixed pen() const {return iState.back().iPen; }
    //! Return current dash style (always absolute attribute).
    inline String dashStyle() const {return iState.back().iDashStyle; }
    void dashStyle(std::vector<double> &dashes, double &offset) const;
    //! Return current line cap.
    inline TLineCap lineCap() const {return iState.back().iLineCap; }
    //! Return current line join.
    inline TLineJoin lineJoin() const {return iState.back().iLineJoin; }
    //! Return current fill rule.
    inline TFillRule fillRule() const {return iState.back().iFillRule; }
    //! Return current symbol stroke color.
    inline Color symStroke() const { return iState.back().iSymStroke; }
    //! Return current symbol fill color.
    inline Color symFill() const { return iState.back().iSymFill; }
    //! Return current symbol pen.
    inline Fixed symPen() const { return iState.back().iSymPen; }
    //! Return current opacity.
    inline Fixed opacity() const { return iState.back().iOpacity; }
    //! Return current stroke opacity.
    inline Fixed strokeOpacity() const { return iState.back().iStrokeOpacity; }
    //! Return current tiling.
    inline Attribute tiling() const { return iState.back().iTiling; }
    //! Return current gradient fill.
    inline Attribute gradient() const { return iState.back().iGradient; }

    //! Return full current graphics state.
    inline const State & state() const { return iState.back(); }
    void setState(const State &state);

  protected:
    virtual void doPush();
    virtual void doPop();
    virtual void doNewPath();
    virtual void doMoveTo(const Vector &v);
    virtual void doLineTo(const Vector &v);
    virtual void doCurveTo(const Vector &v1, const Vector &v2,
			   const Vector &v3);
    virtual void doDrawArc(const Arc &arc);
    virtual void doClosePath();
    virtual void doDrawPath(TPathMode mode);
    virtual void doDrawBitmap(Bitmap bitmap);
    virtual void doDrawText(const Text *text);
    virtual void doDrawSymbol(Attribute symbol);
    virtual void doAddClipPath();

    void drawArcAsBezier(double alpha);

  protected:
    std::list<State> iState;
    std::list<Matrix> iMatrix;
    const Cascade *iCascade;
    const AttributeMap *iAttributeMap;
    int iInPath;
  };

  // --------------------------------------------------------------------

  //! Overloaded function.
  /*! Assumes current position is \c bezier.iV[0] */
  inline void Painter::curveTo(const Bezier &bezier)
  {
    curveTo(bezier.iV[1], bezier.iV[2], bezier.iV[3]);
  }

} // namespace

// --------------------------------------------------------------------
#endif

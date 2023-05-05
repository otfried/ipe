// -*- C++ -*-
// --------------------------------------------------------------------
// Canvas tools
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

#ifndef TOOLS_H
#define TOOLS_H

#include "ipetool.h"
#include "ipecanvas.h"

// avoid including Lua headers here
typedef struct lua_State lua_State;

// --------------------------------------------------------------------

using namespace ipe;

extern void push_button(lua_State *L, int button);

class IpeTransformTool : public TransformTool {
public:
  IpeTransformTool(CanvasBase *canvas, Page *page, int view, TType type,
		   bool withShift, lua_State *L0, int method);
  ~IpeTransformTool();
  virtual void report();
private:
  lua_State *L;
  int iMethod;
};

class LuaTool  : public Tool {
public:
  LuaTool(CanvasBase *canvas, lua_State *L0, int luatool);
  ~LuaTool();

  void setColor(Color color) { iColor = color; }

  virtual void mouseButton(int button, bool press);
  virtual void mouseMove();
  virtual bool key(String text, int modifiers);
protected:
  lua_State *L;
  int iLuaTool;
  Color iColor;
};

class ShapeTool : public LuaTool {
public:
  enum TMarkType { EVertex = 1, ESplineCP, ECenter,
		   ERadius, EMinor, ECurrent, EScissor,
		   ENumMarkTypes };

  ShapeTool(CanvasBase *canvas, lua_State *L0, int luatool);

  void setShape(Shape shape, int which = 0, double pen=1.0);
  void setSnapping(bool snap, bool skipLast);
  void clearMarks();
  void addMark(const Vector &v, TMarkType t);

  virtual void draw(Painter &painter) const;
  virtual void snapVtx(const Vector &mouse, Vector &pos,
		       double &bound, bool cp) const;
private:
  double iPen;
  Shape iShape;
  Shape iAuxShape;
  struct SMark {
    Vector v;
    TMarkType t;
  };
  std::vector<SMark> iMarks;
  bool iSnap;
  bool iSkipLast;
};

class PasteTool : public LuaTool {
public:
  PasteTool(CanvasBase *canvas, lua_State *L0, int luatool, Object *obj);
  ~PasteTool();

  void setMatrix(Matrix m) { iMatrix = m; }

  virtual void draw(Painter &painter) const;
private:
  Object *iObject;
  Matrix iMatrix;
};

// --------------------------------------------------------------------
#endif

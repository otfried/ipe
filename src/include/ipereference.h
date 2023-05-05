// -*- C++ -*-
// --------------------------------------------------------------------
// The reference object
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

#ifndef IPEREF_H
#define IPEREF_H

#include "ipeobject.h"

// --------------------------------------------------------------------

namespace ipe {

  class Cascade;

  class Reference : public Object {
  public:
    enum { EHasStroke = 0x001, EHasFill = 0x002,
	   EHasPen = 0x004, EHasSize = 0x008,
	   EIsMark = 0x010, EIsArrow = 0x020 };

    explicit Reference(const AllAttributes &attr, Attribute name, Vector pos);

    explicit Reference(const XmlAttributes &attr, String data);

    virtual Object *clone() const;

    virtual Reference *asReference();

    virtual Type type() const;

    virtual void accept(Visitor &visitor) const;

    virtual void saveAsXml(Stream &stream, String layer) const;
    virtual void draw(Painter &painter) const;
    virtual void drawSimple(Painter &painter) const;
    virtual void addToBBox(Rect &box, const Matrix &m, bool cp) const;
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const;
    virtual void snapVtx(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;
    virtual void snapBnd(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;

    virtual void checkStyle(const Cascade *sheet, AttributeSeq &seq) const;

    void setName(Attribute name);
    //! Return symbolic name.
    inline Attribute name() const { return iName; }

    void setStroke(Attribute color);
    //! Return stroke color.
    inline Attribute stroke() const { return iStroke; }
    void setFill(Attribute color);
    //! Return fill color.
    inline Attribute fill() const { return iFill; }
    //! Return pen.
    inline Attribute pen() const { return iPen; }
    void setPen(Attribute pen);
    void setSize(Attribute size);
    //! Return symbol size.
    inline Attribute size() const { return iSize; }
    //! Return position of symbol on page.
    inline Vector position() const { return iPos; }

    virtual bool setAttribute(Property prop, Attribute value);
    virtual Attribute getAttribute(Property prop) const noexcept;

    inline uint32_t flags() const { return iFlags; }
    static uint32_t flagsFromName(String name);

  private:
    Attribute iName;
    Vector iPos;
    Attribute iSize;
    Attribute iStroke;
    Attribute iFill;
    Attribute iPen;
    uint32_t iFlags;
    mutable std::vector<Vector> iSnap;   // caching info from the symbol itself
  };

} // namespace

// --------------------------------------------------------------------
#endif

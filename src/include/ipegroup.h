// -*- C++ -*-
// --------------------------------------------------------------------
// The group object.
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

#ifndef IPEGROUP_H
#define IPEGROUP_H

#include "ipeobject.h"
#include "ipeshape.h"

// --------------------------------------------------------------------

namespace ipe {

  class Shape;

  class Group : public Object {
  private:
    typedef std::vector<Object *> List;

  public:
    typedef List::const_iterator const_iterator;

    explicit Group();
    Group(const Group &rhs);
    virtual ~Group();

    explicit Group(const XmlAttributes &attr);

    Group &operator=(const Group &rhs);
    virtual Object *clone() const;

    virtual Group *asGroup();
    virtual const Group *asGroup() const;

    virtual Type type() const;

    virtual TPinned pinned() const;

    virtual void accept(Visitor &visitor) const;

    virtual void saveAsXml(Stream &stream, String layer) const;
    virtual void draw(Painter &painter) const;
    virtual void drawSimple(Painter &painter) const;
    virtual void addToBBox(Rect &box, const Matrix &m, bool cp) const;
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const;
    virtual void snapVtx(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;
    virtual void snapCtl(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;
    virtual void snapBnd(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;

    inline const Shape &clip() const { return iClip; }
    void setClip(const Shape &clip);

    inline String url() const { return iUrl; }
    void setUrl(String url);

    //! Return number of component objects.
    inline int count() const { return iImp->iObjects.size(); }
    //! Return object at index \a i.
    inline const Object *object(int i) const { return iImp->iObjects[i]; }
    //! Return iterator for first object.
    inline const_iterator begin() const { return iImp->iObjects.begin(); }
    //! Return iterator for end of sequence.
    inline const_iterator end() const { return iImp->iObjects.end(); }

    void push_back(Object *);

    void saveComponentsAsXml(Stream &stream) const;

    virtual void checkStyle(const Cascade *sheet, AttributeSeq &seq) const;

    virtual Attribute getAttribute(Property prop) const noexcept;
    virtual bool setAttribute(Property prop, Attribute value);

  private:
    void detach();

  private:
    struct Imp {
      List iObjects;
      int iRefCount;
      TPinned iPinned; // is any of the objects in the list pinned?
    };

    Imp *iImp;
    Shape iClip;
    String iUrl;
    Attribute iDecoration;
  };

} // namespace

// --------------------------------------------------------------------
#endif

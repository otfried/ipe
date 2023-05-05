// -*- C++ -*-
// --------------------------------------------------------------------
// The image object
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

#ifndef IPEIMAGE_H
#define IPEIMAGE_H

#include "ipebitmap.h"
#include "ipeobject.h"

// --------------------------------------------------------------------

namespace ipe {

  class Image : public Object {
  public:
    explicit Image(const Rect &rect, Bitmap bitmap);
    explicit Image(const XmlAttributes &attr, String data);
    explicit Image(const XmlAttributes &attr, Bitmap bitmap);

    virtual Object *clone() const override;

    virtual Image *asImage() override;

    virtual Type type() const override;

    virtual void saveAsXml(Stream &stream, String layer) const override;
    virtual void draw(Painter &painter) const override;
    virtual void drawSimple(Painter &painter) const override;

    virtual void accept(Visitor &visitor) const override;

    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const override;
    virtual void addToBBox(Rect &box, const Matrix &m, bool) const override;
    virtual void snapCtl(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const override;

    inline Rect rect() const;
    inline Bitmap bitmap() const;

    virtual bool setAttribute(Property prop, Attribute value) override;
    virtual Attribute getAttribute(Property prop) const noexcept override;

    void setOpacity(Attribute opaq);
    //! Return opacity of the opject.
    inline Attribute opacity() const { return iOpacity; }

  private:
    void init(const XmlAttributes &attr);
  private:
    Rect iRect;
    Bitmap iBitmap;
    Attribute iOpacity;
  };

  // --------------------------------------------------------------------

  //! Return the rectangle occupied by the image on the paper.
  /*! The transformation matrix is applied to this, of course. */
  inline Rect Image::rect() const
  {
    return iRect;
  }

  //! Return Bitmap of the image.
  inline Bitmap Image::bitmap() const
  {
    return iBitmap;
  }

} // namespace

// --------------------------------------------------------------------
#endif

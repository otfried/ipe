// -*- C++ -*-
// --------------------------------------------------------------------
// The text object.
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

#ifndef IPETEXT_H
#define IPETEXT_H

#include "ipeobject.h"

// --------------------------------------------------------------------

namespace ipe {

  class Text : public Object {
  public:
    enum TextType { ELabel, EMinipage };

    explicit Text();
    explicit Text(const AllAttributes &attr, String data,
		  const Vector &pos, TextType type, double width = 10.0);
    Text(const Text &rhs);
    ~Text();

    explicit Text(const XmlAttributes &attr, String data);

    virtual Object *clone() const;

    virtual Text *asText();

    virtual Type type() const;

    virtual void saveAsXml(Stream &stream, String layer) const;
    virtual void draw(Painter &painter) const;
    virtual void drawSimple(Painter &painter) const;

    virtual void accept(Visitor &visitor) const;

    virtual void addToBBox(Rect &box, const Matrix &m, bool) const;
    virtual double distance(const Vector &v, const Matrix &m,
			    double bound) const;
    virtual void snapCtl(const Vector &mouse, const Matrix &m,
			 Vector &pos, double &bound) const;

    virtual void checkStyle(const Cascade *sheet, AttributeSeq &seq) const;

    virtual bool setAttribute(Property prop, Attribute value);
    virtual Attribute getAttribute(Property prop) const noexcept;

  private:
    void quadrilateral(const Matrix &m, Vector v[4]) const;
  public:
    Vector align() const;

    TextType textType() const;
    inline Vector position() const;
    inline String text() const;
    void setStroke(Attribute stroke);
    void setOpacity(Attribute opaq);

    inline Attribute stroke() const;
    inline Attribute size() const;
    inline Attribute style() const;
    //! Return opacity of the opject.
    inline Attribute opacity() const { return iOpacity; }

    inline bool isMinipage() const;
    void setTextType(TextType type);

    inline THorizontalAlignment horizontalAlignment() const;
    inline TVerticalAlignment verticalAlignment() const;
    void setHorizontalAlignment(THorizontalAlignment align);
    void setVerticalAlignment(TVerticalAlignment align);

    static TVerticalAlignment makeVAlign(String str,
					 TVerticalAlignment def);
    static THorizontalAlignment makeHAlign(String str,
					   THorizontalAlignment def);
    static void saveAlignment(Stream &stream, THorizontalAlignment h,
			      TVerticalAlignment v);

    inline double width() const;
    inline double height() const;
    inline double depth() const;
    inline double totalHeight() const;

    void setSize(Attribute size);
    void setStyle(Attribute style);
    void setWidth(double width);
    void setText(String text);

    struct XForm {
      int iRefCount;
      Rect iBBox;
      int iDepth;
      float iStretch;
      String iName;
      Vector iTranslation;
    };

    void setXForm(XForm *xform) const;
    inline const XForm *getXForm() const;

  private:
    Vector iPos;
    String iText;
    Attribute iStroke;
    Attribute iSize;
    Attribute iStyle;
    Attribute iOpacity;
    mutable double iWidth;
    mutable double iHeight;
    mutable double iDepth;
    TextType iType;
    THorizontalAlignment iHorizontalAlignment;
    TVerticalAlignment iVerticalAlignment;
    mutable XForm *iXForm; // reference counted
  };

  // --------------------------------------------------------------------

  //! Return text position.
  inline Vector Text::position() const
  {
    return iPos;
  }

  //! Return text source.
  inline String Text::text() const
  {
    return iText;
  }

  //! Return stroke color.
  inline Attribute Text::stroke() const
  {
    return iStroke;
  }

  //! Return font size.
  inline Attribute Text::size() const
  {
    return iSize;
  }

  //! Return Latex style of text object.
  inline Attribute Text::style() const
  {
    return iStyle;
  }

  //! Return width of text object.
  inline double Text::width() const
  {
    return iWidth;
  }

  //! Return height of text object (from baseline to top).
  inline double Text::height() const
  {
    return iHeight;
  }

  //! Return depth of text object.
  inline double Text::depth() const
  {
    return iDepth;
  }

  //! Return height + depth of text object.
  inline double Text::totalHeight() const
  {
    return iHeight + iDepth;
  }

  //! Return true if text object is formatted as a minipage.
  /*! Equivalent to type being EMinipage. */
  inline bool Text::isMinipage() const
  {
    return (iType == EMinipage);
  }

  //! Return horizontal alignment of text object.
  inline THorizontalAlignment Text::horizontalAlignment() const
  {
    return iHorizontalAlignment;
  }

  //! Return vertical alignment of text object.
  inline TVerticalAlignment Text::verticalAlignment() const
  {
    return iVerticalAlignment;
  }

  //! Return the PDF representation of this text object.
  /*! If Pdflatex has not been run yet, returns 0. */
  inline const Text::XForm *Text::getXForm() const
  {
    return iXForm;
  }

} // namespace

// --------------------------------------------------------------------
#endif

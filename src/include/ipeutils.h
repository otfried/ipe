// -*- C++ -*-
// --------------------------------------------------------------------
// Various utility classes
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

#ifndef IPEUTILS_H
#define IPEUTILS_H

#include "ipebitmap.h"
#include "ipepainter.h"

// --------------------------------------------------------------------

namespace ipe {

  class Page;

  class BitmapFinder : public Visitor {
  public:
    void scanPage(const Page *page);

    virtual void visitGroup(const Group *obj);
    virtual void visitImage(const Image *obj);
  public:
    std::vector<Bitmap> iBitmaps;
  };

  class BBoxPainter : public Painter {
  public:
    BBoxPainter(const Cascade *style);
    Rect bbox() const { return iBBox; }

  protected:
    virtual void doPush();
    virtual void doPop();
    virtual void doNewPath();
    virtual void doMoveTo(const Vector &v);
    virtual void doLineTo(const Vector &v);
    virtual void doCurveTo(const Vector &v1, const Vector &v2,
			   const Vector &v3);
    virtual void doDrawPath(TPathMode mode);
    virtual void doDrawBitmap(Bitmap bitmap);
    virtual void doDrawText(const Text *text);
    virtual void doAddClipPath();

  private:
    Rect iBBox;
    Vector iV;
    Rect iPathBox;
    std::list<Rect> iClipBox;
  };

  class A85Stream : public Stream {
  public:
    A85Stream(Stream &stream);
    virtual void putChar(char ch);
    virtual void close();
  private:
    Stream &iStream;
    uint8_t iCh[4];
    int iN;
    int iCol;
  };

  class Base64Stream : public Stream {
  public:
    Base64Stream(Stream &stream);
    virtual void putChar(char ch);
    virtual void close();
  private:
    Stream &iStream;
    uint8_t iCh[3];
    int iN;
    int iCol;
  };

  class DeflateStream : public Stream {
  public:
    DeflateStream(Stream &stream, int level);
    virtual ~DeflateStream();
    virtual void putChar(char ch);
    virtual void close();

    static Buffer deflate(const char *data, int size,
			  int &deflatedSize, int compressLevel);

  private:
    struct Private;

    Stream &iStream;
    Private *iPriv;
    int iN;
    Buffer iIn;
    Buffer iOut;
  };

  class A85Source : public DataSource {
  public:
    A85Source(DataSource &source);
    //! Get one more character, or EOF.
    virtual int getChar();
  private:
    DataSource &iSource;
    bool iEof;
    int iN;
    int iIndex;
    uint8_t iBuf[4];
  };

  class Base64Source : public DataSource {
  public:
    Base64Source(DataSource &source);
    //! Get one more character, or EOF.
    virtual int getChar();
  private:
    DataSource &iSource;
    bool  iEof;
    int   iIndex;
    int   iBufLen;
    uint8_t iBuf[3];
  };

  class InflateSource : public DataSource {
  public:
    InflateSource(DataSource &source);
    virtual ~InflateSource();
    //! Get one more character, or EOF.
    virtual int getChar();

  private:
    void fillBuffer();

  private:
    struct Private;

    DataSource &iSource;
    Private *iPriv;
    char *iP;
    Buffer iIn;
    Buffer iOut;
  };

} // namespace

// --------------------------------------------------------------------
#endif

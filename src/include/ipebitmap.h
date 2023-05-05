// -*- C++ -*-
// --------------------------------------------------------------------
// Bitmaps
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

#ifndef IPEBITMAP_H
#define IPEBITMAP_H

#include "ipebase.h"
#include "ipegeo.h"
#include "ipexml.h"

// --------------------------------------------------------------------

namespace ipe {

  class Bitmap {
  public:
    enum Flags {
      ERGB = 0x01,      // not grayscale
      EAlpha = 0x02,    // has alpha channel
      EDCT = 0x04,      // DCT encoded jpg image
      EInflate = 0x08,  // data needs to be inflated
      ENative = 0x10,   // data is already in native-endian ARGB32
    };

    Bitmap();
    Bitmap(int width, int height, uint32_t flags, Buffer data);
    Bitmap(const XmlAttributes &attr, String data);
    Bitmap(const XmlAttributes &attr, Buffer data, Buffer smask);

    Bitmap(const Bitmap &rhs);
    ~Bitmap();
    Bitmap &operator=(const Bitmap &rhs);

    void saveAsXml(Stream &stream, int id, int pdfObjNum = -1) const;

    inline bool isNull() const;
    bool equal(Bitmap rhs) const;

    inline int width() const;
    inline int height() const;

    inline bool isJpeg() const;
    inline bool isGray() const;
    inline bool hasAlpha() const;
    inline int colorKey() const;

    Buffer pixelData();

    inline int objNum() const;
    inline void setObjNum(int objNum) const;

    std::pair<Buffer, Buffer> embed() const;

    inline bool operator==(const Bitmap &rhs) const;
    inline bool operator!=(const Bitmap &rhs) const;
    inline bool operator<(const Bitmap &rhs) const;

    static const char *readJpegInfo(FILE *file, int &width, int &height,
				    Vector &dotsPerInch, uint32_t &flags);
    static Bitmap readJpeg(const char *fname, Vector &dotsPerInch, const char * &errmsg);
    static Bitmap readPNG(const char *fname, Vector &dotsPerInch, const char * &errmsg);

    void savePixels(const char *fname);

  private:
    std::pair<int, int> init(const XmlAttributes &attr);
    void computeChecksum();
    void unpack(Buffer alphaChannel);
    void analyze();

  private:
    struct Imp {
      int iRefCount;
      uint32_t iFlags;
      int iWidth;
      int iHeight;
      int iColorKey;
      Buffer iData;               // native-endian ARGB32 or DCT encoded
      Buffer iPixelData;          // native-endian ARGB32 pre-multiplied for Cairo
      bool iPixelsComputed;
      int iChecksum;
      mutable int iObjNum;        // Object number (e.g. in PDF file)
    };

    Imp *iImp;
  };

  // --------------------------------------------------------------------

  //! Is this a null bitmap?
  inline bool Bitmap::isNull() const
  {
    return (iImp == nullptr);
  }

  //! Return width of pixel array.
  inline int Bitmap::width() const
  {
    return iImp->iWidth;
  }

  //! Return height of pixel array.
  inline int Bitmap::height() const
  {
    return iImp->iHeight;
  }

  //! Is this bitmap a JPEG photo?
  inline bool Bitmap::isJpeg() const
  {
    return (iImp->iFlags & EDCT) != 0;
  }

  //! Is the bitmap grayscale?
  inline bool Bitmap::isGray() const
  {
    return (iImp->iFlags & ERGB) == 0;
  }

  //! Does the bitmap have transparency?
  /*! Bitmaps with color key will return false here. */
  inline bool Bitmap::hasAlpha() const
  {
    return (iImp->iFlags & EAlpha) != 0;
  }

  //! Return the color key or -1 if none.
  inline int Bitmap::colorKey() const
  {
    return iImp->iColorKey;
  }

  //! Return object number of the bitmap.
  inline int Bitmap::objNum() const
  {
    return iImp->iObjNum;
  }

  //! Set object number of the bitmap.
  inline void Bitmap::setObjNum(int objNum) const
  {
    iImp->iObjNum = objNum;
  }

  //! Two bitmaps are equal if they share the same data.
  inline bool Bitmap::operator==(const Bitmap &rhs) const
  {
    return iImp == rhs.iImp;
  }

  //! Two bitmaps are equal if they share the same data.
  inline bool Bitmap::operator!=(const Bitmap &rhs) const
  {
    return iImp != rhs.iImp;
  }

  //! Less operator, to be able to sort bitmaps.
  /*! The checksum is used, when it is equal, the shared address.
    This guarantees that bitmaps that are == (share their implementation)
    are next to each other, and blocks of them are next to blocks that
    are identical in contents. */
  inline bool Bitmap::operator<(const Bitmap &rhs) const
  {
    return (iImp->iChecksum < rhs.iImp->iChecksum ||
	    (iImp->iChecksum == rhs.iImp->iChecksum && iImp < rhs.iImp));
  }

} // namespace

// --------------------------------------------------------------------
#endif

// -*- C++ -*-
// --------------------------------------------------------------------
// Creating Postscript output
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

#ifndef IPEPSWRITER_H
#define IPEPSWRITER_H

#include "ipebase.h"
#include "ipepage.h"
#include "ipedoc.h"
#include "ipeimage.h"
#include "ipepdfwriter.h"

// --------------------------------------------------------------------

namespace ipe {

  struct Font;

  class PsPainter : public PdfPainter {
  public:
    PsPainter(const Cascade *style, Stream &stream);
    ~PsPainter();

  protected:
    virtual void doNewPath();
    virtual void doDrawPath(TPathMode mode);
    virtual void doDrawBitmap(Bitmap bitmap);
    virtual void doAddClipPath();

  private:
    void strokePath();
    void fillPath(bool eoFill, bool preservePath);

  private:
    int iImageNumber;
  };

  // --------------------------------------------------------------------

  class PsWriter {
  public:
    PsWriter(TellStream &stream, const Document *doc, bool noColor);
    ~PsWriter();
    bool createHeader(int pno = 0, int vno = 0);
    void createPageView(int pno = 0, int vno = 0);
    void createXml(int compressLevel);
    void createTrailer();

  private:
    void embedFont(const Font &font);

private:
    TellStream &iStream;
    const Document *iDoc;
    bool iNoColor;
  };

} // namespace

// --------------------------------------------------------------------
#endif

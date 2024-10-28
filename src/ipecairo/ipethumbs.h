// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Thumbnail
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2024 Otfried Cheong

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

#ifndef IPETHUMBS_H
#define IPETHUMBS_H

#include "ipedoc.h"
#include "ipefonts.h"

// --------------------------------------------------------------------

namespace ipe {

class Thumbnail {
public:
    enum TargetFormat { ESVG, EPNG, EPS, EPDF };

    Thumbnail(const Document * doc, int width);

    void setTransparent(bool t) { iTransparent = t; }
    void setNoCrop(bool n) { iNoCrop = n; }

    int width() const { return iWidth; }
    int height() const { return iHeight; }
    Buffer render(const Page * page, int view);
    bool saveRender(TargetFormat fm, const char * dst, const Page * page, int view,
		    double zoom, double tolerance = 0.1);

private:
    const Document * iDoc;
    bool iTransparent;
    bool iNoCrop;
    int iWidth;
    int iHeight;
    double iZoom;
    const Layout * iLayout;
    std::unique_ptr<Fonts> iFonts;
};

class PdfThumbnail {
public:
    PdfThumbnail(const PdfFile * pdf, int width);
    int width() const { return iWidth; }
    int height() const { return iHeight; }
    Buffer render(const PdfDict * page);

private:
    const PdfFile * iPdf;
    int iWidth;
    int iHeight;
    std::unique_ptr<Cascade> iCascade; // dummy stylesheet
    std::unique_ptr<PdfFileResources> iResources;
    std::unique_ptr<Fonts> iFonts;
};

} // namespace ipe

// --------------------------------------------------------------------
#endif

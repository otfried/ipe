// -*- C++ -*-
// --------------------------------------------------------------------
// ipe::Reveal
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (c) 1993-2026 Otfried Cheong

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

#ifndef IPEREVEAL_H
#define IPEREVEAL_H

#include "ipedoc.h"
#include "ipefonts.h"

// --------------------------------------------------------------------

namespace ipe {

class Reveal {
public:
    enum TargetFormat { ESVG, EPNG, EPS, EPDF };

    Reveal(const Document * doc, int fromPage, int toPage);

    int width() const { return iWidth; }
    int height() const { return iHeight; }

    bool createPresentation(const char * destination);

private:
    String createSVG();
    bool writeSlides(Stream & out, String svg);

private:
    const Document * iDoc;
    int iFromPage;
    int iToPage;
    int iWidth;
    int iHeight;
    const Layout * iLayout;
    std::unique_ptr<Fonts> iFonts;
};

} // namespace ipe

// --------------------------------------------------------------------
#endif

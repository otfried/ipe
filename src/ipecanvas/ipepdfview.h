// -*- C++ -*-
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

#ifndef IPEPDFVIEW_H
#define IPEPDFVIEW_H

#include "ipelib.h"
#include "ipepdfparser.h"

// --------------------------------------------------------------------

// Avoid including cairo.h
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;

// --------------------------------------------------------------------

namespace ipe {

class Fonts;

// --------------------------------------------------------------------

class PdfViewBase {

public:
    virtual ~PdfViewBase();

    void setPdf(const PdfFile * pdf, Fonts * fonts);
    void setPage(const PdfDict * page, const Rect & paper);
    void setBackground(const Color & bg);
    void setBlackout(bool bo);

    //! Return current pan.
    inline Vector pan() const { return iPan; }
    //! Return current zoom.
    inline double zoom() const { return iZoom; }
    //! Return center of view.
    inline Vector center() const { return 0.5 * Vector(iWidth, iHeight); }
    //! Return width of view.
    int viewWidth() const { return iWidth; }
    //! Return height of view.
    int viewHeight() const { return iHeight; }
    //! Return current blackout state.
    bool blackout() const { return iBlackout; }

    Vector devToUser(const Vector & arg) const;
    Vector userToDev(const Vector & arg) const;

    void setPan(const Vector & v);
    void setZoom(double zoom);

    Matrix canvasTfm() const;

    void updatePdf();
    virtual void invalidate(int x, int y, int w, int h) = 0;
    virtual void invalidate() = 0;

protected:
    PdfViewBase();
    void drawPaper(cairo_t * cc);
    void refreshSurface();

protected:
    double iWidth, iHeight;
    double iBWidth, iBHeight; // size of backing store
    Vector iPan;
    double iZoom;
    Color iBackground;
    bool iBlackout;

    bool iRepaint;
    cairo_surface_t * iSurface;

    std::unique_ptr<Cascade> iCascade; // dummy stylesheet

    const PdfDict * iPage;
    Rect iPaperBox;
    const PdfDict * iStream;
    const PdfFile * iPdf;
    Fonts * iFonts;
};

} // namespace ipe

// --------------------------------------------------------------------
#endif

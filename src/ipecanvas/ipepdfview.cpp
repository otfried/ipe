// --------------------------------------------------------------------
// ipe::PdfViewBase
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

#include "ipepdfview.h"

#include "ipecairopainter.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::PdfViewBase
  \ingroup canvas
  \brief A widget (control) that displays a PDF document.
*/

//! Construct a new canvas.
PdfViewBase::PdfViewBase()
{
  iSurface = nullptr;
  iPdf = nullptr;
  iPage = nullptr;
  iStream = nullptr;
  iFonts = nullptr;

  iPan = Vector::ZERO;
  iZoom = 1.0;
  iWidth = 0;   // not yet known (view is not yet mapped)
  iHeight = 0;
  iBWidth = 0;
  iBHeight = 0;
  iBackground = Color(500, 500, 500);
  iBlackout = false;

  iRepaint = false;
  iCascade = std::make_unique<Cascade>();
  iCascade->insert(0, StyleSheet::standard());
}

//! destructor.
PdfViewBase::~PdfViewBase()
{
  if (iSurface) {
    ipeDebug("Surface has %d references left", cairo_surface_get_reference_count(iSurface));
    cairo_surface_finish(iSurface);
    cairo_surface_destroy(iSurface);
  }
  ipeDebug("PdfViewBase::~PdfViewBase");
}

// --------------------------------------------------------------------

//! Provide the PDF document.
void PdfViewBase::setPdf(const PdfFile *pdf, Fonts *fonts)
{
  iPage = nullptr;
  iStream = nullptr;
  iPdf = pdf;
  iFonts = fonts;
}

//! Provide the page to view.
void PdfViewBase::setPage(const PdfDict *page, const Rect &paper)
{
  iPage = page;
  iPaperBox = paper;
  const PdfObj *stream = iPage->get("Contents", iPdf);
  iStream = stream ? stream->dict() : nullptr;
}

//! Set background color
void PdfViewBase::setBackground(const Color &bg)
{
  iBackground = bg;
}

//! Set blackout
void PdfViewBase::setBlackout(bool bo)
{
  iBlackout = bo;
}

// --------------------------------------------------------------------

//! Set current pan position.
/*! The pan position is the user coordinate that is displayed at
  the very center of the canvas. */
void PdfViewBase::setPan(const Vector &v)
{
  iPan = v;
}

//! Set current zoom factor.
/*! The zoom factor maps user coordinates to screen pixel coordinates. */
void PdfViewBase::setZoom(double zoom)
{
  iZoom = zoom;
}

//! Convert canvas (device) coordinates to user coordinates.
Vector PdfViewBase::devToUser(const Vector &arg) const
{
  Vector v = arg - center();
  v.x /= iZoom;
  v.y /= -iZoom;
  v += iPan;
  return v;
}

//! Convert user coordinates to canvas (device) coordinates.
Vector PdfViewBase::userToDev(const Vector &arg) const
{
  Vector v = arg - iPan;
  v.x *= iZoom;
  v.y *= -iZoom;
  v += center();
  return v;
}

//! Matrix mapping user coordinates to canvas coordinates
Matrix PdfViewBase::canvasTfm() const
{
  return Matrix(center()) * Linear(iZoom, 0, 0, -iZoom) * Matrix(-iPan);
}

// --------------------------------------------------------------------

void PdfViewBase::drawPaper(cairo_t *cc)
{
  if (!iPaperBox.isEmpty()) {
    cairo_rectangle(cc, iPaperBox.left(), iPaperBox.bottom(),
		    iPaperBox.width(), iPaperBox.height());
    cairo_set_source_rgb(cc, 1.0, 1.0, 1.0);
    cairo_fill(cc);
  }
}

// --------------------------------------------------------------------

//! Mark for update with redrawing of PDF document.
void PdfViewBase::updatePdf()
{
  iRepaint = true;
  invalidate();
}

// --------------------------------------------------------------------

void PdfViewBase::refreshSurface()
{
  if (!iSurface
      || iBWidth != cairo_image_surface_get_width(iSurface)
      || iBHeight != cairo_image_surface_get_height(iSurface)) {
    // size has changed
    // ipeDebug("size has changed to %g x %g (%g x %g)",
    //          iWidth, iHeight, iBWidth, iBHeight);
    if (iSurface)
      cairo_surface_destroy(iSurface);
    iSurface = nullptr;
    iRepaint = true;
  }
  if (iRepaint) {
    iRepaint = false;
    if (!iSurface)
      iSurface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
					    iBWidth, iBHeight);
    cairo_t *cc = cairo_create(iSurface);
    // background
    cairo_set_source_rgb(cc, iBackground.iRed.toDouble(),
			 iBackground.iGreen.toDouble(), iBackground.iBlue.toDouble());
    cairo_rectangle(cc, 0, 0, iBWidth, iBHeight);
    cairo_fill(cc);

    if (!iBlackout) {
      cairo_translate(cc, 0.5 * iBWidth, 0.5 * iBHeight);
      cairo_scale(cc, iBWidth / iWidth, iBHeight / iHeight);
      cairo_scale(cc, iZoom, -iZoom);
      cairo_translate(cc, -iPan.x, -iPan.y);

      drawPaper(cc);
      if (iStream) {
	CairoPainter painter(iCascade.get(), iFonts, cc, iZoom, false, false);
	painter.executeStream(iStream, iPage);
      }
    }
    cairo_surface_flush(iSurface);
    cairo_destroy(cc);
  }
}

// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Making thumbnails of Ipe pages and PDF pages
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

#include "ipethumbs.h"

#include "ipecairopainter.h"
#include <cairo.h>

#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif

#include <cstring>

using namespace ipe;

// --------------------------------------------------------------------

Thumbnail::Thumbnail(const Document *doc, int width)
  : iDoc{doc}, iTransparent{false}, iNoCrop{false}, iWidth{width}
{
  iLayout = iDoc->cascade()->findLayout();
  Rect paper = iLayout->paper();
  iHeight = int(iWidth * paper.height() / paper.width());
  iZoom = iWidth / paper.width();
  iFonts = std::make_unique<Fonts>(doc->resources());
}

Buffer Thumbnail::render(const Page *page, int view)
{
  Buffer buffer(iWidth * iHeight * 4);
  memset(buffer.data(), (iTransparent ? 0x00 : 0xff), iWidth * iHeight * 4);

  cairo_surface_t* surface =
    cairo_image_surface_create_for_data((uint8_t *) buffer.data(),
					CAIRO_FORMAT_ARGB32,
					iWidth, iHeight, iWidth * 4);
  cairo_t *cc = cairo_create(surface);
  cairo_scale(cc, iZoom, -iZoom);
  Vector offset = - iLayout->paper().topLeft();
  cairo_translate(cc, offset.x, offset.y);

  CairoPainter painter(iDoc->cascade(), iFonts.get(), cc, iZoom, true, false);
  const auto viewMap = page->viewMap(view, iDoc->cascade());
  painter.setAttributeMap(&viewMap);
  std::vector<Matrix> layerMatrices = page->layerMatrices(view);
  painter.pushMatrix();
  for (int i = 0; i < page->count(); ++i) {
    if (page->objectVisible(view, i)) {
      painter.pushMatrix();
      painter.transform(layerMatrices[page->layerOf(i)]);
      page->object(i)->draw(painter);
      painter.popMatrix();
    }
  }
  painter.popMatrix();
  cairo_surface_flush(surface);
  cairo_show_page(cc);
  cairo_destroy(cc);
  cairo_surface_destroy(surface);

  return buffer;
}

static cairo_status_t stream_writer(void *closure, const unsigned char *data, unsigned int length)
{
  if (::fwrite(data, 1, length, (std::FILE *) closure) != length)
    return CAIRO_STATUS_WRITE_ERROR;
  return CAIRO_STATUS_SUCCESS;
}

// zoom and transparent are ignored for formats other than EPNG
bool Thumbnail::saveRender(TargetFormat fm, const char *dst,
			   const Page *page, int view, double zoom,
			   double tolerance)
{
  if (fm != EPNG)
    zoom = 1.0;

  int wid, ht;
  Vector offset;
  if (iNoCrop) {
    offset = iLayout->paper().topLeft();
    wid = int(iLayout->paper().width() * zoom);
    ht = int(iLayout->paper().height() * zoom);
  } else {
    Rect bbox = page->pageBBox(iDoc->cascade());
    if (fm != EPNG) {
      // make sure integer coordinates remain integer
      bbox.addPoint(Vector{floor(bbox.left()), ceil(bbox.top())});
    }
    offset = bbox.topLeft();
    wid = int(bbox.width() * zoom + 1);
    ht = int(bbox.height() * zoom + 1);
  }

  Buffer data;
  cairo_surface_t* surface = nullptr;
  std::FILE *file = Platform::fopen(dst, "wb");
  if (!file)
    return false;

  if (fm == EPNG) {
    if (wid * ht > 20000000)
      return false;
    data = Buffer(wid * ht * 4);
    memset(data.data(), (iTransparent ? 0x00 : 0xff), wid * ht * 4);
    surface = cairo_image_surface_create_for_data((uint8_t *) data.data(),
						  CAIRO_FORMAT_ARGB32,
						  wid, ht, wid * 4);
#ifdef CAIRO_HAS_SVG_SURFACE
  } else if (fm == ESVG) {
    surface = cairo_svg_surface_create_for_stream(&stream_writer, (void *) file, wid, ht);
#endif
#ifdef CAIRO_HAS_PS_SURFACE
  } else if (fm == EPS) {
    surface = cairo_ps_surface_create_for_stream(&stream_writer, (void *) file, wid, ht);
    cairo_ps_surface_set_eps(surface, true);
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  } else if (fm == EPDF) {
    surface = cairo_pdf_surface_create_for_stream(&stream_writer, (void *) file, wid, ht);
#endif
  }

  cairo_t *cc = cairo_create(surface);
  cairo_scale(cc, zoom, -zoom);
  cairo_translate(cc, -offset.x, -offset.y);

  cairo_set_tolerance(cc, tolerance);
  CairoPainter painter(iDoc->cascade(), iFonts.get(), cc, zoom, true, true);
  const auto viewMap = page->viewMap(view, iDoc->cascade());
  painter.setAttributeMap(&viewMap);
  std::vector<Matrix> layerMatrices = page->layerMatrices(view);
  painter.pushMatrix();

  if (iNoCrop) {
    Attribute bg = page->backgroundSymbol(iDoc->cascade());
    const Symbol *background = iDoc->cascade()->findSymbol(bg);
    if (background && page->findLayer("BACKGROUND") < 0)
      painter.drawSymbol(bg);

    const Text *title = page->titleText();
    if (title)
      title->draw(painter);
  }

  for (int i = 0; i < page->count(); ++i) {
    if (page->objectVisible(view, i)) {
      painter.pushMatrix();
      painter.transform(layerMatrices[page->layerOf(i)]);
      page->object(i)->draw(painter);
      painter.popMatrix();
    }
  }

  painter.popMatrix();
  cairo_surface_flush(surface);
  cairo_show_page(cc);

  if (fm == EPNG)
    cairo_surface_write_to_png_stream(surface, &stream_writer, (void *) file);

  cairo_destroy(cc);
  cairo_surface_destroy(surface);

  ::fclose(file);
  return true;
}

// --------------------------------------------------------------------

PdfThumbnail::PdfThumbnail(const PdfFile *pdf, int width)
{
  iPdf = pdf;
  iCascade = std::make_unique<Cascade>();
  iCascade->insert(0, StyleSheet::standard());

  iResources = std::make_unique<PdfFileResources>(iPdf);
  iFonts = std::make_unique<Fonts>(iResources.get());

  iWidth = width;
  iHeight = 0;
  for (int i = 0; i < iPdf->countPages(); ++i) {
    Rect paper = iPdf->mediaBox(iPdf->page(i));
    iHeight = std::max(iHeight, (int) (paper.height() * iWidth / paper.width()));
  }
}

Buffer PdfThumbnail::render(const PdfDict *page)
{
  Rect paper = iPdf->mediaBox(page);
  double zoom = iWidth / paper.width();

  const PdfObj *stream0 = page->get("Contents", iPdf);
  const PdfDict *stream = stream0 ? stream0->dict() : nullptr;

  Buffer buffer(iWidth * iHeight * 4);
  cairo_surface_t* surface =
    cairo_image_surface_create_for_data((uint8_t *) buffer.data(), CAIRO_FORMAT_ARGB32,
					iWidth, iHeight, iWidth * 4);
  cairo_t *cc = cairo_create(surface);
  cairo_set_source_rgb(cc, 1.0, 1.0, 1.0);
  cairo_paint(cc);

  cairo_translate(cc, 0.0, iHeight);
  cairo_scale(cc, zoom, -zoom);

  if (stream) {
    CairoPainter painter(iCascade.get(), iFonts.get(), cc, 1.0, false, false);
    painter.executeStream(stream, page);
  }
  cairo_surface_flush(surface);
  cairo_destroy(cc);
  cairo_surface_destroy(surface);
  return buffer;
}

// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Special widgets for JS
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

#include "controls_js.h"

#include "ipethumbs.h"

#include "ipecairopainter.h"

#include <emscripten.h>

using namespace emscripten;

// --------------------------------------------------------------------

PathView::PathView()
{
  iCascade = nullptr;
}

void PathView::setColor(const Color & color)
{
  iColor = color;
}

void PathView::set(const AllAttributes &all, Cascade *sheet)
{
  iCascade = sheet;
  iAll = all;
}

void PathView::paint(val canvas)
{
  int w = int(canvas["width"].as<double>());
  int h = int(canvas["height"].as<double>());

  cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
  cairo_t *cc = cairo_create(sf);
  cairo_set_source_rgb(cc, iColor.iRed.toDouble(),
		       iColor.iGreen.toDouble(),
		       iColor.iBlue.toDouble());
  cairo_rectangle(cc, 0, 0, w, h);
  cairo_fill(cc);

  if (iCascade) {
    cairo_translate(cc, 0, h);
    double zoom = w / 70.0;
    cairo_scale(cc, zoom, -zoom);
    Vector v0 = (1.0/zoom) * Vector(0.1 * w, 0.5 * h);
    Vector v1 = (1.0/zoom) * Vector(0.7 * w, 0.5 * h);
    Vector u1 = (1.0/zoom) * Vector(0.88 * w, 0.8 * h);
    Vector u2 = (1.0/zoom) * Vector(0.80 * w, 0.5 * h);
    Vector u3 = (1.0/zoom) * Vector(0.88 * w, 0.2 * h);
    Vector u4 = (1.0/zoom) * Vector(0.96 * w, 0.5 * h);
    Vector mid = 0.5 * (v0 + v1);
    Vector vf = iAll.iFArrowShape.isMidArrow() ? mid : v1;
    Vector vr = iAll.iRArrowShape.isMidArrow() ? mid : v0;

    CairoPainter painter(iCascade, nullptr, cc, 3.0, false, false);
    painter.setPen(iAll.iPen);
    painter.setDashStyle(iAll.iDashStyle);
    painter.setStroke(iAll.iStroke);
    painter.setFill(iAll.iFill);
    painter.pushMatrix();
    painter.newPath();
    painter.moveTo(v0);
    painter.lineTo(v1);
    painter.drawPath(EStrokedOnly);
    if (iAll.iFArrow)
      Path::drawArrow(painter, vf, Angle(0),
		      iAll.iFArrowShape, iAll.iFArrowSize, 100.0);
    if (iAll.iRArrow)
      Path::drawArrow(painter, vr, Angle(IpePi),
		      iAll.iRArrowShape, iAll.iRArrowSize, 100.0);
    painter.setDashStyle(Attribute::NORMAL());
    painter.setTiling(iAll.iTiling);
    painter.newPath();
    painter.moveTo(u1);
    painter.lineTo(u2);
    painter.lineTo(u3);
    painter.lineTo(u4);
    painter.closePath();
    painter.drawPath(iAll.iPathMode);
    painter.popMatrix();
  }

  cairo_surface_flush(sf);
  cairo_destroy(cc);

  val options = val::object();
  options.set("alpha", false);
  emscripten::val ctx = canvas.call<val>("getContext", val("2d"), options);

  uint8_t * p = cairo_image_surface_get_data(sf);
  const uint32_t * source = (uint32_t *) p;
  int nPixels = cairo_image_surface_get_width(sf) * cairo_image_surface_get_height(sf);
  const uint32_t * fin = source + nPixels;
  while (source != fin) {
    uint32_t bits = *source++;
    p[0] = (bits & 0x00ff0000) >> 16;
    p[1] = (bits & 0x0000ff00) >> 8;
    p[2] = (bits & 0x000000ff);
    p[3] = 0xff;
    p += 4;
  }
  val buffer1 = val(typed_memory_view(nPixels * 4, cairo_image_surface_get_data(sf)));
  val::module_property("ipeBlitSurface")(ctx, buffer1, w, h);
  cairo_surface_destroy(sf);
}

// --------------------------------------------------------------------

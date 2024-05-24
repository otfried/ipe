// --------------------------------------------------------------------
// Painter using Cairo library
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

#include "ipetext.h"
#include "ipepdfparser.h"
#include "ipecairopainter.h"
#include "ipefonts.h"

// for std::memset
#include <cstring>

using namespace ipe;

// in ipebitmap:
extern bool dctDecode(Buffer dctData, Buffer pixelData);

// --------------------------------------------------------------------

/*! \defgroup cairo Ipe Cairo interface
  \brief Drawing Ipe objects using the Cairo library.

  This module contains the classes needed to render Ipe objects using
  the Cairo and Freetype libraries.

  These classes are not in Ipelib, but in a separate library
  libipecairo.
*/

// --------------------------------------------------------------------

static void cairoMatrix(cairo_matrix_t &cm, const Matrix &m)
{
  cm.xx = m.a[0];
  cm.yx = m.a[1];
  cm.xy = m.a[2];
  cm.yy = m.a[3];
  cm.x0 = m.a[4];
  cm.y0 = m.a[5];
}

static void cairoTransform(cairo_t *cr, const Matrix &m)
{
  cairo_matrix_t cm;
  cairoMatrix(cm, m);
  cairo_transform(cr, &cm);
}

// --------------------------------------------------------------------

// not checking /Domain and /N
static bool getFunctionType2(const PdfObj *obj, std::vector<double> &fun)
{
  if (!obj || !obj->dict())
    return false;
  for (int i = 0; i < 2; ++i) {
    std::vector<double> c;
    if (!obj->dict()->getNumberArray(i ? "C1" : "C0", nullptr, c) || c.size() != 3)
      return false;
    for (int j = 0; j < 3; ++j)
      fun.push_back(c[j]);
  }
  return true;
}

static void drawShading(cairo_t *cr, const PdfDict *d, const PdfResourceBase *r)
{
  // ipeDebug("drawShading: %s", d->dictRepr().z());
  int type = d->getInteger("ShadingType");
  if (type < 2 || type > 3)
    return;
  bool axial = (type == 3);
  std::vector<double> coords;
  if (!d->getNumberArray("Coords", nullptr, coords) ||
      coords.size() != (axial ? 6 : 4))
    return;

  bool extend[2] = { false, false };
  const PdfObj *extObj = r->getDeep(d, "Extend");
  if (extObj && extObj->array() && extObj->array()->count() == 2) {
    for (int i = 0; i < 2; ++i) {
      const PdfObj *el = extObj->array()->obj(i, nullptr);
      if (el && el->boolean())
	extend[i] = el->boolean()->value();
    }
  }

  const PdfDict *f = r->getDict(d, "Function");
  if (!f)
    return;
  std::vector<double> funs;
  std::vector<double> domain;
  std::vector<double> bounds;
  int ftype = f->getInteger("FunctionType");
  if (ftype == 2) {
    if (!getFunctionType2(f, funs))
      return;
  } else if (ftype == 3) {
    if (!f->getNumberArray("Domain", nullptr, domain) || domain.size() != 2) {
      domain.clear();
      domain.push_back(0.0);
      domain.push_back(1.0);
    }
    if (!f->getNumberArray("Bounds", nullptr, bounds))
      return;
    const PdfObj *a = r->getDeep(f, "Functions");
    if (!a || !a->array() || a->array()->count() != size(bounds) + 1)
      return;
    for (int i = 0; i < a->array()->count(); ++i) {
      const PdfObj *af = a->array()->obj(i, nullptr);
      if (af && af->ref())
	af = r->object(af->ref()->value());
      if (!getFunctionType2(af, funs))
	return;
    }
  } else
    return; // cannot handle

  cairo_pattern_t *p = axial ?
    cairo_pattern_create_radial(coords[0], coords[1], coords[2],
				coords[3], coords[4], coords[5]) :
    cairo_pattern_create_linear(coords[0], coords[1], coords[2], coords[3]);

  if (extend[0] && extend[1])
    // Cairo cannot control this individually, would have
    // to simulate using transparency or something
    cairo_pattern_set_extend(p, CAIRO_EXTEND_PAD);
  else
    cairo_pattern_set_extend(p, CAIRO_EXTEND_NONE);

  int fi = 0;
  cairo_pattern_add_color_stop_rgb(p, 0.0, funs[fi+0], funs[fi+1], funs[fi+2]);
  for (int i = 0; i < size(bounds); ++i) {
    fi += 6;
    double x = (bounds[i] - domain[0]) / (domain[1] - domain[0]);
    cairo_pattern_add_color_stop_rgb(p, x, funs[fi+0], funs[fi+1], funs[fi+2]);
  }
  cairo_pattern_add_color_stop_rgb(p, 1.0, funs[fi+3], funs[fi+4], funs[fi+5]);


  cairo_set_source(cr, p);
  cairo_paint(cr);
  cairo_pattern_destroy(p);
}

// /DecodeParms << /Colors 3 /Predictor 10 /Columns 466 >>
// https://en.wikipedia.org/wiki/Portable_Network_Graphics#Filtering
// http://www.libpng.org/pub/png/spec/1.2/PNG-Filters.html

static bool applyPngPrediction(Buffer &data, int width, int height, int components)
{
  int stride = width * components + 1;
  if (data.size() != height * stride)  // doesn't seem to be the right prediction
    return false;
  for (int row = 0; row < height; ++row) {
    uint8_t *p = (uint8_t *) data.data() + row * stride;
    const uint8_t *secondPixel = p + 1 + components;
    const uint8_t *fin = p + stride;
    int predictor = *p++;
    while (p < secondPixel) {
      switch (predictor) {
      case 2: // up
      case 4: // Paeth filter
	*p += p[-stride];
	break;
      case 3: // average
	*p += p[-stride] >> 1;
	break;
      default:
	break;  // do nothing
      }
      ++p;
    }
    int left, up, upLeft, pre, pa, pb, pc;
    while (p < fin) {
      switch (predictor) {
      case 1: // left
	*p += p[-components];
	break;
      case 2: // up
	*p += p[-stride];
	break;
      case 3: // average
	*p += (p[-components] + p[-stride]) >> 1;
	break;
      case 4: // Paeth filter
	left = p[-components];
	up = p[-stride];
	upLeft = p[-components-stride];
	pre = left + up - upLeft;
	if ((pa = pre - left) < 0)
	  pa = -pa;
	if ((pb = pre - up) < 0)
	  pb = -pb;
	if ((pc = pre - upLeft) < 0)
	  pc = -pc;
	if (pa <= pb && pa <= pc)
	  *p += left;
	else if (pb <= pc)
	  *p += up;
	else
	  *p += upLeft;
	break;
      case 0:
      default:
	break;  // do nothing
      }
      ++p;
    }
  }
  return true;
}

static void drawImage(cairo_t *cr, const PdfDict *d, const PdfResourceBase *r, double opacity, bool filterBest)
{
  // ipeDebug("Image: %s", d->dictRepr().z());
  int width = d->getInteger("Width");
  int height = d->getInteger("Height");
  int bpc = d->getInteger("BitsPerComponent");
  const PdfObj *cs = d->get("ColorSpace");
  if (width < 0 || height < 0 || bpc != 8 || !cs || !cs->name() ||
      (cs->name()->value() != "DeviceRGB" && cs->name()->value() != "DeviceGray")) {
    ipeDebug("Unsupported image: %s", d->dictRepr().z());
    return;
  }
  int components = 3;
  if (cs->name()->value() == "DeviceGray")
    components = 1;
  bool jpg = false;
  bool pngPrediction = false;
  const PdfObj *filter = d->get("Filter");
  if (filter && filter->name()) {
    String fn = filter->name()->value();
    if (fn == "DCTDecode")
      jpg = true;
    else if (fn == "FlateDecode") {
      const PdfObj *decodeParms = d->get("DecodeParms");
      if (decodeParms && decodeParms->dict() &&
	  decodeParms->dict()->getInteger("Predictor") >= 10)
	pngPrediction = true;
    } else {
      ipeDebug("Unsupported filter in image: %s", d->dictRepr().z());
      return;
    }
  }
  Buffer alphaChannel;
  uint8_t *alpha = nullptr;
  uint32_t colorKey = 0;
  const PdfDict *smask = r->getDict(d, "SMask");
  if (smask) {
    // ipeDebug("Mask: %s", smask->dictRepr().z());
    const PdfObj *mcs = smask->get("ColorSpace");
    if (!mcs || !mcs->name() || mcs->name()->value() != "DeviceGray"
	|| smask->getInteger("BitsPerComponent") != 8) {
      ipeDebug("Unsupported /SMask: %s", smask->dictRepr().z());
    } else {
      alphaChannel = smask->inflate();
      if (alphaChannel.size() == width * height)
	alpha = (uint8_t *) alphaChannel.data();
    }
  } else {
    std::vector<double> ckv;
    if (d->getNumberArray("Mask", nullptr, ckv) &&
	size(ckv) == 2 * components) {
      if (components == 3) {
	uint8_t r = uint8_t(ckv[0]);
	uint8_t g = uint8_t(ckv[2]);
	uint8_t b = uint8_t(ckv[4]);
	colorKey = 0xff000000 | (r << 16) | (g << 8) | b;
      } else {
	uint8_t g = uint8_t(ckv[0]);
	colorKey = 0xff000000 | (g << 16) | (g << 8) | g;
      }
    }
  }
  Buffer stream = d->inflate();
  Buffer pixels(4 * width * height);
  if (jpg) {
    if (!dctDecode(stream, pixels))
      return;
  } else {
    const char *p = stream.data();

    pngPrediction = pngPrediction &&
      applyPngPrediction(stream, width, height, components);

    uint32_t *q = (uint32_t *) pixels.data();
    if (components == 3) {
      for (int y = 0; y < height; ++y) {
	if (pngPrediction)
	  ++p; // skip predictor byte
	for (int x = 0; x < width; ++x) {
	  uint8_t a = alpha ? *alpha++ : 0xff;
	  // premultiply with alpha
	  uint8_t r = uint8_t(*p++) * a / 0xff;
	  uint8_t g = uint8_t(*p++) * a / 0xff;
	  uint8_t b = uint8_t(*p++) * a / 0xff;
	  uint32_t pixel = (a << 24) | (r << 16) | (g << 8) | b;
	  if (pixel == colorKey)
	    pixel = 0x0;
	  *q++ = pixel;
	}
      }
    } else {
      for (int y = 0; y < height; ++y) {
	if (pngPrediction)
	  ++p; // skip predictor byte
	for (int x = 0; x < width; ++x) {
	  uint8_t a = alpha ? *alpha++ : 0xff;
	  uint8_t r = uint8_t(*p++) * a / 0xff;
	  uint32_t pixel = (a << 24) | (r << 16) | (r << 8) | r;
	  if (pixel == colorKey)
	    pixel = 0x0;
	  *q++ = pixel;
	}
      }
    }
  }
  // we cannot use cairo_image_surface_create_for_data, because when rendering to PS or PDF,
  // the surface is kept by cairo until showpage gets called, and pixels is local to this function.
  cairo_surface_t *image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  memcpy(cairo_image_surface_get_data(image), pixels.data(), 4 * width * height);
  cairo_surface_mark_dirty(image);
  cairo_save(cr);
  Matrix tf = Matrix(1.0 / width, 0.0, 0.0, -1.0 / height, 0.0, 1.0);
  cairoTransform(cr, tf);
  cairo_set_source_surface(cr, image, 0, 0);
  cairo_pattern_set_filter(cairo_get_source(cr), filterBest ? CAIRO_FILTER_BEST : CAIRO_FILTER_GOOD);
  cairo_paint_with_alpha(cr, opacity);
  cairo_restore(cr);
}

// --------------------------------------------------------------------

/*! \class ipe::CairoPainter
  \ingroup cairo
  \brief Ipe Painter using Cairo and Freetype as a backend.

  This painter draws to a Cairo surface.
*/

//! Construct a painter.
/*! \a zoom one means 72 pixels per inch.  Set \a pretty to true
  to avoid drawing text without Latex. */
CairoPainter::CairoPainter(const Cascade *sheet, Fonts *fonts,
			   cairo_t *cc, double zoom, bool pretty, bool filterBest)
  : Painter(sheet), iFonts(fonts), iCairo(cc), iZoom(zoom),
    iPretty(pretty), iFilterBest(filterBest), iType3Font(false)
{
  iDimmed = false;
}

void CairoPainter::doPush()
{
  cairo_save(iCairo);
}

void CairoPainter::doPop()
{
  cairo_restore(iCairo);
}

void CairoPainter::doMoveTo(const Vector &u)
{
  cairo_move_to(iCairo, u.x, u.y);
  iAfterMoveTo = true;
}

void CairoPainter::doLineTo(const Vector &u)
{
  cairo_line_to(iCairo, u.x, u.y);
  iAfterMoveTo = false;
}

void CairoPainter::doCurveTo(const Vector &u1, const Vector &u2,
			     const Vector &u3)
{
  cairo_curve_to(iCairo, u1.x, u1.y, u2.x, u2.y, u3.x, u3.y);
  iAfterMoveTo = false;
}

void CairoPainter::doClosePath()
{
  cairo_close_path(iCairo);
}

void CairoPainter::doDrawArc(const Arc &arc)
{
  cairo_save(iCairo);
  Matrix m = matrix() * arc.iM;
  cairoTransform(iCairo, m);
  if (arc.isEllipse()) {
    cairo_new_sub_path(iCairo);
    cairo_arc(iCairo, 0.0, 0.0, 1.0, 0.0, IpeTwoPi);
    cairo_close_path(iCairo);
  } else {
    // this is necessary because of rounding errors:
    // otherwise cairo may insert a near-zero-length segment that messes
    // up line cap
    if (iAfterMoveTo)
      cairo_new_sub_path(iCairo);
    cairo_arc(iCairo, 0.0, 0.0, 1.0, arc.iAlpha, arc.iBeta);
  }
  iAfterMoveTo = false;
  cairo_restore(iCairo);
}

void CairoPainter::doDrawPath(TPathMode mode)
{
  cairo_save(iCairo);
  if (mode >= EStrokedAndFilled) {
    Color fillColor = fill();

    cairo_set_fill_rule(iCairo, (fillRule() == EEvenOddRule) ?
			CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);

    const Tiling *t = nullptr;
    if (!tiling().isNormal())
      t = cascade()->findTiling(tiling());

    const Gradient *g = nullptr;
    if (!gradient().isNormal())
      g = cascade()->findGradient(gradient());

    if (t == nullptr && g == nullptr) {
      // no tiling, no gradient
      cairo_set_source_rgba(iCairo, fillColor.iRed.toDouble(),
			    fillColor.iGreen.toDouble(),
			    fillColor.iBlue.toDouble(),
			    opacity().toDouble());

      if (mode == EStrokedAndFilled)
	cairo_fill_preserve(iCairo);
      else
	cairo_fill(iCairo);

    } else if (t == nullptr) {
      // gradient

      cairo_pattern_t *p;
      if (g->iType == Gradient::ERadial)
	p = cairo_pattern_create_radial(g->iV[0].x, g->iV[0].y, g->iRadius[0],
					g->iV[1].x, g->iV[1].y, g->iRadius[1]);
      else
	p = cairo_pattern_create_linear(g->iV[0].x, g->iV[0].y,
					g->iV[1].x, g->iV[1].y);

      cairo_pattern_set_extend(p, g->iExtend ?
			       CAIRO_EXTEND_PAD : CAIRO_EXTEND_NONE);

      for (const auto & stop : g->iStops) {
	cairo_pattern_add_color_stop_rgb(p, stop.offset,
					 stop.color.iRed.toDouble(),
					 stop.color.iGreen.toDouble(),
					 stop.color.iBlue.toDouble());
      }

      const Matrix &m0 = (matrix()* g->iMatrix).inverse();
      cairo_matrix_t m;
      cairoMatrix(m, m0);
      cairo_pattern_set_matrix(p, &m);

      cairo_set_source(iCairo, p);
      cairo_pattern_destroy(p);  // pass ownership to iCairo

      if (mode == EStrokedAndFilled)
	cairo_fill_preserve(iCairo);
      else
	cairo_fill(iCairo);

      // release pattern
      cairo_set_source_rgb(iCairo, 0.0, 0.0, 0.0);
    } else {
      // tiling

      cairo_surface_t *s =
	cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 32, 32);
      uint8_t *data = cairo_image_surface_get_data(s);
      memset(data, 0, 4 * 32 * 32);

      cairo_t *cc = cairo_create(s);
      cairo_set_source_rgba(cc,
			    fillColor.iRed.toDouble(),
			    fillColor.iGreen.toDouble(),
			    fillColor.iBlue.toDouble(),
			    opacity().toDouble());

      cairo_rectangle(cc, 0, 0, 32, 32 * t->iWidth / t->iStep);
      cairo_fill(cc);
      cairo_destroy(cc);
      cairo_pattern_t *p = cairo_pattern_create_for_surface(s);
      cairo_surface_destroy(s); // pass ownership to pattern
      cairo_pattern_set_extend(p, CAIRO_EXTEND_REPEAT);

      cairo_matrix_t m;
      cairo_matrix_init_scale(&m, 1.0, 32.0 / t->iStep);
      cairo_matrix_rotate(&m, -double(t->iAngle));
      cairo_pattern_set_matrix(p, &m);

      cairo_set_source(iCairo, p);
      cairo_pattern_destroy(p); // pass ownership to iCairo

      if (mode == EStrokedAndFilled)
	cairo_fill_preserve(iCairo);
      else
	cairo_fill(iCairo);

      // release pattern so pattern and surface are destroyed
      cairo_set_source_rgb(iCairo, 0.0, 0.0, 0.0);
    }
  }

  if (mode <= EStrokedAndFilled) {
    Color strokeColor = stroke();

    cairo_set_source_rgba(iCairo, strokeColor.iRed.toDouble(),
			  strokeColor.iGreen.toDouble(),
			  strokeColor.iBlue.toDouble(),
			  strokeOpacity().toDouble());

    cairo_set_line_width(iCairo, pen().toDouble());

    switch (lineJoin()) {
    case EMiterJoin:
      cairo_set_line_join(iCairo, CAIRO_LINE_JOIN_MITER);
      break;
    case ERoundJoin:
    case EDefaultJoin:
      cairo_set_line_join(iCairo, CAIRO_LINE_JOIN_ROUND);
      break;
    case EBevelJoin:
      cairo_set_line_join(iCairo, CAIRO_LINE_JOIN_BEVEL);
      break;
    }
    switch (lineCap()) {
    case EButtCap:
      cairo_set_line_cap(iCairo, CAIRO_LINE_CAP_BUTT);
      break;
    case ERoundCap:
    case EDefaultCap:
      cairo_set_line_cap(iCairo, CAIRO_LINE_CAP_ROUND);
      break;
    case ESquareCap:
      cairo_set_line_cap(iCairo, CAIRO_LINE_CAP_SQUARE);
      break;
    }
    if (dashStyle() != "[]0") {
      std::vector<double> dashes;
      double offset;
      dashStyle(dashes, offset);
      cairo_set_dash(iCairo, dashes.data(), dashes.size(), offset);
    }
    cairo_stroke(iCairo);
  }
  cairo_restore(iCairo);
}

void CairoPainter::doAddClipPath()
{
  cairo_clip(iCairo);
}

// --------------------------------------------------------------------

void CairoPainter::doDrawBitmap(Bitmap bitmap)
{
  Buffer data = bitmap.pixelData();
  if (!data.size())
    return;
  // is this legal?  I don't want cairo to modify my bitmap temporarily.
  cairo_surface_t *image =
    cairo_image_surface_create_for_data((uint8_t *) data.data(),
					CAIRO_FORMAT_ARGB32,
					bitmap.width(), bitmap.height(),
					4 * bitmap.width());
  cairo_save(iCairo);
  Matrix tf = matrix() * Matrix(1.0 / bitmap.width(), 0.0,
				0.0, -1.0 / bitmap.height(),
				0.0, 1.0);
  cairoTransform(iCairo, tf);
  cairo_set_source_surface(iCairo, image, 0, 0);
  cairo_pattern_set_filter(cairo_get_source(iCairo),
			   iFilterBest ? CAIRO_FILTER_BEST : CAIRO_FILTER_GOOD);
  cairo_paint_with_alpha(iCairo, opacity().toDouble());
  cairo_restore(iCairo);
}

void CairoPainter::doDrawText(const Text *text)
{
  // Current origin is lower left corner of text box

  // Draw bounding box rectangle
  if (!iPretty && !iDimmed) {
    cairo_save(iCairo);
    cairo_set_source_rgb(iCairo, 0.0, 1.0, 0.0);
    cairo_set_line_width(iCairo, 1.0 / iZoom);
    double dash = 3.0 / iZoom;
    cairo_set_dash(iCairo, &dash, 1, 0.0);
    Vector u0 = matrix() * Vector::ZERO;
    Vector u1 = matrix() * Vector(0, text->totalHeight());
    Vector u2 = matrix() * Vector(text->width(), text->totalHeight());
    Vector u3 = matrix() * Vector(text->width(), 0);
    cairo_move_to(iCairo, u0.x, u0.y);
    cairo_line_to(iCairo, u1.x, u1.y);
    cairo_line_to(iCairo, u2.x, u2.y);
    cairo_line_to(iCairo, u3.x, u3.y);
    cairo_close_path(iCairo);
    cairo_stroke(iCairo);

    Vector ref = matrix() * text->align();
    cairo_rectangle(iCairo, ref.x - 3.0/iZoom, ref.y - 3.0/iZoom,
		    6.0/iZoom, 6.0/iZoom);
    cairo_fill(iCairo);
    cairo_restore(iCairo);
  }

  const Text::XForm *xf = text->getXForm();
  if (!xf || !iFonts) {
    String s = text->text();
    int i = s.find('\n');
    if (i < 0 || i > 30)
      i = 30;
    if (i < s.size())
      s = s.left(i) + "...";

    Vector pt = matrix().translation();
    // pt.y = pt.y - iPainter->fontMetrics().descent();
    cairo_font_face_t *font = Fonts::screenFont();
    if (font) {
      cairo_save(iCairo);
      cairo_set_font_face(iCairo, font);
      cairo_set_font_size(iCairo, 9.0);
      Color col = stroke();
      cairo_set_source_rgba(iCairo, col.iRed.toDouble(), col.iGreen.toDouble(),
			    col.iBlue.toDouble(), opacity().toDouble());
      cairo_translate(iCairo, pt.x, pt.y);
      cairo_scale(iCairo, 1.0, -1.0);
      cairo_show_text(iCairo, s.z());
      cairo_restore(iCairo);
    }
  } else {
    transform(Matrix(xf->iStretch, 0, 0, xf->iStretch, 0, 0));
    translate(xf->iTranslation);
    const PdfDict *form = findResource("XObject", xf->iName);
    if (form)
      executeStream(form, form);
  }
}

void CairoPainter::executeStream(const PdfDict *stream, const PdfDict *resources)
{
  cairo_save(iCairo);
  cairoTransform(iCairo, matrix());
  PdfState ps;
  ps.iFont = nullptr;
  ps.iFillRgb[0] = ps.iFillRgb[1] = ps.iFillRgb[2] = 0.0;
  ps.iStrokeRgb[0] = ps.iStrokeRgb[1] = ps.iStrokeRgb[2] = 0.0;
  ps.iStrokeOpacity = ps.iFillOpacity = opacity().toDouble();
  ps.iCharacterSpacing = 0.0;
  ps.iWordSpacing = 0.0;
  ps.iHorizontalScaling = 1.0;
  ps.iLeading = 0.0;
  ps.iTextRise = 0.0;
  iPdfState.push_back(ps);
  execute(stream, resources);
  cairo_restore(iCairo);
}

// --------------------------------------------------------------------

//! Clear PDF argument stack
void CairoPainter::clearArgs()
{
  while (!iArgs.empty()) {
    delete iArgs.back();
    iArgs.pop_back();
  }
}

const PdfDict *CairoPainter::findResource(String kind, String name)
{
  if (iResourceStack.size() > 0) {
    const PdfDict *res =
      iFonts->resources()->findResource(iResourceStack.back(), kind, name);
    if (res)
      return res;
  }
  return iFonts->resources()->findResource(kind, name);
}


void CairoPainter::execute(const PdfDict *xform, const PdfDict *resources, bool applyMatrix)
{
  // ipeDebug("execute %s", xform->dictRepr().z());
  iResourceStack.push_back(resources);
  std::vector<double> m;
  if (applyMatrix && xform->getNumberArray("Matrix", nullptr, m) && m.size() == 6) {
    Matrix mx;
    for (int i = 0; i < 6; ++i)
      mx.a[i] = m[i];
    cairoTransform(iCairo, mx);
  }
  Buffer buffer = xform->inflate();
  BufferSource source(buffer);
  PdfParser parser(source);
  clearArgs();                  // if called recursively...
  while (!parser.eos()) {
    PdfToken tok = parser.token();
    if (tok.iType != PdfToken::EOp) {
      const PdfObj *obj = parser.getObject();
      if (!obj)
	break; // no further parsing attempted
      iArgs.push_back(obj);
    } else {
      // its an operator, execute it
      String op = tok.iString;
      parser.getToken();
      if (op == "cm")
	opcm();
      else if (op == "q")
	opq();
      else if (op == "Q")
	opQ();
      else if (op == "rg")
	oprg(false);
      else if (op == "RG")
	oprg(true);
      else if (op == "g")
	opg(false);
      else if (op == "G")
	opg(true);
      else if (op == "k")
	opk(false);
      else if (op == "K")
	opk(true);
      else if (op == "scn")
	opscn(false);
      else if (op == "SCN")
	opscn(true);
      else if (op == "w")
	opw();
      else if (op == "d")
	opd();
      else if (op == "Do")
	opDo();
      else if (op == "sh")
	opsh();
      else if (op == "i")
	opi();
      else if (op == "j")
	opj();
      else if (op == "J")
	opJ();
      else if (op == "M")
	opM();
      else if (op == "W")
	opW(false);
      else if (op == "W*")
	opW(true);
      else if (op == "gs")
	opgs();
      else if (op == "m")
	opm();
      else if (op == "l")
	opl();
      else if (op == "h")
	oph();
      else if (op == "c")
	opc();
      else if (op == "v")
	opv();
      else if (op == "y")
	opy();
      else if (op == "re")
	opre();
      else if (op == "n")
	opn();
      else if (op == "b")
	opStrokeFill(true, true, true, false);
      else if (op == "b*")
	opStrokeFill(true, true, true, true);
      else if (op == "B")
	opStrokeFill(false, true, true, false);
      else if (op == "B*")
	opStrokeFill(false, true, true, true);
      else if (op == "f" || op == "F")
	opStrokeFill(false, true, false, false);
      else if (op == "f*")
	opStrokeFill(false, true, false, true);
      else if (op == "s")
	opStrokeFill(true, false, true, false);
      else if (op == "S")
	opStrokeFill(false, false, true, false);
      else if (op == "Tc")
	opTc(&iPdfState.back().iCharacterSpacing);
      else if (op == "Tw")
	opTc(&iPdfState.back().iWordSpacing);
      else if (op == "TL")
	opTc(&iPdfState.back().iLeading);
      else if (op == "Ts")
	opTc(&iPdfState.back().iTextRise);
      else if (op == "Tz")
	opTz();
      else if (op == "Tf")
	opTf();
      else if (op == "Tm")
	opTm();
      else if (op == "Td")
	opTd(false);
      else if (op == "TD")
	opTd(true);
      else if (op == "T*")
	opTstar();
      else if (op == "TJ")
	opTJ();
      else if (op == "Tj")
	opTj(false, false);
      else if (op == "'")
	opTj(true, false);
      else if (op == "\"")
	opTj(true, true);
      else if (op == "BT")
	opBT();
      else if (op == "ET")
	opET();
      else if (op == "MP" || op == "DP" || op == "BMC"
	       || op == "BDC" || op == "EMC") {
	// content markers, ignore
      } else if (op == "ri") {
	// set rendering intent, ignore
      } else if (op == "cs") {
	// set color space, ignore
      } else {
	String a;
	for (const auto & arg : iArgs)
	  a += arg->repr() + " ";
	ipeDebug("op %s (%s)", op.z(), a.z());
      }
      clearArgs();
    }
  }
  clearArgs();
  iResourceStack.pop_back();
}

void CairoPainter::opg(bool stroke)
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  double gr = iArgs[0]->number()->value();
  auto & ps = iPdfState.back();
  if (stroke)
    ps.iStrokeRgb[0] = ps.iStrokeRgb[1] = ps.iStrokeRgb[2] = gr;
  else
    ps.iFillRgb[0] = ps.iFillRgb[1] = ps.iFillRgb[2] = gr;
}


void CairoPainter::oprg(bool stroke)
{
  if (iArgs.size() != 3 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number())
    return;
  double *col = (stroke ? iPdfState.back().iStrokeRgb :
		 iPdfState.back().iFillRgb);
  for (int i = 0; i < 3; ++i)
    col[i] = iArgs[i]->number()->value();
}

void CairoPainter::opk(bool stroke)
{
  if (iArgs.size() != 4 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number() || !iArgs[3]->number())
    return;
  ipeDebug("PDF setting CMYK color");
  // should use the colorspace of the monitor instead of this crude conversion
  double v = 1.0  - iArgs[3]->number()->value();
  double *col = (stroke ? iPdfState.back().iStrokeRgb :
		 iPdfState.back().iFillRgb);
  for (int i = 0; i < 3; ++i)
    col[i] = v * (1.0 - iArgs[i]->number()->value());
}

void CairoPainter::opscn(bool stroke)
{
  // uncolored tiling pattern arguments actually depend on colorspace set with cs,
  // we simply assume here that it's DeviceRGB
  String pattern;
  auto & ps = iPdfState.back();
  if (iArgs.size() == 1 && iArgs[0]->name()) {
    // colored tiling pattern
    pattern = iArgs[0]->name()->value();
  } else {
    if (iArgs.size() != 4 || !iArgs[0]->number() || !iArgs[1]->number()
	|| !iArgs[2]->number() || !iArgs[3]->name())
      return;
    // uncolored tiling pattern
    pattern = iArgs[3]->name()->value();
    double *col = (stroke ? ps.iStrokeRgb : ps.iFillRgb);
    for (int i = 0; i < 3; ++i)
      col[i] = iArgs[i]->number()->value();
  }
  if (stroke)
    ipeDebug("op scn /%s: stroke pattern not implemented.", pattern.z());
  else
    ps.iFillPattern = pattern;
}

void CairoPainter::opcm()
{
  if (iArgs.size() != 6)
    return;
  Matrix m;
  for (int i = 0; i < 6; ++i) {
    if (!iArgs[i]->number())
      return;
    m.a[i] = iArgs[i]->number()->value();
  }
  cairoTransform(iCairo, m);
}

void CairoPainter::opw()
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  cairo_set_line_width(iCairo, iArgs[0]->number()->value());
}

void CairoPainter::opd()
{
  if (iArgs.size() != 2 || !iArgs[0]->array() || !iArgs[1]->number())
    return;
  std::vector<double> dashes;
  for (int i = 0; i < iArgs[0]->array()->count(); ++i) {
    const PdfObj *obj = iArgs[0]->array()->obj(i, nullptr);
    if (!obj->number())
      return;
    dashes.emplace_back(obj->number()->value());
  }
  double offset = iArgs[1]->number()->value();
  cairo_set_dash(iCairo, dashes.data(), dashes.size(), offset);
}

void CairoPainter::opi()
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  // ipeDebug("Set flatness tolerance to %g", iArgs[0]->number()->value());
}

void CairoPainter::opj()
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  cairo_set_line_join(iCairo, cairo_line_join_t(iArgs[0]->number()->value()));
}

void CairoPainter::opJ()
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  cairo_set_line_cap(iCairo, cairo_line_cap_t(iArgs[0]->number()->value()));
}

void CairoPainter::opM()
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  cairo_set_miter_limit(iCairo, iArgs[0]->number()->value());
}

void CairoPainter::opW(bool eofill)
{
  cairo_set_fill_rule(iCairo, eofill ? CAIRO_FILL_RULE_EVEN_ODD :
		      CAIRO_FILL_RULE_WINDING);
  cairo_clip_preserve(iCairo);
}

// --------------------------------------------------------------------

void CairoPainter::opgs()
{
  if (iArgs.size() != 1 || !iArgs[0]->name())
    return;
  String name = iArgs[0]->name()->value();
  const PdfDict *d = findResource("ExtGState", name);
  if (!d) {
    ipeDebug("gs %s cannot find ExtGState dictionary!", name.z());
    return;
  }
  for (int j = 0; j < d->count(); ++j) {
    String key = d->key(j);
    const PdfObj *val = d->value(j);
    if (key == "ca") {
      if (val->number())
	iPdfState.back().iFillOpacity = val->number()->value();
    } else if (key == "CA") {
      if (val->number())
	iPdfState.back().iStrokeOpacity = val->number()->value();
    } else if (key == "Type" || key == "SA" || key == "TR" || key == "TR2"
	       || key == "SM" || key == "HT" || key == "OP" || key == "op"
	       || key == "RI" || key == "UCR" || key == "UCR2" || key == "BG"
	       || key == "BG2" || key == "OPM" ) {
      // ignore
    } else
      ipeDebug("gs %s %s", key.z(), val->repr().z());
  }
}

void CairoPainter::opsh()
{
  if (iArgs.size() != 1 || !iArgs[0]->name())
    return;
  String name = iArgs[0]->name()->value();
  const PdfDict *d = findResource("Shading", name);
  if (d)
    drawShading(iCairo, d, iFonts->resources());
}

void CairoPainter::opDo()
{
  if (iArgs.size() != 1 || !iArgs[0]->name())
    return;
  String name = iArgs[0]->name()->value();
  // ipeDebug("Do %s at level %d", name.z(), iResourceStack.size());
  const PdfDict *xf = findResource("XObject", name);
  if (!xf)
    return;
  const PdfObj *subtypeObj = xf->get("Subtype");
  if (!subtypeObj || !subtypeObj->name())
    return;
  String subtype = subtypeObj->name()->value();
  if (subtype == "Form") {
    cairo_save(iCairo);
    execute(xf, xf);
    cairo_restore(iCairo);
  } else if (subtype == "Image") {
    drawImage(iCairo, xf, iFonts->resources(), iPdfState.back().iFillOpacity, iFilterBest);
  } else
    ipeDebug("Do operator with unsupported XObject subtype %s", subtype.z());
}

// --------------------------------------------------------------------

void CairoPainter::opq()
{
  if (iArgs.size() != 0)
    return;
  cairo_save(iCairo);
  iPdfState.push_back(iPdfState.back());
}

void CairoPainter::opQ()
{
  if (iArgs.size() != 0)
    return;
  cairo_restore(iCairo);
  iPdfState.pop_back();
}

// --------------------------------------------------------------------

void CairoPainter::opm()
{
  if (iArgs.size() != 2 || !iArgs[0]->number() || !iArgs[1]->number())
    return;
  Vector t(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  cairo_move_to(iCairo, t.x, t.y);
}

void CairoPainter::opl()
{
  if (iArgs.size() != 2 || !iArgs[0]->number() || !iArgs[1]->number())
    return;
  Vector t(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  cairo_line_to(iCairo, t.x, t.y);
}

void CairoPainter::oph()
{
  if (iArgs.size() != 0)
    return;
  cairo_close_path(iCairo);
}

void CairoPainter::opc()
{
  if (iArgs.size() != 6 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number() || !iArgs[3]->number()
      || !iArgs[4]->number() || !iArgs[5]->number())
    return;
  Vector p1(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  Vector p2(iArgs[2]->number()->value(), iArgs[3]->number()->value());
  Vector p3(iArgs[4]->number()->value(), iArgs[5]->number()->value());
  cairo_curve_to(iCairo, p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
}

void CairoPainter::opv()
{
  if (iArgs.size() != 4 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number() || !iArgs[3]->number())
    return;
  double x1, y1;
  cairo_get_current_point(iCairo, &x1, &y1);
  Vector p2(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  Vector p3(iArgs[2]->number()->value(), iArgs[3]->number()->value());
  cairo_curve_to(iCairo, x1, y1, p2.x, p2.y, p3.x, p3.y);
}

void CairoPainter::opy()
{
  if (iArgs.size() != 4 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number() || !iArgs[3]->number())
    return;
  Vector p1(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  Vector p3(iArgs[2]->number()->value(), iArgs[3]->number()->value());
  cairo_curve_to(iCairo, p1.x, p1.y, p3.x, p3.y, p3.x, p3.y);
}

void CairoPainter::opre()
{
  if (iArgs.size() != 4 || !iArgs[0]->number() || !iArgs[1]->number()
      || !iArgs[2]->number() || !iArgs[3]->number())
    return;
  Vector t(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  Vector wh(iArgs[2]->number()->value(), iArgs[3]->number()->value());
  cairo_rectangle(iCairo, t.x, t.y, wh.x, wh.y);
}

void CairoPainter::opn()
{
  // the sequence "W n" updates the clipping path and then clears the current path
  cairo_new_path(iCairo);
}

// TODO: cache patterns instead of recreating them for every object?
// caching would need different handling of uncolored tiling patterns.
// shading patterns are not implemented here, because Ipe and tikz create
// them using the 'sh' operator.
void CairoPainter::createPattern()
{
  auto & ps = iPdfState.back();
  const PdfDict *pat = findResource("Pattern", ps.iFillPattern);
  // handle tiling patterns only
  if (pat && pat->getInteger("PatternType") == 1) {
    int paintType = pat->getInteger("PaintType");
    double xstep, ystep;
    if (!pat->getNumber("XStep", xstep) || !pat->getNumber("YStep", ystep))
      return;
    ipeDebug("Tiling pattern /%s PaintType %d xstep %g ystep %g", ps.iFillPattern.z(),
	     paintType, xstep, ystep);

    // to get good quality patterns, the pattern surface cannot be too small
    // except that for Ipe patterns this isn't necessary.
    double xscale = 1.0;
    double yscale = 1.0;
    // Heuristic: if a matrix exists, let's assume small cell is okay
    if (pat->get("Matrix") == nullptr) {
      while (xscale * xstep < 100)
	xscale *= 2.0;
      while (yscale * ystep < 100)
	yscale *= 2.0;
    }
    int width = (int) std::ceil(xscale * xstep);
    int height = (int) std::ceil(yscale * ystep);
    ipeDebug("Using pattern surface of size %d x %d", width, height);
    cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cc = cairo_create(sf);
    cairo_paint_with_alpha(cc, 0.0); // clear surface to transparent
    cairo_translate(cc, 0.0, height);
    cairo_scale(cc, 1.0, -1.0);

    auto & ps0 = iPdfState.back();
    PdfState ps;
    ps.iFont = nullptr;
    for (int i = 0; i < 3; ++i) {
      ps.iFillRgb[i] = ps0.iFillRgb[i];
      ps.iStrokeRgb[i] = ps0.iStrokeRgb[i];
    }
    ps.iStrokeOpacity = ps0.iStrokeOpacity;
    ps.iFillOpacity = ps0.iFillOpacity;
    ps.iCharacterSpacing = 0.0;
    ps.iWordSpacing = 0.0;
    ps.iHorizontalScaling = 1.0;
    ps.iLeading = 0.0;
    ps.iTextRise = 0.0;

    // drawing the pattern four times is also not necessary for Ipe patterns...
    // we can avoid this if we consult the BBox
    for (int dx = 0; dx < 2; ++dx) {
      for (int dy = 0; dy < 2; ++dy) {
	cairo_save(cc);
	cairo_translate(cc, -dx * width, -dy * height);
	cairo_scale(cc, xscale, yscale);
	CairoPainter painter(iCascade, iFonts, cc, 1.0, false, false);
	painter.iPdfState.push_back(ps);
	painter.execute(pat, pat, false);
	cairo_restore(cc);
      }
    }
    cairo_surface_flush(sf);
    cairo_destroy(cc);

    cairo_pattern_t *cpat = cairo_pattern_create_for_surface(sf);
    cairo_pattern_set_extend(cpat, CAIRO_EXTEND_REPEAT);

    Matrix mx;
    std::vector<double> m;
    if (pat->getNumberArray("Matrix", nullptr, m) && m.size() == 6) {
      for (int i = 0; i < 6; ++i)
	mx.a[i] = m[i];
      // PDF pattern matrix goes from user space to pattern space,
      // Cairo pattern matrix is the opposite
      mx = mx.inverse();
    }
    mx = Matrix(xscale, 0.0, 0.0, -yscale, 0.0, height) * mx;
    cairo_matrix_t cm;
    cairoMatrix(cm, mx);
    cairo_pattern_set_matrix(cpat, &cm);
    cairo_set_source(iCairo, cpat);
    cairo_pattern_destroy(cpat);
  }
}

void CairoPainter::opStrokeFill(bool close, bool fill, bool stroke, bool eofill)
{
  if (close)
    cairo_close_path(iCairo);
  PdfState &ps = iPdfState.back();
  if (fill) {
    if (!ps.iFillPattern.empty())
      createPattern();
    else
      cairo_set_source_rgba(iCairo,
			    ps.iFillRgb[0], ps.iFillRgb[1], ps.iFillRgb[2],
			    ps.iFillOpacity);
    cairo_set_fill_rule(iCairo, eofill ? CAIRO_FILL_RULE_EVEN_ODD :
			CAIRO_FILL_RULE_WINDING);
  }
  if (fill && stroke)
    cairo_fill_preserve(iCairo);
  else if (fill)
    cairo_fill(iCairo);
  if (stroke) {
    cairo_set_source_rgba(iCairo,
			  ps.iStrokeRgb[0], ps.iStrokeRgb[1], ps.iStrokeRgb[2],
			  ps.iStrokeOpacity);
    cairo_stroke(iCairo);
  }
}

// --------------------------------------------------------------------

void CairoPainter::opBT()
{
  iTextMatrix = iTextLineMatrix = Matrix();
}

void CairoPainter::opET()
{
  // nothing
}

void CairoPainter::opTc(double *p)
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  *p = iArgs[0]->number()->value();
}

void CairoPainter::opTz()
{
  if (iArgs.size() != 1 || !iArgs[0]->number())
    return;
  iPdfState.back().iHorizontalScaling = iArgs[0]->number()->value() / 100.0;
}

void CairoPainter::opTm()
{
  if (iArgs.size() != 6)
    return;
  Matrix m;
  for (int i = 0; i < 6; ++i) {
    if (!iArgs[i]->number())
      return;
    m.a[i] = iArgs[i]->number()->value();
  }
  iTextMatrix = iTextLineMatrix = m;
}

void CairoPainter::opTf()
{
  if (iArgs.size() != 2 || !iArgs[0]->name() || !iArgs[1]->number())
    return;
  String name = iArgs[0]->name()->value();
  iPdfState.back().iFontSize = iArgs[1]->number()->value();
  const PdfDict *fd = findResource("Font", name);
  if (fd) {
    Face *f = iFonts->getFace(fd);
    iPdfState.back().iFont = f;
    if (f->type() == FontType::Type3)
      iType3Font = true;
  }
}

void CairoPainter::opTd(bool setLeading)
{
  if (iArgs.size() != 2 || !iArgs[0]->number() || !iArgs[1]->number())
    return;
  Vector t(iArgs[0]->number()->value(), iArgs[1]->number()->value());
  iTextMatrix = iTextLineMatrix = iTextLineMatrix * Matrix(t);
  if (setLeading)
    iPdfState.back().iLeading = t.y;
}

void CairoPainter::opTstar()
{
  if (iArgs.size() != 0)
    return;
  Vector t(0, iPdfState.back().iLeading);
  iTextMatrix = iTextLineMatrix = iTextLineMatrix * Matrix(t);
}

void CairoPainter::opTj(bool nextLine, bool setSpacing)
{
  PdfState &ps = iPdfState.back();
  if (!setSpacing) {
    if (iArgs.size() != 1 || !iArgs[0]->string())
      return;
  } else {
    if (iArgs.size() != 3 || !iArgs[0]->number() || !iArgs[1]->number()
	|| !iArgs[2]->string())
      return;
  }
  String s = iArgs[iArgs.size() - 1]->string()->decode();
  if (setSpacing) {
    ps.iWordSpacing = iArgs[0]->number()->value();
    ps.iCharacterSpacing = iArgs[1]->number()->value();
  }
  if (nextLine) {
    Vector t(0, ps.iLeading);
    iTextMatrix = iTextLineMatrix = iTextLineMatrix * Matrix(t);
  }

  if (!ps.iFont)
    return;

  std::vector<cairo_glyph_t> glyphs;
  Vector textPos(0, 0);
  collectGlyphs(s, glyphs, textPos);
  drawGlyphs(glyphs);
  iTextMatrix = iTextMatrix * Matrix(textPos);
}

void CairoPainter::opTJ()
{
  PdfState &ps = iPdfState.back();
  if (!ps.iFont || iArgs.size() != 1 || !iArgs[0]->array())
    return;
  std::vector<cairo_glyph_t> glyphs;
  Vector textPos(0, 0);
  for (int i = 0; i < iArgs[0]->array()->count(); ++i) {
    const PdfObj *obj = iArgs[0]->array()->obj(i, nullptr);
    if (obj->number())
      textPos.x -=
	0.001 * ps.iFontSize * obj->number()->value() * ps.iHorizontalScaling;
    else if (obj->string())
      collectGlyphs(obj->string()->decode(), glyphs, textPos);
  }
  drawGlyphs(glyphs);
  iTextMatrix = iTextMatrix * Matrix(textPos);
}

void CairoPainter::collectGlyphs(String s, std::vector<cairo_glyph_t> &glyphs,
				 Vector &textPos)
{
  PdfState &ps = iPdfState.back();
  bool ucs = (ps.iFont->type() == FontType::CIDType0 ||
	      ps.iFont->type() == FontType::CIDType2);
  int j = 0;
  while (j < s.size()) {
    int ch = uint8_t(s[j++]);
    if (ucs && j < s.size())
      ch = (ch << 8) | uint8_t(s[j++]);
    cairo_glyph_t g;
    g.index = ps.iFont->glyphIndex(ch);
    Vector p = iTextMatrix.linear() * textPos;
    g.x = p.x;
    g.y = p.y;
    glyphs.push_back(g);
    textPos.x +=
      (0.001 * ps.iFontSize * ps.iFont->width(ch) + ps.iCharacterSpacing)
      * ps.iHorizontalScaling;
    if (ch == ' ')
      textPos.x += ps.iWordSpacing * ps.iHorizontalScaling;
  }
}

// --------------------------------------------------------------------

//! Draw a glyph.
/*! Glyph is drawn with hotspot at position pos. */
void CairoPainter::drawGlyphs(std::vector<cairo_glyph_t> &glyphs)
{
  PdfState &ps = iPdfState.back();
  if (!ps.iFont)
    return;

  Matrix m = iTextMatrix *
    Matrix(ps.iFontSize * ps.iHorizontalScaling, 0, 0, ps.iFontSize,
	   0, ps.iTextRise)
    * Linear(1, 0, 0, -1);

  cairo_matrix_t matrix;
  cairoMatrix(matrix, m);

  if (ps.iFont->type() == FontType::Type3) {
    cairo_save(iCairo);
    cairo_set_font_face(iCairo, Fonts::screenFont());
    cairo_set_source_rgba(iCairo,
			  ps.iFillRgb[0], ps.iFillRgb[1], ps.iFillRgb[2],
			  0.5);
    cairo_save(iCairo);
    cairo_set_font_matrix(iCairo, &matrix);
    cairo_show_glyphs(iCairo, glyphs.data(), glyphs.size());
    cairo_restore(iCairo);
    double s = ps.iFontSize;
    cairo_set_font_size(iCairo, 0.23 * s);
    for (int i = 0; i < size(glyphs); ++i) {
      auto pt = iTextMatrix * Vector(glyphs[i].x, glyphs[i].y);
      cairo_save(iCairo);
      cairo_translate(iCairo, pt.x, pt.y);
      cairo_rotate(iCairo, 0.4 * IPE_PI);
      cairo_scale(iCairo, 1.0, -1.0);
      cairo_move_to(iCairo, -0.05 * s, 0.3 * s);
      cairo_show_text(iCairo, "Type3");
      cairo_restore(iCairo);
    }
    cairo_restore(iCairo);
  } else {
    cairo_save(iCairo);
    cairo_set_font_face(iCairo, ps.iFont->cairoFont());
    cairo_set_font_matrix(iCairo, &matrix);
    cairo_set_source_rgba(iCairo,
			  ps.iFillRgb[0], ps.iFillRgb[1], ps.iFillRgb[2],
			  ps.iFillOpacity);
    cairo_show_glyphs(iCairo, glyphs.data(), glyphs.size());
    cairo_restore(iCairo);
  }
}

// --------------------------------------------------------------------

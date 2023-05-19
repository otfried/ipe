// --------------------------------------------------------------------
// iperender
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

#include "ipedoc.h"
#include "ipethumbs.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using ipe::Document;
using ipe::Page;
using ipe::Thumbnail;

// --------------------------------------------------------------------

static int renderPage(Thumbnail::TargetFormat fm,
		      const char *src, const char *dst,
		      const char *pageSpec, const char *viewSpec,
		      double zoom, double tolerance,
		      bool transparent, bool nocrop)
{
  Document *doc = Document::loadWithErrorReport(src);

  if (!doc)
    return 1;

  int pageIdx = pageSpec ? doc->findPage(pageSpec) : 0;
  if (pageIdx < 0) {
    fprintf(stderr, "Incorrect -page specification.\n");
    delete doc;
    return 1;
  }

  const Page *page = doc->page(pageIdx);

  int viewIdx = viewSpec ? page->findView(viewSpec) : 0;
  if (viewIdx < 0) {
    fprintf(stderr, "Incorrect -view specification.\n");
    delete doc;
    return 1;
  }

  if (doc->runLatex(src)) {
    delete doc;
    return 1;
  }

  Thumbnail tn(doc, 0);
  tn.setTransparent(transparent);
  tn.setNoCrop(nocrop);
  if (!tn.saveRender(fm, dst, page, viewIdx, zoom, tolerance))
    fprintf(stderr, "Failure to render page.\n");
  delete doc;
  return 0;
}

// --------------------------------------------------------------------

static void usage()
{
  fprintf(stderr, "Usage: iperender [ -png ");
#ifdef CAIRO_HAS_PS_SURFACE
  fprintf(stderr, "| -eps ");
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  fprintf(stderr, "| -pdf ");
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  fprintf(stderr, "| -svg ");
#endif
  fprintf(stderr, "] "
	  "[ -page <page> ] [ -view <view> ] [ -resolution <dpi> ] "
	  "[ -transparent ] [ -nocrop ] "
	  "infile outfile\n"
	  "Iperender saves a single page of the Ipe document in some formats.\n"
	  " -page       : page to save (default 1).\n"
	  " -view       : view to save (default 1).\n"
	  " -resolution : resolution for png format (default 72.0 ppi).\n"
	  " -tolerance  : tolerance when rendering curves (default 0.1).\n"
	  " -transparent: use transparent background in png format.\n"
	  " -nocrop     : do not crop page.\n"
	  "<page> can be a page number or a page name.\n"
	  );
  exit(1);
}

int main(int argc, char *argv[])
{
  ipe::Platform::initLib(ipe::IPELIB_VERSION);

  // ensure at least three arguments (handles -help as well :-)
  if (argc < 4)
    usage();

  Thumbnail::TargetFormat fm = Thumbnail::EPNG;
  if (!strcmp(argv[1], "-png"))
    fm = Thumbnail::EPNG;
#ifdef CAIRO_HAS_PS_SURFACE
  else if (!strcmp(argv[1], "-eps"))
    fm = Thumbnail::EPS;
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  else if (!strcmp(argv[1], "-pdf"))
    fm = Thumbnail::EPDF;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  else if (!strcmp(argv[1], "-svg"))
    fm = Thumbnail::ESVG;
#endif
  else
    usage();

  const char *page = nullptr;
  const char *view = nullptr;
  double dpi = 72.0;
  double tolerance = 0.1;
  bool transparent = false;
  bool nocrop = false;

  int i = 2;
  while (i < argc - 2) {
    if (!strcmp(argv[i], "-page")) {
      if (i + 1 == argc)
	usage();
      page = argv[i+1];
      i += 2;
    } else if (!strcmp(argv[i], "-view")) {
      if (i + 1 == argc)
	usage();
      view = argv[i+1];
      i += 2;
    } else if (!strcmp(argv[i], "-resolution")) {
      if (i + 1 == argc)
	usage();
      dpi = ipe::Lex(ipe::String(argv[i+1])).getDouble();
      i += 2;
    } else if (!strcmp(argv[i], "-tolerance")) {
      if (i + 1 == argc)
	usage();
      tolerance = ipe::Lex(ipe::String(argv[i+1])).getDouble();
      i += 2;
    } else if (!strcmp(argv[i], "-transparent")) {
      transparent = true;
      ++i;
    } else if (!strcmp(argv[i], "-nocrop")) {
      nocrop = true;
      ++i;
    } else
      usage();
  }

  // remaining arguments must be two filenames
  const char *src = argv[i];
  const char *dst = argv[i+1];

  return renderPage(fm, src, dst, page, view, dpi / 72.0,
		    tolerance, transparent, nocrop);
}

// --------------------------------------------------------------------

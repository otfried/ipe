// --------------------------------------------------------------------
// ipetoipe
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

#include "ipedoc.h"

#include <cstdlib>
#include <cstring>

using ipe::Document;
using ipe::String;
using ipe::FileFormat;
using ipe::SaveFlag;

static int topdf(Document *doc, String src, String dst, uint32_t flags,
		 int fromPage = -1, int toPage = -1, int viewNo = -1)
{
  int res = doc->runLatex(src);
  if (res) return res;

  bool result = false;
  if (viewNo >= 0) {
    result = doc->exportView(dst.z(), FileFormat::Pdf, flags, fromPage, viewNo);
  } else if (toPage >= 0) {
    result = doc->exportPages(dst.z(), flags, fromPage, toPage);
  } else {
    result = doc->save(dst.z(), FileFormat::Pdf, flags);
  }
  if (!result) {
    fprintf(stderr, "Failed to save or export document!\n");
    return 1;
  }

  if (flags & SaveFlag::Export)
    fprintf(stderr, "Warning: the exported file contains no Ipe markup.\n"
	    "It cannot be read by Ipe - make sure you keep the original!\n");
  return 0;
}

static void usage()
{
  fprintf(stderr,
	  "Usage: ipetoipe ( -xml | -pdf ) <options> "
	  "infile [ outfile ]\n"
	  "Ipetoipe converts between the different Ipe file formats.\n"
	  " -export      : output contains no Ipe markup.\n"
	  " -pages <n-m> : export only these pages (implies -export).\n"
	  " -view <p-v>  : export only this view (implies -export).\n"
	  " -markedview  : export only marked views on marked pages (implies -export).\n"
	  "     If all views of a marked page are unmarked, the last view is exported.\n"
	  "     This is convenient to make handouts for slides.\n"
	  " -runlatex    : run Latex even for XML output.\n"
	  " -nozip       : do not compress PDF streams.\n"
	  " -keepnotes   : save page notes as PDF annotations even when exporting.\n"
	  "Pages can be specified by page number or by section title.\n"
	  );
  exit(1);
}

int main(int argc, char *argv[]) {
  ipe::Platform::initLib(ipe::IPELIB_VERSION);

  // ensure at least two arguments (handles -help as well :-)
  if (argc < 3)
    usage();

  FileFormat frm = FileFormat::Unknown;
  if (!strcmp(argv[1], "-xml"))
    frm = FileFormat::Xml;
  else if (!strcmp(argv[1], "-pdf"))
    frm = FileFormat::Pdf;

  if (frm == FileFormat::Unknown)
    usage();

  uint32_t flags = SaveFlag::SaveNormal;
  bool runLatex = false;
  const char *pages = nullptr;
  const char *view = nullptr;
  int i = 2;

  String infile;
  String outfile;

  while (i < argc) {

    if (!strcmp(argv[i], "-export")) {
      flags |= SaveFlag::Export;
      ++i;
    } else if (!strcmp(argv[i], "-view")) {
      flags |= SaveFlag::Export;
      if (i + 1 == argc)
	usage();
      view = argv[i+1];
      i += 2;
    } else if (!strcmp(argv[i], "-pages")) {
      flags |= SaveFlag::Export;
      if (i + 1 == argc)
	usage();
      pages = argv[i+1];
      i += 2;
    } else if (!strcmp(argv[i], "-markedview")) {
      flags |= SaveFlag::MarkedView;
      flags |= SaveFlag::Export;
      ++i;
    } else if (!strcmp(argv[i], "-runlatex")) {
      runLatex = true;
      ++i;
    } else if (!strcmp(argv[i], "-nozip")) {
      flags |= SaveFlag::NoZip;
      ++i;
    } else if (!strcmp(argv[i], "-keepnotes")) {
      flags |= SaveFlag::KeepNotes;
      ++i;
    } else {
      // last one or two arguments must be filenames
      infile = argv[i];
      ++i;
      if (i < argc) {
	outfile = argv[i];
	++i;
      }
      if (i != argc)
	usage();
    }
  }

  if (infile.empty())
    usage();

  if ((flags & SaveFlag::Export) && frm == FileFormat::Xml) {
    fprintf(stderr, "-export only available with -pdf.\n");
    exit(1);
  }

  if (pages && frm != FileFormat::Pdf) {
    fprintf(stderr, "-pages only available with -pdf.\n");
    exit(1);
  }

  if (pages && view) {
    fprintf(stderr, "cannot specify both -pages and -view.\n");
    exit(1);
  }

  if (outfile.empty()) {
    outfile = infile;
    String ext = infile.right(4);
    if (ext == ".ipe" || ext == ".pdf" || ext == ".xml")
      outfile = infile.left(infile.size() - 4);
    switch (frm) {
    case FileFormat::Xml:
      outfile += ".ipe";
      break;
    case FileFormat::Pdf:
      outfile += ".pdf";
    default:
      break;
    }
    if (outfile == infile) {
      fprintf(stderr, "Cannot guess output filename.\n");
      exit(1);
    }
  }

  std::unique_ptr<Document> doc(Document::loadWithErrorReport(infile.z()));

  if (!doc)
    return 1;

  fprintf(stderr, "Document %s has %d pages (%d views)\n",
	  infile.z(), doc->countPages(), doc->countTotalViews());

  // parse pages and view
  int fromPage = -1;
  int toPage = -1;
  int viewNo = -1;

  if (pages) {
    String p(pages);
    int j = p.find('-');
    if (j >= 0) {
      fromPage = (j > 0) ? doc->findPage(p.left(j)) : 0;
      toPage = (j < p.size() - 1) ? doc->findPage(p.substr(j+1)) : doc->countPages() - 1;
    }
    if (fromPage < 0 || fromPage > toPage) {
      fprintf(stderr, "incorrect -pages specification.\n");
      exit(1);
    }
  } else if (view) {
    String v(view);
    int j = v.find('-');
    if (j > 0) {
      fromPage = doc->findPage(v.left(j));
      if (fromPage >= 0)
	viewNo = doc->page(fromPage)->findView(v.substr(j+1));
    }
    if (fromPage < 0 || viewNo < 0) {
      fprintf(stderr, "incorrect -view specification.\n");
      exit(1);
    }
  }

  char buf[64];
  sprintf(buf, "ipetoipe %d.%d.%d",
	  ipe::IPELIB_VERSION / 10000,
	  (ipe::IPELIB_VERSION / 100) % 100,
	  ipe::IPELIB_VERSION % 100);
  Document::SProperties props = doc->properties();
  props.iCreator = buf;
  doc->setProperties(props);

  switch (frm) {
  case FileFormat::Xml:
    if (runLatex)
      return topdf(doc.get(), infile, outfile, flags);
    else
      doc->save(outfile.z(), FileFormat::Xml, SaveFlag::SaveNormal);
  default:
    return 0;

  case FileFormat::Pdf:
    return topdf(doc.get(), infile, outfile, flags, fromPage, toPage, viewNo);
  }
}

// --------------------------------------------------------------------

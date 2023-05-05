// -*- C++ -*-
// --------------------------------------------------------------------
// Parse an Ipe XML file
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

#ifndef IPEIML_H
#define IPEIML_H

#include "ipegroup.h"
#include "ipepage.h"
#include "ipedoc.h"
#include "ipebitmap.h"

// --------------------------------------------------------------------

namespace ipe {

  class ImlParser : public XmlParser {
  public:
    enum Errors { ESuccess = 0, EVersionTooOld, EVersionTooRecent,
		  ESyntaxError };
    explicit ImlParser(DataSource &source);
    int parseDocument(Document &doc);
    bool parsePage(Page &page);
    bool parseView(Page &page, AttributeMap &map);
    Object *parseObject(String tag, Page *page = nullptr,
			int *currentLayer = nullptr);
    Object *parseObject(String tag, String &layer);
    StyleSheet *parseStyleSheet();
    bool parseStyle(StyleSheet &sheet);
    // bool parseSelection(PageObjectSeq &seq);
    Page *parsePageSelection();
    virtual Buffer pdfStream(int objNum);
    bool parseBitmap();
  private:
    std::vector<Bitmap> iBitmaps;
  };

} // namespace

// --------------------------------------------------------------------

#endif

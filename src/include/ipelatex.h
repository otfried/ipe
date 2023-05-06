// -*- C++ -*-
// --------------------------------------------------------------------
// Latex source to PDF converter
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

#ifndef PDFLATEX_P_H
#define PDFLATEX_P_H

#include <list>

#include "ipepage.h"
#include "ipetext.h"
#include "ipepdfparser.h"
#include "iperesources.h"

// --------------------------------------------------------------------

namespace ipe {

  class Cascade;
  class TextCollectingVisitor;

  class Latex {
  public:
    Latex(const Cascade *sheet, LatexType latexType, bool sequentialText);
    ~Latex();

    int scanObject(const Object *obj);
    int scanPage(Page *page);
    void addPageNumber(int pno, int vno, int npages, int nviews);
    int createLatexSource(Stream &stream, String preamble);
    bool readPdf(DataSource &source);
    bool updateTextObjects();
    PdfResources *takeResources();

  private:
    bool getXForm(String key, const PdfDict *ipeInfo);
    void warn(String msg);

  private:
    struct SText {
      const Text *iText;
      Attribute iSize;
      Fixed iStretch;
      String iSource;
    };

    typedef std::vector<SText> TextList;
    typedef std::vector<Text::XForm *> XFormList;

    const Cascade *iCascade;
    bool iXetex;
    bool iSequentialText;
    LatexType iLatexType;

    PdfFile iPdf;

    //! List of text objects scanned. Objects not owned.
    TextList iTextObjects;

    //! List of XForm objects read from PDF file.  Objects owned!
    XFormList iXForms;

    //! The resources from the generated PDF file.
    PdfResources *iResources;

    friend class ipe::TextCollectingVisitor;
  };

} // namespace

// --------------------------------------------------------------------
#endif

// -*- C++ -*-
// --------------------------------------------------------------------
// The PDF resources created by Pdflatex/Xelatex
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

#ifndef IPERESOURCES_H
#define IPERESOURCES_H

#include "ipebase.h"
#include "ipegeo.h"
#include "ipepdfparser.h"
#include "ipetext.h"

#include <unordered_set>

// --------------------------------------------------------------------

namespace ipe {

  class PdfResourceBase {
  public:
    PdfResourceBase();
    virtual ~PdfResourceBase();

    virtual const PdfObj *object(int num) const noexcept = 0;

    const PdfObj *getDeep(const PdfDict *d, String key) const noexcept;
    const PdfDict *getDict(const PdfDict *d, String key) const noexcept;
    const PdfDict *resourcesOfKind(String kind) const noexcept;
    const PdfDict *findResource(String kind, String name) const noexcept;
    const PdfDict *findResource(const PdfDict *xf, String kind,
				String name) const noexcept;
  protected:
    std::unique_ptr<PdfDict> iPageResources;
  };

  class PdfFileResources : public PdfResourceBase {
  public:
    PdfFileResources(const PdfFile *file);
    ~PdfFileResources() = default;

    virtual const PdfObj *object(int num) const noexcept;
  private:
    const PdfFile *iPdf;
  };

  class PdfResources : public PdfResourceBase {
  public:
    struct SPageNumber {
      int page;
      int view;
      std::unique_ptr<Text> text;
    };
  public:
    PdfResources();
    virtual ~PdfResources() = default;
    bool collect(const PdfDict *resources, PdfFile *file);
    virtual const PdfObj *object(int num) const noexcept;
    virtual const PdfDict *baseResources() const noexcept;
    void addPageNumber(SPageNumber &pn) noexcept;
    const Text *pageNumber(int page, int view) const noexcept;
    inline const std::vector<int> &embedSequence() const noexcept {
      return iEmbedSequence; }
    void show() const noexcept;
    bool isIpeXForm(int num) const;
    void setIpeXForm(int num);
  private:
    void add(int num, PdfFile *file);
    void addIndirect(const PdfObj *q, PdfFile *file);
    bool addToResource(PdfDict *d, String key,
		       const PdfObj *el, PdfFile *file);
  private:
    std::unordered_map<int, std::unique_ptr<const PdfObj>> iObjects;
    std::vector<int> iEmbedSequence;
    // which of the objects in the PDF file are XForms corresponding to Ipe text objects
    std::unordered_set<int> ipeXForms;
    //! Page number objects.
    std::vector<SPageNumber> iPageNumbers;
  };

} // namespace

// --------------------------------------------------------------------
#endif

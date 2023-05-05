// -*- C++ -*-
// --------------------------------------------------------------------
// The Ipe document.
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

#ifndef IPEDOC_H
#define IPEDOC_H

#include "ipeobject.h"
#include "ipepage.h"
#include "ipeimage.h"
#include "ipestyle.h"

// --------------------------------------------------------------------

namespace ipe {

  class BitmapFinder;
  class PdfResources;
  class Latex;

  //! Flags for saving Ipe documents (to PDF)
  class SaveFlag {
  public:
    enum {
      SaveNormal = 0,   //!< Nothing special
      Export = 1,       //!< Don't include Ipe markup
      NoZip = 2,        //!< Do not compress streams
      MarkedView = 4,   //!< Create marked views only
      KeepNotes = 8,    //!< Keep page notes as PDF annotations even when exporting
    };
  };

  //! There are several Ipe document formats.
  enum class FileFormat {
    Xml,  //!< Save as XML
    Pdf,  //!< Save as PDF
    Unknown //!< Unknown file format
  };

  class Document {
  public:
    //! Properties of a document
    struct SProperties {
      String iTitle;
      String iAuthor;
      String iSubject;
      String iKeywords;
      String iPreamble;
      LatexType iTexEngine { LatexType::Default };
      bool iFullScreen { false };
      bool iNumberPages { false };
      //! Date/time in PDF style "D:20010428191400" format.
      String iCreated;
      String iModified;
      //! Program that created this document (e.g. "Ipe 7.5").
      String iCreator;
    };

    //! Errors that can happen while loading documents
    enum LoadErrors {
      EVersionTooOld = -1,    //!< The version of the file is too old.
      EVersionTooRecent = -2, //!< The file version is newer than this Ipelib.
      EFileOpenError = -3,    //!< Error opening the file
      ENotAnIpeFile = -4,     //!< The file was not created by Ipe.
    };

    Document();
    Document(const Document &rhs);
    Document &operator=(const Document &rhs) = delete;
    ~Document();

    static FileFormat fileFormat(DataSource &source);
    static FileFormat formatFromFilename(String fn);

    static Document *load(DataSource &source, FileFormat format, int &reason);

    static Document *load(const char *fname, int &reason);
    static Document *loadWithErrorReport(const char *fname);

    bool save(TellStream &stream, FileFormat format, uint32_t flags) const;
    bool save(const char *fname, FileFormat format, uint32_t flags) const;
    bool exportPages(const char *fname, uint32_t flags,
		     int fromPage, int toPage) const;
    bool exportView(const char *fname, FileFormat format,
		    uint32_t flags, int pno, int vno) const;

    void saveAsXml(Stream &stream, bool usePdfBitmaps = false) const;

    //! Return number of pages of document.
    int countPages() const { return int(iPages.size()); }
    int countTotalViews() const;

    //! Return page (const version).
    /*! The first page is no 0. */
    const Page *page(int no) const { return iPages[no]; }
    //! Return page.
    /*! The first page is no 0. */
    Page *page(int no) { return iPages[no]; }

    int findPage(String nameOrNumber) const;

    Page *set(int no, Page *page);
    void insert(int no, Page *page);
    void push_back(Page *page);
    Page *remove(int no);

    //! Return document properties.
    inline SProperties properties() const { return iProperties; }
    void setProperties(const SProperties &info);

    //! Return stylesheet cascade.
    Cascade *cascade() { return iCascade; }
    //! Return stylesheet cascade (const version).
    const Cascade *cascade() const { return iCascade; }
    Cascade *replaceCascade(Cascade *cascade);

    void setResources(PdfResources *resources);
    //! Return the current PDF resources.
    inline const PdfResources *resources() const noexcept { return iResources; }

    void findBitmaps(BitmapFinder &bm) const;
    bool checkStyle(AttributeSeq &seq) const;

    //! Error codes returned by RunLatex.
    enum { ErrNone, ErrNoText, ErrNoDir, ErrWritingSource,
	   ErrRunLatex, ErrLatex, ErrLatexOutput };
    int runLatex(String docname, String &logFile);
    int runLatex(String docname);
    int runLatexAsync(String docname, String &texLog, Latex **pConverter);
    int completeLatexRun(Latex *converter);

  private:
    std::vector<Page *> iPages;
    Cascade *iCascade;
    SProperties iProperties;
    PdfResources *iResources;
  };

} // namespace

// --------------------------------------------------------------------
#endif

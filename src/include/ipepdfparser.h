// -*- C++ -*-
// --------------------------------------------------------------------
// PDF file parser
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

#ifndef IPEPDFPARSER_H
#define IPEPDFPARSER_H

#include "ipebase.h"
#include "ipegeo.h"

#include <unordered_map>

// --------------------------------------------------------------------

namespace ipe {

  using PdfRenumber = std::unordered_map<int,int>;

  class PdfNull;
  class PdfBool;
  class PdfNumber;
  class PdfString;
  class PdfName;
  class PdfRef;
  class PdfArray;
  class PdfDict;

  class PdfFile;

  class PdfObj {
  public:
    virtual ~PdfObj() = 0;
    virtual const PdfNull *null() const noexcept;
    virtual const PdfBool *boolean() const noexcept;
    virtual const PdfNumber *number() const noexcept;
    virtual const PdfString *string() const noexcept;
    virtual const PdfName *name() const noexcept;
    virtual const PdfRef *ref() const noexcept;
    virtual const PdfArray *array() const noexcept;
    virtual const PdfDict *dict() const noexcept;
    virtual void write(Stream &stream, const PdfRenumber *renumber = nullptr,
		       bool inflate = false) const noexcept = 0;
    String repr() const noexcept;
  };

  class PdfNull : public PdfObj {
  public:
    explicit PdfNull() { /* nothing */ }
    virtual const PdfNull *null() const noexcept;
    virtual void write(Stream &stream, const PdfRenumber *renumber,
		       bool inflate) const noexcept;
  };

  class PdfBool : public PdfObj {
  public:
    explicit PdfBool(bool val) : iValue(val) { /* nothing */ }
    virtual const PdfBool *boolean() const noexcept;
    inline bool value() const noexcept { return iValue; }
    virtual void write(Stream &stream, const PdfRenumber *renumber,
		       bool inflate) const noexcept;
  private:
    bool iValue;
  };

  class PdfNumber : public PdfObj {
  public:
    explicit PdfNumber(double val) : iValue(val) { /* nothing */ }
    virtual const PdfNumber *number() const noexcept;
    virtual void write(Stream &stream, const PdfRenumber *renumber,
		       bool inflate) const noexcept;
    inline double value() const noexcept { return iValue; }
  private:
    double iValue;
  };

  class PdfString : public PdfObj {
  public:
    explicit PdfString(const String &val, bool binary=false)
      : iBinary(binary), iValue(val) { /* nothing */ }
    virtual const PdfString *string() const noexcept;
    virtual void write(Stream &stream, const PdfRenumber *renumber,
		       bool inflate) const noexcept;
    inline String value() const noexcept { return iValue; }
    String decode() const noexcept;
  private:
    bool iBinary;
    String iValue;
  };

  class PdfName : public PdfObj {
  public:
    explicit PdfName(const String &val) : iValue(val) { /* nothing */ }
    virtual const PdfName *name() const noexcept;
    virtual void write(Stream &stream, const PdfRenumber *renumber,
		       bool inflate) const noexcept;
    inline String value() const noexcept { return iValue; }
  private:
    String iValue;
  };

  class PdfRef : public PdfObj {
  public:
  explicit PdfRef(int val) : iValue(val) { /* nothing */ }
    virtual const PdfRef *ref() const noexcept;
    virtual void write(Stream &stream, const PdfRenumber *renumber,
		       bool inflate) const noexcept;
    inline int value() const noexcept { return iValue; }
  private:
    int iValue;
  };

  class PdfArray : public PdfObj {
  public:
    explicit PdfArray() { /* nothing */ }
    ~PdfArray();
    virtual const PdfArray *array() const noexcept;
    virtual void write(Stream &stream, const PdfRenumber *renumber,
		       bool inflate) const noexcept;
    void append(const PdfObj *);
    inline int count() const noexcept { return iObjects.size(); }
    const PdfObj *obj(int index, const PdfFile *file) const noexcept;
  private:
    std::vector<const PdfObj *> iObjects;
  };

  class PdfDict : public PdfObj {
  public:
    explicit PdfDict(): iLateStreamPosition{0} { /* nothing */ }
    ~PdfDict();
    virtual const PdfDict *dict() const noexcept;
    String dictRepr() const noexcept;
    void dictWrite(Stream &stream, const PdfRenumber *renumber,
		   bool inflate, int length) const noexcept;
    virtual void write(Stream &stream, const PdfRenumber *renumber,
		       bool inflate) const noexcept;
    void setStream(const Buffer &stream);
    void add(String key, const PdfObj *obj);
    const PdfObj *get(String key, const PdfFile *file = nullptr) const noexcept;
    const PdfDict *getDict(String key, const PdfFile *file = nullptr) const noexcept;
    const PdfArray *getArray(String key, const PdfFile *file = nullptr) const noexcept;
    String getName(String key, const PdfFile *file = nullptr) const noexcept;
    bool getNumber(String key, double &val, const PdfFile *file = nullptr) const noexcept;
    int getInteger(String key, const PdfFile *file = nullptr) const noexcept;
    bool getNumberArray(String key, const PdfFile *file,
			std::vector<double> &vals) const noexcept;
    inline int count() const noexcept { return iItems.size(); }
    inline String key(int index) const noexcept { return iItems[index].iKey; }
    inline const PdfObj *value(int index) const noexcept {
      return iItems[index].iVal; }
    inline Buffer stream() const noexcept { return iStream; }
    bool deflated() const noexcept;
    Buffer inflate() const noexcept;
    void setLateStream(int pos) noexcept { iLateStreamPosition = pos; }
    int lateStream() const noexcept { return iLateStreamPosition; }
  private:
    struct Item {
      String iKey;
      const PdfObj *iVal;
    };
    std::vector<Item> iItems;
    int iLateStreamPosition;
    Buffer iStream;
  };

  // --------------------------------------------------------------------

  //! A PDF lexical token.
  struct PdfToken {
    //! The various types of tokens.
    enum TToken { EErr, EOp, EName, ENumber, EString, EStringBinary,
		  ETrue, EFalse, ENull,
		  EArrayBg, EArrayEnd, EDictBg, EDictEnd };
    //! The type of this token.
    TToken iType;
    //! The string representing this token.
    String iString;
  };

  class PdfParser {
  public:
    PdfParser(DataSource &source);

    inline void getChar() { iCh = iSource.getChar(); }
    inline bool eos() const noexcept { return (iCh == EOF); }
    inline PdfToken token() const noexcept { return iTok; }

    void getToken();
    PdfObj *getObject(bool lateStream = false);
    PdfObj *getObjectDef(bool lateStream);
    PdfDict *getTrailer();
    void skipXRef();
    std::vector<int> readXRef();

  private:
    void skipWhiteSpace();
    PdfArray *makeArray();
    PdfDict *makeDict(bool lateStream);

  private:
    DataSource &iSource;
    int iCh;
    PdfToken iTok;
  };

  class PdfFile {
  public:
    bool parse(DataSource &source);
    const PdfObj *object(int num) const noexcept;
    const PdfDict *catalog() const noexcept;
    const PdfDict *page(int pno = 0) const noexcept;
    std::unique_ptr<const PdfObj> take(int num);
    //! Return number of pages.
    int countPages() const { return static_cast<int>(iPages.size()); }
    Rect mediaBox(const PdfDict *page) const;
    int findPageFromPageObjectNumber(int objNum) const;
  private:
    bool readPageTree(const PdfObj *ptn = nullptr);
    bool parseFromXRefObj(PdfParser &parser, DataSource &source);
    bool parseSequentially(DataSource &source);
    bool parseObjectStream(const PdfDict *d);
    bool readDelayedStreams(std::vector<int> &delayed, DataSource &source);
  private:
    std::unordered_map<int, std::unique_ptr<const PdfObj>> iObjects;
    std::unique_ptr<const PdfDict> iTrailer;
    std::vector<const PdfDict *> iPages;
    std::vector<int> iPageObjectNumbers;
  };

} // namespace

// --------------------------------------------------------------------
#endif

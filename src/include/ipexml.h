// -*- C++ -*-
// --------------------------------------------------------------------
// XML parsing
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

#ifndef IPEXML_H
#define IPEXML_H

#include "ipebase.h"

#include <map>

// --------------------------------------------------------------------

namespace ipe {

  class XmlAttributes {
  private:
    using Map = std::map<String, String>;
  public:
    //! Iterator for (key, value) pairs.
    typedef Map::const_iterator const_iterator;

    //! Return const iterator for first attribute.
    const_iterator begin() const { return iMap.begin(); }
    //! Return const iterator for end of attributes.
    const_iterator end() const { return iMap.end(); }

    XmlAttributes();
    void clear();
    String operator[](String str) const;
    bool has(String str) const;
    bool has(String str, String &val) const;
    void add(String key, String val);
    //! Set that the tag contains the final /.
    inline void setSlash() { iSlash = true; }
    //! Return whether tag contains the final /.
    inline bool slash() const { return iSlash; }

  private:
    Map iMap;
    bool iSlash;
  };

  class XmlParser {
  public:
    XmlParser(DataSource &source);
    virtual ~XmlParser();

    int parsePosition() const { return iPos; }

    String parseToTag();
    bool parseAttributes(XmlAttributes &attr, bool qm = false);
    bool parsePCDATA(String tag, String &pcdata);

    inline bool isTagChar(int ch) {
      return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z')
	|| ch == '-'; }

    inline void getChar() { iCh = iSource.getChar(); ++iPos; }
    inline bool eos() { return (iCh == EOF); }
    void skipWhitespace();

  protected:
    String parseToTagX();

  protected:
    DataSource &iSource;
    String iTopElement;
    int iCh;  // current character
    int iPos; // position in input stream
  };

} // namespace

// --------------------------------------------------------------------
#endif

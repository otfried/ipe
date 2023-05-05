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

#include "ipexml.h"

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::XmlAttributes
  \ingroup base
  \brief Stores attributes of an XML tag.
*/

//! Constructor for an empty collection.
XmlAttributes::XmlAttributes()
{
  iSlash = false;
}

//! Remove all attributes.
void XmlAttributes::clear()
{
  iSlash = false;
  iMap.clear();
}

//! Return attribute with given key.
/*! Returns an empty string if no attribute with this key exists. */
String XmlAttributes::operator[](String str) const
{
  std::map<String, String>::const_iterator it = iMap.find(str);
  if (it == iMap.end())
    return String();
  else
    return it->second;
}

//! Add a new attribute.
void XmlAttributes::add(String key, String val)
{
  iMap[key] = val;
}

//! Check whether attribute exists, set \c val if so.
bool XmlAttributes::has(String str, String &val) const
{
  std::map<String, String>::const_iterator it = iMap.find(str);
  if (it != iMap.end()) {
    val = it->second;
    return true;
  }
  return false;
}

//! Check whether attribute exists.
bool XmlAttributes::has(String str) const
{
  return (iMap.find(str) != iMap.end());
}

// --------------------------------------------------------------------

/*! \class ipe::XmlParser
 * \ingroup base
 * \brief Base class for XML stream parsing.

 This is the base class for Ipe's XML parser. It only provides some
 utility functions for parsing tags and PCDATA.  Derived classes
 implement the actual parsing using recursive descent parsers---after
 experimenting with various schemes for XML parsing, this seems to
 work best for Ipe.

 Tag names and attribute names must consist of ASCII letters only.
 Only entities for '&', '<', and '>' are recognized.

*/

//! Construct with a data source.
XmlParser::XmlParser(DataSource &source)
  : iSource(source)
{
  iPos = 0;
  getChar(); // init iCh
}

//! Virtual destructor, so one can destroy through pointer.
XmlParser::~XmlParser()
{
  // nothing
}

void XmlParser::skipWhitespace()
{
  while (iCh <= ' ' && !eos())
    getChar();
}

//! Parse whitespace and the name of a tag.
/*! If the tag is a closing tag, skips > and returns with stream after that.
  Otherwise, returns with stream just after the tag name.

  Comments and <!TAG .. > are skipped silently.
*/

String XmlParser::parseToTagX()
{
  bool comment_found;
  do {
    skipWhitespace();
    if (iCh != '<')
      return String();
    getChar();
    // <!DOCTYPE ... >
    // <!-- comment -->
    comment_found = (iCh == '!');
    if (comment_found) {
      getChar();
      if (iCh == '-') {
	int last[2] = { ' ', ' ' };
	while (!eos() && (iCh != '>' || last[0] != '-' || last[1] != '-')) {
	  last[0] = last[1];
	  last[1] = iCh;
	  getChar();
	}
      } else {
	// skip to end of tag
	while (!eos() && iCh != '>')
	  getChar();
      }
      getChar();
      if (eos())
	return String();
    }
  } while (comment_found);
  String tagname;
  if (iCh == '?' || iCh == '/') {
    tagname += char(iCh);
    getChar();
  }
  while (isTagChar(iCh)) {
    tagname += char(iCh);
    getChar();
  }
  if (tagname[0] == '/') {
    skipWhitespace();
    if (iCh != '>')
      return String();
    getChar();
  }
  // ipeDebug("<%s>", tagname.z());
  return tagname;
}

//! Parse whitespace and the name of a tag.
/*! Like ParseToTagX, but silently skips over all tags whose name
  starts with "x-" */
String XmlParser::parseToTag()
{
  for (;;) {
    String s = parseToTagX();
    if (s.size() < 3 ||
	(s[0] != '/' && (s[0] != 'x' || s[1] != '-')) ||
	(s[0] == '/' && (s[1] != 'x' || s[2] != '-')))
      return s;
    if (s[0] != '/') {
      XmlAttributes attr;
      if (!parseAttributes(attr))
	return String();
    }
  }
}

static String fromXml(String source)
{
  String s;
  for (int i = 0; i < source.size(); ) {
    if (source[i] == '&') {
      int j = i;
      while (j < source.size() && source[j] != ';')
	++j;
      String ent(source, i + 1, j - i - 1);
      char ent1 = 0;
      if (ent == "amp")
	ent1 = '&';
      else if (ent == "lt")
	ent1 = '<';
      else if (ent == "gt")
	ent1 = '>';
      else if (ent == "quot")
	ent1 = '"';
      else if (ent == "apos")
	ent1 = '\'';
      if (ent1) {
	s += ent1;
	i = j+1;
	continue;
      }
      // entity not found: copy normally
    }
    s += source[i++];
  }
  return s;
}

//! Parse XML attributes.
/*! Returns with stream just after \>.  Caller can check whether the
  tag ended with a / by checking attr.slash().

  Set \a qm to true to allow a question mark just before the \>.
*/
bool XmlParser::parseAttributes(XmlAttributes &attr, bool qm)
{
  // looking at char after tagname
  attr.clear();
  skipWhitespace();
  while (iCh != '>' && iCh != '/' && iCh != '?') {
    String attname;
    while (isTagChar(iCh)) {
      attname += char(iCh);
      getChar();
    }
    // XML allows whitespace before and after the '='
    skipWhitespace();
    if (attname.empty() || iCh != '=')
      return false;
    getChar();
    skipWhitespace();
    // XML allows double or single quotes
    int quote = iCh;
    if (iCh != '\"' && iCh != '\'')
      return false;
    getChar();
    String val;
    while (!eos() && iCh != quote) {
      val += char(iCh);
      getChar();
    }
    if (iCh != quote)
      return false;
    getChar();
    skipWhitespace();
    attr.add(attname, fromXml(val));
  }
  // looking at '/' or '>' (or '?' in <?xml> tag)
  if (iCh == '/' || (qm && iCh == '?')) {
    attr.setSlash();
    getChar();
    skipWhitespace();
  }
  // looking at '>'
  if (iCh != '>')
    return false;
  getChar();
  return true;
}

//! Parse PCDATA.
/*! Checks whether the data is terminated by \c \</tag\>, and returns
  with stream past the \>. */
bool XmlParser::parsePCDATA(String tag, String &pcdata)
{
  String s;
  bool haveEntity = false;
  for (;;) {
    if (eos())
      return false;
    if (iCh == '<') {
      getChar();
      if (iCh != '/')
	return false;
      getChar();
      for (int i = 0; i < tag.size(); i++) {
	if (iCh != tag[i])
	  return false;
	getChar();
      }
      skipWhitespace();
      if (iCh != '>')
	return false;
      getChar();
      if (haveEntity)
	pcdata = fromXml(s);
      else
	pcdata = s;
      return true;
    } else {
      if (iCh == '&')
	haveEntity = true;
      s += char(iCh);
    }
    getChar();
  }
}

// --------------------------------------------------------------------

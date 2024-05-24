// --------------------------------------------------------------------
// PDF parsing
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

#include "ipepdfparser.h"
#include "ipeutils.h"
#include <cstdlib>

using namespace ipe;

//------------------------------------------------------------------------

// A '1' in this array means the character is white space.
// A '1' or '2' means the character ends a name or command.
// '2' == () {} [] <> / %
static char specialChars[256] = {
  1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0,   // 0x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 1x
  1, 0, 0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2,   // 2x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0,   // 3x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 4x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0,   // 5x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 6x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0,   // 7x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 8x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 9x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // ax
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // bx
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // cx
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // dx
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // ex
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // fx
};

// --------------------------------------------------------------------

inline int toInt(String &s)
{
  return std::strtol(s.z(), nullptr, 10);
}

// --------------------------------------------------------------------

/*! \class ipe::PdfObj
 * \ingroup base
 * \brief Abstract base class for PDF objects.
 */

//! Pure virtual destructor.
PdfObj::~PdfObj()
{
  // nothing
}

//! Return this object as PDF null object.
const PdfNull *PdfObj::null() const noexcept { return nullptr; }

//! Return this object as PDF bool object.
const PdfBool *PdfObj::boolean() const noexcept { return nullptr; }

//! Return this object as PDF number object.
const PdfNumber *PdfObj::number() const noexcept { return nullptr; }

//! Return this object as PDF string object.
const PdfString *PdfObj::string() const noexcept { return nullptr; }

//! Return this object as PDF name object.
const PdfName *PdfObj::name() const noexcept { return nullptr; }

//! Return this object as PDF reference object.
const PdfRef *PdfObj::ref() const noexcept { return nullptr; }

//! Return this object as PDF array object.
const PdfArray *PdfObj::array() const noexcept { return nullptr; }

//! Return this object as PDF dictionary object.
const PdfDict *PdfObj::dict() const noexcept { return nullptr; }

//! Return PDF representation of the object.
String PdfObj::repr() const noexcept
{
  String d;
  StringStream ss(d);
  write(ss);
  return d;
}

/*! \class ipe::PdfNull
 * \ingroup base
 * \brief The PDF null object.
 */
const PdfNull *PdfNull::null() const noexcept { return this; }

void PdfNull::write(Stream &stream, const PdfRenumber *, bool infl) const noexcept
{
  stream << "null";
}

/*! \class ipe::PdfBool
 * \ingroup base
 * \brief The PDF bool object.
 */
const PdfBool *PdfBool::boolean() const noexcept { return this; }

void PdfBool::write(Stream &stream, const PdfRenumber *, bool infl) const noexcept
{
  stream << (iValue ? "true" : "false");
}

/*! \class ipe::PdfNumber
 * \ingroup base
 * \brief The PDF number object.
 */
const PdfNumber *PdfNumber::number() const noexcept { return this; }

void PdfNumber::write(Stream &stream, const PdfRenumber *, bool infl) const noexcept
{
  stream << iValue;
}

/*! \class ipe::PdfString
 * \ingroup base
 * \brief The PDF string object.
 */
const PdfString *PdfString::string() const noexcept { return this; }

void PdfString::write(Stream &stream, const PdfRenumber *, bool infl)
  const noexcept
{
  if (iBinary) {
    stream << "<" << iValue << ">";
  } else {
    char octbuf[5];
    stream << "(";
    for (int i = 0; i < iValue.size(); ++i) {
      int ch = iValue[i];
      if ((0 <= ch && ch < 0x20) || ch == '\\' || ch == '(' || ch == ')') {
	sprintf(octbuf, "\\%.3o", (ch & 0xff));
	stream << octbuf;
      } else
	stream.putChar(ch);
    }
    stream << ")";
  }
}

//! Return value of string after decoding binary strings.
String PdfString::decode() const noexcept
{
  if (!iBinary && iValue.hasPrefix("\376\377")) {
    String result;
    int i = 2;
    while (i + 1 < iValue.size()) {
      int ch = iValue[i++] << 8;
      ch |= iValue[i++];
      result.appendUtf8(ch);
    }
    return result;
  }
  if (!iBinary)
    return iValue;
  String result;
  Lex lex(iValue);
  if (iValue.hasPrefix("FEFF")) {
    lex.getHexByte(); // skip unicode mark
    lex.getHexByte();
    while (!lex.eos()) {
      int ch = lex.getHexByte() << 8;
      ch |= lex.getHexByte();
      result.appendUtf8(ch);
    }
  } else {
    while (!lex.eos())
      result += char(lex.getHexByte());
  }
  return result;
}

/*! \class ipe::PdfName
 * \ingroup base
 * \brief The PDF name object.
 */
const PdfName *PdfName::name() const noexcept { return this; }

void PdfName::write(Stream &stream, const PdfRenumber *, bool infl) const noexcept
{
  stream << "/" << iValue;
}

/*! \class ipe::PdfRef
 * \ingroup base
 * \brief The PDF reference object (indirect object).
 */
const PdfRef *PdfRef::ref() const noexcept { return this; }

void PdfRef::write(Stream &stream, const PdfRenumber *renumber,
		   bool infl) const noexcept
{
  if (renumber) {
    auto it = renumber->find(iValue);
    if (it != renumber->end()) {
      stream << it->second << " 0 R";
      return;
    }
  }
  stream << iValue << " 0 R";
}

/*! \class ipe::PdfArray
 * \ingroup base
 * \brief The PDF array object.
 */
const PdfArray *PdfArray::array() const noexcept { return this; }

void PdfArray::write(Stream &stream, const PdfRenumber *renumber,
		     bool infl) const noexcept
{
  stream << "[";
  String sep = "";
  for (int i = 0; i < count(); ++i) {
    stream << sep;
    sep = " ";
    obj(i, nullptr)->write(stream, renumber);
  }
  stream << "]";
}

PdfArray::~PdfArray()
{
  for (std::vector<const PdfObj *>::iterator it = iObjects.begin();
       it != iObjects.end(); ++it) {
    delete *it;
    *it = nullptr;
  }
}

//! Append an object to array.
/*! Array takes ownership of the object. */
void PdfArray::append(const PdfObj *obj)
{
  iObjects.push_back(obj);
}

//! Return object with \a index in array.
/*! Indirect objects (references) are looked up if \a file is not
  nullptr, and the object referred to is returned (nullptr if it does not
  exist).  Object remains owned by array.
*/
const PdfObj *PdfArray::obj(int index, const PdfFile *file) const noexcept
{
  const PdfObj *obj = iObjects[index];
  if (file && obj->ref()) {
    int n = obj->ref()->value();
    return file->object(n);
  }
  return obj;
}

/*! \class ipe::PdfDict
 * \ingroup base
 * \brief The PDF dictionary and stream objects.

 A dictionary may or may not have attached stream data.
 */

const PdfDict *PdfDict::dict() const noexcept { return this; }

//! Return PDF representation of the PdfDict without the stream.
String PdfDict::dictRepr() const noexcept
{
  String d;
  StringStream ss(d);
  dictWrite(ss, nullptr, false, iStream.size());
  return d;
}

void PdfDict::dictWrite(Stream &stream,
			const PdfRenumber *renumber,
			bool infl, int length) const noexcept
{
  stream << "<<";
  for (std::vector<Item>::const_iterator it = iItems.begin();
       it != iItems.end(); ++it) {
    if (it != iItems.begin())
      stream << " ";
    if (infl && it->iKey == "Filter" && it->iVal->name()
	&& it->iVal->name()->value() == "FlateDecode")
      continue;   // remove filter and inflate stream
    stream << "/" << it->iKey << " ";
    if (it->iKey == "Length")
      stream << length;
    else
      it->iVal->write(stream, renumber);
  }
  stream << ">>";
}

void PdfDict::write(Stream &stream, const PdfRenumber *renumber,
		    bool infl) const noexcept
{
  Buffer s = infl ? inflate() : iStream;
  dictWrite(stream, renumber, infl, s.size());
  if (s.size() > 0) {
    stream << "\nstream\n";
    for (int i = 0; i < s.size(); ++i)
      stream.putChar(s[i]);
    stream << "\nendstream";
  }
}

PdfDict::~PdfDict()
{
  for (std::vector<Item>::iterator it = iItems.begin();
       it != iItems.end(); ++it) {
    delete it->iVal;
    it->iVal = nullptr;
  }
}

//! Add stream data to this dictionary.
void PdfDict::setStream(const Buffer &stream)
{
  iStream = stream;
}

//! Add a (key, value) pair to the dictionary.
/*! Dictionary takes ownership of \a obj. */
void PdfDict::add(String key, const PdfObj *obj)
{
  Item item;
  item.iKey = key;
  item.iVal = obj;
  iItems.push_back(item);
}

//! Look up key in dictionary.
/*! Indirect objects (references) are looked up if \a file is not nullptr,
  and the object referred to is returned.
  Returns nullptr if key is not in dictionary.
*/
const PdfObj *PdfDict::get(String key, const PdfFile *file) const noexcept
{
  for (std::vector<Item>::const_iterator it = iItems.begin();
       it != iItems.end(); ++it) {
    if (it->iKey == key) {
      if (file && it->iVal->ref())
	return file->object(it->iVal->ref()->value());
      else
	return it->iVal;
    }
  }
  return nullptr; // not in dictionary
}

//! Look up key and return if it is a dictionary.
const PdfDict *PdfDict::getDict(String key, const PdfFile *file) const noexcept
{
  const PdfObj *obj = get(key, file);
  if (obj && obj->dict())
    return obj->dict();
  return nullptr;
}

//! Look up key and return if it is an array.
const PdfArray *PdfDict::getArray(String key, const PdfFile *file) const noexcept
{
  const PdfObj *obj = get(key, file);
  if (obj && obj->array())
    return obj->array();
  return nullptr;
}

//! Look up key and return its value if it is a /Name, otherwise return empty string.
String PdfDict::getName(String key, const PdfFile *file) const noexcept
{
  const PdfObj *obj = get(key, file);
  if (obj && obj->name())
    return obj->name()->value();
  return String();
}

//! Retrieve a single number and stor in \a val.
bool PdfDict::getNumber(String key, double &val, const PdfFile *file)
  const noexcept
{
  const PdfObj *obj = get(key, file);
  if (!obj || !obj->number())
    return false;
  val = obj->number()->value();
  return true;
}

//! Retrieve a single integer.
/*! Returns -1 on failure. */
int PdfDict::getInteger(String key, const PdfFile *file) const noexcept
{
  double val;
  if (!getNumber(key, val, file))
    return -1;
  return int(val);
}

//! Retrieve an array of numbers and store in \a vals.
bool PdfDict::getNumberArray(String key, const PdfFile *file,
			     std::vector<double> &vals) const noexcept
{
  const PdfObj *obj = get(key, file);
  if (!obj || !obj->array())
    return false;
  vals.clear();
  for (int i = 0; i < obj->array()->count(); i++) {
    const PdfObj *a = obj->array()->obj(i, file);
    if (!a || !a->number())
      return false;
    vals.push_back(a->number()->value());
  }
  return true;
}

//! Is this stream compressed with flate compression?
bool PdfDict::deflated() const noexcept
{
  const PdfObj *f = get("Filter", nullptr);
  if (!f) return false;
  if (f->array()) {
    const PdfArray *a = f->array();
    if (a->count() != 1) return false;
    f = a->obj(0, nullptr);
  }
  return f->name() && f->name()->value() == "FlateDecode";
}

//! Return the (uncompressed) stream data.
/*! This only handles the /Flate compression. */
Buffer PdfDict::inflate() const noexcept
{
  if (iStream.size() == 0 || !deflated())
    return iStream;

  String dest;
  BufferSource bsource(iStream);
  InflateSource source(bsource);

  int ch = source.getChar();
  while (ch != EOF) {
    dest += char(ch);
    ch = source.getChar();
  }
  return Buffer(dest.data(), dest.size());
}

// --------------------------------------------------------------------

/*! \class ipe::PdfParser
 * \ingroup base
 * \brief PDF parser

 The parser understands the syntax of PDF files, but very little of
 its semantics.  It is meant to be able to parse PDF documents created
 by Ipe for loading, and to extract information from PDF files created
 by Pdflatex or Xelatex.

 The parser reads a PDF file sequentially from front to back, ignores
 the contents of 'xref' sections, stores only generation 0 objects,
 and stops after reading the first 'trailer' section (so it cannot
 deal with files with incremental updates).  It cannot handle stream
 objects whose /Length entry has been deferred (using an indirect
 object).

*/

//! Construct with a data source.
PdfParser::PdfParser(DataSource &source)
  : iSource(source)
{
  getChar();  // init iCh
  getToken(); // init iTok
}

//! Skip white space and comments.
void PdfParser::skipWhiteSpace()
{
  while (!eos() && (specialChars[iCh] == 1 || iCh == '%')) {
    // handle comment
    if (iCh == '%') {
      while (!eos() && iCh != '\n' && iCh != '\r')
	getChar();
    }
    getChar();
  }
}

//! Read the next token from the input stream.
void PdfParser::getToken()
{
  iTok.iString.erase();
  iTok.iType = PdfToken::EErr;
  skipWhiteSpace();
  if (eos())
    return; // Err

  // parse string
  if (iCh == '(') {
    int nest = 0;
    getChar();
    while (iCh != ')' || nest > 0) {
      if (eos())
	return; // Err
      if (iCh == '\\') {
	getChar();
	if ('0' <= iCh && iCh <= '9') {
	  // octal char code
	  char buf[4];
	  int i = 0;
	  buf[i++] = char(iCh);
	  getChar();
	  if ('0' <= iCh && iCh <= '9') {
	    buf[i++] = char(iCh);
	    getChar();
	  }
	  if ('0' <= iCh && iCh <= '9') {
	    buf[i++] = char(iCh);
	    getChar();
	  }
	  buf[i] = '\0';
	  iTok.iString.append(char(std::strtol(buf, nullptr, 8)));
	} else {
	  iTok.iString.append(char(iCh));
	  getChar();
	}
      } else {
	if (iCh == '(')
	  ++nest;
	else if (iCh == ')')
	  --nest;
	iTok.iString.append(char(iCh));
	getChar();
      }
    }
    getChar(); // skip closing ')'
    iTok.iType = PdfToken::EString;
    return;
  }

  if (iCh == '<') {
    getChar();
    // recognize dictionary separator "<<"
    if (iCh == '<') {
      getChar();
      iTok.iType = PdfToken::EDictBg;
      return;
    }
    // otherwise it's a binary string <hex>
    while (iCh != '>') {
      if (eos())
	return; // Err
      iTok.iString.append(char(iCh));
      getChar();
    }
    // We don't bother to decode it
    getChar(); // skip '>'
    iTok.iType = PdfToken::EStringBinary;
    return;
  }

  int ch = iCh;

  iTok.iString.append(char(iCh));
  getChar();

  // recognize array separators
  if (ch == '[') {
    iTok.iType = PdfToken::EArrayBg;
    return;
  } else if (ch == ']') {
    iTok.iType = PdfToken::EArrayEnd;
    return;
  }

  // recognize dictionary separator ">>"
  if (ch == '>') {
    if (iCh != '>')
      return; // Err
    getChar();
    iTok.iType = PdfToken::EDictEnd;
    return;
  }

  // collect all characters up to white-space or separator
  while (!specialChars[iCh]) {
    if (eos())
      return; // Err
    iTok.iString.append(char(iCh));
    getChar();
  }

  if (('0' <= ch && ch <= '9') || ch == '+' || ch == '-' || ch == '.')
    iTok.iType = PdfToken::ENumber;
  else if (ch == '/')
    iTok.iType = PdfToken::EName;
  else if (iTok.iString == "null")
    iTok.iType = PdfToken::ENull;
  else if (iTok.iString == "true")
    iTok.iType = PdfToken::ETrue;
  else if (iTok.iString == "false")
    iTok.iType = PdfToken::EFalse;
  else
    iTok.iType = PdfToken::EOp;
}

// --------------------------------------------------------------------

//! Parse elements of an array.
PdfArray *PdfParser::makeArray()
{
  std::unique_ptr<PdfArray> arr(new PdfArray);
  for (;;) {
    if (iTok.iType == PdfToken::EArrayEnd) {
      // finish array
      getToken();
      return arr.release();
    }
    // check for reference object
    if (iTok.iType == PdfToken::ENumber) {
      PdfToken t1 = iTok;
      getToken();
      if (iTok.iType == PdfToken::ENumber) {
	PdfToken t2 = iTok;
	getToken();
	if (iTok.iType == PdfToken::EOp && iTok.iString == "R") {
	  arr->append(new PdfRef(toInt(t1.iString)));
	  getToken();
	} else {
	  arr->append(new PdfNumber(Platform::toDouble(t1.iString)));
	  arr->append(new PdfNumber(Platform::toDouble(t2.iString)));
	}
      } else {
	arr->append(new PdfNumber(Platform::toDouble(t1.iString)));
      }
    } else {
      PdfObj *obj = getObject();
      if (!obj)
	return nullptr;
      arr->append(obj);
    }
  }
}

PdfDict *PdfParser::makeDict(bool lateStream)
{
  std::unique_ptr<PdfDict> dict(new PdfDict);
  for (;;) {
    if (iTok.iType == PdfToken::EDictEnd) {
      // finish
      getToken();

      // check whether stream follows
      if (iTok.iType != PdfToken::EOp || iTok.iString != "stream")
	return dict.release();

      // time to read the stream
      while (!eos() && iCh != '\n')
	getChar();
      int streamPos = iSource.position();
      getChar(); // skip '\n'
      // now at beginning of stream
      const PdfObj *len = dict->get("Length");
      if (len && len->ref()) {
	if (lateStream) {
	  dict->setLateStream(streamPos);
	  return dict.release();
	} else {
	  ipeDebug("/Length entry of dictionary is a reference");
	  return nullptr;
	}
      }
      if (!len || !len->number())
	return nullptr;
      int bytes = int(len->number()->value());
      Buffer buf(bytes);
      char *p = buf.data();
      while (bytes--) {
	*p++ = char(iCh);
	getChar();
      }
      dict->setStream(buf);
      getToken();
      if (iTok.iType != PdfToken::EOp || iTok.iString != "endstream")
	return nullptr;
      getToken();
      return dict.release();
    }

    // must read name
    if (iTok.iType != PdfToken::EName)
      return nullptr;
    String name = iTok.iString.substr(1);
    getToken();

    // check for reference object
    if (iTok.iType == PdfToken::ENumber) {
      PdfToken t1 = iTok;
      getToken();
      if (iTok.iType == PdfToken::ENumber) {
	PdfToken t2 = iTok;
	getToken();
	if (iTok.iType == PdfToken::EOp && iTok.iString == "R") {
	  dict->add(name, new PdfRef(toInt(t1.iString)));
	  getToken();
	} else
	  return nullptr; // should be name or '>>'
      } else
	dict->add(name, new PdfNumber(Platform::toDouble(t1.iString)));
    } else {
      PdfObj *obj = getObject();
      if (!obj)
	return nullptr;
      dict->add(name, obj);
    }
  }
}

//! Read one object from input stream.
PdfObj *PdfParser::getObject(bool lateStream)
{
  PdfToken tok = iTok;
  getToken();

  switch (tok.iType) {
  case PdfToken::ENumber:
    return new PdfNumber(Platform::toDouble(tok.iString));
  case PdfToken::EString:
    return new PdfString(tok.iString);
  case PdfToken::EStringBinary:
    return new PdfString(tok.iString, true);
  case PdfToken::EName:
    return new PdfName(tok.iString.substr(1));
  case PdfToken::ENull:
    return new PdfNull;
  case PdfToken::ETrue:
    return new PdfBool(true);
  case PdfToken::EFalse:
    return new PdfBool(false);
  case PdfToken::EArrayBg:
    return makeArray();
  case PdfToken::EDictBg:
    return makeDict(lateStream);
    // anything else is an error
  case PdfToken::EErr:
  default:
    return nullptr;
  }
}

//! Parse an object definition (current token is object number).
PdfObj *PdfParser::getObjectDef(bool lateStream)
{
  getToken();
  if (iTok.iType != PdfToken::ENumber || iTok.iString != "0")
    return nullptr;
  getToken();
  if (iTok.iType != PdfToken::EOp || iTok.iString != "obj")
    return nullptr;
  getToken();
  PdfObj *obj = getObject(lateStream);
  if (!obj)
    return nullptr;
  if (obj->dict() && obj->dict()->lateStream() > 0)
    return obj;
  if (iTok.iType != PdfToken::EOp || iTok.iString != "endobj")
    return nullptr;
  getToken();
  return obj;
}

//! Skip xref table (current token is 'xref')
void PdfParser::skipXRef()
{
  getToken(); // first object number
  getToken(); // number of objects
  int k = toInt(iTok.iString);
  getToken();
  while (k--) {
    getToken(); // position
    getToken(); // gen num
    getToken(); // n or f
  }
}

//! Read xref table (current token is 'xref')
std::vector<int> PdfParser::readXRef()
{
  getToken(); // first object number
  getToken(); // number of objects
  int k = toInt(iTok.iString);
  std::vector<int> objects(k, 0); // fill with zeros
  getToken(); // position
  for (int obj = 0; obj < k; ++obj) {
    int pos = toInt(iTok.iString);
    getToken(); // gen num
    getToken(); // n or f
    if (iTok.iString == "n")
      objects[obj] = pos;
    getToken(); // next token
  }
  return objects;
}

//! Parse trailer dictionary (current token is 'trailer')
PdfDict *PdfParser::getTrailer()
{
  getToken();
  if (iTok.iType != PdfToken::EDictBg)
    return nullptr;
  getToken();
  return makeDict(false);
}

// --------------------------------------------------------------------

static bool addStreamToDict(DataSource &source, PdfDict *d, const PdfFile *file)
{
  int pos = d->lateStream();
  if (pos == 0)
    return true;
  source.setPosition(pos);

  int bytes = d->getInteger("Length", file);
  if (bytes < 0)
    return false;

  Buffer buf(bytes);
  char *p = buf.data();
  while (bytes--) {
    *p++ = source.getChar();
  }
  d->setStream(buf);
  d->setLateStream(0);
  PdfParser parser(source);
  PdfToken t = parser.token();
  return (t.iType == PdfToken::EOp && t.iString == "endstream");
}

static int readBytes(DataSource &source, int n)
{
  int value = 0;
  while (n--) {
    int ch = uint8_t(source.getChar());
    value = value * 256 + ch;
  }
  return value;
}

// return size or -1
static int checkXRefObj(const PdfDict *d)
{
  const PdfObj *type = d->get("Type");
  if (!type || !type->name() || type->name()->value() != "XRef")
    return -1;
  int size = d->getInteger("Size");
  if (size < 0)
    return -1;
  const PdfObj *indexObj = d->get("Index");
  if (indexObj) {
    const PdfArray *index = indexObj->array();
    if (!index || index->count() != 2) return false;
    const PdfObj *val = index->obj(0, nullptr);
    if (!val->number() || val->number()->value() != 0.0) return -1;
    val = index->obj(1, nullptr);
    if (!val->number() || val->number()->value() != size) return -1;
  }
  return size;
}

// --------------------------------------------------------------------

/*! \class ipe::PdfFile
 * \ingroup base
 * \brief All information obtained by parsing a PDF file.
 */

//! Parse entire PDF stream, and store objects.
bool PdfFile::parse(DataSource &source)
{
  int length = source.length();
  if (length < 0)
    // could not seek to end
    return parseSequentially(source);

  if (length < 400)
    return false;

  source.setPosition(length - 40);
  String s;
  int ch;
  while ((ch = source.getChar()) != EOF)
    s += ch;
  int i = s.find("startxref");
  if (i < 0)
    return parseSequentially(source);

  Lex lex(s.substr(i+9));
  int xrefPos = lex.getInt();
  source.setPosition(xrefPos);

  PdfParser parser(source);
  PdfToken t = parser.token();

  if (t.iType == PdfToken::ENumber)
    // seems to be the /XRef object
    return parseFromXRefObj(parser, source);

  if (t.iType != PdfToken::EOp || t.iString != "xref")
    return parseSequentially(source);

  std::vector<int> xref = parser.readXRef();
  t = parser.token();

  if (t.iType != PdfToken::EOp || t.iString != "trailer")
    return false;

  iTrailer = std::unique_ptr<const PdfDict>(parser.getTrailer());
  if (!iTrailer)
    return false;

  std::vector<int> delayed;
  // read objects
  for (int num = 0; num < size(xref); ++num) {
    if (xref[num] > 0) {
      source.setPosition(xref[num]);
      PdfParser objParser(source);
      std::unique_ptr<const PdfObj> obj(objParser.getObjectDef(true));
      if (!obj) {
	ipeDebug("Failed to get object %d", num);
	return false;
      }
      if (obj->dict() && obj->dict()->lateStream() > 0)
	delayed.push_back(num);
      // ipeDebug("Object: %s", obj->repr().z());
      iObjects[num] = std::move(obj);
    }
  }
  return readDelayedStreams(delayed, source);
}

// read all the streams we had to postpone
bool PdfFile::readDelayedStreams(std::vector<int> &delayed, DataSource &source)
{
  for (int num : delayed) {
    const PdfDict *d = iObjects[num]->dict();
    if (!addStreamToDict(source, (PdfDict *) d, this)) {
      ipeDebug("Failed to read stream for object %d", num);
      return false;
    }
  }
  return readPageTree();
}

// ------------------------------------------------------------------------------------------

bool PdfFile::parseFromXRefObj(PdfParser &parser, DataSource &source)
{
  std::unique_ptr<const PdfObj> obj(parser.getObjectDef(false));
  if (!obj)
    return false;
  iTrailer = std::unique_ptr<const PdfDict>(obj.release()->dict());
  int size = checkXRefObj(iTrailer.get());
  std::vector<double> w;

  if (size < 0 || !iTrailer->getNumberArray("W", nullptr, w) || w.size() != 3)
    return parseSequentially(source);

  std::vector<int> delayed;

  Buffer stream = iTrailer->inflate();
  BufferSource xb(stream);
  for (int num = 0; num < size; ++num) {
    int objType = readBytes(xb, int(w[0]));
    int pos = readBytes(xb, int(w[1]));
    readBytes(xb, int(w[2]));  // not used
    if (objType == 1) {
      source.setPosition(pos);
      PdfParser objParser(source);
      std::unique_ptr<const PdfObj> obj(objParser.getObjectDef(true));
      if (!obj) {
	ipeDebug("Failed to get object %d from XRef object", num);
	return false;
      }
      const PdfDict *d = obj->dict();
      const PdfObj *type = d ? d->get("Type", this) : nullptr;
      if (type && type->name() && type->name()->value() == "ObjStm") {
	if (!parseObjectStream(d))
	  return false;
      } else {
	// ipeDebug("Object: %s", obj->repr().z());
	if (obj->dict() && obj->dict()->lateStream() > 0)
	  delayed.push_back(num);
	iObjects[num] = std::move(obj);
      }
    }
  }
  return readDelayedStreams(delayed, source);
}

// ------------------------------------------------------------------------------------------

// Read PDF file from front to back, collecting objects
// Used when startxref access and /XRef object access fails
bool PdfFile::parseSequentially(DataSource &source)
{
  ipeDebug("Falling back on sequential PDF parser");
  source.setPosition(0);
  PdfParser parser(source);

  for (;;) {
    PdfToken t = parser.token();

    if (t.iType == PdfToken::ENumber) {
      // <num> 0 obj starts an object
      int num = toInt(t.iString);
      std::unique_ptr<const PdfObj> obj(parser.getObjectDef(false));
      if (!obj) {
	ipeDebug("Failed to get object %d in sequential reader", num);
	return false;
      }
      const PdfDict *d = obj->dict();
      const PdfObj *type = d ? d->get("Type", this) : nullptr;
      if (type && type->name() && type->name()->value() == "ObjStm") {
	if (!parseObjectStream(d))
	  return false;
      } else if (type && type->name() && type->name()->value() == "XRef") {
	iTrailer = std::unique_ptr<const PdfDict>(obj.release()->dict());
      } else {
	// ipeDebug("Object: %s", obj->repr().z());
	iObjects[num] = std::move(obj);
      }
    } else if (t.iType == PdfToken::EOp) {
      if (t.iString == "trailer") {
	iTrailer = std::unique_ptr<const PdfDict>(parser.getTrailer());
	if (!iTrailer) {
	  ipeDebug("Failed to get trailer");
	  return false;
	}
	return readPageTree();
      } else if (t.iString == "xref") {
	parser.skipXRef();
      } else if (t.iString == "startxref") {
	return readPageTree(); // this is the end
      } else {
	ipeDebug("Weird token: %s", t.iString.z());
	// don't know what's happening
	return false;
      }
    } else {
      ipeDebug("Weird token type: %d %s", t.iType, t.iString.z());
      // don't know what's happening
      return false;
    }
  }
}

// ------------------------------------------------------------------------------------------

bool PdfFile::parseObjectStream(const PdfDict *d)
{
  const PdfObj *objn = d->get("N", this);
  const PdfObj *objfirst = d->get("First", this);
  int n = objn->number() ? objn->number()->value() : -1;
  int first = objfirst->number() ? objfirst->number()->value() : -1;
  if (n < 0 || first < 0)
    return false;
  Buffer stream = d->inflate();
  BufferSource source(stream);
  PdfParser parser(source);
  std::vector<int> dir;
  for (int i = 0; i < 2 * n; ++i) {
    PdfToken t = parser.token();
    if (t.iType != PdfToken::ENumber)
      return false;
    dir.push_back(toInt(t.iString));
    parser.getToken();
  }
  for (int i = 0; i < n; ++i) {
    int num = dir[2*i];
    source.setPosition(first + dir[2*i+1]);
    parser.getChar();
    parser.getToken();
    PdfObj *obj = parser.getObject();
    if (!obj)
      return false;
    // ipeDebug("Object: %s", obj->repr().z());
    iObjects[num] = std::unique_ptr<const PdfObj>(obj);
  }
  return true;
}

// ------------------------------------------------------------------------------------------

//! Return object with number \a num.
const PdfObj *PdfFile::object(int num) const noexcept
{
  auto got = iObjects.find(num);
  if (got != iObjects.end())
    return got->second.get();
  else
    return nullptr;
}

//! Take ownership of object with number \a num, remove from PdfFile.
std::unique_ptr<const PdfObj> PdfFile::take(int num)
{
  auto got = iObjects.find(num);
  if (got != iObjects.end()) {
    std::unique_ptr<const PdfObj> obj = std::move(got->second);
    iObjects.erase(got);
    return obj;
  } else
    return std::unique_ptr<const PdfObj>();
}

//! Return root catalog of PDF file.
const PdfDict *PdfFile::catalog() const noexcept
{
  const PdfObj *root = iTrailer->get("Root", this);
  assert(root && root->dict());
  return root->dict();
}

bool PdfFile::readPageTree(const PdfObj *ptn)
{
  if (ptn == nullptr)
    ptn = catalog()->get("Pages", this);
  if (!ptn || !ptn->dict())
    return false;
  const PdfArray *kids = ptn->dict()->getArray("Kids", this);
  if (!kids)
    return false;
  for (int i = 0; i < kids->count(); ++i) {
    const PdfObj *pageRef = kids->obj(i, nullptr);
    if (!pageRef || !pageRef->ref())
      return false;
    int pageObjNum = pageRef->ref()->value();
    const PdfObj *page = object(pageObjNum);
    if (!page || !page->dict())
      return false;
    String type = page->dict()->getName("Type", this);
    if (type == "Pages")
      readPageTree(page);
    else if (type == "Page") {
      iPages.push_back(page->dict());
      iPageObjectNumbers.push_back(pageObjNum);
    } else
      return false;
  }
  return true;
}

//! Return a page of the document.
const PdfDict *PdfFile::page(int pno) const noexcept
{
  if (pno < 0 || pno >= countPages())
    return nullptr;
  return iPages[pno];
}

//! Return page number given the PDF object number.
/*! Returns -1 if the object number is not a page. */
int PdfFile::findPageFromPageObjectNumber(int objNum) const
{
  for (int i = 0; i < size(iPageObjectNumbers); ++i) {
    if (iPageObjectNumbers[i] == objNum)
      return i;
  }
  return -1;
}

//! Return mediabox of a page.
Rect PdfFile::mediaBox(const PdfDict *pg) const
{
  Rect box;
  std::vector<double> a;
  if (pg && pg->getNumberArray("MediaBox", this, a) && a.size() == 4) {
    box.addPoint(Vector(a[0], a[1]));
    box.addPoint(Vector(a[2], a[3]));
  }
  return box;
}

// --------------------------------------------------------------------

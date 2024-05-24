// --------------------------------------------------------------------
// Basic classes
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

#include "ipebase.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

using namespace ipe;

// --------------------------------------------------------------------

/*! \defgroup base Ipe Base
  \brief Basic classes for Ipe.

  Some very basic type definitions, streams, lexical analysis, and XML
  parsing.

  All parts of Ipe make use of the STL.  The C++ I/O streams library
  is not used, as Ipelib doesn't do much I/O.  Ipe objects support
  internalization and externalization through an abstract interface
  based on ipe::Stream's.

  Clients of Ipelib can use any I/O library that implements this
  interface.  Ipe simply uses \c cstdio.
*/

// --------------------------------------------------------------------

/*! \class ipe::String
  \ingroup base
  \brief Strings and buffers.

  String is is an implicitly shared byte string.  It is designed to
  be efficient for strings of arbitrary length, and supposed to be
  passed by value (the size of String is a single pointer).
  Sharing is implicit---the string creates its own representation as
  soon as it is modified.

  String can be used for binary data.  For text, it is usually
  assumed that the string is UTF-8 encoded, but only the unicode
  member function actually requires this. In particular, all indices
  into the string are byte indices, not Unicode character indices.
*/

String::Imp *String::theEmptyString = { nullptr };

String::Imp *String::emptyString() noexcept
{
  if (theEmptyString == nullptr) {
    theEmptyString = new Imp;
    theEmptyString->iRefCount = 10; // always > 1
    theEmptyString->iSize = 0;
    theEmptyString->iCapacity = 0;
    theEmptyString->iData = nullptr;
  }
  // increment every time it's requested to make sure it's never destroyed
  ++theEmptyString->iRefCount;
  return theEmptyString;
}

//! Construct an empty string.
String::String() noexcept
{
  iImp = emptyString();
}

//! Construct a string by making copy of \a str.
String::String(const char *str) noexcept
{
  if (!str || !str[0]) {
    iImp = emptyString();
  } else {
    int len = strlen(str);
    iImp = new Imp;
    iImp->iRefCount = 1;
    iImp->iSize = len;
    iImp->iCapacity = (len + 32) & ~15;
    iImp->iData = new char[iImp->iCapacity];
    memcpy(iImp->iData, str, iImp->iSize);
  }
}

//! Construct string by taking ownership of given \a data.
String String::withData(char *data, int len) noexcept
{
  if (!len)
    len = strlen(data);
  String r; // empty string
  --r.iImp->iRefCount;
  r.iImp = new Imp;
  r.iImp->iRefCount = 1;
  r.iImp->iSize = len;
  r.iImp->iCapacity = len;
  r.iImp->iData = data;
  return r;
}

//! Construct string by making copy of \a str with given \a len.
String::String(const char *str, int len) noexcept
{
  if (!str || !len)
    iImp = emptyString();
  else {
    iImp = new Imp;
    iImp->iRefCount = 1;
    iImp->iSize = len;
    iImp->iCapacity = (len + 32) & ~15;
    iImp->iData = new char[iImp->iCapacity];
    memcpy(iImp->iData, str, iImp->iSize);
  }
}

//! Copy constructor.
//! This only copies the reference and takes constant time.
String::String(const String &rhs) noexcept
{
  iImp = rhs.iImp;
  iImp->iRefCount++;
}

//! Construct a substring.
/*! \a index must be >= 0.  \a len can be negative or too large to
  return entire string. */
String::String(const String &rhs, int index, int len) noexcept
{
  // actually available data
  int len1 = index < rhs.size() ? rhs.size() - index : 0;
  if (len < 0 || len1 < len)
    len = len1;
  if (!len) {
    iImp = emptyString();
  } else {
    iImp = new Imp;
    iImp->iRefCount = 1;
    iImp->iSize = len;
    iImp->iCapacity = (len + 32) & ~15;
    iImp->iData = new char[iImp->iCapacity];
    memcpy(iImp->iData, rhs.iImp->iData + index, len);
  }
}

//! Assignment takes constant time.
String &String::operator=(const String &rhs) noexcept
{
  if (iImp != rhs.iImp) {
    if (iImp->iRefCount == 1) {
      delete [] iImp->iData;
      delete iImp;
    } else
      iImp->iRefCount--;
    iImp = rhs.iImp;
    iImp->iRefCount++;
  }
  return *this;
}

//! Destruct string if reference count has reached zero.
String::~String() noexcept
{
  if (iImp->iRefCount == 1) {
    delete [] iImp->iData;
    delete iImp;
  } else
    iImp->iRefCount--;
}

//! Make a private copy of the string with \a n bytes to spare.
/*! When a private copy has to be made an extra 32 bytes are ensured. */
void String::detach(int n) noexcept
{
  if (iImp == theEmptyString) {
    iImp = new Imp;
    iImp->iRefCount = 1;
    iImp->iSize = 0;
    n = (n + 0x1f) & ~0x1f;
    iImp->iCapacity = (n >= 16) ? n : 16;
    iImp->iData = new char[iImp->iCapacity];
  } else if (iImp->iRefCount > 1 || (iImp->iSize + n > iImp->iCapacity)) {
    Imp *imp = new Imp;
    imp->iRefCount = 1;
    imp->iSize = iImp->iSize;
    imp->iCapacity = iImp->iCapacity;
    while (imp->iSize + 32 + n > imp->iCapacity)
      imp->iCapacity *= 2;
    imp->iData = new char[imp->iCapacity];
    memcpy(imp->iData, iImp->iData, imp->iSize);
    iImp->iRefCount--;
    if (iImp->iRefCount == 0) {
      delete [] iImp->iData;
      delete iImp;
    }
    iImp = imp;
  }
}

//! Return a C style string with final zero byte.
const char *String::z() const noexcept
{
  if (iImp == theEmptyString)
    return "";
  String *This = const_cast<String *>(this);
  if (iImp->iSize == iImp->iCapacity)
    This->detach(1);
  This->iImp->iData[iImp->iSize] = '\0';
  return data();
}

//! Return index of first occurrence of ch.
/*! Return -1 if character does not appear. */
int String::find(char ch) const noexcept
{
  for (int i = 0; i < size(); ++i)
    if (iImp->iData[i] == ch)
      return i;
  return -1;
}

//! Return index of first occurrence of rhs.
/*! Return -1 if not substring is not present. */
int String::find(const char *rhs) const noexcept
{
  int s = strlen(rhs);
  for (int i = 0; i < size() - s; ++i)
    if (::strncmp(iImp->iData + i, rhs, s) == 0)
      return i;
  return -1;
}

//! Return line starting at position \a index.
//! Index is updated to point to next line.
String String::getLine(int &index) const noexcept
{
  int i = index;
  while (i < size() && iImp->iData[i] != '\r' && iImp->iData[i] != '\n')
    ++i;
  String result = substr(index, i-index);
  if (i < size() && iImp->iData[i] == '\r')
    ++i;
  if (i < size() && iImp->iData[i] == '\n')
    ++i;
  index = i;
  return result;
}

//! Return index of last occurrence of ch.
/*! Return -1 if character does not appear. */
int String::rfind(char ch) const noexcept
{
  for (int i = size() - 1; i >= 0; --i)
    if (iImp->iData[i] == ch)
      return i;
  return -1;
}

//! Make string empty.
void String::erase() noexcept
{
  detach(0);
  iImp->iSize = 0;
}

//! Append \a rhs to this string.
void String::append(const String &rhs) noexcept
{
  int n = rhs.size();
  detach(n);
  memcpy(iImp->iData + iImp->iSize, rhs.iImp->iData, n);
  iImp->iSize += n;
}

//! Append \a rhs to this string.
void String::append(const char *rhs) noexcept
{
  int n = strlen(rhs);
  if (n) {
    detach(n);
    memcpy(iImp->iData + iImp->iSize, rhs, n);
    iImp->iSize += n;
  }
}

//! Append \a ch to this string.
void String::append(char ch) noexcept
{
  detach(1);
  iImp->iData[iImp->iSize++] = ch;
}

//! Append a single unicode character \a ch in UTF-8 encoding.
void String::appendUtf8(uint16_t ch) noexcept
{
  if (ch < 0x80) {
    append(char(ch));
  } else if (ch < 0x800) {
    detach(2);
    iImp->iData[iImp->iSize++] = 0xc0 | (ch >> 6);
    iImp->iData[iImp->iSize++] = 0x80 | (ch & 0x3f);
  } else {
    detach(3);
    iImp->iData[iImp->iSize++] = 0xe0 | (ch >> 12);
    iImp->iData[iImp->iSize++] = 0x80 | ((ch >> 6) & 0x3f);
    iImp->iData[iImp->iSize++] = 0x80 | (ch & 0x3f);
  }
}

//! Create substring at the right.
/*! Returns the entire string if \a i is larger than its length. */
String String::right(int i) const noexcept
{
  if (i < size())
    return String(*this, size() - i, i);
  else
    return *this;
}

//! Does string start with this prefix? (bytewise comparison)
bool String::hasPrefix(const char *rhs) const noexcept
{
  int n = strlen(rhs);
  return (size() >= n && !strncmp(iImp->iData, rhs, n));
}

//! Equality operator (bytewise comparison).
bool String::operator==(const String &rhs) const noexcept
{
  return (size() == rhs.size() &&
	  !strncmp(iImp->iData, rhs.iImp->iData, size()));
}

//! Equality operator (bytewise comparison).
bool String::operator==(const char *rhs) const noexcept
{
  int n = strlen(rhs);
  return (size() == n && !strncmp(iImp->iData, rhs, n));
}

//! Inequality operator (bytewise comparison).
bool String::operator<(const String &rhs) const noexcept
{
  int n = size() < rhs.size() ? size() : rhs.size();
  int cmp = ::strncmp(iImp->iData, rhs.iImp->iData, n);
  return (cmp < 0 || (cmp == 0 && size() < rhs.size()));
}

//! Concatenate this string with \a rhs.
String String::operator+(const String &rhs) const noexcept
{
  String s(*this);
  s.append(rhs);
  return s;
}

static const uint8_t bytesFromUTF8[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const uint8_t firstByteMark[7] = {
  0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0
};

//! Return Unicode value from UTF-8 string.
/*! The \a index is incremented to the next UTF-8 character.
  This returns 0xfffd if there is any problem in parsing UTF-8. */
int String::unicode(int &index) const noexcept
{
  int wch = uint8_t(iImp->iData[index++]);
  if ((wch & 0xc0) == 0x80) {
    // not on first byte of a UTF-8 sequence
    while (index < iImp->iSize && (iImp->iData[index] & 0xc0) == 0x80)
      index++;
    return 0xfffd;
  }
  int extraBytes = bytesFromUTF8[wch & 0xff];
  wch -= firstByteMark[extraBytes];
  while (extraBytes--) {
    if (index >= iImp->iSize)
      return 0xfffd; // UTF-8 sequence is incomplete
    if ((iImp->iData[index] & 0xc0) != 0x80)
      return 0xfffd; // UTF-8 sequence is incorrect
    wch <<= 6;
    wch |= (iImp->iData[index++] & 0x3f);
  }
  return wch;
}

// --------------------------------------------------------------------

/*! \class ipe::Fixed
  \ingroup base
  \brief Fixed point number with three (decimal) fractional digits
*/

//! Return value times (a/b)
Fixed Fixed::mult(int a, int b) const
{
  return Fixed::fromInternal(iValue * a / b);
}

Fixed Fixed::fromDouble(double val)
{
  return Fixed::fromInternal(int(val * 1000 + 0.5));
}

namespace ipe {
  /*! \relates Fixed */
  Stream &operator<<(Stream &stream, const Fixed &f)
  {
    stream << (f.iValue / 1000);
    if (f.iValue % 1000) {
      stream << "." << ((f.iValue / 100) % 10);
      if (f.iValue % 100) {
	stream << ((f.iValue / 10) % 10);
	if (f.iValue % 10)
	  stream << (f.iValue % 10);
      }
    }
    return stream;
  }
}

// --------------------------------------------------------------------

/*! \class ipe::Lex
  \ingroup base
  \brief Lexical analyser. Seeded with a string.
*/

//! Construct lexical analyzer from a string.
Lex::Lex(String str) : iString(str), iPos(0)
{
  // nothing
}

//! Return NextToken, but without extracting it.
String Lex::token()
{
  int pos = iPos;
  String str = nextToken();
  iPos = pos;
  return str;
}

//! Extract next token.
/*! Skips any whitespace before the token.
  Returns empty string if end of string is reached. */
String Lex::nextToken()
{
  skipWhitespace();
  int mark = iPos;
  while (!(eos() || uint8_t(iString[iPos]) <= ' '))
    ++iPos;
  return iString.substr(mark, iPos - mark);
}

//! Extract integer token (skipping whitespace).
int Lex::getInt()
{
  String str = nextToken();
  return std::strtol(str.z(), nullptr, 10);
}

inline int hexDigit(int ch)
{
  if ('0' <= ch && ch <= '9')
    return ch - '0';
  if ('a' <= ch && ch <= 'f')
    return ch - 'a' + 10;
  if ('A' <= ch && ch <= 'F')
    return ch - 'A' + 10;
  return 0;
}

//! Extract byte in hex (skipping whitespace).
int Lex::getHexByte()
{
  int ch1 = '0', ch2 = '0';
  skipWhitespace();
  if (!eos())
    ch1 = iString[iPos++];
  skipWhitespace();
  if (!eos())
    ch2 = iString[iPos++];
  return (hexDigit(ch1) << 4) | hexDigit(ch2);
}

//! Extract hexadecimal token (skipping whitespace).
unsigned long int Lex::getHexNumber()
{
  String str = nextToken();
  return std::strtoul(str.z(), nullptr, 16);
}

//! Extract Fixed token (skipping whitespace).
Fixed Lex::getFixed()
{
  String str = nextToken();
  int i = 0;
  while (i < str.size() && str[i] != '.')
    ++i;
  int integral = std::strtol(str.substr(0, i).z(), nullptr, 10);
  int fractional = 0;
  if (i < str.size()) {
    String s = (str.substr(i+1) + "000").substr(0, 3);
    fractional = std::strtol(s.z(), nullptr, 10);
  }
  return Fixed::fromInternal(integral * 1000 + fractional);
}

//! Extract double token (skipping whitespace).
double Lex::getDouble()
{
  return Platform::toDouble(nextToken());
}

//! Skip over whitespace.
void Lex::skipWhitespace()
{
  while (!eos() && uint8_t(iString[iPos]) <= ' ')
    ++iPos;
}

// --------------------------------------------------------------------

/*! \class ipe::Buffer
  \ingroup base
  \brief A memory buffer.

  Can be be copied in constant time, the actual data is shared.
*/

//! Create buffer of specified size.
Buffer::Buffer(int size)
{
  iData = std::shared_ptr<std::vector<char>>(new std::vector<char>(size));
}

//! Create buffer by copying the data.
Buffer::Buffer(const char *data, int size)
{
  iData = std::shared_ptr<std::vector<char>>(new std::vector<char>(size));
  std::memcpy(&(*iData)[0], data, size);
}

// --------------------------------------------------------------------

/*! \class ipe::Stream
  \ingroup base
  \brief Abstract base class for output streams.
*/

Stream::~Stream()
{
  // empty implementation of pure virtual destructor
}

//! Close the stream.  No more writing allowed!
void Stream::close()
{
  // nothing
}

//! Default implementation uses PutChar.
void Stream::putString(String s)
{
  for (int i = 0; i < s.size(); ++i)
    putChar(s[i]);
}

//! Default implementation uses PutChar.
void Stream::putCString(const char *s)
{
  while (*s)
    putChar(*s++);
}

//! Default implementation uses PutChar.
void Stream::putRaw(const char *data, int size)
{
  for (int i = 0; i < size; i++)
    putChar(data[i]);
}

//! Output integer.
Stream &Stream::operator<<(int i)
{
  char buf[30];
  std::sprintf(buf, "%d", i);
  *this << buf;
  return *this;
}

//! Output double.
Stream &Stream::operator<<(double d)
{
  char buf[30];
  if (d < 0.0) {
    putChar('-');
    d = -d;
  }
  if (d >= 1e9) {
    // PDF will not be able to read this, but we have to write something.
    // Such large numbers should only happen if something is wrong.
    std::sprintf(buf, "%g", d);
    putCString(buf);
  } else if (d < 1e-8) {
    putChar('0');
  } else {
    // Print six significant digits, but omit trailing zeros.
    // Probably I'll want to have adjustable precision later.
    int factor;
    if (d > 1000.0)
      factor = 100L;
    else if (d > 100.0)
      factor = 1000L;
    else if (d > 10.0)
      factor = 10000L;
    else if (d > 1.0)
      factor = 100000L;
    else if (d > 0.1)
      factor = 1000000L;
    else if (d > 0.01)
      factor = 10000000L;
    else
      factor = 100000000L;
    double dd = trunc(d);
    int intpart = int(dd + 0.5);
    // 10^9 < 2^31
    int v = int(factor * (d - dd) + 0.5);
    if (v >= factor) {
      ++intpart;
      v -= factor;
    }
    std::sprintf(buf, "%d", intpart);
    putCString(buf);
    int mask = factor / 10;
    if (v != 0) {
      putChar('.');
      while (v != 0) {
	putChar('0' + v / mask);
	v = (10 * v) % factor;
      }
    }
  }
  return *this;
}

//! Output byte in hexadecimal.
void Stream::putHexByte(char b)
{
  char buf[3];
  std::sprintf(buf, "%02x", (b & 0xff));
  *this << buf;
}

//! Save a string with XML escaping of &, >, <, ", '.
void Stream::putXmlString(String s)
{
  for (int i = 0; i < s.size(); ++i) {
    char ch = s[i];
    switch (ch) {
    case '&': *this << "&amp;"; break;
    case '<': *this << "&lt;"; break;
    case '>': *this << "&gt;"; break;
    case '"': *this << "&quot;"; break;
    case '\'': *this << "&apos;"; break;
    default:
      *this << ch;
      break;
    }
  }
}

// --------------------------------------------------------------------

/*! \class ipe::StringStream
  \ingroup base
  \brief Stream writing into an String.
*/

//! Construct with string reference.
StringStream::StringStream(String &string)
  : iString(string)
{
  // nothing
}


void StringStream::putChar(char ch)
{
  iString += ch;
}

void StringStream::putString(String s)
{
  iString += s;
}

void StringStream::putCString(const char *s)
{
  iString += s;
}

void StringStream::putRaw(const char *data, int size)
{
  for (int i = 0; i < size; i++)
    iString += data[i];
}

long StringStream::tell() const
{
  return iString.size();
}

// --------------------------------------------------------------------

/*! \class ipe::FileStream
  \ingroup base
  \brief Stream writing into an open file.
*/

//! Constructor.
FileStream::FileStream(std::FILE *file)
  : iFile(file)
{
  // nothing
}

void FileStream::putChar(char ch)
{
  std::fputc(ch, iFile);
}

void FileStream::putString(String s)
{
  for (int i = 0; i < s.size(); ++i)
    std::fputc(s[i], iFile);
}

void FileStream::putCString(const char *s)
{
  fputs(s, iFile);
}

void FileStream::putRaw(const char *data, int size)
{
  for (int i = 0; i < size; i++)
    std::fputc(data[i], iFile);
}

long FileStream::tell() const
{
  return std::ftell(iFile);
}

// --------------------------------------------------------------------

/*! \class ipe::DataSource
 * \ingroup base
 * \brief Interface for getting data for parsing.
 */

//! Pure virtual destructor.
DataSource::~DataSource()
{
  // nothing
}

//! Return length of input stream in characters.
/*! Returns -1 if the stream is not seekable.
  Calling this function will invalidate the current position.
*/
int DataSource::length() const
{
  return -1;
}

//! Set position in stream.
/*! Does nothing if the stream is not seekable. */
void DataSource::setPosition(int pos)
{
  // do nothing
}

//! Return position in stream.
/*! Returns -1 if the stream is not seekable. */
int DataSource::position() const
{
  return -1;
}

// --------------------------------------------------------------------

/*! \class ipe::FileSource
  \ingroup base
  \brief Data source for parsing from a file.
*/

FileSource::FileSource(FILE *file)
  : iFile(file)
{
  // nothing
}

int FileSource::getChar()
{
  return std::fgetc(iFile);
}

int FileSource::length() const
{
  if (std::fseek(iFile, 0L, SEEK_END) == 0)
    return std::ftell(iFile);
  else
    return -1;
}

void FileSource::setPosition(int pos)
{
  std::fseek(iFile, pos, SEEK_SET);
}

int FileSource::position() const
{
  return std::ftell(iFile);
}

// --------------------------------------------------------------------

/*! \class ipe::BufferSource
  \ingroup base
  \brief Data source for parsing from a buffer.
*/

BufferSource::BufferSource(const Buffer &buffer)
  : iBuffer(buffer)
{
  iPos = 0;
}

int BufferSource::getChar()
{
  if (iPos >= iBuffer.size())
    return EOF;
  return uint8_t(iBuffer[iPos++]);
}

int BufferSource::length() const
{
  return iBuffer.size();
}

void BufferSource::setPosition(int pos)
{
  iPos = pos;
}

int BufferSource::position() const
{
  return iPos;
}

// --------------------------------------------------------------------

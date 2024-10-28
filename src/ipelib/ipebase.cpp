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

String::Imp * String::theEmptyString = {nullptr};

String::Imp * String::emptyString() noexcept {
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
String::String() noexcept { iImp = emptyString(); }

//! Construct a string by making copy of \a str.
String::String(const char * str) noexcept {
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
String String::withData(char * data, int len) noexcept {
    if (!len) len = strlen(data);
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
String::String(const char * str, int len) noexcept {
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

String::String(const std::string & rhs) noexcept {
    if (rhs.empty()) {
	iImp = emptyString();
    } else {
	iImp = new Imp;
	iImp->iRefCount = 1;
	iImp->iSize = rhs.length();
	iImp->iCapacity = (rhs.length() + 32) & ~15;
	iImp->iData = new char[iImp->iCapacity];
	memcpy(iImp->iData, rhs.c_str(), iImp->iSize);
    }
}

//! Copy constructor.
//! This only copies the reference and takes constant time.
String::String(const String & rhs) noexcept {
    iImp = rhs.iImp;
    iImp->iRefCount++;
}

//! Construct a substring.
/*! \a index must be >= 0.  \a len can be negative or too large to
  return entire string. */
String::String(const String & rhs, int index, int len) noexcept {
    // actually available data
    int len1 = index < rhs.size() ? rhs.size() - index : 0;
    if (len < 0 || len1 < len) len = len1;
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
String & String::operator=(const String & rhs) noexcept {
    if (iImp != rhs.iImp) {
	if (iImp->iRefCount == 1) {
	    delete[] iImp->iData;
	    delete iImp;
	} else
	    iImp->iRefCount--;
	iImp = rhs.iImp;
	iImp->iRefCount++;
    }
    return *this;
}

//! Destruct string if reference count has reached zero.
String::~String() noexcept {
    if (iImp->iRefCount == 1) {
	delete[] iImp->iData;
	delete iImp;
    } else
	iImp->iRefCount--;
}

//! Make a private copy of the string with \a n bytes to spare.
/*! When a private copy has to be made an extra 32 bytes are ensured. */
void String::detach(int n) noexcept {
    if (iImp == theEmptyString) {
	iImp = new Imp;
	iImp->iRefCount = 1;
	iImp->iSize = 0;
	n = (n + 0x1f) & ~0x1f;
	iImp->iCapacity = (n >= 16) ? n : 16;
	iImp->iData = new char[iImp->iCapacity];
    } else if (iImp->iRefCount > 1 || (iImp->iSize + n > iImp->iCapacity)) {
	Imp * imp = new Imp;
	imp->iRefCount = 1;
	imp->iSize = iImp->iSize;
	imp->iCapacity = iImp->iCapacity;
	while (imp->iSize + 32 + n > imp->iCapacity) imp->iCapacity *= 2;
	imp->iData = new char[imp->iCapacity];
	memcpy(imp->iData, iImp->iData, imp->iSize);
	iImp->iRefCount--;
	if (iImp->iRefCount == 0) {
	    delete[] iImp->iData;
	    delete iImp;
	}
	iImp = imp;
    }
}

//! Return a C style string with final zero byte.
const char * String::z() const noexcept {
    if (iImp == theEmptyString) return "";
    String * This = const_cast<String *>(this);
    if (iImp->iSize == iImp->iCapacity) This->detach(1);
    This->iImp->iData[iImp->iSize] = '\0';
    return data();
}

//! Return index of first occurrence of ch.
/*! Return -1 if character does not appear. */
int String::find(char ch) const noexcept {
    for (int i = 0; i < size(); ++i)
	if (iImp->iData[i] == ch) return i;
    return -1;
}

//! Return index of first occurrence of rhs.
/*! Return -1 if not substring is not present. */
int String::find(const char * rhs) const noexcept {
    int s = strlen(rhs);
    for (int i = 0; i < size() - s; ++i)
	if (::strncmp(iImp->iData + i, rhs, s) == 0) return i;
    return -1;
}

//! Return line starting at position \a index.
//! Index is updated to point to next line.
String String::getLine(int & index) const noexcept {
    int i = index;
    while (i < size() && iImp->iData[i] != '\r' && iImp->iData[i] != '\n') ++i;
    String result = substr(index, i - index);
    if (i < size() && iImp->iData[i] == '\r') ++i;
    if (i < size() && iImp->iData[i] == '\n') ++i;
    index = i;
    return result;
}

//! Return index of last occurrence of ch.
/*! Return -1 if character does not appear. */
int String::rfind(char ch) const noexcept {
    for (int i = size() - 1; i >= 0; --i)
	if (iImp->iData[i] == ch) return i;
    return -1;
}

//! Make string empty.
void String::erase() noexcept {
    detach(0);
    iImp->iSize = 0;
}

//! Append \a rhs to this string.
void String::append(const String & rhs) noexcept {
    int n = rhs.size();
    detach(n);
    memcpy(iImp->iData + iImp->iSize, rhs.iImp->iData, n);
    iImp->iSize += n;
}

//! Append \a rhs to this string.
void String::append(const char * rhs) noexcept {
    int n = strlen(rhs);
    if (n) {
	detach(n);
	memcpy(iImp->iData + iImp->iSize, rhs, n);
	iImp->iSize += n;
    }
}

//! Append \a ch to this string.
void String::append(char ch) noexcept {
    detach(1);
    iImp->iData[iImp->iSize++] = ch;
}

//! Append a single unicode character \a ch in UTF-8 encoding.
void String::appendUtf8(uint16_t ch) noexcept {
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
String String::right(int i) const noexcept {
    if (i < size())
	return String(*this, size() - i, i);
    else
	return *this;
}

//! Does string start with this prefix? (bytewise comparison)
bool String::hasPrefix(const char * rhs) const noexcept {
    int n = strlen(rhs);
    return (size() >= n && !strncmp(iImp->iData, rhs, n));
}

//! Equality operator (bytewise comparison).
bool String::operator==(const String & rhs) const noexcept {
    return (size() == rhs.size() && !strncmp(iImp->iData, rhs.iImp->iData, size()));
}

//! Equality operator (bytewise comparison).
bool String::operator==(const char * rhs) const noexcept {
    int n = strlen(rhs);
    return (size() == n && !strncmp(iImp->iData, rhs, n));
}

//! Inequality operator (bytewise comparison).
bool String::operator<(const String & rhs) const noexcept {
    int n = size() < rhs.size() ? size() : rhs.size();
    int cmp = ::strncmp(iImp->iData, rhs.iImp->iData, n);
    return (cmp < 0 || (cmp == 0 && size() < rhs.size()));
}

//! Concatenate this string with \a rhs.
String String::operator+(const String & rhs) const noexcept {
    String s(*this);
    s.append(rhs);
    return s;
}

static const uint8_t bytesFromUTF8[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

static const uint8_t firstByteMark[7] = {0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0};

//! Return Unicode value from UTF-8 string.
/*! The \a index is incremented to the next UTF-8 character.
  This returns 0xfffd if there is any problem in parsing UTF-8. */
int String::unicode(int & index) const noexcept {
    int wch = uint8_t(iImp->iData[index++]);
    if ((wch & 0xc0) == 0x80) {
	// not on first byte of a UTF-8 sequence
	while (index < iImp->iSize && (iImp->iData[index] & 0xc0) == 0x80) index++;
	return 0xfffd;
    }
    int extraBytes = bytesFromUTF8[wch & 0xff];
    wch -= firstByteMark[extraBytes];
    while (extraBytes--) {
	if (index >= iImp->iSize) return 0xfffd; // UTF-8 sequence is incomplete
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
Fixed Fixed::mult(int a, int b) const { return Fixed::fromInternal(iValue * a / b); }

Fixed Fixed::fromDouble(double val) { return Fixed::fromInternal(int(val * 1000 + 0.5)); }

namespace ipe {
/*! \relates Fixed */
Stream & operator<<(Stream & stream, const Fixed & f) {
    stream << (f.iValue / 1000);
    if (f.iValue % 1000) {
	stream << "." << ((f.iValue / 100) % 10);
	if (f.iValue % 100) {
	    stream << ((f.iValue / 10) % 10);
	    if (f.iValue % 10) stream << (f.iValue % 10);
	}
    }
    return stream;
}
} // namespace ipe

// --------------------------------------------------------------------

/*! \class ipe::Lex
  \ingroup base
  \brief Lexical analyser. Seeded with a string.
*/

//! Construct lexical analyzer from a string.
Lex::Lex(String str)
    : iString(str)
    , iPos(0) {
    // nothing
}

//! Return NextToken, but without extracting it.
String Lex::token() {
    int pos = iPos;
    String str = nextToken();
    iPos = pos;
    return str;
}

//! Extract next token.
/*! Skips any whitespace before the token.
  Returns empty string if end of string is reached. */
String Lex::nextToken() {
    skipWhitespace();
    int mark = iPos;
    while (!(eos() || uint8_t(iString[iPos]) <= ' ')) ++iPos;
    return iString.substr(mark, iPos - mark);
}

//! Extract integer token (skipping whitespace).
int Lex::getInt() {
    String str = nextToken();
    return std::strtol(str.z(), nullptr, 10);
}

inline int hexDigit(int ch) {
    if ('0' <= ch && ch <= '9') return ch - '0';
    if ('a' <= ch && ch <= 'f') return ch - 'a' + 10;
    if ('A' <= ch && ch <= 'F') return ch - 'A' + 10;
    return 0;
}

//! Extract byte in hex (skipping whitespace).
int Lex::getHexByte() {
    int ch1 = '0', ch2 = '0';
    skipWhitespace();
    if (!eos()) ch1 = iString[iPos++];
    skipWhitespace();
    if (!eos()) ch2 = iString[iPos++];
    return (hexDigit(ch1) << 4) | hexDigit(ch2);
}

//! Extract hexadecimal token (skipping whitespace).
unsigned long int Lex::getHexNumber() {
    String str = nextToken();
    return std::strtoul(str.z(), nullptr, 16);
}

//! Extract Fixed token (skipping whitespace).
Fixed Lex::getFixed() {
    String str = nextToken();
    int i = 0;
    while (i < str.size() && str[i] != '.') ++i;
    int integral = std::strtol(str.substr(0, i).z(), nullptr, 10);
    int fractional = 0;
    if (i < str.size()) {
	String s = (str.substr(i + 1) + "000").substr(0, 3);
	fractional = std::strtol(s.z(), nullptr, 10);
    }
    return Fixed::fromInternal(integral * 1000 + fractional);
}

//! Extract double token (skipping whitespace).
double Lex::getDouble() { return Platform::toDouble(nextToken()); }

//! Skip over whitespace.
void Lex::skipWhitespace() {
    while (!eos() && uint8_t(iString[iPos]) <= ' ') ++iPos;
}

// --------------------------------------------------------------------

/*! \class ipe::Buffer
  \ingroup base
  \brief A memory buffer.

  Can be be copied in constant time, the actual data is shared.
*/

//! Create buffer of specified size.
Buffer::Buffer(int size) {
    iData = std::shared_ptr<std::vector<char>>(new std::vector<char>(size));
}

//! Create buffer by copying the data.
Buffer::Buffer(const char * data, int size) {
    iData = std::shared_ptr<std::vector<char>>(new std::vector<char>(size));
    std::memcpy(&(*iData)[0], data, size);
}

static uint32_t crc32(uint32_t crc, const uint8_t * data, size_t len) {
    static const uint32_t table[256] = {
	0x00000000U, 0x04C11DB7U, 0x09823B6EU, 0x0D4326D9U, 0x130476DCU, 0x17C56B6BU,
	0x1A864DB2U, 0x1E475005U, 0x2608EDB8U, 0x22C9F00FU, 0x2F8AD6D6U, 0x2B4BCB61U,
	0x350C9B64U, 0x31CD86D3U, 0x3C8EA00AU, 0x384FBDBDU, 0x4C11DB70U, 0x48D0C6C7U,
	0x4593E01EU, 0x4152FDA9U, 0x5F15ADACU, 0x5BD4B01BU, 0x569796C2U, 0x52568B75U,
	0x6A1936C8U, 0x6ED82B7FU, 0x639B0DA6U, 0x675A1011U, 0x791D4014U, 0x7DDC5DA3U,
	0x709F7B7AU, 0x745E66CDU, 0x9823B6E0U, 0x9CE2AB57U, 0x91A18D8EU, 0x95609039U,
	0x8B27C03CU, 0x8FE6DD8BU, 0x82A5FB52U, 0x8664E6E5U, 0xBE2B5B58U, 0xBAEA46EFU,
	0xB7A96036U, 0xB3687D81U, 0xAD2F2D84U, 0xA9EE3033U, 0xA4AD16EAU, 0xA06C0B5DU,
	0xD4326D90U, 0xD0F37027U, 0xDDB056FEU, 0xD9714B49U, 0xC7361B4CU, 0xC3F706FBU,
	0xCEB42022U, 0xCA753D95U, 0xF23A8028U, 0xF6FB9D9FU, 0xFBB8BB46U, 0xFF79A6F1U,
	0xE13EF6F4U, 0xE5FFEB43U, 0xE8BCCD9AU, 0xEC7DD02DU, 0x34867077U, 0x30476DC0U,
	0x3D044B19U, 0x39C556AEU, 0x278206ABU, 0x23431B1CU, 0x2E003DC5U, 0x2AC12072U,
	0x128E9DCFU, 0x164F8078U, 0x1B0CA6A1U, 0x1FCDBB16U, 0x018AEB13U, 0x054BF6A4U,
	0x0808D07DU, 0x0CC9CDCAU, 0x7897AB07U, 0x7C56B6B0U, 0x71159069U, 0x75D48DDEU,
	0x6B93DDDBU, 0x6F52C06CU, 0x6211E6B5U, 0x66D0FB02U, 0x5E9F46BFU, 0x5A5E5B08U,
	0x571D7DD1U, 0x53DC6066U, 0x4D9B3063U, 0x495A2DD4U, 0x44190B0DU, 0x40D816BAU,
	0xACA5C697U, 0xA864DB20U, 0xA527FDF9U, 0xA1E6E04EU, 0xBFA1B04BU, 0xBB60ADFCU,
	0xB6238B25U, 0xB2E29692U, 0x8AAD2B2FU, 0x8E6C3698U, 0x832F1041U, 0x87EE0DF6U,
	0x99A95DF3U, 0x9D684044U, 0x902B669DU, 0x94EA7B2AU, 0xE0B41DE7U, 0xE4750050U,
	0xE9362689U, 0xEDF73B3EU, 0xF3B06B3BU, 0xF771768CU, 0xFA325055U, 0xFEF34DE2U,
	0xC6BCF05FU, 0xC27DEDE8U, 0xCF3ECB31U, 0xCBFFD686U, 0xD5B88683U, 0xD1799B34U,
	0xDC3ABDEDU, 0xD8FBA05AU, 0x690CE0EEU, 0x6DCDFD59U, 0x608EDB80U, 0x644FC637U,
	0x7A089632U, 0x7EC98B85U, 0x738AAD5CU, 0x774BB0EBU, 0x4F040D56U, 0x4BC510E1U,
	0x46863638U, 0x42472B8FU, 0x5C007B8AU, 0x58C1663DU, 0x558240E4U, 0x51435D53U,
	0x251D3B9EU, 0x21DC2629U, 0x2C9F00F0U, 0x285E1D47U, 0x36194D42U, 0x32D850F5U,
	0x3F9B762CU, 0x3B5A6B9BU, 0x0315D626U, 0x07D4CB91U, 0x0A97ED48U, 0x0E56F0FFU,
	0x1011A0FAU, 0x14D0BD4DU, 0x19939B94U, 0x1D528623U, 0xF12F560EU, 0xF5EE4BB9U,
	0xF8AD6D60U, 0xFC6C70D7U, 0xE22B20D2U, 0xE6EA3D65U, 0xEBA91BBCU, 0xEF68060BU,
	0xD727BBB6U, 0xD3E6A601U, 0xDEA580D8U, 0xDA649D6FU, 0xC423CD6AU, 0xC0E2D0DDU,
	0xCDA1F604U, 0xC960EBB3U, 0xBD3E8D7EU, 0xB9FF90C9U, 0xB4BCB610U, 0xB07DABA7U,
	0xAE3AFBA2U, 0xAAFBE615U, 0xA7B8C0CCU, 0xA379DD7BU, 0x9B3660C6U, 0x9FF77D71U,
	0x92B45BA8U, 0x9675461FU, 0x8832161AU, 0x8CF30BADU, 0x81B02D74U, 0x857130C3U,
	0x5D8A9099U, 0x594B8D2EU, 0x5408ABF7U, 0x50C9B640U, 0x4E8EE645U, 0x4A4FFBF2U,
	0x470CDD2BU, 0x43CDC09CU, 0x7B827D21U, 0x7F436096U, 0x7200464FU, 0x76C15BF8U,
	0x68860BFDU, 0x6C47164AU, 0x61043093U, 0x65C52D24U, 0x119B4BE9U, 0x155A565EU,
	0x18197087U, 0x1CD86D30U, 0x029F3D35U, 0x065E2082U, 0x0B1D065BU, 0x0FDC1BECU,
	0x3793A651U, 0x3352BBE6U, 0x3E119D3FU, 0x3AD08088U, 0x2497D08DU, 0x2056CD3AU,
	0x2D15EBE3U, 0x29D4F654U, 0xC5A92679U, 0xC1683BCEU, 0xCC2B1D17U, 0xC8EA00A0U,
	0xD6AD50A5U, 0xD26C4D12U, 0xDF2F6BCBU, 0xDBEE767CU, 0xE3A1CBC1U, 0xE760D676U,
	0xEA23F0AFU, 0xEEE2ED18U, 0xF0A5BD1DU, 0xF464A0AAU, 0xF9278673U, 0xFDE69BC4U,
	0x89B8FD09U, 0x8D79E0BEU, 0x803AC667U, 0x84FBDBD0U, 0x9ABC8BD5U, 0x9E7D9662U,
	0x933EB0BBU, 0x97FFAD0CU, 0xAFB010B1U, 0xAB710D06U, 0xA6322BDFU, 0xA2F33668U,
	0xBCB4666DU, 0xB8757BDAU, 0xB5365D03U, 0xB1F740B4U,
    };
    while (len > 0) {
	crc = table[*data ^ ((crc >> 24) & 0xff)] ^ (crc << 8);
	data++;
	len--;
    }
    return crc;
}

uint32_t Buffer::checksum() const {
    return crc32(0xffffffffU, (uint8_t *)data(), size());
}

// --------------------------------------------------------------------

/*! \class ipe::Stream
  \ingroup base
  \brief Abstract base class for output streams.
*/

Stream::~Stream() {
    // empty implementation of pure virtual destructor
}

//! Close the stream.  No more writing allowed!
void Stream::close() {
    // nothing
}

//! Default implementation uses PutChar.
void Stream::putString(String s) {
    for (int i = 0; i < s.size(); ++i) putChar(s[i]);
}

//! Default implementation uses PutChar.
void Stream::putCString(const char * s) {
    while (*s) putChar(*s++);
}

//! Default implementation uses PutChar.
void Stream::putRaw(const char * data, int size) {
    for (int i = 0; i < size; i++) putChar(data[i]);
}

//! Output integer.
Stream & Stream::operator<<(int i) {
    char buf[30];
    std::sprintf(buf, "%d", i);
    *this << buf;
    return *this;
}

//! Output double.
Stream & Stream::operator<<(double d) {
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
void Stream::putHexByte(char b) {
    char buf[3];
    std::sprintf(buf, "%02x", (b & 0xff));
    *this << buf;
}

//! Save a string with XML escaping of &, >, <, ", '.
void Stream::putXmlString(String s) {
    for (int i = 0; i < s.size(); ++i) {
	char ch = s[i];
	switch (ch) {
	case '&': *this << "&amp;"; break;
	case '<': *this << "&lt;"; break;
	case '>': *this << "&gt;"; break;
	case '"': *this << "&quot;"; break;
	case '\'': *this << "&apos;"; break;
	default: *this << ch; break;
	}
    }
}

// --------------------------------------------------------------------

/*! \class ipe::StringStream
  \ingroup base
  \brief Stream writing into an String.
*/

//! Construct with string reference.
StringStream::StringStream(String & string)
    : iString(string) {
    // nothing
}

void StringStream::putChar(char ch) { iString += ch; }

void StringStream::putString(String s) { iString += s; }

void StringStream::putCString(const char * s) { iString += s; }

void StringStream::putRaw(const char * data, int size) {
    for (int i = 0; i < size; i++) iString += data[i];
}

long StringStream::tell() const { return iString.size(); }

// --------------------------------------------------------------------

/*! \class ipe::FileStream
  \ingroup base
  \brief Stream writing into an open file.
*/

//! Constructor.
FileStream::FileStream(std::FILE * file)
    : iFile(file) {
    // nothing
}

void FileStream::putChar(char ch) { std::fputc(ch, iFile); }

void FileStream::putString(String s) {
    for (int i = 0; i < s.size(); ++i) std::fputc(s[i], iFile);
}

void FileStream::putCString(const char * s) { fputs(s, iFile); }

void FileStream::putRaw(const char * data, int size) {
    for (int i = 0; i < size; i++) std::fputc(data[i], iFile);
}

long FileStream::tell() const { return std::ftell(iFile); }

// --------------------------------------------------------------------

/*! \class ipe::DataSource
 * \ingroup base
 * \brief Interface for getting data for parsing.
 */

//! Pure virtual destructor.
DataSource::~DataSource() {
    // nothing
}

//! Return length of input stream in characters.
/*! Returns -1 if the stream is not seekable.
  Calling this function will invalidate the current position.
*/
int DataSource::length() const { return -1; }

//! Set position in stream.
/*! Does nothing if the stream is not seekable. */
void DataSource::setPosition(int pos) {
    // do nothing
}

//! Return position in stream.
/*! Returns -1 if the stream is not seekable. */
int DataSource::position() const { return -1; }

// --------------------------------------------------------------------

/*! \class ipe::FileSource
  \ingroup base
  \brief Data source for parsing from a file.
*/

FileSource::FileSource(FILE * file)
    : iFile(file) {
    // nothing
}

int FileSource::getChar() { return std::fgetc(iFile); }

int FileSource::length() const {
    if (std::fseek(iFile, 0L, SEEK_END) == 0)
	return std::ftell(iFile);
    else
	return -1;
}

void FileSource::setPosition(int pos) { std::fseek(iFile, pos, SEEK_SET); }

int FileSource::position() const { return std::ftell(iFile); }

// --------------------------------------------------------------------

/*! \class ipe::BufferSource
  \ingroup base
  \brief Data source for parsing from a buffer.
*/

BufferSource::BufferSource(const Buffer & buffer)
    : iBuffer(buffer) {
    iPos = 0;
}

int BufferSource::getChar() {
    if (iPos >= iBuffer.size()) return EOF;
    return uint8_t(iBuffer[iPos++]);
}

int BufferSource::length() const { return iBuffer.size(); }

void BufferSource::setPosition(int pos) { iPos = pos; }

int BufferSource::position() const { return iPos; }

// --------------------------------------------------------------------

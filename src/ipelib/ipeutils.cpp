// --------------------------------------------------------------------
// Various utility classes
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

#include "ipeutils.h"
#include "ipegroup.h"
#include "ipeimage.h"
#include "ipelet.h"
#include "ipepage.h"
#include "ipereference.h"
#include "ipetext.h"

#include <zlib.h>

using namespace ipe;

// --------------------------------------------------------------------

/*! \class ipe::BitmapFinder
  \ingroup high
  \brief A visitor that recursively scans objects and collects all bitmaps.
*/

void BitmapFinder::scanPage(const Page * page) {
    for (int i = 0; i < page->count(); ++i) page->object(i)->accept(*this);
}

void BitmapFinder::visitGroup(const Group * obj) {
    for (Group::const_iterator it = obj->begin(); it != obj->end(); ++it)
	(*it)->accept(*this);
}

void BitmapFinder::visitImage(const Image * obj) { iBitmaps.push_back(obj->bitmap()); }

// --------------------------------------------------------------------

/*! \class ipe::BBoxPainter
  \ingroup high
  \brief Paint objects using this painter to compute an accurate bounding box.

  The Object::bbox member function computes a bounding box useful
  for distance calculations and optimizations.  To find a bounding box
  that is accurate for the actual \b drawn object, paint the object
  using a BBoxPainter, and retrieve the box with bbox.
*/

BBoxPainter::BBoxPainter(const Cascade * style)
    : Painter(style) {
    iClipBox.push_back(Rect()); // no clipping yet
}

void BBoxPainter::doPush() { iClipBox.push_back(iClipBox.back()); }

void BBoxPainter::doPop() { iClipBox.pop_back(); }

void BBoxPainter::doNewPath() { iPathBox.clear(); }

void BBoxPainter::doMoveTo(const Vector & v) {
    iV = v;
    iPathBox.addPoint(iV);
}

void BBoxPainter::doLineTo(const Vector & v) {
    iV = v;
    iPathBox.addPoint(iV);
}

void BBoxPainter::doCurveTo(const Vector & v1, const Vector & v2, const Vector & v3) {
    Bezier bez(iV, v1, v2, v3);
    Rect bb = bez.bbox();
    iPathBox.addPoint(bb.bottomLeft());
    iPathBox.addPoint(bb.topRight());
    iV = v3;
}

void BBoxPainter::doDrawBitmap(Bitmap) {
    Rect box;
    box.addPoint(matrix() * Vector(0.0, 0.0));
    box.addPoint(matrix() * Vector(0.0, 1.0));
    box.addPoint(matrix() * Vector(1.0, 1.0));
    box.addPoint(matrix() * Vector(1.0, 0.0));
    box.clipTo(iClipBox.back());
    iBBox.addRect(box);
}

void BBoxPainter::doDrawText(const Text * text) {
    // this is not correct if the text is transformed
    // as documented in the manual
    Rect box;
    box.addPoint(matrix() * Vector(0, 0));
    box.addPoint(matrix() * Vector(0, text->totalHeight()));
    box.addPoint(matrix() * Vector(text->width(), text->totalHeight()));
    box.addPoint(matrix() * Vector(text->width(), 0));
    const TextPadding * pad = cascade()->findTextPadding();
    box.addPoint(box.bottomLeft() - Vector(pad->iLeft, pad->iBottom));
    box.addPoint(box.topRight() + Vector(pad->iRight, pad->iTop));
    box.clipTo(iClipBox.back());
    iBBox.addRect(box);
}

void BBoxPainter::doDrawPath(TPathMode mode) {
    double lw = pen().toDouble() / 2.0;
    if (!iPathBox.isEmpty()) {
	iPathBox.clipTo(iClipBox.back());
	if (!iPathBox.isEmpty()) {
	    iBBox.addPoint(iPathBox.bottomLeft() - Vector(lw, lw));
	    iBBox.addPoint(iPathBox.topRight() + Vector(lw, lw));
	}
    }
}

void BBoxPainter::doAddClipPath() {
    if (iClipBox.back().isEmpty())
	iClipBox.back() = iPathBox;
    else
	iClipBox.back().clipTo(iPathBox);
}

// --------------------------------------------------------------------

/*! \class ipe::A85Stream
  \ingroup high
  \brief Filter stream adding ASCII85 encoding.
*/

inline uint32_t a85word(const uint8_t * p) {
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

inline void a85encode(uint32_t w, char * p) {
    p[4] = char(w % 85 + 33);
    w /= 85;
    p[3] = char(w % 85 + 33);
    w /= 85;
    p[2] = char(w % 85 + 33);
    w /= 85;
    p[1] = char(w % 85 + 33);
    p[0] = char(w / 85 + 33);
}

A85Stream::A85Stream(Stream & stream)
    : iStream(stream) {
    iN = 0;
    iCol = 0;
}

void A85Stream::putChar(char ch) {
    iCh[iN++] = ch;
    if (iN == 4) {
	// encode and write
	uint32_t w = a85word(iCh);
	if (w == 0) {
	    iStream.putChar('z');
	    ++iCol;
	} else {
	    char buf[6];
	    buf[5] = '\0';
	    a85encode(w, buf);
	    iStream.putCString(buf);
	    iCol += 5;
	}
	if (iCol > 70) {
	    iStream.putChar('\n');
	    iCol = 0;
	}
	iN = 0;
    }
}

void A85Stream::close() {
    if (iN) {
	for (int k = iN; k < 4; ++k) iCh[k] = 0;
	uint32_t w = a85word(iCh);
	char buf[6];
	a85encode(w, buf);
	buf[iN + 1] = '\0';
	iStream.putCString(buf);
    }
    iStream.putCString("~>\n");
    iStream.close();
}

// --------------------------------------------------------------------

/*! \class ipe::Base64Stream
  \ingroup high
  \brief Filter stream adding Base64 encoding.
*/

static const char base64letter[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline uint32_t base64word(const uint8_t * p) {
    return (p[0] << 16) | (p[1] << 8) | p[2];
}

inline void base64encode(uint32_t w, char * p) {
    p[4] = '\0';
    p[3] = base64letter[w % 64];
    w >>= 6;
    p[2] = base64letter[w % 64];
    w >>= 6;
    p[1] = base64letter[w % 64];
    w >>= 6;
    p[0] = base64letter[w % 64];
}

Base64Stream::Base64Stream(Stream & stream)
    : iStream(stream) {
    iN = 0;
    iCol = 0;
}

void Base64Stream::putChar(char ch) {
    iCh[iN++] = ch;
    if (iN == 3) {
	// encode and write
	uint32_t w = base64word(iCh);
	char buf[5];
	base64encode(w, buf);
	iStream.putCString(buf);
	iCol += 4;
	if (iCol > 70) {
	    iStream.putChar('\n');
	    iCol = 0;
	}
	iN = 0;
    }
}

void Base64Stream::close() {
    if (iN) {
	for (int k = iN; k < 3; ++k) iCh[k] = 0;
	uint32_t w = base64word(iCh);
	char buf[5];
	base64encode(w, buf);
	for (int k = iN + 1; k < 4; ++k) buf[k] = '=';
	iStream.putCString(buf);
    }
    iStream.putCString("\n");
    iStream.close();
}

// --------------------------------------------------------------------

/*! \class ipe::A85Source
  \ingroup high
  \brief Filter source adding ASCII85 decoding.
*/

A85Source::A85Source(DataSource & source)
    : iSource(source) {
    iEof = false;
    iN = 0;     // number of characters buffered
    iIndex = 0; // next character to return
}

int A85Source::getChar() {
    if (iIndex < iN) return uint8_t(iBuf[iIndex++]);

    if (iEof) return EOF;

    int ch;
    do { ch = iSource.getChar(); } while (ch == '\n' || ch == '\r' || ch == ' ');

    if (ch == '~' || ch == EOF) {
	iEof = true;
	iN = 0; // no more data, immediate EOF
	return EOF;
    }

    iIndex = 1;
    iN = 4;

    if (ch == 'z') {
	iBuf[0] = iBuf[1] = iBuf[2] = iBuf[3] = 0;
	return uint8_t(iBuf[0]);
    }

    int c[5];
    c[0] = ch;
    for (int k = 1; k < 5; ++k) {
	do {
	    c[k] = iSource.getChar();
	} while (c[k] == '\n' || c[k] == '\r' || c[k] == ' ');
	if (c[k] == '~' || c[k] == EOF) {
	    iN = k - 1;
	    iEof = true;
	    break;
	}
    }

    for (int k = iN + 1; k < 5; ++k) c[k] = 0x21 + 84;

    uint32_t t = 0;
    for (int k = 0; k < 5; ++k) t = t * 85 + (c[k] - 0x21);

    for (int k = 3; k >= 0; --k) {
	iBuf[k] = char(t & 0xff);
	t >>= 8;
    }

    return iBuf[0];
};

// --------------------------------------------------------------------

/*! \class ipe::Base64Source
  \ingroup high
  \brief Filter source adding Base64 decoding.
*/

static signed char base64_value[] = {
    62, -1, -1, -1, 63,                                             // 2b..2f
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 0,  -1, -1, // 30..3f
    -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, // 40..4f
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, // 50..5f
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 60..6f
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,                     // 70..7a
};

inline bool base64illegal(int ch) {
    return (ch < '+' || ch > 'z' || base64_value[ch - '+'] < 0);
}

inline int base64value(int ch) { return base64_value[ch - '+']; }

Base64Source::Base64Source(DataSource & source)
    : iSource(source) {
    iEof = false;
    iIndex = 0; // buffer empty
    iBufLen = 0;
}

int Base64Source::getChar() {
    if (iEof) return EOF;

    if (iIndex < iBufLen) return uint8_t(iBuf[iIndex++]);

    char buf[4];
    for (int i = 0; i < 4; ++i) {
	int ch;
	do { ch = iSource.getChar(); } while (ch == '\n' || ch == '\r' || ch == ' ');

	// non-base64 characters terminate stream
	if (ch == EOF || base64illegal(ch)) {
	    iEof = true; // no more data, immediate EOF
	    return EOF;
	}

	buf[i] = ch;
    }

    uint32_t w = base64value(buf[0]) << 18;
    w |= (base64value(buf[1]) << 12);
    w |= (base64value(buf[2]) << 6);
    w |= base64value(buf[3]);

    iBuf[0] = (w >> 16) & 0xff;
    iBuf[1] = (w >> 8) & 0xff;
    iBuf[2] = w & 0xff;

    iBufLen = 3;
    if (buf[3] == '=') {
	--iBufLen;
	if (buf[2] == '=') --iBufLen;
    }
    iIndex = 1;
    return uint8_t(iBuf[0]);
};

// --------------------------------------------------------------------

/*! \class ipe::DeflateStream
  \ingroup high
  \brief Filter stream adding flate compression.
*/

struct DeflateStream::Private {
    z_stream iFlate;
};

DeflateStream::DeflateStream(Stream & stream, int level)
    : iStream(stream)
    , iIn(0x400)
    , iOut(0x400) // create buffers
{
    iPriv = new Private;
    z_streamp z = &iPriv->iFlate;

    z->zalloc = nullptr;
    z->zfree = nullptr;
    z->opaque = nullptr;

    int err = ::deflateInit(z, level);
    if (err != Z_OK) {
	ipeDebug("deflateInit returns error %d", err);
	assert(false);
    }

    iN = 0;
}

DeflateStream::~DeflateStream() {
    if (iPriv) {
	z_streamp z = &iPriv->iFlate;
	::deflateEnd(z);
	delete iPriv;
    }
}

void DeflateStream::putChar(char ch) {
    iIn[iN++] = ch;
    if (iN < iIn.size()) return;

    // compress and write
    z_streamp z = &iPriv->iFlate;
    z->next_in = (Bytef *)iIn.data();
    z->avail_in = iIn.size();
    while (z->avail_in) {
	z->next_out = (Bytef *)iOut.data();
	z->avail_out = iOut.size();
	int err = ::deflate(z, Z_NO_FLUSH);
	if (err != Z_OK) {
	    ipeDebug("deflate returns error %d", err);
	    assert(false);
	}
	// save output
	iStream.putRaw(iOut.data(), z->next_out - (Bytef *)iOut.data());
    }
    iN = 0;
}

void DeflateStream::close() {
    // compress and write remaining data
    z_streamp z = &iPriv->iFlate;
    z->next_in = (Bytef *)iIn.data();
    z->avail_in = iN;

    int err;
    do {
	z->next_out = (Bytef *)iOut.data();
	z->avail_out = iOut.size();
	err = ::deflate(z, Z_FINISH);
	if (err != Z_OK && err != Z_STREAM_END) {
	    ipeDebug("deflate returns error %d", err);
	    assert(false);
	}
	iStream.putRaw(iOut.data(), z->next_out - (Bytef *)iOut.data());
    } while (err == Z_OK);

    err = ::deflateEnd(z);
    if (err != Z_OK) {
	ipeDebug("deflateEnd returns error %d", err);
	assert(false);
    }

    delete iPriv;
    iPriv = nullptr; // make sure no more writing possible
    iStream.close();
}

//! Deflate a buffer in a single run.
/*! The returned buffer may be larger than necessary: \a deflatedSize
  is set to the number of bytes actually used. */
Buffer DeflateStream::deflate(const char * data, int size, int & deflatedSize,
			      int compressLevel) {
    uLong dfsize = uLong(size * 1.001 + 13);
    Buffer deflatedData(dfsize);
    int err = ::compress2((Bytef *)deflatedData.data(), &dfsize, (const Bytef *)data,
			  size, compressLevel);
    if (err != Z_OK) {
	ipeDebug("Zlib compress2 returns errror %d", err);
	assert(false);
    }
    deflatedSize = dfsize;
    return deflatedData;
}

// --------------------------------------------------------------------

/*! \class ipe::InflateSource
  \ingroup high
  \brief Filter source adding flate decompression.
*/

struct InflateSource::Private {
    z_stream iFlate;
};

InflateSource::InflateSource(DataSource & source)
    : iSource(source)
    , iIn(0x400)
    , iOut(0x400) {
    iPriv = new Private;
    z_streamp z = &iPriv->iFlate;

    z->zalloc = nullptr;
    z->zfree = nullptr;
    z->opaque = nullptr;

    fillBuffer();

    int err = ::inflateInit(z);
    if (err != Z_OK) {
	ipeDebug("inflateInit returns error %d", err);
	delete iPriv;
	iPriv = nullptr; // set EOF
	return;
    }

    iP = iOut.data();
    z->next_out = (Bytef *)iP;
}

InflateSource::~InflateSource() {
    if (iPriv) {
	z_streamp z = &iPriv->iFlate;
	::inflateEnd(z);
	delete iPriv;
    }
}

void InflateSource::fillBuffer() {
    char * p = iIn.data();
    char * p1 = iIn.data() + iIn.size();
    z_streamp z = &iPriv->iFlate;
    z->next_in = (Bytef *)p;
    z->avail_in = 0;
    while (p < p1) {
	int ch = iSource.getChar();
	if (ch == EOF) return;
	*p++ = char(ch);
	z->avail_in++;
    }
}

//! Get one more character, or EOF.
int InflateSource::getChar() {
    if (!iPriv) return EOF;

    z_streamp z = &iPriv->iFlate;
    if (iP < (char *)z->next_out) return uint8_t(*iP++);

    // next to decompress some data
    if (z->avail_in == 0) fillBuffer();

    if (z->avail_in > 0) {
	// data is available
	z->next_out = (Bytef *)iOut.data();
	z->avail_out = iOut.size();
	int err = ::inflate(z, Z_NO_FLUSH);
	if (err != Z_OK && err != Z_STREAM_END) {
	    ipeDebug("inflate returns error %d", err);
	    ::inflateEnd(z);
	    delete iPriv;
	    iPriv = nullptr; // set EOF
	    return EOF;
	}
	iP = iOut.data();
	if (iP < (char *)z->next_out) return uint8_t(*iP++);
	// didn't get any new data, must be EOF
    }

    // fillBuffer didn't get any data, must be EOF, so we are done
    ::inflateEnd(z);
    delete iPriv;
    iPriv = nullptr;
    return EOF;
}

// --------------------------------------------------------------------

/*! \defgroup ipelet The Ipelet interface
  \brief Implementation of Ipe plugins.

  Ipelets are dynamically loaded plugins for Ipe written in Lua.

  The Ipelet class makes it easy for ipelet authors to write ipelets
  in C++ without using Lua's C API.  They only need to provide some
  boilerplate Lua code to define the labels and functions of the
  ipelet, and use the Lua function "loadIpelet" to load a DLL
  containing a C++ class derived from Ipelet.  The run() method of
  this class can then be called from Lua.  The C++ code has access to
  services provided by Ipe through an IpeletHelper object.

  Ipelet derived classes are restricted to operate on the current page
  of the document, and cannot modify the StyleSheet or other
  properties of the document.  If you wish to write an ipelet doing
  this, you need to work in Lua (or create a C++ library using the Lua
  C API).
*/

/*! \class ipe::Ipelet
  \ingroup ipelet
  \brief Abstract base class for Ipelets.
*/

//! Pure virtual destructor.
Ipelet::~Ipelet() {
    // nothing
}

// --------------------------------------------------------------------

/*! \class ipe::IpeletHelper
  \ingroup ipelet
  \brief Service provider for Ipelets.

  C++ Ipelets can ask Ipe to perform various services and request
  information using this class.
*/

//! Pure virtual destructor.
IpeletHelper::~IpeletHelper() {
    // nothing
}

// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Bitmaps
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

#include "ipebitmap.h"
#include "ipeutils.h"
#include <zlib.h>

using namespace ipe;

extern bool dctDecode(Buffer dctData, Buffer pixelData);

// --------------------------------------------------------------------

/*! \class ipe::Bitmap
  \ingroup base
  \brief A bitmap.

  Bitmaps are explicitely shared using reference-counting.  Copying is
  cheap, so Bitmap objects are meant to be passed by value.

  The bitmap provides a slot for short-term storage of an "object
  number".  The PDF embedder, for instance, sets it to the PDF object
  number when embedding the bitmap, and can reuse it when "drawing"
  the bitmap.
*/

//! Default constructor constructs null bitmap.
Bitmap::Bitmap()
{
  iImp = nullptr;
}

//! Create from XML stream.
Bitmap::Bitmap(const XmlAttributes &attr, String pcdata)
{
  auto lengths = init(attr);
  int length = lengths.first;
  if (length == 0)
    length = height() * width() * (isGray() ? 1 : 3);
  int alphaLength = lengths.second;
  // decode data
  iImp->iData = Buffer(length);
  char *p = iImp->iData.data();
  Buffer alpha;
  char *q = nullptr;
  if (alphaLength > 0) {
    alpha = Buffer(alphaLength);
    q = alpha.data();
  }
  if (attr["encoding"] ==  "base64") {
    Buffer dbuffer(pcdata.data(), pcdata.size());
    BufferSource source(dbuffer);
    Base64Source b64source(source);
    while (length-- > 0)
      *p++ = b64source.getChar();
    while (alphaLength-- > 0)
      *q++ = b64source.getChar();
  } else {
    Lex datalex(pcdata);
    while (length-- > 0)
      *p++ = char(datalex.getHexByte());
    while (alphaLength-- > 0)
      *q++ = char(datalex.getHexByte());
  }
  unpack(alpha);
  computeChecksum();
  analyze();
}

//! Create from XML using external raw data
Bitmap::Bitmap(const XmlAttributes &attr, Buffer data, Buffer alpha)
{
  init(attr);
  iImp->iData = data;
  unpack(alpha);
  computeChecksum();
  analyze();
}

std::pair<int, int> Bitmap::init(const XmlAttributes &attr)
{
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iFlags = 0;
  iImp->iColorKey = -1;
  iImp->iPixelsComputed = false;
  iImp->iObjNum = Lex(attr["id"]).getInt();
  iImp->iWidth = Lex(attr["width"]).getInt();
  iImp->iHeight = Lex(attr["height"]).getInt();
  int length = Lex(attr["length"]).getInt();
  int alphaLength = Lex(attr["alphaLength"]).getInt();
  assert(iImp->iWidth > 0 && iImp->iHeight > 0);
  String cs = attr["ColorSpace"];
  if (cs.right(5) =="Alpha") {
    iImp->iFlags |= EAlpha;
    cs = cs.left(cs.size() - 5);
  }
  if (cs == "DeviceRGB")
    iImp->iFlags |= ERGB;
  String fi = attr["Filter"];
  if (fi == "DCTDecode")
    iImp->iFlags |= EDCT;
  else if (fi == "FlateDecode")
    iImp->iFlags |= EInflate;
  String cc;
  if (!isJpeg() && attr.has("ColorKey", cc))
    iImp->iColorKey = Lex(cc).getHexNumber();
  return std::make_pair(length, alphaLength);
}

//! Create a new image from given image data.
/*! If you already have data in native-endian ARGB32 without premultiplication,
  pass it with flag ENative.
  Otherwise pass a byte stream and set ERGB and EAlpha correctly:
  EAlpha: each pixel starts with one byte of alpha channel,
  ERGB: each pixel has three bytes of R, G, B, in this order,
  otherwise each pixel has one byte of gray value. */
Bitmap::Bitmap(int width, int height, uint32_t flags, Buffer data)
{
  iImp = new Imp;
  iImp->iRefCount = 1;
  iImp->iFlags = flags;
  iImp->iColorKey = -1;
  iImp->iObjNum = -1;
  iImp->iWidth = width;
  iImp->iHeight = height;
  iImp->iData = data;
  iImp->iPixelsComputed = false;
  assert(iImp->iWidth > 0 && iImp->iHeight > 0);
  unpack(Buffer());
  computeChecksum();
  analyze();
}

//! Take care of inflating, converting grayscale to rgb, and merging the alpha channel
void Bitmap::unpack(Buffer alphaChannel)
{
  if (isJpeg() || iImp->iFlags & ENative)
    return;
  int npixels = width() * height();
  if (iImp->iFlags & EInflate) {
    // inflate data
    int components = isGray() ? 1 : 3;
    if (hasAlpha() && alphaChannel.size() == 0)
      components += 1;
    uLongf inflatedSize = npixels * components;
    Buffer inflated(inflatedSize);
    assert(uncompress((Bytef *) inflated.data(), &inflatedSize,
		      (const Bytef *) iImp->iData.data(), iImp->iData.size()) == Z_OK);
    iImp->iData = inflated;
    if (alphaChannel.size() > 0) {
      inflatedSize = npixels;
      Buffer inflatedAlpha(inflatedSize);
      assert(uncompress((Bytef *) inflatedAlpha.data(), &inflatedSize,
			(const Bytef *) alphaChannel.data(), alphaChannel.size()) == Z_OK);
      alphaChannel = inflatedAlpha;
    }
  }
  // convert data to ARGB32 format
  bool alphaInMain = hasAlpha() && alphaChannel.size() == 0;
  Buffer pixels(npixels * sizeof(uint32_t));
  const char *p = iImp->iData.data();
  uint32_t *q = (uint32_t *) pixels.data();
  uint32_t *fin = q + npixels;
  if (!isGray()) {
    while (q < fin) {
      uint8_t alpha = (alphaInMain ? uint8_t(*p++) : 0xff);
      uint8_t r = uint8_t(*p++);
      uint8_t g = uint8_t(*p++);
      uint8_t b = uint8_t(*p++);
      uint32_t pixel = (alpha << 24) | (r << 16) | (g << 8) | b;
      *q++ = pixel;
    }
  } else {
    while (q < fin) {
      uint8_t alpha = (alphaInMain ? uint8_t(*p++) : 0xff);
      uint8_t r = uint8_t(*p++);
      *q++ = (alpha << 24) | (r << 16) | (r << 8) | r;
    }
  }
  // merge separate alpha channel
  if (hasAlpha() && alphaChannel.size() > 0) {
    q = (uint32_t *) pixels.data();
    p = alphaChannel.data();
    uint32_t pixel;
    while (q < fin) {
      pixel = *q;
      pixel = (pixel & 0x00ffffff) | (*p++ << 24);
      *q++ = pixel;
    }
  }
  if (iImp->iColorKey >= 0) {
    uint32_t colorKey = (iImp->iColorKey | 0xff000000);
    q = (uint32_t *) pixels.data();
    while (q < fin) {
      if (*q == colorKey)
	*q = iImp->iColorKey;
      ++q;
    }
  }
  iImp->iData = pixels;
}

//! Determine if bitmap has alpha channel, colorkey, rgb values (does nothing for JPG).
void Bitmap::analyze()
{
  iImp->iColorKey = -1;
  iImp->iFlags &= EDCT|ERGB;  // clear all other flags, we recompute them
  if (isJpeg())
    return;
  iImp->iFlags &= EDCT;  // ERGB will also be recomputed
  const uint32_t *q = (const uint32_t *) iImp->iData.data();
  const uint32_t *fin = q + width() * height();
  uint32_t pixel, gray;
  while (q < fin) {
    pixel = *q++ & 0x00ffffff;
    gray = (pixel & 0xff);
    gray |=  (gray << 8) | (gray << 16);
    if (pixel != gray) {
      iImp->iFlags |= ERGB;
      break;
    }
  }
  int candidate = -1, color;
  uint32_t alpha;
  q = (const uint32_t *) iImp->iData.data();
  while (q < fin) {
    pixel = *q++;
    alpha = pixel & 0xff000000;
    color = pixel & 0x00ffffff;
    if (alpha != 0 && alpha != 0xff000000) {
      iImp->iFlags |= EAlpha;
      return;
    }
    if (alpha == 0) { // transparent color found
      if (candidate < 0)
	candidate = color;
      else if (candidate != color) { // two different transparent colors
	iImp->iFlags |= EAlpha;
	return;
      }
    } else if (color == candidate) {   // opaque copy of candidate found
      iImp->iFlags |= EAlpha;
      return;
    }
  }
  iImp->iColorKey = candidate;
}

//! Copy constructor.
/*! Since Bitmaps are reference counted, this is very fast. */
Bitmap::Bitmap(const Bitmap &rhs)
{
  iImp = rhs.iImp;
  if (iImp)
    iImp->iRefCount++;
}

//! Destructor.
Bitmap::~Bitmap()
{
  if (iImp && --iImp->iRefCount == 0) {
    delete iImp;
  }
}

//! Assignment operator (takes care of reference counting).
/*! Very fast. */
Bitmap &Bitmap::operator=(const Bitmap &rhs)
{
  if (this != &rhs) {
    if (iImp && --iImp->iRefCount == 0)
      delete iImp;
    iImp = rhs.iImp;
    if (iImp)
      iImp->iRefCount++;
  }
  return *this;
}

//! Save bitmap in XML stream.
void Bitmap::saveAsXml(Stream &stream, int id, int pdfObjNum) const
{
  assert(iImp);
  stream << "<bitmap"
	 << " id=\"" << id << "\""
	 << " width=\"" << width() << "\""
	 << " height=\"" << height() << "\""
	 << " BitsPerComponent=\"8\""; // no longer used but required by earlier Ipe versions

  stream << " ColorSpace=\"Device";
  if (isGray())
    stream << "Gray";
  else
    stream << "RGB";
  if (hasAlpha())
    stream << "Alpha";
  stream << "\"";
  if (isJpeg())
    stream << " Filter=\"DCTDecode\"";
  else
    stream << " Filter=\"FlateDecode\"";

  if (colorKey() >= 0) {
    char buf[10];
    sprintf(buf, "%x", colorKey());
    stream << " ColorKey=\"" << buf << "\"";
  }

  if (pdfObjNum >= 0) {
    stream << " pdfObject=\"" << pdfObjNum;
    if (hasAlpha())
      stream << " " << pdfObjNum-1;
    stream << "\"/>\n";
  } else {
    // save data
    auto data = embed();
    stream << " length=\"" << data.first.size() << "\"";
    if (hasAlpha())
      stream << " alphaLength=\"" << data.second.size() << "\"";
    stream << " encoding=\"base64\">\n";
    Base64Stream b64(stream);
    for (const auto & buffer : { data.first, data.second } ) {
      const char *p = buffer.data();
      const char *fin = p + buffer.size();
      while (p != fin)
	b64.putChar(*p++);
    }
    b64.close();
    stream << "</bitmap>\n";
  }
}

bool Bitmap::equal(Bitmap rhs) const
{
  if (iImp == rhs.iImp)
    return true;
  if (!iImp || !rhs.iImp)
    return false;

  if (iImp->iFlags != rhs.iImp->iFlags ||
      iImp->iWidth != rhs.iImp->iWidth ||
      iImp->iHeight != rhs.iImp->iHeight ||
      iImp->iChecksum != rhs.iImp->iChecksum ||
      iImp->iData.size() != rhs.iImp->iData.size())
    return false;
  // check actual data
  int len = iImp->iData.size();
  char *p = iImp->iData.data();
  char *q = rhs.iImp->iData.data();
  while (len--) {
    if (*p++ != *q++)
      return false;
  }
  return true;
}

static uint32_t crc32(uint32_t crc, const uint8_t *data, size_t len)
{
  static const uint32_t table[256] =
    {
     0x00000000U,0x04C11DB7U,0x09823B6EU,0x0D4326D9U,
     0x130476DCU,0x17C56B6BU,0x1A864DB2U,0x1E475005U,
     0x2608EDB8U,0x22C9F00FU,0x2F8AD6D6U,0x2B4BCB61U,
     0x350C9B64U,0x31CD86D3U,0x3C8EA00AU,0x384FBDBDU,
     0x4C11DB70U,0x48D0C6C7U,0x4593E01EU,0x4152FDA9U,
     0x5F15ADACU,0x5BD4B01BU,0x569796C2U,0x52568B75U,
     0x6A1936C8U,0x6ED82B7FU,0x639B0DA6U,0x675A1011U,
     0x791D4014U,0x7DDC5DA3U,0x709F7B7AU,0x745E66CDU,
     0x9823B6E0U,0x9CE2AB57U,0x91A18D8EU,0x95609039U,
     0x8B27C03CU,0x8FE6DD8BU,0x82A5FB52U,0x8664E6E5U,
     0xBE2B5B58U,0xBAEA46EFU,0xB7A96036U,0xB3687D81U,
     0xAD2F2D84U,0xA9EE3033U,0xA4AD16EAU,0xA06C0B5DU,
     0xD4326D90U,0xD0F37027U,0xDDB056FEU,0xD9714B49U,
     0xC7361B4CU,0xC3F706FBU,0xCEB42022U,0xCA753D95U,
     0xF23A8028U,0xF6FB9D9FU,0xFBB8BB46U,0xFF79A6F1U,
     0xE13EF6F4U,0xE5FFEB43U,0xE8BCCD9AU,0xEC7DD02DU,
     0x34867077U,0x30476DC0U,0x3D044B19U,0x39C556AEU,
     0x278206ABU,0x23431B1CU,0x2E003DC5U,0x2AC12072U,
     0x128E9DCFU,0x164F8078U,0x1B0CA6A1U,0x1FCDBB16U,
     0x018AEB13U,0x054BF6A4U,0x0808D07DU,0x0CC9CDCAU,
     0x7897AB07U,0x7C56B6B0U,0x71159069U,0x75D48DDEU,
     0x6B93DDDBU,0x6F52C06CU,0x6211E6B5U,0x66D0FB02U,
     0x5E9F46BFU,0x5A5E5B08U,0x571D7DD1U,0x53DC6066U,
     0x4D9B3063U,0x495A2DD4U,0x44190B0DU,0x40D816BAU,
     0xACA5C697U,0xA864DB20U,0xA527FDF9U,0xA1E6E04EU,
     0xBFA1B04BU,0xBB60ADFCU,0xB6238B25U,0xB2E29692U,
     0x8AAD2B2FU,0x8E6C3698U,0x832F1041U,0x87EE0DF6U,
     0x99A95DF3U,0x9D684044U,0x902B669DU,0x94EA7B2AU,
     0xE0B41DE7U,0xE4750050U,0xE9362689U,0xEDF73B3EU,
     0xF3B06B3BU,0xF771768CU,0xFA325055U,0xFEF34DE2U,
     0xC6BCF05FU,0xC27DEDE8U,0xCF3ECB31U,0xCBFFD686U,
     0xD5B88683U,0xD1799B34U,0xDC3ABDEDU,0xD8FBA05AU,
     0x690CE0EEU,0x6DCDFD59U,0x608EDB80U,0x644FC637U,
     0x7A089632U,0x7EC98B85U,0x738AAD5CU,0x774BB0EBU,
     0x4F040D56U,0x4BC510E1U,0x46863638U,0x42472B8FU,
     0x5C007B8AU,0x58C1663DU,0x558240E4U,0x51435D53U,
     0x251D3B9EU,0x21DC2629U,0x2C9F00F0U,0x285E1D47U,
     0x36194D42U,0x32D850F5U,0x3F9B762CU,0x3B5A6B9BU,
     0x0315D626U,0x07D4CB91U,0x0A97ED48U,0x0E56F0FFU,
     0x1011A0FAU,0x14D0BD4DU,0x19939B94U,0x1D528623U,
     0xF12F560EU,0xF5EE4BB9U,0xF8AD6D60U,0xFC6C70D7U,
     0xE22B20D2U,0xE6EA3D65U,0xEBA91BBCU,0xEF68060BU,
     0xD727BBB6U,0xD3E6A601U,0xDEA580D8U,0xDA649D6FU,
     0xC423CD6AU,0xC0E2D0DDU,0xCDA1F604U,0xC960EBB3U,
     0xBD3E8D7EU,0xB9FF90C9U,0xB4BCB610U,0xB07DABA7U,
     0xAE3AFBA2U,0xAAFBE615U,0xA7B8C0CCU,0xA379DD7BU,
     0x9B3660C6U,0x9FF77D71U,0x92B45BA8U,0x9675461FU,
     0x8832161AU,0x8CF30BADU,0x81B02D74U,0x857130C3U,
     0x5D8A9099U,0x594B8D2EU,0x5408ABF7U,0x50C9B640U,
     0x4E8EE645U,0x4A4FFBF2U,0x470CDD2BU,0x43CDC09CU,
     0x7B827D21U,0x7F436096U,0x7200464FU,0x76C15BF8U,
     0x68860BFDU,0x6C47164AU,0x61043093U,0x65C52D24U,
     0x119B4BE9U,0x155A565EU,0x18197087U,0x1CD86D30U,
     0x029F3D35U,0x065E2082U,0x0B1D065BU,0x0FDC1BECU,
     0x3793A651U,0x3352BBE6U,0x3E119D3FU,0x3AD08088U,
     0x2497D08DU,0x2056CD3AU,0x2D15EBE3U,0x29D4F654U,
     0xC5A92679U,0xC1683BCEU,0xCC2B1D17U,0xC8EA00A0U,
     0xD6AD50A5U,0xD26C4D12U,0xDF2F6BCBU,0xDBEE767CU,
     0xE3A1CBC1U,0xE760D676U,0xEA23F0AFU,0xEEE2ED18U,
     0xF0A5BD1DU,0xF464A0AAU,0xF9278673U,0xFDE69BC4U,
     0x89B8FD09U,0x8D79E0BEU,0x803AC667U,0x84FBDBD0U,
     0x9ABC8BD5U,0x9E7D9662U,0x933EB0BBU,0x97FFAD0CU,
     0xAFB010B1U,0xAB710D06U,0xA6322BDFU,0xA2F33668U,
     0xBCB4666DU,0xB8757BDAU,0xB5365D03U,0xB1F740B4U,
    };
  while (len > 0) {
    crc = table[*data ^ ((crc >> 24) & 0xff)] ^ (crc << 8);
    data++;
    len--;
  }
  return crc;
}

void Bitmap::computeChecksum()
{
  iImp->iChecksum = crc32(0xffffffffU, (uint8_t *) iImp->iData.data(), iImp->iData.size());
}

//! Create the data to be embedded in an XML or PDF file.
/*! For Jpeg images, this is simply the bitmap data.  For other
  images, rgb/grayscale data and alpha channel are split and deflated
  separately. */
std::pair<Buffer, Buffer> Bitmap::embed() const
{
  if (isJpeg())
    return std::make_pair(iImp->iData, Buffer());
  int npixels = width() * height();
  uint32_t *src = (uint32_t *) iImp->iData.data();
  uint32_t *fin = src + npixels;
  uint32_t pixel;
  Buffer rgb(npixels * (isGray() ? 1 : 3));
  char *p = rgb.data();
  while (src < fin) {
    pixel = *src++;
    if (isGray()) {
      *p++ = pixel & 0xff;
    } else {
      *p++ = (pixel & 0xff0000) >> 16;
      *p++ = (pixel & 0x00ff00) >> 8;
      *p++ = (pixel & 0x0000ff);
    }
  }
  int deflatedSize;
  Buffer deflated = DeflateStream::deflate(rgb.data(), rgb.size(), deflatedSize, 9);
  rgb = Buffer(deflated.data(), deflatedSize);
  Buffer alpha;
  if (hasAlpha()) {
    alpha = Buffer(npixels);
    src = (uint32_t *) iImp->iData.data();
    p = alpha.data();
    while (src < fin)
      *p++ = (*src++ & 0xff000000) >> 24;
    deflated = DeflateStream::deflate(alpha.data(), alpha.size(), deflatedSize, 9);
    alpha = Buffer(deflated.data(), deflatedSize);
  }
  return std::make_pair(rgb, alpha);
}

// --------------------------------------------------------------------

void Bitmap::savePixels(const char *fname)
{
  FILE *file = Platform::fopen(fname, "wb");
  if (!file)
    return;
  if (isJpeg()) {
    fwrite(iImp->iData.data(), 1, iImp->iData.size(), file);
  } else {
    fprintf(file, "PyRGBA\n%d %d\n255\n", width(), height());
    Buffer pixels = Buffer(iImp->iData.size());
    uint32_t *p = (uint32_t *) iImp->iData.data();
    uint8_t *q = (uint8_t *) pixels.data();
    uint32_t *fin = p + width() * height();
    while (p < fin) {
      uint32_t pixel = *p++;
      *q++ = (pixel & 0x00ff0000) >> 16;
      *q++ = (pixel & 0x0000ff00) >> 8;
      *q++ = (pixel & 0x000000ff);
      *q++ = (pixel & 0xff000000) >> 24;
    }
    fwrite(pixels.data(), 1, iImp->iData.size(), file);
  }
  fclose(file);
}

// --------------------------------------------------------------------

//! Return pixels for rendering.
/*! Returns empty buffer if it cannot decode the bitmap information.
  Otherwise, returns a buffer of size width() * height() uint32_t's.
  The data is in cairo ARGB32 format, that is native-endian uint32_t's
  with premultiplied alpha.
*/
Buffer Bitmap::pixelData()
{
  if (!iImp->iPixelsComputed) {
    iImp->iPixelsComputed = true;
    if (isJpeg()) {
      Buffer stream = iImp->iData;
      Buffer pixels;
      pixels = Buffer(4 * width() * height());
      if (!dctDecode(stream, pixels))
	return Buffer();
      iImp->iPixelData = pixels;
    } else {
      if (hasAlpha() || colorKey() >= 0) {
	// premultiply RGB data
	iImp->iPixelData = Buffer(iImp->iData.size());
	uint32_t *p = (uint32_t *) iImp->iData.data();
	uint32_t *q = (uint32_t *) iImp->iPixelData.data();
	uint32_t *fin = p + width() * height();
	uint32_t pixel, alpha, alphaM, r, g, b;
	while (p < fin) {
	  pixel = *p++;
	  alpha = (pixel & 0xff000000);
	  alphaM = alpha >> 24;
	  r = alphaM * (pixel & 0xff0000) / 255;
	  g = alphaM * (pixel & 0x00ff00) / 255;
	  b = alphaM * (pixel & 0x0000ff) / 255;
	  *q++ = alpha | (r & 0xff0000) | (g & 0x00ff00) | (b & 0x0000ff);
	}
      } else
	iImp->iPixelData = iImp->iData;
    }
  }
  return iImp->iPixelData;
}

// --------------------------------------------------------------------

/*
 JPG reading code
 Copyright (c) 1996-2002 Han The Thanh, <thanh@pdftex.org>

 This code is part of pdfTeX.

 pdfTeX is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
*/

#define JPG_GRAY  1     /* Gray color space, use /DeviceGray  */
#define JPG_RGB   3     /* RGB color space, use /DeviceRGB    */
#define JPG_CMYK  4     /* CMYK color space, use /DeviceCMYK  */

enum JPEG_MARKER {      /* JPEG marker codes                    */
  M_SOF0  = 0xc0,       /* baseline DCT                         */
  M_SOF1  = 0xc1,       /* extended sequential DCT              */
  M_SOF2  = 0xc2,       /* progressive DCT                      */
  M_SOF3  = 0xc3,       /* lossless (sequential)                */

  M_SOF5  = 0xc5,       /* differential sequential DCT          */
  M_SOF6  = 0xc6,       /* differential progressive DCT         */
  M_SOF7  = 0xc7,       /* differential lossless                */

  M_JPG   = 0xc8,       /* JPEG extensions                      */
  M_SOF9  = 0xc9,       /* extended sequential DCT              */
  M_SOF10 = 0xca,       /* progressive DCT                      */
  M_SOF11 = 0xcb,       /* lossless (sequential)                */

  M_SOF13 = 0xcd,       /* differential sequential DCT          */
  M_SOF14 = 0xce,       /* differential progressive DCT         */
  M_SOF15 = 0xcf,       /* differential lossless                */

  M_DHT   = 0xc4,       /* define Huffman tables                */

  M_DAC   = 0xcc,       /* define arithmetic conditioning table */

  M_RST0  = 0xd0,       /* restart                              */
  M_RST1  = 0xd1,       /* restart                              */
  M_RST2  = 0xd2,       /* restart                              */
  M_RST3  = 0xd3,       /* restart                              */
  M_RST4  = 0xd4,       /* restart                              */
  M_RST5  = 0xd5,       /* restart                              */
  M_RST6  = 0xd6,       /* restart                              */
  M_RST7  = 0xd7,       /* restart                              */

  M_SOI   = 0xd8,       /* start of image                       */
  M_EOI   = 0xd9,       /* end of image                         */
  M_SOS   = 0xda,       /* start of scan                        */
  M_DQT   = 0xdb,       /* define quantization tables           */
  M_DNL   = 0xdc,       /* define number of lines               */
  M_DRI   = 0xdd,       /* define restart interval              */
  M_DHP   = 0xde,       /* define hierarchical progression      */
  M_EXP   = 0xdf,       /* expand reference image(s)            */

  M_APP0  = 0xe0,       /* application marker, used for JFIF    */
  M_APP1  = 0xe1,       /* application marker                   */
  M_APP2  = 0xe2,       /* application marker                   */
  M_APP3  = 0xe3,       /* application marker                   */
  M_APP4  = 0xe4,       /* application marker                   */
  M_APP5  = 0xe5,       /* application marker                   */
  M_APP6  = 0xe6,       /* application marker                   */
  M_APP7  = 0xe7,       /* application marker                   */
  M_APP8  = 0xe8,       /* application marker                   */
  M_APP9  = 0xe9,       /* application marker                   */
  M_APP10 = 0xea,       /* application marker                   */
  M_APP11 = 0xeb,       /* application marker                   */
  M_APP12 = 0xec,       /* application marker                   */
  M_APP13 = 0xed,       /* application marker                   */
  M_APP14 = 0xee,       /* application marker, used by Adobe    */
  M_APP15 = 0xef,       /* application marker                   */

  M_JPG0  = 0xf0,       /* reserved for JPEG extensions         */
  M_JPG13 = 0xfd,       /* reserved for JPEG extensions         */
  M_COM   = 0xfe,       /* comment                              */

  M_TEM   = 0x01,       /* temporary use                        */
};

inline int read2bytes(FILE *f)
{
  uint8_t c1 = fgetc(f);
  uint8_t c2 = fgetc(f);
  return (c1 << 8) + c2;
}

// --------------------------------------------------------------------

//! Read information about JPEG image from file.
/*! Returns NULL on success, an error message otherwise.  Sets flags
  to EDCT and possibly ERGB. */
const char *Bitmap::readJpegInfo(FILE *file, int &width, int &height,
				 Vector &dotsPerInch, uint32_t &flags)
{
  static char jpg_id[] = "JFIF";
  bool app0_seen = false;

  dotsPerInch = Vector(0, 0);
  flags = EDCT;

  if (read2bytes(file) != 0xFFD8) {
    return "The file does not appear to be a JPEG image";
  }

  for (;;) {
    int ch = fgetc(file);
    if (ch != 0xff)
      return "Reading JPEG image failed";
    do {
      ch = fgetc(file);
    } while (ch == 0xff);
    ipeDebug("JPEG tag %x", ch & 0xff);
    int fpos = ftell(file);
    switch (ch & 0xff) {

    case M_SOF5:
    case M_SOF6:
    case M_SOF7:
    case M_SOF9:
    case M_SOF10:
    case M_SOF11:
    case M_SOF13:
    case M_SOF14:
    case M_SOF15:
      return "Unsupported type of JPEG compression";

    case M_SOF0:
    case M_SOF1:
    case M_SOF2: // progressive DCT allowed since PDF-1.3
    case M_SOF3:
      read2bytes(file);    /* read segment length  */
      ch = fgetc(file);
      if (ch != 8)
	return "Unsupported bit width of pixels in JPEG image";
      height = read2bytes(file);
      width = read2bytes(file);
      ch = fgetc(file);
      switch (ch & 0xff) {
      case JPG_GRAY:
        break;
      case JPG_RGB:
	flags |= ERGB;
        break;
      default:
	return "Unsupported color space in JPEG image";
      }
      fseek(file, 0, SEEK_SET);
      return nullptr;      //  success!

    case M_APP0: {
      int len = read2bytes(file);
      if (app0_seen) {
	fseek(file, fpos + len, SEEK_SET);
	break;
      }
      for (int i = 0; i < 5; i++) {
	ch = fgetc(file);
	if (ch != jpg_id[i]) {
	  return "Reading JPEG image failed";
	}
      }
      read2bytes(file); // JFIF version
      char units = fgetc(file);
      int xres = read2bytes(file);
      int yres = read2bytes(file);
      if (xres != 0 && yres != 0) {
	switch (units) {
	case 1: /* pixels per inch */
	  dotsPerInch = Vector(xres, yres);
	  break;
	case 2: /* pixels per cm */
	  dotsPerInch = Vector(xres * 2.54, yres * 2.54);
	  break;
	default: // 0: aspect ratio only
	  break;
	}
      }
      app0_seen = true;
      fseek(file, fpos + len, SEEK_SET);
      break; }

    case M_SOI:      // ignore markers without parameters
    case M_EOI:
    case M_TEM:
    case M_RST0:
    case M_RST1:
    case M_RST2:
    case M_RST3:
    case M_RST4:
    case M_RST5:
    case M_RST6:
    case M_RST7:
      break;

    default:         // skip variable length markers
	fseek(file, fpos + read2bytes(file), SEEK_SET);
      break;
    }
  }
}

//! Read JPEG image from file.
/*! Returns the image as a DCT-encoded Bitmap.
  Sets \a dotsPerInch if the image file contains a resolution,
  otherwise sets it to (0,0).
  If reading the file fails, returns a null Bitmap,
  and sets the error message \a errmsg.
*/
Bitmap Bitmap::readJpeg(const char *fname, Vector &dotsPerInch, const char * &errmsg)
{
  FILE *file = Platform::fopen(fname, "rb");
  if (!file) {
    errmsg = "Error opening file";
    return Bitmap();
  }

  int width, height;
  uint32_t flags;

  errmsg = Bitmap::readJpegInfo(file, width, height, dotsPerInch, flags);

  fclose(file);
  if (errmsg)
    return Bitmap();

  String a = Platform::readFile(fname);
  return  Bitmap(width, height, flags, Buffer(a.data(), a.size()));
}

// --------------------------------------------------------------------

// --------------------------------------------------------------------
// Rendering fonts onto the canvas
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

#include "ipefonts.h"
#include "ipepdfparser.h"
#include "ipexml.h"

#include <string>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/t1tables.h>

#include <cairo-ft.h>

using namespace ipe;

// --------------------------------------------------------------------

struct FaceEntry {
  String iName;
  uint32_t iChecksum;
  Buffer iFontFile;
  FT_Face iFace;
  cairo_font_face_t *iCairoFont;
};

static const cairo_user_data_key_t datakey = { 0 };

struct Engine {
public:
  Engine();
  ~Engine();
  cairo_font_face_t *screenFont();
  std::pair<cairo_font_face_t *, FT_Face> getCairoFont(String name, const Buffer & data);
  void discard(FT_Face ftFace);

private:
  bool iScreenFontLoaded;
  cairo_font_face_t *iScreenFont;
  std::vector<FaceEntry> iCache;

public:
  bool iOk;
  FT_Library iLib;
  int iFacesCreated;
  int iFacesDiscarded;
  int iFacesLoaded;
  int iFacesUnloaded;
};

// Auto-constructed and destructed Freetype engine.
static Engine engine;

// --------------------------------------------------------------------

Engine::Engine()
{
  iOk = false;
  iScreenFont = nullptr;
  iScreenFontLoaded = false;
  iFacesCreated = 0;
  iFacesDiscarded = 0;
  iFacesLoaded = 0;
  iFacesUnloaded = 0;
  if (FT_Init_FreeType(&iLib))
    return;
  iOk = true;
}

Engine::~Engine()
{
  if (iScreenFont)
    cairo_font_face_destroy(iScreenFont);
  // clear Cairo caches so we can check we have unloaded everything
  cairo_debug_reset_static_data();
  ipeDebug("Freetype engine: %d faces created, %d faces discarded, "
	   "%d faces loaded, %d faces unloaded.",
	   iFacesCreated, iFacesDiscarded, iFacesLoaded, iFacesUnloaded);
  FT_Done_FreeType(iLib);
}

// --------------------------------------------------------------------

cairo_font_face_t *Engine::screenFont()
{
  if (!iOk) return nullptr;
  if (!iScreenFontLoaded) {
    iScreenFontLoaded = true;
    iScreenFont = cairo_toy_font_face_create("Sans", CAIRO_FONT_SLANT_NORMAL,
					     CAIRO_FONT_WEIGHT_BOLD);
  }
  return iScreenFont;
}

void Engine::discard(FT_Face ftFace)
{
  ++iFacesDiscarded;
  auto it = std::find_if(iCache.begin(), iCache.end(),
			 [ftFace](const FaceEntry & entry) {
			   return entry.iFace == ftFace;
			 });
  if (it != iCache.end()) {
    ipeDebug("Discarding face %s", it->iName.z());
    FT_Done_Face(ftFace);  // discard Freetype face
    iCache.erase(it); // frees the buffer
  } else
    ipeDebug("Discarded face not found in cache!");
}

static void face_data_destroy(FT_Face ftFace)
{
  engine.discard(ftFace);
}

std::pair<cairo_font_face_t *, FT_Face> Engine::getCairoFont(String name, const Buffer & data)
{
  uint32_t checksum = data.checksum();
  for (auto & entry : iCache) {
    if (entry.iName == name && entry.iChecksum == checksum) {
      ipeDebug("Found font %s in cache with %d references", name.z(),
	       cairo_font_face_get_reference_count(entry.iCairoFont));
      cairo_font_face_reference(entry.iCairoFont);
      return std::pair{entry.iCairoFont, entry.iFace};
    }
  }

  FaceEntry entry;
  entry.iName = name;
  entry.iChecksum = checksum;
  entry.iFontFile = data;
  int error = FT_New_Memory_Face(engine.iLib, (const uint8_t *) entry.iFontFile.data(),
				 entry.iFontFile.size(), 0, &entry.iFace);
  if (error) {
    ipeDebug("Error creating Cairo font %s", name.z());
    return std::pair{nullptr, nullptr};
  }

  entry.iCairoFont = cairo_ft_font_face_create_for_ft_face(entry.iFace, 0);

  // see cairo_ft_font_face_create_for_ft_face docs,
  // it explains why the user_data is necessary
  cairo_status_t status =
    cairo_font_face_set_user_data(entry.iCairoFont, &datakey, entry.iFace,
				  (cairo_destroy_func_t) face_data_destroy);
  if (status) {
    ipeDebug("Failed to set user data for Cairo font %s", name.z());
    cairo_font_face_destroy(entry.iCairoFont);
    FT_Done_Face(entry.iFace);
    return std::pair{nullptr, nullptr};
  }
  ++engine.iFacesCreated;
  iCache.push_back(entry);
  return std::pair{entry.iCairoFont, entry.iFace};
}

// --------------------------------------------------------------------

/*! \class ipe::Fonts
  \ingroup cairo
  \brief Provides the fonts used to render text.
*/

Fonts::Fonts(const PdfResourceBase *resources) : iResources(resources)
{
  // nothing
}

String Fonts::freetypeVersion()
{
  int major, minor, patch;
  FT_Library_Version(engine.iLib, &major, &minor, &patch);
  char buf[128];
  sprintf(buf, "Freetype %d.%d.%d / %d.%d.%d",
	  FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH,
	  major, minor, patch);
  return String(buf);
}

//! Return a Cairo font to render to the screen w/o Latex font.
cairo_font_face_t *Fonts::screenFont()
{
  return engine.screenFont();
}

//! Get a typeface.
/*! Corresponds to a Freetype "face", or a PDF font resource.  A Face
  can be loaded at various sizes (transformations), resulting in
  individual FaceSize's. */
Face *Fonts::getFace(const PdfDict *d)
{
  if (!engine.iOk)
    return nullptr;

  auto it = std::find_if(iFaces.begin(), iFaces.end(),
			 [d](std::unique_ptr<Face> &f) { return f->matches(d); } );
  if (it != iFaces.end())
    return it->get();

  iFaces.emplace_back(std::make_unique<Face>(d, iResources));
  return iFaces.back().get();
}

bool Fonts::hasType3Font() const noexcept
{
  for (const auto &face: iFaces)
    if (face->type() == FontType::Type3)
      return true;
  return false;
}

// --------------------------------------------------------------------

/*! \class ipe::Face
  \ingroup cairo
  \brief A typeface (aka font), actually loaded (from a font file or PDF file).
*/

Face::Face(const PdfDict *d, const PdfResourceBase *resources) noexcept
:  iFontDict(d), iResources(resources)
{
  /* d:
    /Type /Font
    /Subtype /Type1
    /Encoding 24 0 R
    /FirstChar 6
    /LastChar 49
    /Widths 25 0 R
    /BaseFont /YEHLEP+CMR10
    /FontDescriptor 7 0 R
  */
  ++engine.iFacesLoaded;
  const PdfObj *type = d->get("Type");
  if (!type || !type->name() || type->name()->value() != "Font")
    return;
  type = d->get("Subtype");
  if (!type || !type->name())
    return;
  String t = type->name()->value();
  const PdfDict *d0 = d;
  if (t == "Type0") {
    const PdfObj *desc = getPdf(d, "DescendantFonts");
    if (!desc || !desc->array())
      return;
    desc = desc->array()->obj(0, nullptr);
    if (!desc)
      return;
    if (desc->ref())
      desc = iResources->object(desc->ref()->value());
    if (!desc || !desc->dict())
      return;
    d = desc->dict();
    type = d->get("Subtype");
    if (!type || !type->name())
      return;
    t = type->name()->value();
  }

  const PdfObj *name = getPdf(d, "BaseFont");
  if (name && name->name())
    iName = name->name()->value();

  // ipeDebug("Font '%s' of type '%s'", iName.z(), t.z());

  if (t == "Type1")
    iType = FontType::Type1;
  else if (t == "TrueType")
    iType = FontType::Truetype;
  else if (t == "CIDFontType0")
    iType = FontType::CIDType0;
  else if (t == "CIDFontType2")
    iType = FontType::CIDType2;
  else if (t == "Type3") {
    iType = FontType::Type3;
    getType3Width(d);
    return;
  } else {
    iType = FontType::Unsupported;
    return;
  }

  Buffer data;
  if (!getFontFile(d, data)) {
    ipeDebug("Failed to get font file for %s", iName.z());
    return;
  }

  std::tie(iCairoFont, iFace) = engine.getCairoFont(iName, data);
  if (!iCairoFont) {
    ipeDebug("Failed to create Cairo font for %s", iName.z());
    return;
  }

  if (iType == FontType::CIDType0 || iType == FontType::CIDType2) {
    getCIDWidth(d);
    const PdfObj *enc = getPdf(d0, "Encoding");
    if (!enc || !enc->name())
      return;
    String encoding = enc->name()->value();
    if (encoding != "Identity-H")
      ipeDebug("Unsupported encoding: %s", encoding.z());
    if (iType == FontType::CIDType2)
      getCIDToGIDMap(d);
    // ipeDebug("FT Face has %d charmaps, is cid-keyed: %d",
    // iFace->num_charmaps, FT_IS_CID_KEYED(iFace));
  } else {
    getSimpleWidth(d);
    if (iType == FontType::Type1)
      getType1Encoding(d);
    else
      setupTruetypeEncoding();
  }
  ipeDebug("Loaded font %s with %d references", iName.z(),
	   cairo_font_face_get_reference_count(iCairoFont));
}

Face::~Face() noexcept
{
  if (iCairoFont) {
    ipeDebug("Unloading Cairo face %s (%d references left)", iName.z(),
	     cairo_font_face_get_reference_count(iCairoFont));
    ++engine.iFacesUnloaded;
    cairo_font_face_destroy(iCairoFont);
  }
}

const PdfObj *Face::getPdf(const PdfDict *d, String key) const noexcept
{
  return iResources->getDeep(d, key);
}

// --------------------------------------------------------------------

int Face::width(int ch) const noexcept
{
  int i = 0;
  while (i < size(iWidth)) {
    if (iWidth[i] <= ch && ch <= iWidth[i+1]) {
      // found interval
      if (iWidth[i+2] < 0)
	return -iWidth[i+1] - 1;
      return iWidth[i + 2 + (ch - iWidth[i])];
    } else {
      if (iWidth[i+2] < 0)
	i += 3;
      else
	i += 3 + (iWidth[i+1] - iWidth[i]);
    }
  }
  return iDefaultWidth;
}

int Face::glyphIndex(int ch) noexcept
{
  if (!iCairoFont)
    return 0;
  switch (iType) {
  case FontType::Type1:
    return iEncoding[ch];
  case FontType::Truetype:
    return FT_Get_Char_Index(iFace, (FT_ULong) ch);
  case FontType::CIDType2:
    if (0 <= ch && ch < size(iCID2GID))
      return iCID2GID[ch];
    return ch; // cid-2-gid map is identity
  case FontType::CIDType0:
    return ch; // for cid-keyed font, this is a cid
  default:
    return 0;
  }
}

// --------------------------------------------------------------------

void Face::getCIDWidth(const PdfDict *d) noexcept
{
  const PdfObj *dw = getPdf(d, "DW");
  const PdfObj *w = getPdf(d, "W");
  iDefaultWidth = (!dw || !dw->number()) ? 0 : int(dw->number()->value());
  if (!w || !w->array())
    return;
  int i = 0;
  while (i + 1 < w->array()->count()) {
    const PdfObj *obj = w->array()->obj(i, nullptr);
    if (!obj->number())
      return;
    int beg = int(obj->number()->value());
    obj = w->array()->obj(i+1, nullptr);
    if (obj->number()) {
      int fin = int(obj->number()->value());
      if (i+2 == w->array()->count())
	return;
      obj = w->array()->obj(i+2, nullptr);
      if (!obj || !obj->number())
	return;
      iWidth.push_back(beg);
      iWidth.push_back(fin);
      iWidth.push_back(-int(obj->number()->value()) - 1);
      i += 3;
    } else if (obj->array()) {
      int fin = beg + obj->array()->count() - 1;
      iWidth.push_back(beg);
      iWidth.push_back(fin);
      for (int j = 0; j < obj->array()->count(); ++j) {
	const PdfObj *val = obj->array()->obj(j, nullptr);
	iWidth.push_back(val->number() ? int(val->number()->value())
			    : 1000);
      }
      i += 2;
    } else
      return;
  }
}

void Face::getCIDToGIDMap(const PdfDict *d) noexcept
{
  const PdfObj *map = getPdf(d, "CIDToGIDMap");
  if (!map || !map->dict()) return;
  Buffer buf = map->dict()->inflate();
  const uint8_t *p = (const uint8_t *) buf.data();
  const uint8_t *fin = p + buf.size();
  while (p < fin) {
    uint16_t gid = *p++ << 8;
    gid |= *p++;
    iCID2GID.push_back(gid);
  }
  ipeDebug("Font '%s' has CIDToGIDMap with %d entries", iName.z(), iCID2GID.size());
}

void Face::getSimpleWidth(const PdfDict *d) noexcept
{
  const PdfObj *fc = getPdf(d, "FirstChar");
  const PdfObj *wid = getPdf(d, "Widths");
  if (!fc || !fc->number() || !wid || !wid->array())
    return;
  int firstChar = int(fc->number()->value());
  iWidth.push_back(firstChar);
  iWidth.push_back(firstChar + wid->array()->count() - 1);
  for (int i = 0; i < wid->array()->count(); ++i) {
    const PdfObj *obj = wid->array()->obj(i, nullptr);
    iWidth.push_back(obj->number() ? int(obj->number()->value()) : 0);
  }
  // ipeDebug("Got %d widths entries", iWidth.size());
}

void Face::getType3Width(const PdfDict *d) noexcept
{
  getSimpleWidth(d);
  std::vector<double> a;
  if (!d->getNumberArray("FontMatrix", nullptr, a) || a.size() != 6)
    return;
  for (int i = 2; i < size(iWidth); ++i)
    iWidth[i] *= 1000 * a[0];
}

// --------------------------------------------------------------------

void Face::getType1Encoding(const PdfDict *d) noexcept
{
  const PdfObj *enc = getPdf(d, "Encoding");
  const PdfArray *darr = nullptr;
  if (enc && enc->dict()) {
    const PdfObj *diff = getPdf(enc->dict(), "Differences");
    if (diff && diff->array())
      darr = diff->array();
  }
  if (darr) { // have an encoding as expected
    String name[0x100];
    for (int i = 0; i < 0x100; ++i)
      name[i] = ".notdef";
    int idx = 0;
    for (int i = 0; i < darr->count(); ++i) {
      const PdfObj *obj = darr->obj(i, nullptr);
      if (obj->number())
	idx = int(obj->number()->value());
      else if (obj->name() && idx < 0x100)
	name[idx++] = obj->name()->value();
    }
    for (int i = 0; i < 0x100; ++i) {
      int glyph = FT_Get_Name_Index(iFace, const_cast<char *>(name[i].z()));
      iEncoding.push_back(glyph);
    }
  } else {
    // font descriptor has no encoding, use information in Postscript font
    if (FT_Has_PS_Glyph_Names(iFace)) {
      T1_EncodingType type;
      FT_Get_PS_Font_Value(iFace, PS_DICT_ENCODING_TYPE, 0, (void *) &type, sizeof(type));
      if (type == T1_ENCODING_TYPE_ARRAY) {
	for (int i = 0; i < 0x100; ++i)
	  iEncoding.push_back(FT_Get_Char_Index(iFace, i));
	char name[100];
	for (int i = 0; i < 0x100; ++i) {
	  FT_Get_PS_Font_Value(iFace, PS_DICT_ENCODING_ENTRY, i, (void *) name, 100);
	  iEncoding[i] = FT_Get_Name_Index(iFace, name);
	}
	return;
      }
    }
    // no Postscript glyph names or no Postscript encoding array, fall back
    for (int k = 0; k < iFace->num_charmaps; ++k) {
      if (iFace->charmaps[k]->encoding == FT_ENCODING_ADOBE_CUSTOM) {
	FT_Set_Charmap(iFace, iFace->charmaps[k]);
	break;
      }
    }
    for (int i = 0; i < 0x100; ++i)
      iEncoding.push_back(FT_Get_Char_Index(iFace, i));
  }
}

void Face::setupTruetypeEncoding() noexcept
{
  FT_Set_Charmap(iFace, iFace->charmaps[0]);
  if (iFace->charmaps[0]->platform_id != 1 ||
      iFace->charmaps[0]->encoding_id != 0) {
    ipeDebug("TrueType face %s has strange first charmap (of %d)",
	     iName.z(), iFace->num_charmaps);
    for (int i = 0; i < iFace->num_charmaps; ++i) {
      ipeDebug("Map %d has platform %d, encoding %d",
	       i, iFace->charmaps[i]->platform_id,
	       iFace->charmaps[i]->encoding_id);
    }
  }
}

bool Face::getFontFile(const PdfDict *d, Buffer &data) noexcept
{
  const PdfObj *fontDescriptor = getPdf(d, "FontDescriptor");
  if (!fontDescriptor || !fontDescriptor->dict())
    return false;
  const PdfDict *fd = fontDescriptor->dict();
  const PdfObj *fontFile = getPdf(fd, "FontFile");
  if (!fontFile)
    fontFile = getPdf(fd, "FontFile2");
  if (!fontFile)
    fontFile = getPdf(fd, "FontFile3");
  if (!fontFile || !fontFile->dict() || fontFile->dict()->stream().size() == 0)
    return false;
  data = fontFile->dict()->inflate();
  // Fix strange header in some pdftex fonts that will cause EPS
  // export to break.
  /*
  size_t m = data.size() > 1024 ? 1024 : data.size();
  std::string s { (const char *) data.data(), m };
  size_t i = s.find("FontDirectory");
  if (i != std::string::npos) {
    size_t j = s.find("{save true}{false}ifelse}{false}ifelse", i);
    if (j != std::string::npos)
      memset(data.data() + i, ' ', j - i + 38);
  }
  */
  return true;
}

// --------------------------------------------------------------------

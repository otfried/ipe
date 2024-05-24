// --------------------------------------------------------------------
// ipelib.cpp
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

#include "ipelua.h"
#include "ipedoc.h"
#include "ipebitmap.h"

#include <cerrno>
#include <cstring>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

static const char * const format_name[] =
  { "xml", "pdf", "unknown" };

void ipelua::make_metatable(lua_State *L, const char *name,
			    const struct luaL_Reg *methods)
{
  luaL_newmetatable(L, name);
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);   /* metatable.__index = metatable */
  luaL_setfuncs(L, methods, 0);
  lua_pop(L, 1);
}

bool ipelua::is_type(lua_State *L, int ud, const char *tname)
{
  if (lua_isuserdata(L, ud) && lua_getmetatable(L, ud)) {
    lua_getfield(L, LUA_REGISTRYINDEX, tname);
    if (lua_rawequal(L, -1, -2)) {
      lua_pop(L, 2);
      return true;
    }
  }
  return false;
}

String ipelua::check_filename(lua_State *L, int index)
{
  return String(luaL_checklstring(L, index, nullptr));
}

// --------------------------------------------------------------------
// Document
// --------------------------------------------------------------------

static int document_constructor(lua_State *L)
{
  bool has_fname = (lua_gettop(L) > 0);
  Document **d = (Document **) lua_newuserdata(L, sizeof(Document *));
  *d = nullptr;
  luaL_getmetatable(L, "Ipe.document");
  lua_setmetatable(L, -2);

  // should we load a document?
  if (has_fname) {
    String fname = check_filename(L, 1);
    int reason;
    *d = Document::load(fname.z(), reason);
    if (*d)
      return 1;

    lua_pop(L, 1);   // pop empty document udata
    lua_pushnil(L);  // return nil and ...

    switch (reason) {
    case Document::EVersionTooOld:
      lua_pushliteral(L, "The Ipe version of this document is too old");
      break;
    case Document::EVersionTooRecent:
      lua_pushliteral(L, "The document was created by a newer version of Ipe");
      break;
    case Document::EFileOpenError:
      lua_pushfstring(L, "Error opening file: %s", strerror(errno));
      break;
    case Document::ENotAnIpeFile:
      lua_pushliteral(L, "The document was not created by Ipe");
      break;
    default:
      lua_pushfstring(L, "Parsing error at position %d", reason);
      break;
    }
    lua_pushnumber(L, reason);
    return 3;
  } else {
    // create new empty document
    *d = new Document();
    // create the first page
    (*d)->insert(0, Page::basic());
    return 1;
  }
}

static int document_destruct(lua_State *L)
{
  Document **d = check_document(L, 1);
  delete (*d);
  *d = nullptr;
  return 0;
}

static int document_tostring(lua_State *L)
{
  check_document(L, 1);
  lua_pushfstring(L, "Document@%p", lua_topointer(L, 1));
  return 1;
}

// --------------------------------------------------------------------

static int check_pageno(lua_State *L, int i, Document *d, int extra = 0)
{
  int n = (int)luaL_checkinteger(L, i);
  luaL_argcheck(L, 1 <= n && n <= d->countPages() + extra, i,
		"invalid page number");
  return n - 1;
}

static int document_index(lua_State *L)
{
  Document **d = check_document(L, 1);
  if (lua_type(L, 2) == LUA_TNUMBER) {
    int n = check_pageno(L, 2, *d);
    push_page(L, (*d)->page(n), false);
  } else {
    const char *key = luaL_checklstring(L, 2, nullptr);
    if (!luaL_getmetafield(L, 1, key))
      lua_pushnil(L);
  }
  return 1;
}

// Document --> int
static int document_len(lua_State *L)
{
  Document **d = check_document(L, 1);
  lua_pushinteger(L, (*d)->countPages());
  return 1;
}

// arguments: document, counter
static int document_page_iterator(lua_State *L)
{
  Document **d = check_document(L, 1);
  int i = luaL_checkinteger(L, 2);
  i = i + 1;
  if (i <= (*d)->countPages()) {
    lua_pushinteger(L, i);               // new counter
    push_page(L, (*d)->page(i-1), false); // page
    return 2;
  } else
    return 0;
}

// returns page iterator for use in for loop
// returns iterator function, invariant state, control variable
static int document_pages(lua_State *L)
{
  (void) check_document(L, 1);
  lua_pushcfunction(L, document_page_iterator); // iterator function
  lua_pushvalue(L, 1);          // document
  lua_pushinteger(L, 0);         // counter
  return 3;
}

// "export", "nozip", "markedview"
static uint32_t check_flags(lua_State *L, int index)
{
  if (lua_isnoneornil(L, index))
    return 0;
  luaL_argcheck(L, lua_istable(L, index), index, "argument is not a table");
  uint32_t flags = 0;
  lua_getfield(L, index, "export");
  if (lua_toboolean(L, -1))
    flags |= SaveFlag::Export;
  lua_pop(L, 1);
  lua_getfield(L, index, "nozip");
  if (lua_toboolean(L, -1))
    flags |= SaveFlag::NoZip;
  lua_pop(L, 1);
  lua_getfield(L, index, "keepnotes");
  if (lua_toboolean(L, -1))
    flags |= SaveFlag::KeepNotes;
  lua_pop(L, 1);
  lua_getfield(L, index, "markedview");
  if (lua_toboolean(L, -1))
    flags |= SaveFlag::MarkedView;
  lua_pop(L, 1);
  return flags;
}

static int document_save(lua_State *L)
{
  Document **d = check_document(L, 1);
  String fname = check_filename(L, 2);
  FileFormat format;
  if (lua_isnoneornil(L, 3))
    format = Document::formatFromFilename(fname);
  else
    format = FileFormat(luaL_checkoption(L, 3, nullptr, format_name));
  uint32_t flags = check_flags(L, 4);
  bool result = (*d)->save(fname.z(), format, flags);
  lua_pushboolean(L, result);
  return 1;
}

static int document_exportPages(lua_State *L)
{
  Document **d = check_document(L, 1);
  String fname = check_filename(L, 2);
  uint32_t flags = check_flags(L, 3);
  int fromPage = check_pageno(L, 4, *d);
  int toPage = check_pageno(L, 5, *d);
  bool result = (*d)->exportPages(fname.z(), flags, fromPage, toPage);
  lua_pushboolean(L, result);
  return 1;
}

static int document_exportView(lua_State *L)
{
  Document **d = check_document(L, 1);
  String fname = check_filename(L, 2);
  FileFormat format;
  if (lua_isnoneornil(L, 3))
    format = Document::formatFromFilename(fname);
  else
    format = FileFormat(luaL_checkoption(L, 3, nullptr, format_name));
  uint32_t flags = check_flags(L, 4);
  int pno = check_pageno(L, 5, *d);
  int vno = check_viewno(L, 6, (*d)->page(pno));
  bool result = (*d)->exportView(fname.z(), format, flags, pno, vno);
  lua_pushboolean(L, result);
  return 1;
}

// Document --> int
static int document_countTotalViews(lua_State *L)
{
  Document **d = check_document(L, 1);
  lua_pushinteger(L, (*d)->countTotalViews());
  return 1;
}

static int document_sheets(lua_State *L)
{
  Document **d = check_document(L, 1);
  push_cascade(L, (*d)->cascade(), false);
  return 1;
}

static int document_replaceSheets(lua_State *L)
{
  Document **d = check_document(L, 1);
  SCascade *p = check_cascade(L, 2);
  Cascade *sheets = p->cascade;
  if (!p->owned)
    sheets = new Cascade(*p->cascade);
  Cascade *old = (*d)->replaceCascade(sheets);
  p->owned = false;  // now owned by document
  push_cascade(L, old);
  return 1;
}

static int document_runLatex(lua_State *L)
{
  Document **d = check_document(L, 1);
  String docname;
  if (!lua_isnoneornil(L, 2))
    docname = luaL_checklstring(L, 2, nullptr);
  bool async = lua_toboolean(L, 3);
  String log;
  ipe::Latex *converter;
  int result = async ? (*d)->runLatexAsync(docname, log, &converter)
    : (*d)->runLatex(docname, log);

  if (result == Document::ErrNone) {
    if (async)
      lua_pushlightuserdata(L, converter);
    else
      lua_pushboolean(L, true);
    lua_pushnil(L);
    lua_pushnil(L);
  } else if (result == Document::ErrNoText) {
    lua_pushboolean(L, true);
    lua_pushnil(L);
    lua_pushliteral(L, "notext");
  } else {
    lua_pushboolean(L, false);
    switch (result) {
    case Document::ErrNoDir:
      lua_pushliteral(L, "Directory does not exist and cannot be created");
      lua_pushliteral(L, "nodir");
      break;
    case Document::ErrWritingSource:
      lua_pushliteral(L, "Error writing Latex source");
      lua_pushliteral(L, "writingsource");
      break;
    case Document::ErrRunLatex:
      lua_pushliteral(L, "There was an error trying to run Pdflatex");
      lua_pushliteral(L, "runlatex");
      break;
    case Document::ErrLatex:
      lua_pushliteral(L, "There were Latex errors");
      lua_pushliteral(L, "latex");
      break;
    case Document::ErrLatexOutput:
      lua_pushliteral(L, "There was an error reading the Pdflatex output");
      lua_pushliteral(L, "latexoutput");
      break;
    }
  }
  push_string(L, log);
  return 4;
}

static int document_completeLatexRun(lua_State *L)
{
  Document **d = check_document(L, 1);
  ipe::Latex *converter = (ipe::Latex *) lua_touserdata(L, 2);
  if (converter == nullptr)
    luaL_error(L, "no Latex converter given");
  lua_pushboolean(L, (*d)->completeLatexRun(converter));
  return 1;
}

static int document_checkStyle(lua_State *L)
{
  Document **d = check_document(L, 1);
  AttributeSeq seq;
  (*d)->checkStyle(seq);
  lua_createtable(L, 0, seq.size());
  for (int i = 0; i < size(seq); ++i) {
    push_attribute(L, seq[i]);
    lua_rawseti(L, -2, i+1);
  }
  return 1;
}

static int document_set(lua_State *L)
{
  Document **d = check_document(L, 1);
  int no = check_pageno(L, 2, *d);
  Page *p = check_page(L, 3)->page;
  Page *old = (*d)->set(no, new Page(*p));
  push_page(L, old);
  return 1;
}

static int document_insert(lua_State *L)
{
  Document **d = check_document(L, 1);
  int no = check_pageno(L, 2, *d, 1);
  SPage *p = check_page(L, 3);
  (*d)->insert(no, new Page(*p->page));
  return 0;
}

static int document_append(lua_State *L)
{
  Document **d = check_document(L, 1);
  SPage *p = check_page(L, 2);
  (*d)->push_back(new Page(*p->page));
  return 0;
}

static int document_remove(lua_State *L)
{
  Document **d = check_document(L, 1);
  int no = check_pageno(L, 2, *d);
  Page *old = (*d)->remove(no);
  push_page(L, old);
  return 1;
}

static const char * const tex_engine_names[] = {
  "default", "pdftex", "xetex", "luatex" };

static int document_properties(lua_State *L)
{
  Document **d = check_document(L, 1);
  Document::SProperties prop = (*d)->properties();
  lua_createtable(L, 11, 0);
  push_string(L, prop.iTitle);
  lua_setfield(L, -2, "title");
  push_string(L, prop.iAuthor);
  lua_setfield(L, -2, "author");
  push_string(L, prop.iSubject);
  lua_setfield(L, -2, "subject");
  push_string(L, prop.iKeywords);
  lua_setfield(L, -2, "keywords");
  push_string(L, prop.iLanguage);
  lua_setfield(L, -2, "language");
  push_string(L, prop.iPreamble);
  lua_setfield(L, -2, "preamble");
  push_string(L, prop.iCreated);
  lua_setfield(L, -2, "created");
  push_string(L, prop.iModified);
  lua_setfield(L, -2, "modified");
  push_string(L, prop.iCreator);
  lua_setfield(L, -2, "creator");
  lua_pushboolean(L, prop.iFullScreen);
  lua_setfield(L, -2, "fullscreen");
  lua_pushboolean(L, prop.iNumberPages);
  lua_setfield(L, -2, "numberpages");
  lua_pushboolean(L, prop.iSequentialText);
  lua_setfield(L, -2, "sequentialtext");
  lua_pushstring(L, tex_engine_names[int(prop.iTexEngine)]);
  lua_setfield(L, -2, "tex");
  return 1;
}

static void propFlag(lua_State *L, const char *name, bool &flag)
{
  lua_getfield(L, 2, name);
  if (!lua_isnil(L, -1))
    flag = lua_toboolean(L, -1);
  lua_pop(L, 1);
}

static void propString(lua_State *L, const char *name, String &str)
{
  lua_getfield(L, 2, name);
  if (lua_isstring(L, -1))
    str = lua_tolstring(L, -1, nullptr);
  lua_pop(L, 1);
}

static int document_setProperties(lua_State *L)
{
  Document **d = check_document(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  Document::SProperties prop = (*d)->properties();
  // take from table
  propFlag(L, "numberpages", prop.iNumberPages);
  propFlag(L, "sequentialtext", prop.iSequentialText);
  propFlag(L, "fullscreen", prop.iFullScreen);
  propString(L, "title", prop.iTitle);
  propString(L, "author", prop.iAuthor);
  propString(L, "subject", prop.iSubject);
  propString(L, "keywords", prop.iKeywords);
  propString(L, "language", prop.iLanguage);
  propString(L, "preamble", prop.iPreamble);
  propString(L, "created", prop.iCreated);
  propString(L, "modified", prop.iModified);
  propString(L, "creator", prop.iCreator);
  String tex;
  propString(L, "tex", tex);
  for (int i = 0; i < 4; ++i) {
    if (!strcmp(tex.z(), tex_engine_names[i]))
      prop.iTexEngine = LatexType(i);
  }
  (*d)->setProperties(prop);
  return 0;
}

// --------------------------------------------------------------------

static const struct luaL_Reg document_methods[] = {
  { "__gc", document_destruct },
  { "__tostring", document_tostring },
  { "__len", document_len },
  { "__index", document_index },
  { "pages", document_pages },
  { "save", document_save },
  { "exportPages", document_exportPages },
  { "exportView", document_exportView },
  { "set", document_set },
  { "insert", document_insert },
  { "append", document_append },
  { "remove", document_remove },
  { "countTotalViews", document_countTotalViews },
  { "sheets", document_sheets },
  { "replaceSheets", document_replaceSheets },
  { "runLatex", document_runLatex },
  { "completeLatexRun", document_completeLatexRun },
  { "checkStyle", document_checkStyle },
  { "properties", document_properties },
  { "setProperties", document_setProperties },
  { nullptr, nullptr },
};

// --------------------------------------------------------------------

static int file_format(lua_State *L)
{
  String fname = check_filename(L, 1);
  FILE *fd = Platform::fopen(fname.z(), "rb");
  if (!fd)
    luaL_error(L, "fopen error: %s", strerror(errno));
  FileSource source(fd);
  FileFormat format = Document::fileFormat(source);
  fclose(fd);
  lua_pushstring(L, format_name[int(format)]);
  return 1;
}

static int ipe_normalizeangle(lua_State *L)
{
  Angle alpha(luaL_checknumber(L, 1));
  double low = luaL_checknumber(L, 2);
  lua_pushnumber(L, double(alpha.normalize(low)));
  return 1;
}

static int ipe_splinetobeziers(lua_State *L)
{
  luaL_argcheck(L, lua_istable(L, 1), 1, "argument is not a table");
  std::vector<Vector> v;
  int no = lua_rawlen(L, 1);
  for (int i = 1; i <= no; ++i) {
    lua_rawgeti(L, 1, i);
    luaL_argcheck(L, is_type(L, -1, "Ipe.vector"), 1,
		  "element is not a vector");
    Vector *u = check_vector(L, -1);
    v.push_back(*u);
    lua_pop(L, 1);
  }
  bool closed = lua_toboolean(L, 2);
  std::vector<Bezier> result;
  if (closed) {
    Bezier::closedSpline(v.size(), v.data(), result);
  } else {
    // get spline type
    lua_getfield(L, 1, "type");
    if (!lua_isstring(L, -1))
      luaL_error(L, "spline has no type");
    int type = test_option(L, -1, segtype_names);
    if (type < CurveSegment::ESpline)
      luaL_error(L, "spline has invalid type");
    lua_pop(L, 1); // pop type
    switch (type) {
    case CurveSegment::ESpline:
      Bezier::spline(v.size(), v.data(), result);
      break;
    case CurveSegment::EOldSpline:
      Bezier::oldSpline(v.size(), v.data(), result);
      break;
    case CurveSegment::ECardinalSpline: {
      lua_getfield(L, 1, "tension");
      if (!lua_isnumber(L, -1))
	luaL_error(L, "spline has no tension");
      float tension = lua_tonumberx(L, -1, nullptr);
      Bezier::cardinalSpline(v.size(), v.data(), tension, result);
      lua_pop(L, 1); // tension
      break; }
    case CurveSegment::ESpiroSpline:
      Bezier::spiroSpline(v.size(), v.data(), result);
      break;
    default:
      break;
    }
  }
  lua_createtable(L, result.size(), 0);
  for (int i = 0; i < size(result); ++i) {
    lua_createtable(L, 4, 1);
    lua_pushliteral(L, "spline");
    lua_setfield(L, -2, "type");
    for (int k = 0; k < 4; ++k) {
      if (k == 0 && i > 0)
	push_vector(L, result[i-1].iV[3]);
      else
	push_vector(L, result[i].iV[k]);
      lua_rawseti(L, -2, k+1);
    }
    lua_rawseti(L, -2, i+1);
  }
  return 1;
}

static int ipe_fileExists(lua_State *L)
{
  String s = check_filename(L, 1);
  lua_pushboolean(L, Platform::fileExists(s));
  return 1;
}

static int ipe_realpath(lua_State *L)
{
  String s = check_filename(L, 1);
  push_string(L, Platform::realPath(s));
  return 1;
}

static int ipe_directory(lua_State *L)
{
  const char *path = luaL_checklstring(L, 1, nullptr);
  std::vector<String> files;
  if (!Platform::listDirectory(path, files))
    luaL_error(L, "cannot list directory '%s'", path);
  lua_createtable(L, 0, files.size());
  for (int i = 0; i < size(files); ++i) {
    push_string(L, files[i]);
    lua_rawseti(L, -2, i+1);
  }
  return 1;
}

#define l_checkmode(mode) \
	(*mode != '\0' && strchr("rwa", *(mode++)) != nullptr &&	\
	(*mode != '+' || ++mode) &&  /* skip if char is '+' */	\
	(*mode != 'b' || ++mode) &&  /* skip if char is 'b' */	\
	(*mode == '\0'))

static int ipe_fclose (lua_State *L) {
  luaL_Stream *p = (luaL_Stream *) luaL_checkudata(L, 1, LUA_FILEHANDLE);
  int res = fclose(p->f);
  return luaL_fileresult(L, (res == 0), nullptr);
}

// open a file for reading or writing from Lua,
// correctly handling UTF-8 filenames on Windows
static int ipe_openFile(lua_State *L)
{
  const char *filename = luaL_checklstring(L, 1, nullptr);
  const char *mode = luaL_optlstring(L, 2, "r", nullptr);

  luaL_Stream *p = (luaL_Stream *)lua_newuserdata(L, sizeof(luaL_Stream));
  p->closef = nullptr;  /* mark file handle as 'closed' */
  luaL_setmetatable(L, LUA_FILEHANDLE);
  p->f = nullptr;
  p->closef = &ipe_fclose;

  const char *md = mode;  /* to traverse/check mode */
  luaL_argcheck(L, l_checkmode(md), 2, "invalid mode");
  p->f = Platform::fopen(filename, mode);
  return (p->f == nullptr) ? luaL_fileresult(L, 0, filename) : 1;
}

static const char * const image_format_names[] = { "png", "jpeg" };

static int ipe_readImage(lua_State *L)
{
  String s = check_filename(L, 1);
  int fmt = luaL_checkoption(L, 2, nullptr, image_format_names);

  Vector dotsPerInch;
  const char *errmsg = nullptr;

  Bitmap bmp = fmt ?
    Bitmap::readJpeg(s.z(), dotsPerInch, errmsg) :
    Bitmap::readPNG(s.z(), dotsPerInch, errmsg);

  if (bmp.isNull()) {
    lua_pushnil(L);
    lua_pushstring(L, errmsg);
    return 2;
  }

  Rect r(Vector::ZERO, Vector(bmp.width(), bmp.height()));
  Image *img = new Image(r, bmp);
  push_object(L, img);
  push_vector(L, dotsPerInch);
  return 2;
}

static int image_constructor(lua_State *L)
{
  Rect *r = check_rect(L, 1);
  Object *s = check_object(L, 2)->obj;
  luaL_argcheck(L, s->type() == Object::EImage, 2, "not an image object");
  Bitmap bm = s->asImage()->bitmap();

  Image *img = new Image(*r, bm);
  push_object(L, img);
  return 1;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipelib_functions[] = {
  { "Document", document_constructor },
  { "Page", page_constructor },
  { "Vector",  vector_constructor },
  { "Direction",  direction_constructor },
  { "Matrix",  matrix_constructor },
  { "Translation",  translation_constructor },
  { "Rotation",  rotation_constructor },
  { "Rect", rect_constructor },
  { "Line", line_constructor },
  { "LineThrough", line_through },
  { "Bisector", line_bisector },
  { "Segment", segment_constructor },
  { "Bezier", bezier_constructor },
  { "Quad", quad_constructor },
  { "Arc", arc_constructor },
  { "Reference", reference_constructor },
  { "Text", text_constructor },
  { "Path", path_constructor },
  { "Group", group_constructor },
  { "Object", xml_constructor },
  { "Sheet", sheet_constructor },
  { "Sheets", cascade_constructor },
  { "fileFormat", file_format },
  { "Ipelet", ipelet_constructor },
  { "normalizeAngle", ipe_normalizeangle },
  { "splineToBeziers", ipe_splinetobeziers },
  { "fileExists", ipe_fileExists },
  { "realPath", ipe_realpath },
  { "directory", ipe_directory },
  { "openFile", ipe_openFile },
  { "readImage", ipe_readImage },
  { "Image", image_constructor },
  { nullptr, nullptr }
};

extern "C" int luaopen_ipe(lua_State *L)
{
  Platform::initLib(IPELIB_VERSION);

  open_ipegeo(L);
  open_ipeobj(L);
  open_ipestyle(L);
  open_ipepage(L);
  open_ipelets(L);

  luaL_newmetatable(L, "Ipe.document");
  luaL_setfuncs(L, document_methods, 0);
  lua_pop(L, 1);

  luaL_newlib(L, ipelib_functions);
  lua_setglobal(L, "ipe");
  return 1;
}

// --------------------------------------------------------------------

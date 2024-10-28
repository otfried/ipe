// --------------------------------------------------------------------
// ipelua.h
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

#ifndef IPELUA_H
#define IPELUA_H

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "ipedoc.h"
#include "ipelet.h"
#include "ipepage.h"
#include "ipeshape.h"
#include "ipestyle.h"

namespace ipelua {

struct SSheet {
    bool owned;
    ipe::StyleSheet * sheet;
};

struct SCascade {
    bool owned;
    ipe::Cascade * cascade;
};

struct SPage {
    bool owned;
    ipe::Page * page;
};

struct SObject {
    bool owned;
    ipe::Object * obj;
};

inline ipe::Document ** check_document(lua_State * L, int i) {
    return (ipe::Document **)luaL_checkudata(L, i, "Ipe.document");
}

inline ipe::Vector * check_vector(lua_State * L, int i) {
    return (ipe::Vector *)luaL_checkudata(L, i, "Ipe.vector");
}

inline ipe::Matrix * check_matrix(lua_State * L, int i) {
    return (ipe::Matrix *)luaL_checkudata(L, i, "Ipe.matrix");
}

inline ipe::Rect * check_rect(lua_State * L, int i) {
    return (ipe::Rect *)luaL_checkudata(L, i, "Ipe.rect");
}

inline ipe::Line * check_line(lua_State * L, int i) {
    return (ipe::Line *)luaL_checkudata(L, i, "Ipe.line");
}

inline ipe::Segment * check_segment(lua_State * L, int i) {
    return (ipe::Segment *)luaL_checkudata(L, i, "Ipe.segment");
}

inline ipe::Bezier * check_bezier(lua_State * L, int i) {
    return (ipe::Bezier *)luaL_checkudata(L, i, "Ipe.bezier");
}

inline ipe::Arc * check_arc(lua_State * L, int i) {
    return (ipe::Arc *)luaL_checkudata(L, i, "Ipe.arc");
}

inline SObject * check_object(lua_State * L, int i) {
    return (SObject *)luaL_checkudata(L, i, "Ipe.object");
}

inline SSheet * check_sheet(lua_State * L, int i) {
    return (SSheet *)luaL_checkudata(L, i, "Ipe.sheet");
}

inline SCascade * check_cascade(lua_State * L, int i) {
    return (SCascade *)luaL_checkudata(L, i, "Ipe.cascade");
}

inline SPage * check_page(lua_State * L, int i) {
    return (SPage *)luaL_checkudata(L, i, "Ipe.page");
}

inline ipe::Ipelet ** check_ipelet(lua_State * L, int i) {
    return (ipe::Ipelet **)luaL_checkudata(L, i, "Ipe.ipelet");
}

inline void luacall(lua_State * L, int nargs, int nresults) {
    lua_callk(L, nargs, nresults, 0, nullptr);
}

// --------------------------------------------------------------------

extern void make_metatable(lua_State * L, const char * name,
			   const struct luaL_Reg * methods);

extern bool is_type(lua_State * L, int ud, const char * tname);

extern const char * const linejoin_names[];
extern const char * const linecap_names[];
extern const char * const fillrule_names[];
extern const char * const segtype_names[];
extern const char * const horizontal_alignment_names[];
extern const char * const vertical_alignment_names[];

extern ipe::String check_filename(lua_State * L, int index);

// geo

extern void push_vector(lua_State * L, const ipe::Vector & v);
extern int vector_constructor(lua_State * L);
extern int direction_constructor(lua_State * L);
extern void push_matrix(lua_State * L, const ipe::Matrix & m);
extern int matrix_constructor(lua_State * L);
extern int rotation_constructor(lua_State * L);
extern int translation_constructor(lua_State * L);
extern void push_rect(lua_State * L, const ipe::Rect & r);
extern int rect_constructor(lua_State * L);
extern void push_line(lua_State * L, const ipe::Line & l);
extern int line_constructor(lua_State * L);
extern int line_through(lua_State * L);
extern int line_bisector(lua_State * L);
extern void push_segment(lua_State * L, const ipe::Segment & s);
extern int segment_constructor(lua_State * L);
extern void push_bezier(lua_State * L, const ipe::Bezier & b);
extern int bezier_constructor(lua_State * L);
extern int quad_constructor(lua_State * L);
extern void push_arc(lua_State * L, const ipe::Arc & a);
extern int arc_constructor(lua_State * L);

// obj

extern void push_string(lua_State * L, ipe::String str);
extern void push_color(lua_State * L, ipe::Color color);
extern void push_attribute(lua_State * L, ipe::Attribute att);
extern ipe::Attribute check_color_attribute(lua_State * L, int i);
extern ipe::Attribute check_number_attribute(lua_State * L, int i);
extern ipe::Attribute check_bool_attribute(lua_State * L, int i);
extern ipe::Color check_color(lua_State * L, int i);
extern ipe::Attribute check_property(ipe::Property prop, lua_State * L, int i);
extern void check_allattributes(lua_State * L, int i, ipe::AllAttributes & all);

extern void push_object(lua_State * L, ipe::Object * obj, bool owned = true);

extern int reference_constructor(lua_State * L);
extern int text_constructor(lua_State * L);
extern int path_constructor(lua_State * L);
extern int group_constructor(lua_State * L);
extern int xml_constructor(lua_State * L);
extern ipe::Shape check_shape(lua_State * L, int index);

// style

extern void push_sheet(lua_State * L, ipe::StyleSheet * s, bool owned = true);
extern void push_cascade(lua_State * L, ipe::Cascade * s, bool owned = true);
extern int sheet_constructor(lua_State * L);
extern int cascade_constructor(lua_State * L);
extern int test_option(lua_State * L, int i, const char * const * names);

// page

extern void push_page(lua_State * L, ipe::Page * page, bool owned = true);
extern int check_layer(lua_State * L, int i, ipe::Page * p);
extern int check_viewno(lua_State * L, int i, ipe::Page * p, int extra = 0);
extern int page_constructor(lua_State * L);

// ipelet

extern int ipelet_constructor(lua_State * L);
extern void get_snap(lua_State * L, int i, ipe::Snap & snap);

// open components

extern int open_ipegeo(lua_State * L);
extern int open_ipeobj(lua_State * L);
extern int open_ipestyle(lua_State * L);
extern int open_ipepage(lua_State * L);
extern int open_ipelets(lua_State * L);

} // namespace ipelua

extern "C" int luaopen_ipe(lua_State * L);

// --------------------------------------------------------------------
#endif

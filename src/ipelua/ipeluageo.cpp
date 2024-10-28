// --------------------------------------------------------------------
// ipeluageo.cpp
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

#include <cstring>

using namespace ipe;
using namespace ipelua;

// --------------------------------------------------------------------

void ipelua::push_vector(lua_State * L, const Vector & v0) {
    Vector * v = (Vector *)lua_newuserdata(L, sizeof(Vector));
    luaL_getmetatable(L, "Ipe.vector");
    lua_setmetatable(L, -2);
    new (v) Vector(v0);
}

int ipelua::vector_constructor(lua_State * L) {
    if (lua_gettop(L) == 0)
	push_vector(L, Vector::ZERO);
    else
	push_vector(L, Vector(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

int ipelua::direction_constructor(lua_State * L) {
    Angle alpha(luaL_checknumber(L, 1));
    push_vector(L, Vector(alpha));
    return 1;
}

static int vector_get(lua_State * L) {
    Vector * v = check_vector(L, 1);
    const char * key = lua_tolstring(L, 2, nullptr);
    if (!strcmp(key, "x"))
	lua_pushnumber(L, v->x);
    else if (!strcmp(key, "y"))
	lua_pushnumber(L, v->y);
    else if (!luaL_getmetafield(L, 1, key))
	lua_pushnil(L);
    return 1;
}

static int vector_tostring(lua_State * L) {
    Vector * v = check_vector(L, 1);
    lua_pushfstring(L, "(%f, %f)", v->x, v->y);
    return 1;
}

static int vector_add(lua_State * L) {
    Vector * v1 = check_vector(L, 1);
    Vector * v2 = check_vector(L, 2);
    push_vector(L, *v1 + *v2);
    return 1;
}

static int vector_unm(lua_State * L) {
    Vector * v = check_vector(L, 1);
    push_vector(L, Vector(-v->x, -v->y));
    return 1;
}

static int vector_sub(lua_State * L) {
    Vector * v1 = check_vector(L, 1);
    Vector * v2 = check_vector(L, 2);
    push_vector(L, *v1 - *v2);
    return 1;
}

static int vector_eq(lua_State * L) {
    Vector * v1 = check_vector(L, 1);
    Vector * v2 = check_vector(L, 2);
    lua_pushboolean(L, *v1 == *v2);
    return 1;
}

static int vector_dot(lua_State * L) {
    Vector * v1 = check_vector(L, 1);
    Vector * v2 = check_vector(L, 2);
    lua_pushnumber(L, dot(*v1, *v2));
    return 1;
}

static int vector_mul(lua_State * L) {
    if (lua_type(L, 1) == LUA_TNUMBER) {
	double scalar = luaL_checknumber(L, 1);
	Vector * v = check_vector(L, 2);
	push_vector(L, scalar * *v);
    } else {
	Vector * v = check_vector(L, 1);
	double scalar = luaL_checknumber(L, 2);
	push_vector(L, *v * scalar);
    }
    return 1;
}

static int vector_len(lua_State * L) {
    Vector * v = check_vector(L, 1);
    lua_pushnumber(L, v->len());
    return 1;
}

static int vector_sqLen(lua_State * L) {
    Vector * v = check_vector(L, 1);
    lua_pushnumber(L, v->sqLen());
    return 1;
}

static int vector_normalized(lua_State * L) {
    Vector * v = check_vector(L, 1);
    push_vector(L, v->normalized());
    return 1;
}

static int vector_orthogonal(lua_State * L) {
    Vector * v = check_vector(L, 1);
    push_vector(L, v->orthogonal());
    return 1;
}

static int vector_factorize(lua_State * L) {
    Vector * v = check_vector(L, 1);
    Vector * unit = check_vector(L, 2);
    lua_pushnumber(L, v->factorize(*unit));
    return 1;
}

static int vector_angle(lua_State * L) {
    Vector * v = check_vector(L, 1);
    lua_pushnumber(L, double(v->angle()));
    return 1;
}

static const struct luaL_Reg vector_methods[] = {
    {"__index", vector_get},
    {"__tostring", vector_tostring},
    {"__add", vector_add},
    {"__unm", vector_unm},
    {"__sub", vector_sub},
    {"__eq", vector_eq},
    {"__mul", vector_mul},
    {"__concat", vector_dot}, // for historical reasons
    {"__pow", vector_dot},
    {"len", vector_len},
    {"sqLen", vector_sqLen},
    {"normalized", vector_normalized},
    {"orthogonal", vector_orthogonal},
    {"factorize", vector_factorize},
    {"angle", vector_angle},
    {nullptr, nullptr},
};

// --------------------------------------------------------------------

void ipelua::push_matrix(lua_State * L, const Matrix & m0) {
    Matrix * m = (Matrix *)lua_newuserdata(L, sizeof(Matrix));
    luaL_getmetatable(L, "Ipe.matrix");
    lua_setmetatable(L, -2);
    new (m) Matrix(m0);
}

int ipelua::matrix_constructor(lua_State * L) {
    int top = lua_gettop(L);
    if (top == 0)
	push_matrix(L, Matrix());
    else if (top == 4 || top == 6) {
	double a[6];
	a[4] = a[5] = 0.0;
	for (int i = 0; i < top; ++i) a[i] = luaL_checknumber(L, i + 1);
	push_matrix(L, Matrix(a[0], a[1], a[2], a[3], a[4], a[5]));
    } else if (top == 1 && lua_type(L, 1) == LUA_TTABLE) {
	double a[6];
	for (int i = 0; i < 6; ++i) {
	    lua_rawgeti(L, 1, i + 1);
	    a[i] = luaL_checknumber(L, -1);
	    lua_pop(L, 1);
	}
	push_matrix(L, Matrix(a[0], a[1], a[2], a[3], a[4], a[5]));
    } else
	luaL_error(L, "incorrect arguments for constructor");
    return 1;
}

int ipelua::rotation_constructor(lua_State * L) {
    double alpha = luaL_checknumber(L, 1);
    push_matrix(L, Matrix(Linear(Angle(alpha))));
    return 1;
}

int ipelua::translation_constructor(lua_State * L) {
    if (lua_gettop(L) == 1) {
	Vector * v = check_vector(L, 1);
	push_matrix(L, Matrix(*v));
    } else {
	double x = luaL_checknumber(L, 1);
	double y = luaL_checknumber(L, 2);
	push_matrix(L, Matrix(Vector(x, y)));
    }
    return 1;
}

static int matrix_tostring(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    lua_pushfstring(L, "[%f %f %f %f %f %f]", m->a[0], m->a[1], m->a[2], m->a[3], m->a[4],
		    m->a[5]);
    return 1;
}

static int matrix_eq(lua_State * L) {
    Matrix * m1 = check_matrix(L, 1);
    Matrix * m2 = check_matrix(L, 2);
    lua_pushboolean(L, *m1 == *m2);
    return 1;
}

static int matrix_coeff(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    lua_newtable(L);
    for (int i = 0; i < 6; ++i) {
	lua_pushnumber(L, m->a[i]);
	lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int matrix_isIdentity(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    lua_pushboolean(L, m->isIdentity());
    return 1;
}

static int matrix_isSingular(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    double x = luaL_checknumber(L, 2);
    double t = m->a[0] * m->a[3] - m->a[1] * m->a[2];
    lua_pushboolean(L, t < 0 ? -t <= x : t <= x);
    return 1;
}

static int matrix_inverse(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    double t = m->a[0] * m->a[3] - m->a[1] * m->a[2];
    luaL_argcheck(L, t != 0, 1, "matrix is singular");
    push_matrix(L, m->inverse());
    return 1;
}

static int matrix_translation(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    push_vector(L, m->translation());
    return 1;
}

static int matrix_linear(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    push_matrix(L, Matrix(m->linear()));
    return 1;
}

static int matrix_elements(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    lua_createtable(L, 6, 0);
    for (int i = 0; i < 6; ++i) {
	lua_pushnumber(L, m->a[i]);
	lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int matrix_mul(lua_State * L) {
    Matrix * lhs = check_matrix(L, 1);
    if (is_type(L, 2, "Ipe.matrix")) {
	Matrix * rhs = check_matrix(L, 2);
	push_matrix(L, *lhs * *rhs);
    } else if (is_type(L, 2, "Ipe.arc")) {
	Arc * rhs = check_arc(L, 2);
	push_arc(L, *lhs * *rhs);
    } else {
	Vector * v = check_vector(L, 2);
	push_vector(L, *lhs * *v);
    }
    return 1;
}

static const struct luaL_Reg matrix_methods[] = {{"__tostring", matrix_tostring},
						 {"__eq", matrix_eq},
						 {"coeff", matrix_coeff},
						 {"isIdentity", matrix_isIdentity},
						 {"linear", matrix_linear},
						 {"translation", matrix_translation},
						 {"__mul", matrix_mul},
						 {"isSingular", matrix_isSingular},
						 {"inverse", matrix_inverse},
						 {"elements", matrix_elements},
						 {nullptr, nullptr}};

// --------------------------------------------------------------------

void ipelua::push_rect(lua_State * L, const Rect & r0) {
    Rect * r = (Rect *)lua_newuserdata(L, sizeof(Rect));
    luaL_getmetatable(L, "Ipe.rect");
    lua_setmetatable(L, -2);
    new (r) Rect(r0);
}

int ipelua::rect_constructor(lua_State * L) {
    push_rect(L, Rect());
    return 1;
}

static int rect_tostring(lua_State * L) {
    Rect * r = check_rect(L, 1);
    lua_pushfstring(L, "Rect(%f,%f,%f,%f)", r->bottomLeft().x, r->bottomLeft().y,
		    r->topRight().x, r->topRight().y);
    return 1;
}

static int rect_isEmpty(lua_State * L) {
    Rect * r = check_rect(L, 1);
    lua_pushboolean(L, r->isEmpty());
    return 1;
}

static int rect_topRight(lua_State * L) {
    Rect * r = check_rect(L, 1);
    push_vector(L, r->topRight());
    return 1;
}

static int rect_bottomLeft(lua_State * L) {
    Rect * r = check_rect(L, 1);
    push_vector(L, r->bottomLeft());
    return 1;
}

static int rect_topLeft(lua_State * L) {
    Rect * r = check_rect(L, 1);
    push_vector(L, r->topLeft());
    return 1;
}

static int rect_bottomRight(lua_State * L) {
    Rect * r = check_rect(L, 1);
    push_vector(L, r->bottomRight());
    return 1;
}

static int rect_left(lua_State * L) {
    Rect * r = check_rect(L, 1);
    lua_pushnumber(L, r->left());
    return 1;
}

static int rect_right(lua_State * L) {
    Rect * r = check_rect(L, 1);
    lua_pushnumber(L, r->right());
    return 1;
}

static int rect_bottom(lua_State * L) {
    Rect * r = check_rect(L, 1);
    lua_pushnumber(L, r->bottom());
    return 1;
}

static int rect_top(lua_State * L) {
    Rect * r = check_rect(L, 1);
    lua_pushnumber(L, r->top());
    return 1;
}

static int rect_width(lua_State * L) {
    Rect * r = check_rect(L, 1);
    lua_pushnumber(L, r->width());
    return 1;
}

static int rect_height(lua_State * L) {
    Rect * r = check_rect(L, 1);
    lua_pushnumber(L, r->height());
    return 1;
}

static int rect_add(lua_State * L) {
    Rect * r = check_rect(L, 1);
    if (is_type(L, 2, "Ipe.vector"))
	r->addPoint(*check_vector(L, 2));
    else
	r->addRect(*check_rect(L, 2));
    return 0;
}

static int rect_clipTo(lua_State * L) {
    Rect * r1 = check_rect(L, 1);
    Rect * r2 = check_rect(L, 2);
    r1->clipTo(*r2);
    return 0;
}

static int rect_contains(lua_State * L) {
    Rect * r = check_rect(L, 1);
    if (is_type(L, 2, "Ipe.vector"))
	lua_pushboolean(L, r->contains(*check_vector(L, 2)));
    else
	lua_pushboolean(L, r->contains(*check_rect(L, 2)));
    return 1;
}

static int rect_intersects(lua_State * L) {
    Rect * r1 = check_rect(L, 1);
    Rect * r2 = check_rect(L, 2);
    lua_pushboolean(L, r1->intersects(*r2));
    return 1;
}

static const struct luaL_Reg rect_methods[] = {{"__tostring", rect_tostring},
					       {"isEmpty", rect_isEmpty},
					       {"topRight", rect_topRight},
					       {"bottomLeft", rect_bottomLeft},
					       {"topLeft", rect_topLeft},
					       {"bottomRight", rect_bottomRight},
					       {"left", rect_left},
					       {"right", rect_right},
					       {"bottom", rect_bottom},
					       {"top", rect_top},
					       {"width", rect_width},
					       {"height", rect_height},
					       {"add", rect_add},
					       {"clipTo", rect_clipTo},
					       {"contains", rect_contains},
					       {"intersects", rect_intersects},
					       {nullptr, nullptr}};

// --------------------------------------------------------------------

void ipelua::push_line(lua_State * L, const Line & l0) {
    Line * l = (Line *)lua_newuserdata(L, sizeof(Line));
    luaL_getmetatable(L, "Ipe.line");
    lua_setmetatable(L, -2);
    new (l) Line(l0);
}

int ipelua::line_constructor(lua_State * L) {
    Vector * p = check_vector(L, 1);
    Vector * dir = check_vector(L, 2);
    push_line(L, Line(*p, *dir));
    return 1;
}

int ipelua::line_through(lua_State * L) {
    Vector * p = check_vector(L, 1);
    Vector * q = check_vector(L, 2);
    push_line(L, Line::through(*p, *q));
    return 1;
}

int ipelua::line_bisector(lua_State * L) {
    Vector * p = check_vector(L, 1);
    Vector * q = check_vector(L, 2);
    luaL_argcheck(L, *p != *q, 2, "points are not distinct");
    Vector mid = 0.5 * (*p + *q);
    Vector dir = (*p - *q).normalized().orthogonal();
    push_line(L, Line(mid, dir));
    return 1;
}

static int line_tostring(lua_State * L) {
    Line * l = check_line(L, 1);
    lua_pushfstring(L, "Line[(%f,%f)->(%f,%f)]", l->iP.x, l->iP.y, l->dir().x,
		    l->dir().y);
    return 1;
}

static int line_side(lua_State * L) {
    Line * l = check_line(L, 1);
    Vector * p = check_vector(L, 2);
    double s = l->side(*p);
    if (s > 0.0)
	lua_pushnumber(L, 1.0);
    else if (s < 0.0)
	lua_pushnumber(L, -1.0);
    else
	lua_pushnumber(L, 0.0);
    return 1;
}

static int line_point(lua_State * L) {
    Line * l = check_line(L, 1);
    push_vector(L, l->iP);
    return 1;
}

static int line_dir(lua_State * L) {
    Line * l = check_line(L, 1);
    push_vector(L, l->dir());
    return 1;
}

static int line_normal(lua_State * L) {
    Line * l = check_line(L, 1);
    push_vector(L, l->normal());
    return 1;
}

static int line_distance(lua_State * L) {
    Line * l = check_line(L, 1);
    Vector * v = check_vector(L, 2);
    lua_pushnumber(L, l->distance(*v));
    return 1;
}

static int line_intersects(lua_State * L) {
    Line * l1 = check_line(L, 1);
    Line * l2 = check_line(L, 2);
    Vector pt;
    if (l1->intersects(*l2, pt))
	push_vector(L, pt);
    else
	lua_pushnil(L);
    return 1;
}

static int line_project(lua_State * L) {
    Line * l = check_line(L, 1);
    Vector * v = check_vector(L, 2);
    push_vector(L, l->project(*v));
    return 1;
}

static const struct luaL_Reg line_methods[] = {{"__tostring", line_tostring},
					       {"side", line_side},
					       {"point", line_point},
					       {"dir", line_dir},
					       {"normal", line_normal},
					       {"distance", line_distance},
					       {"intersects", line_intersects},
					       {"project", line_project},
					       {nullptr, nullptr}};

// --------------------------------------------------------------------

void ipelua::push_segment(lua_State * L, const Segment & s0) {
    Segment * s = (Segment *)lua_newuserdata(L, sizeof(Segment));
    luaL_getmetatable(L, "Ipe.segment");
    lua_setmetatable(L, -2);
    new (s) Segment(s0);
}

int ipelua::segment_constructor(lua_State * L) {
    Vector * p = check_vector(L, 1);
    Vector * q = check_vector(L, 2);
    push_segment(L, Segment(*p, *q));
    return 1;
}

static int segment_tostring(lua_State * L) {
    Segment * s = check_segment(L, 1);
    lua_pushfstring(L, "Segment[(%f,%f)-(%f,%f)]", s->iP.x, s->iP.y, s->iQ.x, s->iQ.y);
    return 1;
}

static int segment_endpoints(lua_State * L) {
    Segment * s = check_segment(L, 1);
    push_vector(L, s->iP);
    push_vector(L, s->iQ);
    return 2;
}

static int segment_line(lua_State * L) {
    Segment * s = check_segment(L, 1);
    push_line(L, s->line());
    return 1;
}

static int segment_project(lua_State * L) {
    Segment * s = check_segment(L, 1);
    Vector * v = check_vector(L, 2);
    Vector pt;
    if (s->project(*v, pt))
	push_vector(L, pt);
    else
	lua_pushnil(L);
    return 1;
}

static int segment_distance(lua_State * L) {
    Segment * s = check_segment(L, 1);
    Vector * v = check_vector(L, 2);
    lua_pushnumber(L, s->distance(*v));
    return 1;
}

static int segment_intersects(lua_State * L) {
    Segment * s = check_segment(L, 1);
    Vector pt;
    if (is_type(L, 2, "Ipe.segment")) {
	Segment * rhs = check_segment(L, 2);
	if (s->intersects(*rhs, pt))
	    push_vector(L, pt);
	else
	    lua_pushnil(L);
    } else {
	Line * rhs = check_line(L, 2);
	if (s->intersects(*rhs, pt))
	    push_vector(L, pt);
	else
	    lua_pushnil(L);
    }
    return 1;
}

static const struct luaL_Reg segment_methods[] = {{"__tostring", segment_tostring},
						  {"endpoints", segment_endpoints},
						  {"line", segment_line},
						  {"project", segment_project},
						  {"distance", segment_distance},
						  {"intersects", segment_intersects},
						  {nullptr, nullptr}};

// --------------------------------------------------------------------

void ipelua::push_bezier(lua_State * L, const Bezier & b0) {
    Bezier * b = (Bezier *)lua_newuserdata(L, sizeof(Bezier));
    luaL_getmetatable(L, "Ipe.bezier");
    lua_setmetatable(L, -2);
    new (b) Bezier(b0);
}

int ipelua::bezier_constructor(lua_State * L) {
    Vector * p[4];
    for (int i = 0; i < 4; ++i) p[i] = check_vector(L, i + 1);
    push_bezier(L, Bezier(*p[0], *p[1], *p[2], *p[3]));
    return 1;
}

int ipelua::quad_constructor(lua_State * L) {
    Vector * p[3];
    for (int i = 0; i < 3; ++i) p[i] = check_vector(L, i + 1);
    push_bezier(L, Bezier::quadBezier(*p[0], *p[1], *p[2]));
    return 1;
}

static int bezier_tostring(lua_State * L) {
    check_bezier(L, 1);
    lua_pushfstring(L, "Bezier@%p", lua_topointer(L, 1));
    return 1;
}

static int bezier_controlpoints(lua_State * L) {
    Bezier * b = check_bezier(L, 1);
    for (int i = 0; i < 4; ++i) push_vector(L, b->iV[i]);
    return 4;
}

static int bezier_point(lua_State * L) {
    Bezier * b = check_bezier(L, 1);
    double t = luaL_checknumber(L, 2);
    push_vector(L, b->point(t));
    return 1;
}

static int bezier_bbox(lua_State * L) {
    Bezier * b = check_bezier(L, 1);
    push_rect(L, b->bbox());
    return 1;
}

static int bezier_intersect(lua_State * L) {
    Bezier * b = check_bezier(L, 1);
    std::vector<Vector> pts;
    if (is_type(L, 2, "Ipe.segment")) {
	Segment * rhs = check_segment(L, 2);
	b->intersect(*rhs, pts);
    } else if (is_type(L, 2, "Ipe.line")) {
	Line * rhs = check_line(L, 2);
	b->intersect(*rhs, pts);
    } else if (is_type(L, 2, "Ipe.bezier")) {
	Bezier * rhs = check_bezier(L, 2);
	b->intersect(*rhs, pts);
    }
    lua_createtable(L, pts.size(), 0);
    for (int i = 0; i < int(pts.size()); ++i) {
	push_vector(L, pts[i]);
	lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int bezier_snap(lua_State * L) {
    Bezier * b = check_bezier(L, 1);
    Vector * v = check_vector(L, 2);
    double t;
    Vector pos;
    double bound = 10e9;
    if (b->snap(*v, t, pos, bound)) {
	lua_pushnumber(L, t);
	push_vector(L, pos);
	return 2;
    } else
	return 0;
}

static const struct luaL_Reg bezier_methods[] = {{"__tostring", bezier_tostring},
						 {"controlpoints", bezier_controlpoints},
						 {"point", bezier_point},
						 {"bbox", bezier_bbox},
						 {"intersect", bezier_intersect},
						 {"snap", bezier_snap},
						 {nullptr, nullptr}};

// --------------------------------------------------------------------

/*
    inline Arc(const Matrix &m, Angle alpha, Angle beta);
    Arc(const Matrix &m0, const Vector &begp, const Vector &endp);
*/

void ipelua::push_arc(lua_State * L, const Arc & a0) {
    Arc * a = (Arc *)lua_newuserdata(L, sizeof(Arc));
    luaL_getmetatable(L, "Ipe.arc");
    lua_setmetatable(L, -2);
    new (a) Arc(a0);
}

int ipelua::arc_constructor(lua_State * L) {
    Matrix * m = check_matrix(L, 1);
    if (lua_gettop(L) == 1) {
	push_arc(L, Arc(*m));
    } else if (is_type(L, 2, "Ipe.vector")) {
	Vector * v1 = check_vector(L, 2);
	Vector * v2 = check_vector(L, 3);
	push_arc(L, Arc(*m, *v1, *v2));
    } else {
	double alpha = luaL_checknumber(L, 2);
	double beta = luaL_checknumber(L, 3);
	push_arc(L, Arc(*m, Angle(alpha), Angle(beta)));
    }
    return 1;
}

static int arc_tostring(lua_State * L) {
    (void)check_arc(L, 1);
    lua_pushfstring(L, "Arc@%p", lua_topointer(L, 1));
    return 1;
}

static int arc_endpoints(lua_State * L) {
    Arc * b = check_arc(L, 1);
    push_vector(L, b->beginp());
    push_vector(L, b->endp());
    return 2;
}

static int arc_angles(lua_State * L) {
    Arc * b = check_arc(L, 1);
    lua_pushnumber(L, b->iAlpha);
    lua_pushnumber(L, b->iBeta);
    return 2;
}

static int arc_bbox(lua_State * L) {
    Arc * b = check_arc(L, 1);
    push_rect(L, b->bbox());
    return 1;
}

static int arc_matrix(lua_State * L) {
    Arc * b = check_arc(L, 1);
    push_matrix(L, b->iM);
    return 1;
}

static int arc_isEllipse(lua_State * L) {
    Arc * b = check_arc(L, 1);
    lua_pushboolean(L, b->isEllipse());
    return 1;
}

static int arc_intersect(lua_State * L) {
    Arc * a = check_arc(L, 1);
    std::vector<Vector> pts;
    if (is_type(L, 2, "Ipe.segment")) {
	Segment * rhs = check_segment(L, 2);
	a->intersect(*rhs, pts);
    } else if (is_type(L, 2, "Ipe.line")) {
	Line * rhs = check_line(L, 2);
	a->intersect(*rhs, pts);
    } else if (is_type(L, 2, "Ipe.arc")) {
	Arc * rhs = check_arc(L, 2);
	a->intersect(*rhs, pts);
    } else if (is_type(L, 2, "Ipe.bezier")) {
	Bezier * rhs = check_bezier(L, 2);
	a->intersect(*rhs, pts);
    }
    lua_createtable(L, pts.size(), 0);
    for (int i = 0; i < int(pts.size()); ++i) {
	push_vector(L, pts[i]);
	lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

static int arc_snap(lua_State * L) {
    Arc * a = check_arc(L, 1);
    Vector * v = check_vector(L, 2);
    Vector pos;
    Angle alpha;
    (void)a->distance(*v, 10e9, pos, alpha);
    lua_pushnumber(L, double(alpha));
    push_vector(L, pos);
    return 2;
}

static const struct luaL_Reg arc_methods[] = {{"__tostring", arc_tostring},
					      {"endpoints", arc_endpoints},
					      {"angles", arc_angles},
					      {"bbox", arc_bbox},
					      {"matrix", arc_matrix},
					      {"isEllipse", arc_isEllipse},
					      {"intersect", arc_intersect},
					      {"snap", arc_snap},
					      {nullptr, nullptr}};

// --------------------------------------------------------------------

int ipelua::open_ipegeo(lua_State * L) {
    luaL_newmetatable(L, "Ipe.vector");
    luaL_setfuncs(L, vector_methods, 0);
    lua_pop(L, 1);

    make_metatable(L, "Ipe.matrix", matrix_methods);
    make_metatable(L, "Ipe.rect", rect_methods);
    make_metatable(L, "Ipe.line", line_methods);
    make_metatable(L, "Ipe.segment", segment_methods);
    make_metatable(L, "Ipe.bezier", bezier_methods);
    make_metatable(L, "Ipe.arc", arc_methods);

    return 0;
}

// --------------------------------------------------------------------

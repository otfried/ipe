// --------------------------------------------------------------------
// ipeluaipelets.cpp
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

#ifdef WIN32
#include <windows.h>
#else
#include <cstdlib>
#include <dlfcn.h>
#endif

using namespace ipe;
using namespace ipelua;

typedef Ipelet * (*PNewIpeletFn)();

// --------------------------------------------------------------------

static void snapFlag(lua_State * L, int & flags, const char * mode, uint32_t bits) {
    lua_getfield(L, -1, mode);
    if (!lua_isnil(L, -1))
	flags = lua_toboolean(L, -1) ? (flags | bits) : (flags & ~bits);
    lua_pop(L, 1);
}

void ipelua::get_snap(lua_State * L, int i, ipe::Snap & snap) {
    luaL_checktype(L, i, LUA_TTABLE);
    snapFlag(L, snap.iSnap, "snapvtx", Snap::ESnapVtx);
    snapFlag(L, snap.iSnap, "snapctl", Snap::ESnapCtl);
    snapFlag(L, snap.iSnap, "snapbd", Snap::ESnapBd);
    snapFlag(L, snap.iSnap, "snapint", Snap::ESnapInt);
    snapFlag(L, snap.iSnap, "snapgrid", Snap::ESnapGrid);
    snapFlag(L, snap.iSnap, "snapangle", Snap::ESnapAngle);
    snapFlag(L, snap.iSnap, "snapcustom", Snap::ESnapCustom);
    snapFlag(L, snap.iSnap, "snapauto", Snap::ESnapAuto);
    lua_getfield(L, i, "grid_visible");
    if (!lua_isnil(L, -1)) snap.iGridVisible = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, i, "gridsize");
    if (!lua_isnil(L, -1)) snap.iGridSize = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, i, "anglesize");
    if (!lua_isnil(L, -1)) snap.iAngleSize = IpePi * luaL_checknumber(L, -1) / 180.0;
    lua_pop(L, 1);
    lua_getfield(L, i, "snap_distance");
    if (!lua_isnil(L, -1)) snap.iSnapDistance = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, i, "with_axes");
    if (!lua_isnil(L, -1)) snap.iWithAxes = lua_toboolean(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, i, "origin");
    if (is_type(L, -1, "Ipe.vector")) snap.iOrigin = *check_vector(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, i, "orientation");
    if (!lua_isnil(L, -1)) snap.iDir = luaL_checknumber(L, -1);
    lua_pop(L, 1);
}

// --------------------------------------------------------------------

static int ipelet_destructor(lua_State * L) {
    ipeDebug("Ipelet destructor");
    Ipelet ** p = check_ipelet(L, 1);
    delete *p;
    *p = nullptr;
    return 0;
}

static int ipelet_tostring(lua_State * L) {
    check_ipelet(L, 1);
    lua_pushfstring(L, "Ipelet@%p", lua_topointer(L, 1));
    return 1;
}

// --------------------------------------------------------------------

int ipelua::ipelet_constructor(lua_State * L) {
    String fname(luaL_checklstring(L, 1, nullptr));
#if defined(WIN32)
    String dllname = fname + ".dll";
#else
    String dllname = fname + ".so";
#endif
    ipeDebug("Loading dll '%s'", dllname.z());
    PNewIpeletFn pIpelet = nullptr;
#ifdef WIN32
    HMODULE hMod = LoadLibraryW(dllname.w().data());
    if (hMod) {
	pIpelet = (PNewIpeletFn)GetProcAddress(hMod, "newIpelet");
	if (!pIpelet) pIpelet = (PNewIpeletFn)GetProcAddress(hMod, "_newIpelet");
    }
#else
    void * handle = dlopen(dllname.z(), RTLD_NOW);
    if (handle) {
	pIpelet = (PNewIpeletFn)dlsym(handle, "newIpelet");
	if (!pIpelet) pIpelet = (PNewIpeletFn)dlsym(handle, "_newIpelet");
    }
#endif
    if (pIpelet) {
	Ipelet ** p = (Ipelet **)lua_newuserdata(L, sizeof(Ipelet *));
	*p = nullptr;
	luaL_getmetatable(L, "Ipe.ipelet");
	lua_setmetatable(L, -2);

	Ipelet * ipelet = pIpelet();
	if (ipelet == nullptr) {
	    lua_pushnil(L);
	    lua_pushstring(L, "ipelet returns no object");
	    return 2;
	}

	if (ipelet->ipelibVersion() != IPELIB_VERSION) {
	    delete ipelet;
	    lua_pushnil(L);
	    lua_pushstring(L, "ipelet linked against older version of Ipelib");
	    return 2;
	}

	*p = ipelet;
	ipeDebug("Ipelet '%s' loaded", fname.z());
	return 1;
    } else {
	lua_pushnil(L);
#ifdef WIN32
	if (hMod)
	    lua_pushfstring(L, "Error %d finding DLL entry point", GetLastError());
	else
	    lua_pushfstring(L, "Error %d loading Ipelet DLL '%s'", GetLastError(),
			    dllname.z());
#else
	lua_pushfstring(L, "Error loading Ipelet '%s': %s", dllname.z(), dlerror());
#if defined(__APPLE__)
	ipeDebug("DYLD_LIBRARY_PATH=%s", getenv("DYLD_LIBRARY_PATH"));
	ipeDebug("DYLD_FALLBACK_LIBRARY_PATH=%s", getenv("DYLD_FALLBACK_LIBRARY_PATH"));
#endif
#endif
	return 2;
    }
}

// --------------------------------------------------------------------

class Helper : public IpeletHelper {
public:
    Helper(lua_State * L0, int luahelper);
    ~Helper();
    virtual void message(const char * msg);
    virtual int messageBox(const char * text, const char * details, int buttons);
    virtual bool getString(const char * prompt, String & str);
    virtual String getParameter(const char * key);

private:
    lua_State * L;
    int iHelper;
};

Helper::Helper(lua_State * L0, int luahelper) {
    L = L0;
    iHelper = luahelper;
}

Helper::~Helper() { luaL_unref(L, LUA_REGISTRYINDEX, iHelper); }

void Helper::message(const char * msg) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, iHelper);
    lua_getfield(L, -1, "message");
    lua_pushvalue(L, -2); // luahelper
    lua_remove(L, -3);
    lua_pushstring(L, msg);
    luacall(L, 2, 0);
}

int Helper::messageBox(const char * text, const char * details, int buttons) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, iHelper);
    lua_getfield(L, -1, "messageBox");
    lua_pushvalue(L, -2); // luahelper
    lua_remove(L, -3);
    lua_pushstring(L, text);
    if (details)
	lua_pushstring(L, details);
    else
	lua_pushnil(L);
    lua_pushnumber(L, buttons);
    luacall(L, 4, 1);
    if (lua_isnumber(L, -1))
	return int(lua_tonumberx(L, -1, nullptr));
    else
	return 0;
}

bool Helper::getString(const char * prompt, String & str) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, iHelper);
    lua_getfield(L, -1, "getString");
    lua_pushvalue(L, -2); // luahelper
    lua_remove(L, -3);
    lua_pushstring(L, prompt);
    push_string(L, str);
    luacall(L, 3, 1);
    if (lua_isstring(L, -1)) {
	str = lua_tolstring(L, -1, nullptr);
	return true;
    } else
	return false;
}

String Helper::getParameter(const char * key) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, iHelper);
    lua_getfield(L, -1, "parameters");
    String value;
    if (lua_istable(L, -1)) {
	lua_getfield(L, -1, key);
	const char * t = lua_tolstring(L, -1, nullptr);
	if (t) value = t;
	lua_pop(L, 1); // parameters[key]
    }
    lua_pop(L, 2); // helper, parameters
    return value;
}

// --------------------------------------------------------------------

static int ipelet_run(lua_State * L) {
    Ipelet ** p = check_ipelet(L, 1);
    int num = (int)luaL_checkinteger(L, 2) - 1; // Lua counts starting from one
    IpeletData data;
    data.iPage = check_page(L, 3)->page;
    data.iDoc = *check_document(L, 4);
    data.iPageNo = (int)luaL_checkinteger(L, 5);
    data.iView = (int)luaL_checkinteger(L, 6);
    data.iLayer = check_layer(L, 7, data.iPage);
    check_allattributes(L, 8, data.iAttributes);
    get_snap(L, 9, data.iSnap);
    lua_pushvalue(L, 10);
    int luahelper = luaL_ref(L, LUA_REGISTRYINDEX);
    Helper helper(L, luahelper);
    bool result = (*p)->run(num, &data, &helper);
    lua_pushboolean(L, result);
    return 1;
}

// --------------------------------------------------------------------

static const struct luaL_Reg ipelet_methods[] = {{"__tostring", ipelet_tostring},
						 {"__gc", ipelet_destructor},
						 {"run", ipelet_run},
						 {nullptr, nullptr}};

// --------------------------------------------------------------------

int ipelua::open_ipelets(lua_State * L) {
    make_metatable(L, "Ipe.ipelet", ipelet_methods);

    return 0;
}

// --------------------------------------------------------------------

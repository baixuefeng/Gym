#include "lua/lua_wrapper.h"
#include "UI/GraphicLayer/ConfigEngine/LayerLuaAdaptor.h"
#include "UI/GraphicLayer/Layers/GraphicLayer.h"
#include <cassert>

using shr::lua_istream;
using shr::lua_ostream;

static lua_istream & operator >> (lua_istream & is, RECT & rc)
{
    is >> rc.left >> rc.top >> rc.right >> rc.bottom;
    return is;
}

static lua_ostream & operator << (lua_ostream & os, const RECT & rc)
{
    os << lua_ostream::table_begin
        << rc.left << rc.top << rc.right << rc.bottom
        << lua_ostream::table_end;
    return os;
}

static lua_istream & operator >> (lua_istream & is, POINT & pt)
{
    is >> pt.x >> pt.y;
    return is;
}

static lua_ostream & operator << (lua_ostream & os, const POINT & pt)
{
    os << lua_ostream::table_begin
        << pt.x << pt.y
        << lua_ostream::table_end;
    return os;
}

static lua_istream & operator >> (lua_istream & is, SIZE & sz)
{
    is >> sz.cx >> sz.cy;
    return is;
}

static lua_ostream & operator << (lua_ostream & os, const SIZE & sz)
{
    os << lua_ostream::table_begin
        << sz.cx << sz.cy
        << lua_ostream::table_end;
    return os;
}

SHARELIB_BEGIN_NAMESPACE

BEGIN_LUA_CPP_MAP_IMPLEMENT(RegisterGraphicLayerToLua, "Layer")
ENTRY_LUA_CPP_MAP_IMPLEMENT("root", &GraphicLayer::root)
ENTRY_LUA_CPP_MAP_IMPLEMENT("is_root", &GraphicLayer::is_root)
ENTRY_LUA_CPP_MAP_IMPLEMENT("parent", &GraphicLayer::parent)
ENTRY_LUA_CPP_MAP_IMPLEMENT("child_count", &GraphicLayer::child_count)
ENTRY_LUA_CPP_MAP_IMPLEMENT("previous_sibling", &GraphicLayer::previous_sibling)
ENTRY_LUA_CPP_MAP_IMPLEMENT("next_sibling", &GraphicLayer::next_sibling)
ENTRY_LUA_CPP_MAP_IMPLEMENT("first_child", &GraphicLayer::first_child)
ENTRY_LUA_CPP_MAP_IMPLEMENT("last_child", &GraphicLayer::last_child)
ENTRY_LUA_CPP_MAP_IMPLEMENT("nth_child", &GraphicLayer::nth_child)
ENTRY_LUA_CPP_MAP_IMPLEMENT("GetOrigin", &GraphicLayer::GetOrigin)
ENTRY_LUA_CPP_MAP_IMPLEMENT("SetOrigin", &GraphicLayer::SetOrigin)
ENTRY_LUA_CPP_MAP_IMPLEMENT("OffsetOrigin", &GraphicLayer::OffsetOrigin)
ENTRY_LUA_CPP_MAP_IMPLEMENT("AlignXY", &GraphicLayer::AlignXY)
ENTRY_LUA_CPP_MAP_IMPLEMENT("GetLayerBounds", &GraphicLayer::GetLayerBounds)
ENTRY_LUA_CPP_MAP_IMPLEMENT("SetLayerBounds", &GraphicLayer::SetLayerBounds)
END_LUA_CPP_MAP_IMPLEMENT()

SHARELIB_END_NAMESPACE

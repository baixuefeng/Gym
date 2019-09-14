#pragma once
#include "MacroDefBase.h"

struct lua_State;

SHARELIB_BEGIN_NAMESPACE

void RegisterGraphicLayerToLua(lua_State * pLua);

SHARELIB_END_NAMESPACE

﻿#pragma once
#include "MacroDefBase.h"
#include <vector>
#include <dxgi.h>

SHARELIB_BEGIN_NAMESPACE

bool GetVideoAdapterDescription(std::vector<DXGI_ADAPTER_DESC>& adapters);

SHARELIB_END_NAMESPACE

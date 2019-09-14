#include "MacroDefBase.h"
#include "UI/GraphicLayer/Layers/GraphicLayer.h"
#include "UI/GraphicLayer/Layers/GraphicBitmap.h"
#include "UI/GraphicLayer/Layers/GraphicText.h"
#include "UI/GraphicLayer/Layers/GraphicScrollLayer.h"

SHARELIB_BEGIN_NAMESPACE

void InitLayersDynimicCreateInfo()
{
    GraphicLayer::InitDynimicCreateInfo();
    GraphicText::InitDynimicCreateInfo();
    GraphicBitmap::InitDynimicCreateInfo();
    GraphicScrollLayer::InitDynimicCreateInfo();
}

SHARELIB_END_NAMESPACE

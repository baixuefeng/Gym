#pragma once
#include "MacroDefBase.h"
#include "pugixml/pugixml.hpp"
#include "UI/GraphicLayer/Layers/GraphicLayerTypeDef.h"
#include <cstdint>
#include <windows.h>
#include <d2d1helper.h>

SHARELIB_BEGIN_NAMESPACE

namespace XmlAttributeUtility
{
//----下面读取属性，如果没有找到则保持原值不变---------------------
 
    bool ReadXmlValue(const wchar_t * pValue, RECT & rc);
    bool ReadXmlValue(const wchar_t * pValue, SIZE & sz);
    bool ReadXmlValue(const wchar_t * pValue, POINT & pt);
    bool ReadXmlValue(const wchar_t * pValue, D2D1_RECT_F & rc);
    bool ReadXmlValue(const wchar_t * pValue, D2D1_SIZE_F & sz);
    bool ReadXmlValue(const wchar_t * pValue, D2D1_POINT_2F & pt);

    //16进制, 从高到低 argb 格式颜色值, COLOR32
    bool ReadXmlValueColor(const wchar_t * pValue, COLOR32 & color);

//----写入属性----------------------------------
    bool WriteValueToXml(pugi::xml_attribute attr, const RECT & rc);
    bool WriteValueToXml(pugi::xml_attribute attr, const SIZE & sz);
    bool WriteValueToXml(pugi::xml_attribute attr, const POINT & pt);
    bool WriteValueToXml(pugi::xml_attribute attr, const D2D1_RECT_F & rc);
    bool WriteValueToXml(pugi::xml_attribute attr, const D2D1_SIZE_F & sz);
    bool WriteValueToXml(pugi::xml_attribute attr, const D2D1_POINT_2F & pt);
    //16进制, 从高到低 argb 格式颜色值, COLOR32
    bool WriteValueColorToXml(pugi::xml_attribute attr, COLOR32 color);
}

SHARELIB_END_NAMESPACE

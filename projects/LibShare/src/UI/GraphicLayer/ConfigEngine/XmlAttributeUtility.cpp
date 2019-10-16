#include "UI/GraphicLayer/ConfigEngine/XmlAttributeUtility.h"
#include <cassert>
#include <cstdlib>
#include <cwctype>
#include <strsafe.h>

SHARELIB_BEGIN_NAMESPACE

#define READ_XML_ARRAY_VALUE_IMPL(type, func)                                                      \
    static bool ReadXmlArrayValueImpl(const wchar_t *pValue, type *pArray, size_t nCount)          \
    {                                                                                              \
        wchar_t *pEnd = nullptr;                                                                   \
        for (size_t i = 0; (i < nCount) && pValue && *pValue; i++) {                               \
            pArray[i] = func;                                                                      \
            if (!pEnd || (!*pEnd)) {                                                               \
                break;                                                                             \
            }                                                                                      \
            pValue = std::wcschr(pEnd, L',');                                                      \
            ++pValue;                                                                              \
        }                                                                                          \
        return true;                                                                               \
    }

READ_XML_ARRAY_VALUE_IMPL(LONG, std::wcstol(pValue, &pEnd, 0))
READ_XML_ARRAY_VALUE_IMPL(FLOAT, std::wcstof(pValue, &pEnd))

bool XmlAttributeUtility::ReadXmlValue(const wchar_t *pValue, RECT &rc)
{
    return ReadXmlArrayValueImpl(pValue, (LONG *)&rc, 4);
}

bool XmlAttributeUtility::ReadXmlValue(const wchar_t *pValue, SIZE &sz)
{
    return ReadXmlArrayValueImpl(pValue, (LONG *)&sz, 2);
}

bool XmlAttributeUtility::ReadXmlValue(const wchar_t *pValue, POINT &pt)
{
    return ReadXmlArrayValueImpl(pValue, (LONG *)&pt, 2);
}

bool XmlAttributeUtility::ReadXmlValue(const wchar_t *pValue, D2D1_RECT_F &rc)
{
    return ReadXmlArrayValueImpl(pValue, (FLOAT *)&rc, 4);
}

bool XmlAttributeUtility::ReadXmlValue(const wchar_t *pValue, D2D1_SIZE_F &sz)
{
    return ReadXmlArrayValueImpl(pValue, (FLOAT *)&sz, 2);
}

bool XmlAttributeUtility::ReadXmlValue(const wchar_t *pValue, D2D1_POINT_2F &pt)
{
    return ReadXmlArrayValueImpl(pValue, (FLOAT *)&pt, 2);
}

bool XmlAttributeUtility::ReadXmlValueColor(const wchar_t *pValue, COLOR32 &color)
{
    if (pValue && *pValue) {
        color = VERIFY_COLOR32(std::wcstoul(pValue, nullptr, 16));
    }
    return true;
}

#define WRITE_ARRAY_VALUE_TO_XML_IMPL(type, flag)                                                  \
    static bool WriteArrayValueToXmlImpl(                                                          \
        pugi::xml_attribute attr, const type *pArray, size_t nCount)                               \
    {                                                                                              \
        if (!attr) {                                                                               \
            return false;                                                                          \
        }                                                                                          \
        wchar_t szBuffer[1024] = {0};                                                              \
        wchar_t *pWrite = szBuffer;                                                                \
        size_t nSize = sizeof(szBuffer);                                                           \
        for (size_t i = 0; i < nCount; ++i) {                                                      \
            ::StringCbPrintfExW(pWrite, nSize, &pWrite, &nSize, 0, L#flag, pArray[i]);             \
            if (i + 1 < nCount) {                                                                  \
                ::StringCbCatExW(pWrite, nSize, L",", &pWrite, &nSize, 0);                         \
            }                                                                                      \
        }                                                                                          \
        attr.set_value(szBuffer);                                                                  \
        return true;                                                                               \
    }

WRITE_ARRAY_VALUE_TO_XML_IMPL(LONG, % d)
WRITE_ARRAY_VALUE_TO_XML_IMPL(FLOAT, % g)

bool XmlAttributeUtility::WriteValueToXml(pugi::xml_attribute attr, const RECT &rc)
{
    return WriteArrayValueToXmlImpl(attr, (LONG *)&rc, 4);
}

bool XmlAttributeUtility::WriteValueToXml(pugi::xml_attribute attr, const SIZE &sz)
{
    return WriteArrayValueToXmlImpl(attr, (LONG *)&sz, 2);
}

bool XmlAttributeUtility::WriteValueToXml(pugi::xml_attribute attr, const POINT &pt)
{
    return WriteArrayValueToXmlImpl(attr, (LONG *)&pt, 2);
}

bool XmlAttributeUtility::WriteValueToXml(pugi::xml_attribute attr, const D2D1_RECT_F &rc)
{
    return WriteArrayValueToXmlImpl(attr, (FLOAT *)&rc, 4);
}

bool XmlAttributeUtility::WriteValueToXml(pugi::xml_attribute attr, const D2D1_SIZE_F &sz)
{
    return WriteArrayValueToXmlImpl(attr, (FLOAT *)&sz, 2);
}

bool XmlAttributeUtility::WriteValueToXml(pugi::xml_attribute attr, const D2D1_POINT_2F &pt)
{
    return WriteArrayValueToXmlImpl(attr, (FLOAT *)&pt, 2);
}

bool XmlAttributeUtility::WriteValueColorToXml(pugi::xml_attribute attr, uint32_t color)
{
    if (!attr) {
        return false;
    }
    wchar_t szBuffer[100] = {0};
    ::StringCbPrintf(szBuffer, sizeof(szBuffer), L"%X", VERIFY_COLOR32(color));
    attr.set_value(szBuffer);
    return true;
}

SHARELIB_END_NAMESPACE

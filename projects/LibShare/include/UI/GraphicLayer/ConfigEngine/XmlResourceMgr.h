#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

struct TBitmapInfo
{
    TBitmapInfo(TBitmapInfo &&other);
    TBitmapInfo &operator=(TBitmapInfo &&other);

    std::unique_ptr<uint8_t[]> m_spBitmapData;

    size_t m_nWidth{0};

    size_t m_nHeight{0};

    size_t m_nMarginLeft{0};

    size_t m_nMarginTop{0};

    size_t m_nMarginRight{0};

    size_t m_nMarginBottom{0};

private:
    friend std::shared_ptr<std::unordered_map<std::wstring, TBitmapInfo>> LoadSkinFromXml(
        const void *pData,
        size_t nLength);
    TBitmapInfo();
};

/*
"Path"    : <字符串><wchar_t*><图片路径><>
"Margin"  : <left,top,right,bottom><int32_t><图片边缘空白，不拉伸的部分，用于九宫格绘制><>
*/
std::shared_ptr<std::unordered_map<std::wstring, TBitmapInfo>> LoadSkinFromXml(
    const wchar_t *pXmlFilePath);

std::shared_ptr<std::unordered_map<std::wstring, TBitmapInfo>> LoadSkinFromXml(const void *pData,
                                                                               size_t nLength);

SHARELIB_END_NAMESPACE

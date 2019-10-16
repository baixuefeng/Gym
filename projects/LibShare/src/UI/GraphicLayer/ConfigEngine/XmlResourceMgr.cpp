#include "targetver.h"
#include "UI/GraphicLayer/ConfigEngine/XmlResourceMgr.h"
#include <atomic>
#include <cassert>
#include <codecvt>
#include <condition_variable>
#include <future>
#include <locale>
#include <mutex>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "DataStructure/traverse_tree_node.h"
#include "UI/GraphicLayer/ConfigEngine/XmlAttributeUtility.h"
#include "UI/Utility/GdiplusUtility.h"
#include "pugixml/pugixml.hpp"
#pragma warning(disable : 4458)
#include <GdiPlus.h>
#pragma warning(default : 4458)

SHARELIB_BEGIN_NAMESPACE

static bool DecodePicture(const wchar_t *pPicture, TBitmapInfo &bitmapInfo)
{
    if (!InitializeGdiplus()) {
        return false;
    }
    std::unique_ptr<Gdiplus::Bitmap> spBitmap{Gdiplus::Bitmap::FromFile(pPicture)};
    if (spBitmap && spBitmap->GetWidth() > 0 && spBitmap->GetHeight() > 0) {
        Gdiplus::Rect rcBitmap{0, 0, (int)spBitmap->GetWidth(), (int)spBitmap->GetHeight()};
        Gdiplus::BitmapData data;
        if (Gdiplus::Ok == spBitmap->LockBits(&rcBitmap,
                                              Gdiplus::ImageLockMode::ImageLockModeRead,
                                              PixelFormat32bppPARGB,
                                              &data)) {
            auto nSize = 4 * spBitmap->GetWidth() * spBitmap->GetHeight();
            bitmapInfo.m_spBitmapData.reset(new uint8_t[nSize]{0});
            std::memcpy(bitmapInfo.m_spBitmapData.get(), data.Scan0, nSize);
            bitmapInfo.m_nWidth = spBitmap->GetWidth();
            bitmapInfo.m_nHeight = spBitmap->GetHeight();

            spBitmap->UnlockBits(&data);
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------

TBitmapInfo::TBitmapInfo() {}

TBitmapInfo::TBitmapInfo(TBitmapInfo &&other)
{
    m_spBitmapData = std::move(other.m_spBitmapData);
    m_nWidth = other.m_nWidth;
    m_nHeight = other.m_nHeight;
    m_nMarginLeft = other.m_nMarginLeft;
    m_nMarginTop = other.m_nMarginTop;
    m_nMarginRight = other.m_nMarginRight;
    m_nMarginBottom = other.m_nMarginBottom;
}

TBitmapInfo &TBitmapInfo::operator=(TBitmapInfo &&other)
{
    if (this != &other) {
        m_spBitmapData = std::move(other.m_spBitmapData);
        m_nWidth = other.m_nWidth;
        m_nHeight = other.m_nHeight;
        m_nMarginLeft = other.m_nMarginLeft;
        m_nMarginTop = other.m_nMarginTop;
        m_nMarginRight = other.m_nMarginRight;
        m_nMarginBottom = other.m_nMarginBottom;
    }
    return *this;
}

std::shared_ptr<std::unordered_map<std::wstring, TBitmapInfo>> LoadSkinFromXml(
    const wchar_t *pXmlFilePath)
{
    if (!pXmlFilePath) {
        return nullptr;
    }
    try {
        auto &cvtFacet = std::use_facet<std::codecvt_utf16<wchar_t>>(std::locale());
        std::wstring_convert<std::codecvt_utf16<wchar_t>> cvt{&cvtFacet};
        boost::interprocess::file_mapping fileMap{cvt.to_bytes(pXmlFilePath).c_str(),
                                                  boost::interprocess::mode_t::read_only};
        boost::interprocess::mapped_region region{fileMap, boost::interprocess::mode_t::read_only};
        return LoadSkinFromXml(region.get_address(), region.get_size());
    } catch (const std::exception &) {
        return nullptr;
    }
}

std::shared_ptr<std::unordered_map<std::wstring, TBitmapInfo>> LoadSkinFromXml(const void *pData,
                                                                               size_t nLength)
{
    assert(pData && nLength);
    if (!pData || (nLength == 0)) {
        return nullptr;
    }
    pugi::xml_document xmlDoc;
    if (!xmlDoc.load_buffer(pData, nLength)) {
        assert(!"解析xml失败");
        return nullptr;
    }

    auto spSkinMap = std::make_shared<std::unordered_map<std::wstring, TBitmapInfo>>();

    std::mutex mapLock;
    std::mutex notifyLock;
    std::condition_variable cvLock;
    std::atomic<size_t> nCount;

    traverse_tree_node_t2b(
        xmlDoc.first_child(),
        [&spSkinMap, &mapLock, &notifyLock, &cvLock, &nCount](pugi::xml_node &node,
                                                              int nDepth) -> int {
            if (nDepth == 0) {
                return 1;
            }
            ++nCount;
            std::async([&spSkinMap, &mapLock, &notifyLock, &cvLock, &nCount, node]() {
                auto attr = node.attribute(L"Path");
                if (attr) {
                    TBitmapInfo bitmapInfo;
                    if (DecodePicture(attr.value(), bitmapInfo)) {
                        attr = node.attribute(L"Margin");
                        if (attr) {
                            RECT margin{0};
                            XmlAttributeUtility::ReadXmlValue(attr.value(), margin);
                            bitmapInfo.m_nMarginLeft = margin.left;
                            bitmapInfo.m_nMarginTop = margin.top;
                            bitmapInfo.m_nMarginRight = margin.right;
                            bitmapInfo.m_nMarginBottom = margin.bottom;
                        }

                        {
                            std::lock_guard<std::mutex> lock{mapLock};
                            auto it = spSkinMap->find(node.name());
                            if (it == spSkinMap->end()) {
                                spSkinMap->insert(
                                    std::make_pair(node.name(), std::move(bitmapInfo)));
                            } else {
                                it->second = std::move(bitmapInfo);
                            }
                        }
                    }
                }

                if (--nCount == 0) {
                    std::unique_lock<std::mutex> lock{notifyLock};
                    cvLock.notify_all();
                }
            });

            return 0;
        });

    {
        std::unique_lock<std::mutex> lock{notifyLock};
        if (nCount > 0) {
            cvLock.wait(lock);
        }
    }
    return spSkinMap;
}

SHARELIB_END_NAMESPACE

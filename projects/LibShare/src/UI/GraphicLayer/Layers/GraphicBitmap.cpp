#include "targetver.h"
#include "UI/GraphicLayer/Layers/GraphicBitmap.h"
#include <cassert>
#include <cstring>
#include <atlfile.h>
#include "UI/GraphicLayer/Layers/GraphicLayer.h"
#include "UI/GraphicLayer/Layers/GraphicRootLayer.h"

SHARELIB_BEGIN_NAMESPACE

IMPLEMENT_RUNTIME_DYNAMIC_CREATE(GraphicBitmap, GraphicLayer::RegisterLayerClassesInfo)

GraphicBitmap::GraphicBitmap()
    : m_bLayoutByPictureSize(false)
{}

void GraphicBitmap::SetBitmapData(const void *pBitmapData, SIZE sz)
{
    if (pBitmapData && sz.cx > 0 && sz.cy > 0) {
        m_szBitmap = sz;
        if (m_bLayoutByPictureSize || GetLayerBounds().IsRectEmpty()) {
            m_bLayoutByPictureSize = true;
            SetLayerBounds(CRect(GetOrigin(), m_szBitmap));
        }
        m_spBitmapData.reset(new uint8_t[sz.cx * sz.cy * 4]);
        std::memcpy(m_spBitmapData.get(), pBitmapData, sz.cx * sz.cy * 4);
        m_bitmapCacheID.ReleaseD2DResourceCache();
    }
}

void GraphicBitmap::SetLayoutByPicture(bool bLayoutByPicture)
{
    m_bLayoutByPictureSize = bLayoutByPicture;
}

void GraphicBitmap::OnReadingXmlAttribute(const pugi::xml_attribute &attr)
{
    if (std::wcscmp(L"lrSkin", attr.name()) == 0) {
        m_skin = attr.as_string();
    } else {
        __super::OnReadingXmlAttribute(attr);
    }
}

void GraphicBitmap::OnWritingAttributeToXml(pugi::xml_node node)
{
    if (!m_skin.empty()) {
        auto attr = node.append_attribute(L"lrSkin");
        attr.set_value(m_skin.c_str());
    }

    __super::OnWritingAttributeToXml(node);
}

bool GraphicBitmap::OnLayerMsgLayout(GraphicLayer *pLayer, bool &bLayoutChildren)
{
    bool bHandled = __super::OnLayerMsgLayout(pLayer, bLayoutChildren);
    if (m_bLayoutByPictureSize || GetLayerBounds().IsRectEmpty()) {
        m_bLayoutByPictureSize = true;
        if (m_spBitmapData) {
            SetLayerBounds(CRect(GetOrigin(), m_szBitmap));
        } else if (!m_skin.empty()) {
            auto pRoot = GetRootGraphicLayer();
            if (pRoot) {
                auto pBitmapInfo = pRoot->FindSkinByName(m_skin);
                if (pBitmapInfo) {
                    m_szBitmap = SIZE{(LONG)pBitmapInfo->m_nWidth, (LONG)pBitmapInfo->m_nHeight};
                    SetLayerBounds(CRect(GetOrigin(), m_szBitmap));
                }
            }
        }
    }
    return bHandled;
}

void GraphicBitmap::Paint(D2DRenderPack &d2dRender)
{
    ATL::CComQIPtr<ID2D1Bitmap> spBitmap = d2dRender.GetCachedD2DResource(m_bitmapCacheID);
    if (!spBitmap) {
        if (m_spBitmapData) {
            spBitmap = d2dRender.CreateD2DBitmap(m_szBitmap, m_spBitmapData.get());
        } else if (!m_skin.empty()) {
            auto pRoot = GetRootGraphicLayer();
            if (pRoot) {
                auto pBitmapInfo = pRoot->FindSkinByName(m_skin);
                if (pBitmapInfo) {
                    m_szBitmap = SIZE{(LONG)pBitmapInfo->m_nWidth, (LONG)pBitmapInfo->m_nHeight};
                    spBitmap = d2dRender.CreateD2DBitmap(m_szBitmap,
                                                         pBitmapInfo->m_spBitmapData.get());
                }
            }
        }
        m_bitmapCacheID = d2dRender.CacheD2DResource(spBitmap);
    }
    if (spBitmap) {
        d2dRender.m_spD2DRender->DrawBitmap(
            spBitmap,
            CD2DRectF(GetLayerBounds()),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            CD2DRectF(CD2DPointF(), CD2DSizeF(m_szBitmap)));
        //d2dRender.m_spD2DRender->SetAntialiasMode(D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_ALIASED);
        //d2dRender.m_spSolidBrush->SetColor(D2D1::ColorF(D2D_RGB(255, 0, 0)));
        //d2dRender.m_spD2DRender->FillOpacityMask(pBitmap, d2dRender.m_spSolidBrush, D2D1_OPACITY_MASK_CONTENT::D2D1_OPACITY_MASK_CONTENT_GRAPHICS, CD2DRectF(GetBoundsRect()), CD2DRectF(CD2DPointF(), CD2DSizeF(m_szBitmap)));
    }
}

bool GraphicBitmap::RGBA2PBGRA(void *pData, SIZE sz)
{
    if (!pData) {
        assert(!"参数错误");
        return false;
    }

    uint8_t *p = (uint8_t *)pData;
    uint32_t pixelCount = sz.cx * sz.cy;
    uint8_t a = 0;
    uint8_t t = 0;
    for (uint32_t i = 0; i < pixelCount; ++i) {
        a = p[3];
        t = p[0];
        if (a != 0) {
            p[0] = (p[2] * a) / 255;
            p[1] = (p[1] * a) / 255;
            p[2] = (t * a) / 255;
        } else {
            memset(p, 0, 4);
        }
        p += 4;
    }
    return true;
}

bool GraphicBitmap::GetSubBitmapData(const void *pImgData,
                                     SIZE sz,
                                     void *pSubImgData,
                                     const RECT &subRect)
{
    if (!pImgData || !pSubImgData || (subRect.right > sz.cx) || (subRect.bottom > sz.cy) ||
        (subRect.left < 0) || (subRect.top < 0) || ::IsRectEmpty(&subRect)) {
        return false;
    }

    uint8_t *pIndex = (uint8_t *)pSubImgData;
    for (int32_t i = subRect.top; i < subRect.bottom; ++i) {
        std::memcpy(pIndex,
                    (const uint8_t *)pImgData + sz.cx * 4 * i + subRect.left * 4,
                    (subRect.right - subRect.left) * 4);
        pIndex += (subRect.right - subRect.left) * 4;
    }
    return true;
}

SHARELIB_END_NAMESPACE

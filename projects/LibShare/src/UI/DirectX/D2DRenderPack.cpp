#include "UI/DirectX/D2DRenderPack.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <mutex>
#include <atlbase.h>
#include <atlwin.h>

SHARELIB_BEGIN_NAMESPACE
#pragma warning(push)
#pragma warning(disable : 4995)

//角度转化为弧度
#ifndef TO_RADIAN
#    define TO_RADIAN(degree) ((degree)*0.01745329252)
#endif

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

// 离屏渲染时的隐藏的辅助窗口
class D2DRenderPack::OffScreenWnd
    : public ATL::CWindowImpl<OffScreenWnd, ATL::CWindow, ATL::CFrameWinTraits>
{
    BEGIN_MSG_MAP(OffScreenWnd)
    END_MSG_MAP()
};

D2DRenderPack::RectClipGuard::RectClipGuard(D2DRenderPack &render,
                                            const RECT &rcClip,
                                            bool bAntiAliasing /* = false*/)
    : m_render(render)
{
    m_render.PushClipRect(rcClip, bAntiAliasing);
}

D2DRenderPack::RectClipGuard::~RectClipGuard()
{
    m_render.PopClipRect();
}

D2DRenderPack::RenderLayerGuard::RenderLayerGuard(D2DRenderPack &render,
                                                  const D2D1_LAYER_PARAMETERS &layerParam)
    : m_render(render)
{
    m_render.PushD2DLayer(layerParam);
}

D2DRenderPack::RenderLayerGuard::~RenderLayerGuard()
{
    m_render.PopD2DLayer();
}

D2DRenderPack::DrawingStateGuard::DrawingStateGuard(D2DRenderPack &render)
    : m_render(render)
{
    m_render.PushDrawingState();
}

D2DRenderPack::DrawingStateGuard::~DrawingStateGuard()
{
    m_render.PopDrawingState();
}

D2DRenderPack::CachedResID::CachedResID()
    : m_cacheID(0)
    , m_pRenderPack(nullptr)
{}

D2DRenderPack::CachedResID::CachedResID(CachedResID &&other)
{
    m_cacheID = other.m_cacheID;
    other.m_cacheID = 0;
    m_pRenderPack = other.m_pRenderPack;
    other.m_pRenderPack = nullptr;
}

D2DRenderPack::CachedResID &D2DRenderPack::CachedResID::operator=(CachedResID &&other)
{
    if (this != &other) {
        std::swap(m_cacheID, other.m_cacheID);
        std::swap(m_pRenderPack, other.m_pRenderPack);
    }
    return *this;
}

D2DRenderPack::CachedResID::~CachedResID()
{
    ReleaseD2DResourceCache();
}

void D2DRenderPack::CachedResID::ReleaseD2DResourceCache()
{
    if (m_pRenderPack && m_cacheID) {
        m_pRenderPack->ReleaseD2DResourceCache(*this);
        m_cacheID = 0;
        m_pRenderPack = nullptr;
    }
}

D2DRenderPack::CachedResID::operator uint64_t()
{
    return m_cacheID;
}

void D2DRenderPack::CachedResID::Detach()
{
    m_cacheID = 0;
    m_pRenderPack = nullptr;
}

//-------------------------------------------------------------------------

D2DRenderPack::D2DRenderPack()
    : m_nUserResID(0)
{
    m_statePool.push_back(ATL::CComPtr<ID2D1DrawingStateBlock>()); //添加一个空指针作为哨兵
    m_layerPool.push_back(ATL::CComPtr<ID2D1Layer>()); //添加一个空指针作为哨兵
}

D2DRenderPack::~D2DRenderPack()
{
    ClearRenderRes();
    m_userResCache.clear();
    m_statePool.clear();
    m_spD2DFactory.Release();
    if (m_spOffScreenWnd && m_spOffScreenWnd->IsWindow()) {
        m_spOffScreenWnd->DestroyWindow();
    }
}

bool D2DRenderPack::CreateD2DFactory()
{
    using TD2DCreateFunc = HRESULT(WINAPI *)(_In_ D2D1_FACTORY_TYPE factoryType,
                                             _In_ REFIID riid,
                                             _In_opt_ CONST D2D1_FACTORY_OPTIONS * pFactoryOptions,
                                             _Out_ void **ppIFactory);

    static TD2DCreateFunc s_d2dCreateFunc = nullptr;
    static std::once_flag s_d2dInitFlags;
    if (!s_d2dCreateFunc) {
        std::call_once(s_d2dInitFlags, [this]() {
            HMODULE hD2D = ::LoadLibrary(L"d2d1.dll");
            if (hD2D) {
                s_d2dCreateFunc = (TD2DCreateFunc)::GetProcAddress(hD2D, "D2D1CreateFactory");
            }
        });
        if (!s_d2dCreateFunc) {
            return false;
        }
    }

    HRESULT hr = S_OK;
    if (!m_spD2DFactory) {
        hr = s_d2dCreateFunc(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_SINGLE_THREADED,
                             __uuidof(ID2D1Factory),
                             nullptr,
                             (void **)&m_spD2DFactory);
        assert(SUCCEEDED(hr));
        if (!m_spD2DFactory || FAILED(hr)) {
            return false;
        }
    }
    return true;
}

/* 1.基于HWND的CPU（1.5%）、GPU（5%）都最低；基于DC的CPU（7%）最高，GPU（15%）比较高；基于Compatibale的
   CPU比较高（5%），GPU最高（20%）
   2.Compatibal的Render可以和源Render共享资源；
   3.层窗口时基于桌面窗口句柄比实际的实际窗口句柄效率更高，CPU，GPU占用率都更低，绘制效果没有区别，但刷新比较慢，
   闪烁现象比较严重；
   4.离屏渲染两种方式：基于DC；从任意窗口句柄创建基于HWND的Render，再基于它创建Compatibale的Render；
   5.基于WICbitmap的Render不能硬件加速.
   */

bool D2DRenderPack::PrepareRender(HWND hWnd,
                                  const SIZE &szRender,
                                  TRenderType renderType,
                                  float fDPI /* = 1.0f*/)
{
    if (!CreateD2DFactory()) {
        return false;
    }
    assert(!m_spD2DHwndRender || ::IsWindow(m_spD2DHwndRender->GetHwnd()));

    bool isOffScreenMode = !::IsWindow(hWnd);
    if (isOffScreenMode) {
        if (!m_spD2DHwndRender) {
            if (!m_spOffScreenWnd) {
                m_spOffScreenWnd.reset(new OffScreenWnd);
                m_spOffScreenWnd->Create(NULL, NULL);
                assert(m_spOffScreenWnd->IsWindow());
            }
            //为方便创建Render,借用一个隐藏的窗口,使用桌面窗口在某些系统中会出错.
            hWnd = *m_spOffScreenWnd;
        }
    } else {
        if (m_spD2DHwndRender && m_spD2DHwndRender->GetHwnd() != hWnd) {
            ClearRenderRes();
        }
    }

    assert(!m_statePool.front().m_T); //检查是否配对
    assert(!m_layerPool.front().m_T); //检查是否配对
    assert(m_clipStack.empty());      //检查是否配对
    HRESULT hr = S_OK;
    if (!m_spD2DHwndRender) {
        D2D1_RENDER_TARGET_TYPE d2dRender =
            D2D1_RENDER_TARGET_TYPE::D2D1_RENDER_TARGET_TYPE_DEFAULT;
        switch (renderType) {
        case D2DRenderPack::Hardware:
            d2dRender = D2D1_RENDER_TARGET_TYPE::D2D1_RENDER_TARGET_TYPE_HARDWARE;
            break;
        case D2DRenderPack::Software:
            d2dRender = D2D1_RENDER_TARGET_TYPE::D2D1_RENDER_TARGET_TYPE_SOFTWARE;
            break;
        default:
            break;
        }
        hr = m_spD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(
                d2dRender,
                D2D1::PixelFormat(DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
                                  D2D1_ALPHA_MODE::D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.f * fDPI,
                96.f * fDPI,
                D2D1_RENDER_TARGET_USAGE::D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
                D2D1_FEATURE_LEVEL::D2D1_FEATURE_LEVEL_DEFAULT),
            D2D1::HwndRenderTargetProperties(hWnd),
            &m_spD2DHwndRender);
        assert(SUCCEEDED(hr));
        if (FAILED(hr)) {
            ClearRenderRes();
            return false;
        }
    }
    D2D1_SIZE_U size = m_spD2DHwndRender->GetPixelSize();
    if ((size.width != (UINT32)szRender.cx) || (size.height != (UINT32)szRender.cy)) {
        hr = m_spD2DHwndRender->Resize(D2D1::SizeU(szRender.cx, szRender.cy));
        assert(SUCCEEDED(hr));
        m_spD2DOffScreenRender.Release();
    }
    if (FAILED(hr)) {
        ClearRenderRes();
        return false;
    }
    if (isOffScreenMode) {
        if (!m_spD2DOffScreenRender) {
            auto pixelFormt = m_spD2DHwndRender->GetPixelFormat();
            hr = m_spD2DHwndRender->CreateCompatibleRenderTarget(
                nullptr,
                nullptr,
                &pixelFormt,
                D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS::
                    D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_GDI_COMPATIBLE,
                &m_spD2DOffScreenRender);
            assert(SUCCEEDED(hr));
            if (FAILED(hr)) {
                ClearRenderRes();
                return false;
            }
        }
        m_spD2DRender = m_spD2DOffScreenRender;
    } else {
        m_spD2DRender = m_spD2DHwndRender;
    }

    if (!m_spSolidBrush) {
        hr = m_spD2DRender->CreateSolidColorBrush(D2D1::ColorF(0), &m_spSolidBrush);
    }
    if (SUCCEEDED(hr)) {
        return true;
    }

    ClearRenderRes();
    return false;
}

void D2DRenderPack::ClearRenderRes()
{
    m_layerPool.clear();
    m_layerPool.push_back(ATL::CComPtr<ID2D1Layer>());
    m_spD2DOffScreenRender.Release();
    m_spD2DHwndRender.Release();
    assert(m_clipStack.empty());
    //ClearUserRes();
    ReleaseAllD2DResourceCache();

    m_spSolidBrush.Release();
    m_spD2DRender.Release();
}

void D2DRenderPack::PushClipRect(const RECT &rcClip, bool bAntiAliasing /*= false*/)
{
    m_spD2DRender->PushAxisAlignedClip(CD2DRectF(rcClip),
                                       bAntiAliasing
                                           ? D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
                                           : D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_ALIASED);

    SaveCurrentClip(rcClip);
}

void D2DRenderPack::PopClipRect()
{
    m_spD2DRender->PopAxisAlignedClip();

    RestoreCurrentClip();
}

void D2DRenderPack::PushD2DLayer(const D2D1_LAYER_PARAMETERS &layerParam)
{
    HRESULT hr = S_OK;
    //m_layerPool的前面存放正在使用的 ID2DLayer, 后面存放空闲的 ID2DLayer, 中间以一个空指针作为哨兵
    if (m_layerPool.back().m_T) {
        m_spD2DRender->PushLayer(layerParam, m_layerPool.back().m_T);
        m_layerPool.splice(m_layerPool.begin(), m_layerPool, --m_layerPool.end());
    } else {
        ATL::CComPtr<ID2D1Layer> spLayer;
        hr = m_spD2DRender->CreateLayer(&spLayer);
        assert(SUCCEEDED(hr));
        if (SUCCEEDED(hr)) {
            m_spD2DRender->PushLayer(layerParam, spLayer);
            m_layerPool.push_front(spLayer);
        }
    }

    CD2DRectF clip;
    if (layerParam.geometricMask) {
        hr = layerParam.geometricMask->GetBounds(layerParam.maskTransform, &clip);
        assert(SUCCEEDED(hr));
        clip.IntersectRect(clip, layerParam.contentBounds);
    } else {
        clip = layerParam.contentBounds;
    }
    SaveCurrentClip(clip);
}

void D2DRenderPack::PopD2DLayer()
{
    assert(m_layerPool.front().m_T);
    if (m_layerPool.front().m_T) {
        m_spD2DRender->PopLayer();
        m_layerPool.splice(m_layerPool.end(), m_layerPool, m_layerPool.begin());
    }

    RestoreCurrentClip();
}

bool D2DRenderPack::IsClipEmpty()
{
    if (m_clipStack.empty()) {
        return false;
    }
    return m_clipStack.top().IsRectEmpty();
}

bool D2DRenderPack::IsRectInClip(const RECT &rcTest)
{
    if (m_clipStack.empty()) {
        return true;
    }
    CD2DRectF rcTrans = rcTest;
    CalculateTransRect(rcTrans);
    rcTrans.IntersectRect(rcTrans, m_clipStack.top());
    return !rcTrans.IsRectEmpty();
}

void D2DRenderPack::PushDrawingState()
{
    //m_statePool的前面存放正在使用的 ID2D1DrawingStateBlock, 后面存放空闲的 ID2D1DrawingStateBlock,
    // 中间以一个空指针作为哨兵
    if (m_statePool.back().m_T) {
        m_spD2DRender->SaveDrawingState(m_statePool.back().m_T);
        m_statePool.splice(m_statePool.begin(), m_statePool, --m_statePool.end());
    } else {
        ATL::CComPtr<ID2D1DrawingStateBlock> spDrawingState;
        HRESULT hr = m_spD2DFactory->CreateDrawingStateBlock(&spDrawingState);
        assert(SUCCEEDED(hr));
        if (SUCCEEDED(hr)) {
            m_spD2DRender->SaveDrawingState(spDrawingState);
            m_statePool.push_front(spDrawingState);
        }
    }
}

void D2DRenderPack::PopDrawingState()
{
    assert(m_statePool.front().m_T);
    if (m_statePool.front().m_T) {
        m_spD2DRender->RestoreDrawingState(m_statePool.front().m_T);
        m_statePool.splice(m_statePool.end(), m_statePool, m_statePool.begin());
    }
}

ATL::CComPtr<ID2D1Bitmap> D2DRenderPack::CreateD2DBitmap(const SIZE &sz, const void *pBitmapData)
{
    ATL::CComPtr<ID2D1Bitmap> spBitmap;
    HRESULT hr = m_spD2DRender->CreateBitmap(
        D2D1::SizeU(sz.cx, sz.cy),
        pBitmapData,
        sz.cx * 4,
        D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM,
                                                 D2D1_ALPHA_MODE::D2D1_ALPHA_MODE_PREMULTIPLIED)),
        &spBitmap);
    assert(SUCCEEDED(hr));
    if (!spBitmap || FAILED(hr)) {
        return nullptr;
    }
    return spBitmap;
}

D2DRenderPack::CachedResID D2DRenderPack::CacheD2DResource(ID2D1Resource *pResource)
{
    if (!pResource) {
        return CachedResID{};
    }
    m_userResCache[++m_nUserResID] = ATL::CComPtr<ID2D1Resource>(pResource);
    CachedResID id;
    id.m_cacheID = m_nUserResID;
    id.m_pRenderPack = this;
    return std::move(id);
}

ATL::CComPtr<ID2D1Resource> D2DRenderPack::GetCachedD2DResource(const CachedResID &resID)
{
    if ((resID.m_cacheID == 0) || (resID.m_pRenderPack != this) || m_userResCache.empty()) {
        return nullptr;
    } else {
        auto it = m_userResCache.find(resID.m_cacheID);
        if (it != m_userResCache.end()) {
            return it->second;
        } else {
            return nullptr;
        }
    }
}

void D2DRenderPack::ReleaseD2DResourceCache(const CachedResID &resID)
{
    if ((resID.m_cacheID == 0) || (resID.m_pRenderPack != this) || m_userResCache.empty()) {
        return;
    }
    m_userResCache.erase(resID.m_cacheID);
}

void D2DRenderPack::ReleaseAllD2DResourceCache()
{
    m_userResCache.clear();
    //m_nUserResID不能清0，设备改变会调用此函数，但外部缓存不知道，m_nUserResID只增不减
}

void D2DRenderPack::CalculateArcPoint(const RECT &ellipseBound,
                                      float angleStart,
                                      float angleSweep,
                                      D2D1_POINT_2F &ptStart,
                                      D2D1_POINT_2F &ptEnd,
                                      D2D1_SIZE_F &szRadius)
{
    double a = (double)(ellipseBound.right - ellipseBound.left) / 2.0;
    double b = (double)(ellipseBound.bottom - ellipseBound.top) / 2.0;
    szRadius.width = (FLOAT)a;
    szRadius.height = (FLOAT)b;
    double centerX = ellipseBound.left + a;
    double centerY = ellipseBound.top + b;

    ptStart.x = float(a * std::cos(TO_RADIAN(angleStart)) + centerX);
    ptStart.y = float(b * std::sin(TO_RADIAN(angleStart)) + centerY);
    ptEnd.x = float(a * std::cos(TO_RADIAN(angleStart + angleSweep)) + centerX);
    ptEnd.y = float(b * std::sin(TO_RADIAN(angleStart + angleSweep)) + centerY);
}

void D2DRenderPack::CalculateCosBezire(float fPiLength, D2D1_POINT_2F *pBezierPts)
{
    fPiLength = (fPiLength > 0 ? fPiLength : -fPiLength);
    float fUnitOne = (float)(fPiLength / M_PI);
    pBezierPts[0].x = 0;
    pBezierPts[0].y = -fUnitOne;
    pBezierPts[1].x = fPiLength - fUnitOne * 2;
    pBezierPts[1].y = -fUnitOne;
    pBezierPts[2].x = fUnitOne * 2;
    pBezierPts[2].y = fUnitOne;
    pBezierPts[3].x = fPiLength;
    pBezierPts[3].y = fUnitOne;
}

void D2DRenderPack::CalculateTransRect(CD2DRectF &rect)
{
    D2D1::Matrix3x2F mx;
    m_spD2DRender->GetTransform(&mx);
    CD2DPointF lt = mx.TransformPoint(CD2DPointF(rect.left, rect.top));
    CD2DPointF rt = mx.TransformPoint(CD2DPointF(rect.right, rect.top));
    CD2DPointF rb = mx.TransformPoint(CD2DPointF(rect.right, rect.bottom));
    CD2DPointF lb = mx.TransformPoint(CD2DPointF(rect.left, rect.bottom));
    rect.left = (std::min)((std::min)((std::min)(lt.x, rt.x), rb.x), lb.x);
    rect.right = (std::max)((std::max)((std::max)(lt.x, rt.x), rb.x), lb.x);
    rect.top = (std::min)((std::min)((std::min)(lt.y, rt.y), rb.y), lb.y);
    rect.bottom = (std::max)((std::max)((std::max)(lt.y, rt.y), rb.y), lb.y);
}

void D2DRenderPack::SaveCurrentClip(const CD2DRectF &rcClip)
{
    CD2DRectF rcTransClip = rcClip;
    CalculateTransRect(rcTransClip);
    if (!m_clipStack.empty()) {
        rcTransClip.IntersectRect(rcTransClip, m_clipStack.top());
    }
    m_clipStack.push(rcTransClip);
}

void D2DRenderPack::RestoreCurrentClip()
{
    assert(!m_clipStack.empty());
    m_clipStack.pop();
}

void D2DRenderPack::ClearUserRes()
{
    for (auto it = m_userResCache.begin(); it != m_userResCache.end();) {
        if (!it->second.m_T) {
            it = m_userResCache.erase(it);
            continue;
        }

        ATL::CComPtr<ID2D1Resource> spTemp;
        HRESULT hr = S_OK;

        //设备无关资源
        hr = it->second->QueryInterface(__uuidof(ID2D1DrawingStateBlock), (void **)&spTemp);
        if (spTemp && SUCCEEDED(hr)) {
            ++it;
            continue;
        }

        hr = it->second->QueryInterface(__uuidof(ID2D1Geometry), (void **)&spTemp);
        if (spTemp && SUCCEEDED(hr)) {
            ++it;
            continue;
        }

        hr = it->second->QueryInterface(__uuidof(ID2D1StrokeStyle), (void **)&spTemp);
        if (spTemp && SUCCEEDED(hr)) {
            ++it;
            continue;
        }

        hr = it->second->QueryInterface(__uuidof(ID2D1SimplifiedGeometrySink), (void **)&spTemp);
        if (spTemp && SUCCEEDED(hr)) {
            //ID2D1GeometrySink 也包含在内
            ++it;
            continue;
        }

        //设备相关资源
        //it->second->QueryInterface(__uuidof(ID2D1Bitmap), (void**)&spTemp);
        //if (spTemp && SUCCEEDED(hr))
        //{
        //    it = m_userResCache.erase(it);
        //    continue;
        //}

        //hr = it->second->QueryInterface(__uuidof(ID2D1Layer), (void**)&spTemp);
        //if (spTemp && SUCCEEDED(hr))
        //{
        //    it = m_userResCache.erase(it);
        //    continue;
        //}

        //hr = it->second->QueryInterface(__uuidof(ID2D1Brush), (void**)&spTemp);
        //if (spTemp && SUCCEEDED(hr))
        //{
        //    it = m_userResCache.erase(it);
        //    continue;
        //}

        //hr = it->second->QueryInterface(__uuidof(ID2D1RenderTarget), (void**)&spTemp);
        //if (spTemp && SUCCEEDED(hr))
        //{
        //    it = m_userResCache.erase(it);
        //    continue;
        //}

        //未知资源，当设备相关处理
        it = m_userResCache.erase(it);
    }
}

#pragma warning(pop)
SHARELIB_END_NAMESPACE

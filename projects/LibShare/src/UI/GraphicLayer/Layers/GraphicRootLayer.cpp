#include "targetver.h"
#include "UI/GraphicLayer/Layers/GraphicRootLayer.h"
#include <cmath>
#include <atlfile.h>
#include "DataStructure/traverse_tree_node.h"
#include "UI/GraphicLayer/ConfigEngine/LayerLuaAdaptor.h"
#include "UI/GraphicLayer/ConfigEngine/XmlAttributeUtility.h"
#include "pugixml/pugixml.hpp"

SHARELIB_BEGIN_NAMESPACE

class GraphicRootLayer::MouseDragMoveHandler : public LayerMsgCallback
{
    virtual bool OnLayerMsgMouse(GraphicLayer *pLayer,
                                 UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam,
                                 LRESULT & /*lResult*/)
    {
        if ((WM_LBUTTONDOWN == uMsg) || (WM_LBUTTONDBLCLK == uMsg)) {
            pLayer->GetMSWindow()->SetCapture();
            pLayer->SetMouseCapture(true);
            m_ptOffset = CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            m_bPressed = true;
        } else if (WM_LBUTTONUP == uMsg) {
            ::ReleaseCapture();
            pLayer->SetMouseCapture(false);
            m_bPressed = false;
        } else if ((WM_MOUSEMOVE == uMsg) && m_bPressed && (wParam == MK_LBUTTON)) {
            CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            pLayer->OffsetOrigin(pt - m_ptOffset);
        } else {
            return false;
        }
        return true;
    }

    bool m_bPressed = false;
    CPoint m_ptOffset;
};

class GraphicRootLayer::DragMoveWindowHandler : public LayerMsgCallback
{
    virtual bool OnLayerMsgMouse(GraphicLayer *pLayer,
                                 UINT uMsg,
                                 WPARAM /*wParam*/,
                                 LPARAM /*lParam*/,
                                 LRESULT & /*lResult*/)
    {
        if (WM_LBUTTONDOWN == uMsg) {
            auto pWnd = pLayer->GetMSWindow();
            if (pWnd) {
                pWnd->SendMessage(WM_SYSCOMMAND, SC_MOVE | HTCAPTION);
                return true;
            }
        }
        return false;
    }
};

extern void InitLayersDynimicCreateInfo();

GraphicRootLayer::GraphicRootLayer(ATL::CWindow *pWnd)
    : m_pWindow(pWnd)
    , m_renderType(D2DRenderPack::Auto)
    , m_pPaintStart(nullptr)
    , m_pLastMouseHandler(nullptr)
    , m_bMouseCapture(false)
    , m_pFocusLayer(nullptr)
    , m_pFocusRetore(nullptr)
    , m_nVerticalPixel(0)
    , m_nHorizontalPixel(0)
    , m_hTimerQueue(nullptr)
{
    m_bitConfig[TBitConfig::BIT_ROOT_LAYER] = true;
    m_dwrite.CreateDWriteFactory();
    InitLayersDynimicCreateInfo();
}

GraphicRootLayer::~GraphicRootLayer()
{
    //先删除子类,确保子类正确地从RootLayer中反注册
    destroy_children(this);
    m_d2dRenderPack.ClearRenderRes();
    m_bitConfig[TBitConfig::BIT_ROOT_LAYER] = false;
    if (m_hTimerQueue) {
        ::DeleteTimerQueueEx(m_hTimerQueue, INVALID_HANDLE_VALUE);
        m_hTimerQueue = nullptr;
    }
    assert(m_timerData.empty());
}

void GraphicRootLayer::SetSkinMap(
    std::shared_ptr<std::unordered_map<std::wstring, TBitmapInfo>> spSkinMap)
{
    m_spSkinMap = spSkinMap;
}

TBitmapInfo *GraphicRootLayer::FindSkinByName(const std::wstring &key)
{
    if (m_spSkinMap && !m_spSkinMap->empty()) {
        auto it = m_spSkinMap->find(key);
        if (it != m_spSkinMap->end()) {
            return &(it->second);
        }
    }
    return nullptr;
}

bool GraphicRootLayer::LoadLuaFromXml(const wchar_t *pXmlFilePath)
{
    if (!pXmlFilePath) {
        return false;
    }
    ATL::CAtlFile file;
    if (FAILED(file.Create(pXmlFilePath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING))) {
        assert(!"打开文件失败");
        return false;
    }
    ATL::CAtlFileMapping<char> fileMapping;
    if (FAILED(fileMapping.MapFile(file))) {
        return false;
    }
    return LoadLuaFromXml(fileMapping, fileMapping.GetMappingSize());
}

bool GraphicRootLayer::LoadLuaFromXml(const void *pData, size_t nLength)
{
    assert(pData && nLength);
    if (!pData || (nLength == 0)) {
        return false;
    }
    pugi::xml_document xmlDoc;
    if (!xmlDoc.load_buffer(pData, nLength)) {
        assert(!"解析xml失败");
        return false;
    }
    traverse_tree_node_t2b(xmlDoc.first_child(), [this](pugi::xml_node &node, int nDepth) -> int {
        if (nDepth == 0) {
            return 1;
        }
        if (node.child_value()) {
            lua_state_wrapper lua;
            if (lua.create()) {
                if (lua.load_lua_string(node.child_value())) {
                    RegisterGraphicLayerToLua(lua);
                    m_luaPool[node.name()] = std::move(lua);
                } else {
                    assert(!"解析lua脚本失败");
                }
            } else {
                assert(!"创建lua失败");
            }
        }
        return 0;
    });
    return true;
}

lua_state_wrapper *GraphicRootLayer::FindLuaByName(const std::wstring &key)
{
    auto it = m_luaPool.find(key);
    if (it != m_luaPool.end()) {
        return &(it->second);
    }
    return nullptr;
}

void GraphicRootLayer::ClearLua()
{
    m_luaPool.clear();
}

ATL::CWindow *GraphicRootLayer::GetMSWindow() const
{
    return m_pWindow;
}

bool GraphicRootLayer::IsLayeredWindow()
{
    assert(m_pWindow && m_pWindow->IsWindow());
    if (m_pWindow->GetWindowLongPtr(GWL_EXSTYLE) & WS_EX_LAYERED) {
        return true;
    } else {
        return false;
    }
}

D2DRenderPack &GraphicRootLayer::D2DRender()
{
    return m_d2dRenderPack;
}

DWriteUtility &GraphicRootLayer::DWritePack()
{
    return m_dwrite;
}

void GraphicRootLayer::SetD2DRenderType(D2DRenderPack::TRenderType type)
{
    m_renderType = type;
}

GraphicLayer *GraphicRootLayer::GetFocusLayer()
{
    return m_pFocusLayer;
}

void GraphicRootLayer::_OnChildRemoved(GraphicLayer *pChild)
{
    if (pChild == m_pLastMouseHandler) {
        m_pLastMouseHandler = nullptr;
    }
    if (pChild == m_pFocusLayer) {
        m_pFocusLayer = nullptr;
    }
    if (pChild == m_pFocusRetore) {
        m_pFocusRetore = nullptr;
    }
    if (pChild == m_pPaintStart) {
        m_pPaintStart = nullptr;
    }
    _FreeTimer(pChild, nullptr);
}

HANDLE GraphicRootLayer::_AllocTimer(GraphicLayer *pLayer, uint32_t nPeriod)
{
    if (pLayer && nPeriod > 0) {
        if (!m_timerDataHeap.m_hHeap) {
            ATL::CWin32Heap heap(0, 0);
            m_timerDataHeap.Attach(heap.Detach(), true);
            assert(m_timerDataHeap.m_hHeap);
        }
        LayerTimerData *pTimerData = (LayerTimerData *)m_timerDataHeap.Allocate(
            sizeof(LayerTimerData));
        if (pTimerData) {
            pTimerData->m_pLayer = pLayer;
            pTimerData->m_pWindow = m_pWindow;
            pTimerData->m_hTimer = nullptr;

            if (!m_hTimerQueue) {
                m_hTimerQueue = ::CreateTimerQueue();
                assert(m_hTimerQueue);
            }
            if (::CreateTimerQueueTimer(&pTimerData->m_hTimer,
                                        m_hTimerQueue,
                                        TimerCallbackFunction,
                                        pTimerData,
                                        nPeriod,
                                        nPeriod,
                                        WT_EXECUTEDEFAULT)) {
                m_timerData.insert(std::make_pair(pLayer, pTimerData));
                return pTimerData->m_hTimer;
            } else {
                m_timerDataHeap.Free(pTimerData);
            }
        }
    }
    return nullptr;
}

void GraphicRootLayer::_FreeTimer(GraphicLayer *pLayer, HANDLE hTimer)
{
    if (pLayer && m_hTimerQueue) {
        auto itRange = m_timerData.equal_range(pLayer);
        for (auto it = itRange.first; it != itRange.second; ++it) {
            if (hTimer) {
                if (hTimer == it->second->m_hTimer) {
                    //使用INVALID_HANDLE_VALUE，等定时器结束，否则定时器回调可能访问内存违规
                    ::DeleteTimerQueueTimer(m_hTimerQueue, hTimer, INVALID_HANDLE_VALUE);
                    m_timerDataHeap.Free(it->second);
                    m_timerData.erase(it);
                    return;
                }
            } else {
                //使用INVALID_HANDLE_VALUE，等定时器结束，否则定时器回调可能访问内存违规
                ::DeleteTimerQueueTimer(m_hTimerQueue, it->second->m_hTimer, INVALID_HANDLE_VALUE);
                m_timerDataHeap.Free(it->second);
            }
        }
        if (hTimer) {
            assert(!"参数错误");
        } else {
            m_timerData.erase(itRange.first, itRange.second);
        }
    }
}

void GraphicRootLayer::_RegisterCapturedLayer(GraphicLayer *pLayer, bool bCapture)
{
    if (bCapture) {
        m_bMouseCapture = bCapture;
        m_pLastMouseHandler = pLayer;
    } else {
        if (m_pLastMouseHandler == pLayer) {
            m_bMouseCapture = bCapture;
        }
    }
}

void GraphicRootLayer::_RegisterMouseDragMove(GraphicLayer *pLayer, bool bEnableDragMove)
{
    if (bEnableDragMove) {
        if (!m_spMouseDragMove) {
            m_spMouseDragMove.reset(new MouseDragMoveHandler);
        }
        pLayer->AddLayerMsgObserver(m_spMouseDragMove.get());
    } else {
        if (m_spMouseDragMove) {
            pLayer->RemoveLayerMsgObserver(m_spMouseDragMove.get());
        }
    }
}

void GraphicRootLayer::_RegisterDragMoveWindow(GraphicLayer *pLayer, bool bEnableMoveWindow)
{
    if (bEnableMoveWindow) {
        if (!m_spDragMoveWindow) {
            m_spDragMoveWindow.reset(new DragMoveWindowHandler);
        }
        pLayer->AddLayerMsgHook(m_spDragMoveWindow.get());
    } else {
        if (m_spDragMoveWindow) {
            pLayer->RemoveLayerMsgHook(m_spDragMoveWindow.get());
        }
    }
}

void GraphicRootLayer::_RegisterFocusLayer(GraphicLayer *pLayer, bool bFocus)
{
    if (bFocus) {
        if (pLayer && pLayer != m_pFocusLayer) {
            GraphicLayer *p = m_pFocusLayer;
            m_pFocusLayer = pLayer;
            //先赋值,再回调,这样回调响应中获取当前的焦点Layer也是正确的
            if (p) {
                p->SendLayerMsg(&LayerMsgCallback::OnLayerMsgKillFocus, pLayer);
            }
            pLayer->SendLayerMsg(&LayerMsgCallback::OnLayerMsgSetFocus, m_pFocusLayer);
        }
    } else {
        if (!pLayer || pLayer == m_pFocusLayer) {
            GraphicLayer *p = m_pFocusLayer;
            m_pFocusLayer = nullptr;
            if (p) {
                p->SendLayerMsg(&LayerMsgCallback::OnLayerMsgKillFocus, nullptr);
            }
        }
    }
}

void GraphicRootLayer::_RegisterLayeredWndClipRect(const CRect &rcClip)
{
    if (IsLayeredWindow()) {
        m_rcLayeredWndClip |= rcClip;
    }
}

void GraphicRootLayer::_RegisterPaintStart(GraphicLayer *pPaintStart)
{
    if (!pPaintStart || (pPaintStart == this)) {
        m_pPaintStart = nullptr;
        return;
    }

    if (!m_pPaintStart) {
        m_pPaintStart = pPaintStart;
    } else if (m_pPaintStart != pPaintStart) {
        //首先把两者的父子关系路径分别压栈
        std::vector<GraphicLayer *> layerPath1, layerPath2;
        for (GraphicLayer *pCur = pPaintStart; pCur && !pCur->is_root(); pCur = pCur->parent()) {
            layerPath1.push_back(pCur);
        }
        for (GraphicLayer *pCur = m_pPaintStart; pCur && !pCur->is_root(); pCur = pCur->parent()) {
            layerPath2.push_back(pCur);
        }
        assert(!layerPath1.empty() && !layerPath2.empty());

        //二者依次出栈，进行对比，找出二者共同的路径，成为新的绘制起点，
        //如果没有共同路径，m_pPaintStart置空，表示从根结点绘制
        GraphicLayer *pStartToFind = nullptr;
        for (auto it1 = layerPath1.rbegin(), it2 = layerPath2.rbegin();
             (it1 != layerPath1.rend()) && (it2 != layerPath2.rend());
             ++it1, ++it2) {
            if (*it1 != *it2) {
                break;
            }
            if (MayItBePaintStart(*it1)) {
                pStartToFind = *it1;
            }
        }

        m_pPaintStart = pStartToFind;
    }
}

void GraphicRootLayer::_ClearPaintStart()
{
    m_pPaintStart = nullptr;
}

int32_t GraphicRootLayer::_GetSystemWheelSetting(bool bVertical)
{
    int32_t nParam = bVertical ? m_nVerticalPixel : m_nHorizontalPixel;
    if (nParam == 0) {
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
        UINT nAction = (bVertical ? SPI_GETWHEELSCROLLLINES : SPI_GETWHEELSCROLLCHARS);
#else
        UINT nAction = SPI_GETWHEELSCROLLLINES;
#endif
        ::SystemParametersInfo(nAction, 0, &nParam, 0);
        nParam = (nParam == 0 ? 3 : nParam);

        LOGFONT logFont{0};
        UINT cbSize = sizeof(logFont);
        ::SystemParametersInfo(SPI_GETICONTITLELOGFONT, cbSize, &logFont, 0);
        if (logFont.lfHeight != 0) {
            nParam *= std::abs(logFont.lfHeight);
        } else {
            nParam *= 12;
        }

        if (bVertical) {
            m_nVerticalPixel = nParam;
        } else {
            m_nHorizontalPixel = nParam;
        }
    }
    return nParam;
}

void GraphicRootLayer::OnReadingXmlAttribute(const pugi::xml_attribute &attr)
{
    if (std::wcscmp(L"lrColor", attr.name()) == 0) {
        XmlAttributeUtility::ReadXmlValueColor(attr.as_string(), m_crbg);
    }
}

void GraphicRootLayer::OnWritingAttributeToXml(pugi::xml_node node)
{
    auto attr = node.append_attribute(L"lrColor");
    XmlAttributeUtility::WriteValueColorToXml(attr, m_crbg);
}

LRESULT GraphicRootLayer::OnMouse(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
#ifdef _DEBUG
    if (uMsg != WM_MOUSEMOVE) {
        const char *p = "debug break point";
        (void)(p);
    }
#endif // _DEBUG

    CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    if ((uMsg == WM_MOUSEWHEEL) || (uMsg == WM_MOUSEHWHEEL)) {
        m_pWindow->ScreenToClient(&pt);
    }

    GraphicLayer *pHandler = nullptr;
    if (m_bMouseCapture && m_pLastMouseHandler) {
        //capture状态时
        CPoint ptOffset;
        bool bOk = true;
        for (GraphicLayer *pCur = m_pLastMouseHandler; pCur && !pCur->is_root();
             pCur = pCur->parent()) {
            if (pCur->IsEnable() && pCur->IsVisible()) {
                ptOffset += pCur->GetOrigin();
            } else {
                bOk = false;
                break;
            }
        }
        if (bOk) {
            pHandler = m_pLastMouseHandler;
            pt -= ptOffset;
        }
    }
    if (!pHandler) {
        //非capture,或capture失败时的查找
        m_bMouseCapture = false;
        pHandler = FindChildLayerFromPoint(this, pt);
    }

    //查找结束,进行消息处理
    NotifyMouseInOut(pHandler);
    NotifyFocus(pHandler, uMsg);
    if (pHandler) {
        LRESULT lRes = 0;
        for (bHandled = FALSE; !bHandled && pHandler && !pHandler->is_root();
             pHandler = pHandler->parent()) {
            bHandled = pHandler->SendLayerMsg(
                &LayerMsgCallback::OnLayerMsgMouse, uMsg, wParam, MAKELPARAM(pt.x, pt.y), lRes);
            //滚轮消息，如果不处理则一直向上查找
            if (uMsg != WM_MOUSEWHEEL && uMsg != WM_MOUSEHWHEEL) {
                break;
            }
        }
        return lRes;
    } else {
        bHandled = FALSE;
        return 0;
    }
}

LRESULT GraphicRootLayer::OnKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (m_pFocusLayer) {
        LRESULT lRes = 0;
        bHandled = m_pFocusLayer->SendLayerMsg(
            &LayerMsgCallback::OnLayerMsgKey, uMsg, wParam, lParam, lRes);
    } else {
        bHandled = FALSE;
    }
    return 0;
}

void GraphicRootLayer::OnSetFocus(ATL::CWindow /*wndOld*/)
{
    if (m_pFocusRetore) {
        _RegisterFocusLayer(m_pFocusRetore, true);
        m_pFocusRetore = nullptr;
    }
    SetMsgHandled(FALSE);
}

void GraphicRootLayer::OnKillFocus(ATL::CWindow /*wndFocus*/)
{
    m_pFocusRetore = m_pFocusLayer;
    _RegisterFocusLayer(nullptr, false);
    SetMsgHandled(FALSE);
}

void GraphicRootLayer::OnMouseLeave()
{
    NotifyMouseInOut(nullptr);
    SetMsgHandled(FALSE);
}

void GraphicRootLayer::OnSize(UINT nType, CSize /*size*/)
{
    SetMsgHandled(FALSE);
    if (nType != SIZE_MINIMIZED) {
        CRect rcWindow = BorderlessWindowHandler::GetBorderlessDrawRect(*m_pWindow);
        rcWindow.MoveToXY(0, 0);
        if (rcWindow != GetLayerBounds()) {
            SetOrigin(CPoint{}, false);
            SetLayerBounds(rcWindow, true);
            _ClearPaintStart();
            Relayout();
        }
        if (IsLayeredWindow()) {
            ::InvalidateRect(*m_pWindow, nullptr, TRUE);
        }
    }
}

void GraphicRootLayer::OnPaint(WTL::CDCHandle /*dc*/)
{
    bool bLayered = IsLayeredWindow();

    //窗口大小, 也是画布大小
    CRect rcClient = BorderlessWindowHandler::GetBorderlessDrawRect(*m_pWindow);
    rcClient.MoveToXY(0, 0);

    //绘制大小, 也是裁剪大小
    CRect rcDraw;
    PAINTSTRUCT ps{0};
    if (bLayered) {
        if (m_rcLayeredWndClip.IsRectEmpty()) {
            rcDraw = rcClient;
        } else {
            rcDraw = m_rcLayeredWndClip;
            m_rcLayeredWndClip.SetRectEmpty();
        }
    } else {
        ::BeginPaint(*m_pWindow, &ps);
        rcDraw = ps.rcPaint;
    }

    DrawAllLayers(rcClient.Size(), rcDraw, bLayered);

    //善后处理
    if (bLayered) {
        ::DefWindowProc(*m_pWindow, WM_PAINT, 0, 0);
    } else {
        ::EndPaint(*m_pWindow, &ps);
    }
}

void GraphicRootLayer::DrawAllLayers(const CSize &szRender, const CRect &rcDraw, bool bLayeredWnd)
{
    if (!rcDraw.IsRectEmpty() &&
        m_d2dRenderPack.PrepareRender(*m_pWindow, szRender, m_renderType, 1.0f)) {
        m_d2dRenderPack.m_spD2DRender->BeginDraw();
        m_d2dRenderPack.m_spD2DRender->SetTransform(D2D1::Matrix3x2F::Identity());

        {
            D2DRenderPack::DrawingStateGuard drawingState(m_d2dRenderPack);
            D2DRenderPack::RectClipGuard drawClip(m_d2dRenderPack, rcDraw);
            m_d2dRenderPack.m_spD2DRender->Clear(
                D2D1::ColorF(GetBgColor(), COLOR32_GetA(GetBgColor()) / 255.f));
            m_fastPath.clear();
            if (m_pPaintStart && (m_pPaintStart != first_child())) {
                /* 绘制起点功能:
[问题起源]一个不大面积的动画,测试下来CPU,GPU占用很高
[问题分析]把动画的绘制代码全屏蔽了,结果CPU,GPU占用仍然很高,说明不是动画本身的问题.结合代码及测试验证，发现
        原因在于: 绘制是从根结点起,递归遍历所有的结点, 凡是与无效区域有交集的都会重绘, 这样一来, 动画结点
        的所有父结点都会重绘, 兄弟结点中如果有交集也会重绘. 动画结点有一级父结点, 绘制次数就至少翻1倍, 
        动画结点的嵌套深度越大, 这个倍数就越大. 而真正用来绘制动画次数却只有一次. 而且父结点的面积往往都是
        大于子结点的，更加重了CPU、GPU的占用。
[解决方案]1.当一个结点有不透明的背景时，它及它的子结点不管怎么重绘，都不会污染它的父亲及前面的兄弟结点的，
          所以完全可以把不透明背景的结点当作起点重绘，之前的绘制完全是无用的。因此在SchedulePaint中，
          从本身向根结点的遍历过程中，遇到不透明背景的结点(MayItBePaintStart)就记录下来，当作绘制的起点。
        2.绘制流程中，从父结点转到子结点时，会调用 BeforePaintChildren，这里面通常会进行如矩阵变换、透明度
          变换、裁剪等操作，会影响所有子结点的绘制，这时，下面的子结点就都不能当作绘制起点，否则与正常的步骤
          就不一样了。除这个函数，其它地方的操作都只影响自身，不影响子结点，所以需要特殊处理的只有它。问题就
          转化为判断一个对象是否改写了该虚函数，这可以从对象指针中读取虚表，拿它和Graphicayer::BeforePaintChildren
          的函数地址进行比较，如果不相同，说明改写了，其所有子结点就都不能作为绘制起点。获取虚函数地址的方法
          体现在 IsRewriteBeforePaintChildren 里面。
        3.Windows会把多个Paint消息合并，这样一来，如果在没有真正绘制之前，有多次调用SchedulePaint，就必须
          把这些绘制起点合并。需要找到这些绘制起点共同的父亲、并且背景不透明的结点作为新的绘制起点。具体做
          法在 _RegisterPaintStart 中。
        4.绘制时，如果发现有记录的绘制起点，便不再从根结点开始绘制，而是走快速通道，定位到绘制起点处开始绘制。
        5.绘制结束，把绘制起点清空，否则多次绘制之后，绘制起点逐步向根结点移动，起不到效果。
        6.窗口大小改变时,所有Layer的位置都可能改变,清空绘制起点.
        7.当绘制区域大于绘制起点的区域时,清空绘制起点.
*/
                CRect rcStart = m_pPaintStart->GetLayerBounds();
                MapPointToRoot(m_pPaintStart, (CPoint *)&rcStart, 2);
                if ((rcStart & rcDraw) == rcDraw) {
                    for (GraphicLayer *pLayer = m_pPaintStart; pLayer && !pLayer->is_root();
                         pLayer = pLayer->parent()) {
                        m_fastPath.push_back(pLayer);
                    }
                    assert(m_fastPath.back()->parent() == this);
                }
            }
            RecursiveDraw(m_d2dRenderPack, first_child(), m_fastPath);
            m_pPaintStart = nullptr; //绘制之后起点清空
            m_fastPath.clear();
        }

        HRESULT hr = S_OK;
        D2D1_TAG tag1 = 0, tag2 = 0;
        if (bLayeredWnd) {
            hr = m_d2dRenderPack.m_spD2DRender->Flush(&tag1, &tag2);
            //层窗口，把绘制内容复制到DC上，再用UpdateLayeredWindow绘制到窗口中
            ATL::CComQIPtr<ID2D1GdiInteropRenderTarget> spGdiRender =
                m_d2dRenderPack.m_spD2DRender; //always succeeds
            assert(SUCCEEDED(hr));
            if (SUCCEEDED(hr)) {
                HDC hDc = NULL;
                hr = spGdiRender->GetDC(D2D1_DC_INITIALIZE_MODE::D2D1_DC_INITIALIZE_MODE_COPY,
                                        &hDc);
                assert(SUCCEEDED(hr));
                if (SUCCEEDED(hr)) {
                    CRect rcLayerWnd = BorderlessWindowHandler::GetBorderlessDrawRect(*m_pWindow);
                    CPoint ptScreen = rcLayerWnd.TopLeft();
                    CSize sz = rcLayerWnd.Size();
                    BLENDFUNCTION blend = {0};
                    blend.BlendOp = AC_SRC_OVER;
                    blend.SourceConstantAlpha = 255;
                    blend.AlphaFormat = AC_SRC_ALPHA;
                    POINT ptSrc{0};
                    ::UpdateLayeredWindow(
                        *m_pWindow, NULL, &ptScreen, &sz, hDc, &ptSrc, 0, &blend, ULW_ALPHA);
                    CRect temp;
                    spGdiRender->ReleaseDC(&temp);
                }
            }
        }

        hr = m_d2dRenderPack.m_spD2DRender->EndDraw(&tag1, &tag2);
        if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) {
            m_d2dRenderPack.ClearRenderRes();
        }
    }
}

void GraphicRootLayer::RecursiveDraw(D2DRenderPack &d2dRender,
                                     GraphicLayer *pDefaultPath,
                                     std::vector<GraphicLayer *> &fastPath)
{
    if (!fastPath.empty()) {
        assert(pDefaultPath->parent() == fastPath.back()->parent());
        pDefaultPath = fastPath.back();
        fastPath.pop_back();
    }
    for (GraphicLayer *pCur = pDefaultPath; pCur && pCur->IsVisible();
         pCur = pCur->next_sibling()) {
        D2DRenderPack::DrawingStateGuard matrixGuard(d2dRender);
        //坐标转换
        D2D1::Matrix3x2F d2dMxOld;
        d2dRender.m_spD2DRender->GetTransform(&d2dMxOld);
        D2D1::Matrix3x2F d2dMxCur = D2D1::Matrix3x2F::Translation((FLOAT)pCur->GetOrigin().x,
                                                                  (FLOAT)pCur->GetOrigin().y);
        d2dMxCur = d2dMxCur * d2dMxOld;
        d2dRender.m_spD2DRender->SetTransform(d2dMxCur);

        if (fastPath.empty() && !d2dRender.IsRectInClip(pCur->GetLayerBounds())) {
            continue;
        }
        //"快速绘制起点"功能, 必须裁剪
        D2DRenderPack::RectClipGuard clipGuard(d2dRender, pCur->GetLayerBounds());

        if (fastPath.empty()) {
            //绘制
            D2DRenderPack::DrawingStateGuard drawingState(d2dRender);
            COLOR32 crbg = pCur->GetBgColor();
            if (COLOR32_GetA(crbg)) {
                //背景
                d2dRender.m_spSolidBrush->SetColor(D2D1::ColorF(crbg, COLOR32_GetA(crbg) / 255.f));
                d2dRender.m_spD2DRender->FillRectangle(CD2DRectF(pCur->GetLayerBounds()),
                                                       d2dRender.m_spSolidBrush);
            }
            //个性化绘制
            pCur->Paint(d2dRender);
        }

        assert(fastPath.empty() || (pCur->child_count() > 0));
        //递归绘制子Layer
        if (pCur->child_count() > 0) {
            pCur->BeforePaintChildren(d2dRender);
            RecursiveDraw(d2dRender, pCur->first_child(), fastPath);
            pCur->AfterPaintChildren(d2dRender);
        }
    }
}

LRESULT GraphicRootLayer::OnLayerTimerMsg(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
{
    /* WPARAM必须传入Layer指针,否则当出现这种情况时,会导致野指针崩溃:
        1.定时器消息发送成功; 2.定时器消息还未来得及处理,子Layer销毁; 3.收到定时器消息. 
[解决办法] WPARAM传入Layer指针, 收到此消息时通过WPARAM的指针值到定时器数据池
        中查找该Layer是否存在.这样可以确保Layer指针的合法性. 出现上面的情境时:
        1.子Layer销毁时同步等定时器结束成功; 2.同步在RootLayer的定时器数据池中删除自己的数据;
        (由于是同步处理, 而定时器消息需要经过消息队列, 所以定时器消息一定还未处理);
        3.收到定时器消息,根据指针查找不到它的定时器数据,知道该指针无效,不再处理.
*/
    auto itRange = m_timerData.equal_range((GraphicLayer *)wParam);
    for (auto it = itRange.first; it != itRange.second; ++it) {
        if (it->second == (LayerTimerData *)lParam) {
            //检查timer的合法性：有可能定时器已经删除，但定时器已经成功PostMessage了。
            it->second->m_pLayer->SendLayerMsg(&LayerMsgCallback::OnLayerMsgTimer,
                                               it->second->m_hTimer);
            break;
        }
    }
    return 0;
}

LRESULT GraphicRootLayer::OnLayerInternalUse(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
{
    GraphicLayer *pLayer = (GraphicLayer *)wParam;
    if (pLayer) {
        pLayer->OnLayerInternalMsg(lParam);
    }
    return 0;
}

void GraphicRootLayer::OnNcDestroy()
{
    m_d2dRenderPack.ClearRenderRes();
    SetMsgHandled(FALSE);
}

void GraphicRootLayer::OnSettingChange(UINT uFlags, LPCTSTR /*lpszSection*/)
{
    SetMsgHandled(FALSE);
    switch (uFlags) {
    case SPI_SETWHEELSCROLLLINES:
        m_nVerticalPixel = 0;
        _GetSystemWheelSetting(true);
        break;
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    case SPI_SETWHEELSCROLLCHARS:
        m_nHorizontalPixel = 0;
        _GetSystemWheelSetting(false);
        break;
#endif
    default:
        break;
    }
}

void GraphicRootLayer::NotifyMouseInOut(GraphicLayer *pMouseIn)
{
    if (m_pLastMouseHandler != pMouseIn) {
        if (m_pLastMouseHandler && m_pLastMouseHandler->IsEnable() &&
            m_pLastMouseHandler->IsVisible()) {
            m_pLastMouseHandler->SendLayerMsg(&LayerMsgCallback::OnLayerMsgMouseInOut, false);
            GraphicLayer *pParent = m_pLastMouseHandler;
            while (pParent->IsMouseInOutLinkParent() && pParent->parent() &&
                   (pParent->parent() != pMouseIn) && !pParent->parent()->is_root()) {
                pParent = pParent->parent();
                pParent->SendLayerMsg(&LayerMsgCallback::OnLayerMsgMouseInOut, false);
            }
        }
        //查找 pMouseIn时已经把IsEnable，IsVisible考虑在内了，所以它及它的父Layer不用再判断
        if (pMouseIn) {
            pMouseIn->SendLayerMsg(&LayerMsgCallback::OnLayerMsgMouseInOut, true);
            GraphicLayer *pParent = pMouseIn;
            while (pParent->IsMouseInOutLinkParent() && pParent->parent() &&
                   !pParent->parent()->is_root()) {
                pParent = pParent->parent();
                pParent->SendLayerMsg(&LayerMsgCallback::OnLayerMsgMouseInOut, true);
            }
            TRACKMOUSEEVENT tme{sizeof(tme)};
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = GetMSWindow()->m_hWnd;
            ::TrackMouseEvent(&tme);
        }
        m_pLastMouseHandler = pMouseIn;
    }
}

void GraphicRootLayer::NotifyFocus(GraphicLayer *pMouse, UINT uMsg)
{
    if ((uMsg == WM_LBUTTONDOWN) || (uMsg == WM_RBUTTONDOWN) || (uMsg == WM_MBUTTONDOWN) ||
        (uMsg == WM_XBUTTONDOWN)) {
        if (pMouse && pMouse->IsEnable() && pMouse->IsVisible() && pMouse->IsEnableFocus()) {
            _RegisterFocusLayer(pMouse, true);
        }
    }
}

void __stdcall GraphicRootLayer::TimerCallbackFunction(void *lpParameter,
                                                       BOOLEAN /*TimerOrWaitFired*/)
{
    //pTimerData必须是正确可用的, 这一点是通过 当Layer销毁时同步等待定时器成功结束 来确保的.
    LayerTimerData *pTimerData = (LayerTimerData *)lpParameter;
    assert(pTimerData);
    if (pTimerData->m_pWindow && pTimerData->m_pWindow->IsWindow()) {
        pTimerData->m_pWindow->PostMessage(
            LAYER_WM_TIMER_NOTIFY, (WPARAM)pTimerData->m_pLayer, (LPARAM)pTimerData);
    }
}

SHARELIB_END_NAMESPACE

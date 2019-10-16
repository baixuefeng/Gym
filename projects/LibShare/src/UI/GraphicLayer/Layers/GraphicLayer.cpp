#include "targetver.h"
#include "UI/GraphicLayer/Layers/GraphicLayer.h"
#include <algorithm>
#include <cassert>
#include <mutex>
#include <atlfile.h>
#include "DataStructure/traverse_tree_node.h"
#include "UI/GraphicLayer/ConfigEngine/XmlAttributeUtility.h"
#include "UI/GraphicLayer/Layers/GraphicRootLayer.h"

SHARELIB_BEGIN_NAMESPACE

IMPLEMENT_RUNTIME_DYNAMIC_CREATE(GraphicLayer, GraphicLayer::RegisterLayerClassesInfo)

GraphicLayer::GraphicLayer()
    : m_crbg(0)
{
    std::memset(m_bitConfig, 0, sizeof(m_bitConfig));
    m_bitConfig[TBitConfig::BIT_VISIBLE] = true;
    m_bitConfig[TBitConfig::BIT_ENABLE] = true;
}

GraphicLayer::~GraphicLayer()
{
    GraphicRootLayer *pRoot = GetRootGraphicLayer();
    if (pRoot) {
        pRoot->_OnChildRemoved(this);
    }
}

const CPoint &GraphicLayer::GetOrigin() const
{
    return m_origin;
}

void GraphicLayer::SetOrigin(const CPoint &pt, bool bRedraw /* = true*/)
{
    if (bRedraw) {
        CRect rcDraw = m_bounds;
        rcDraw.OffsetRect(pt - m_origin);
        rcDraw.UnionRect(&rcDraw, &m_bounds);
        SchedulePaint(&rcDraw);
    }
    m_origin = pt;
}

void GraphicLayer::OffsetOrigin(const CPoint &pt, bool bRedraw /*= true*/)
{
    if (pt.x == 0 && pt.y == 0) {
        return;
    }
    if (bRedraw) {
        CRect rcDraw = m_bounds;
        rcDraw.OffsetRect(pt);
        rcDraw.UnionRect(&rcDraw, &m_bounds);
        SchedulePaint(&rcDraw);
    }
    m_origin.Offset(pt);
}

bool GraphicLayer::AlignXY(TAlignType xAlign,
                           TAlignType yAlign,
                           GraphicLayer *pDatum,
                           bool bRedraw /*= true*/)
{
    if (!pDatum || root() != pDatum->root()) {
        return false;
    }
    CPoint ptDatum;
    MapPointToRoot(pDatum, &ptDatum);
    CPoint pt;
    MapPointToRoot(this, &pt);
    CPoint ptOffset;
    switch (xAlign) {
    case TAlignType::ALIGN_NEAR:
        ptOffset.x = ptDatum.x + pDatum->GetLayerBounds().left - pt.x - GetLayerBounds().left;
        break;
    case TAlignType::ALIGN_CENTER:
        ptOffset.x = ptDatum.x + pDatum->GetLayerBounds().CenterPoint().x - pt.x -
                     GetLayerBounds().CenterPoint().x;
        break;
    case TAlignType::ALIGN_FAR:
        ptOffset.x = ptDatum.x + pDatum->GetLayerBounds().right - pt.x - GetLayerBounds().right;
        break;
    case TAlignType::ALIGN_FRONT:
        ptOffset.x = ptDatum.x + pDatum->GetLayerBounds().left - pt.x - GetLayerBounds().right;
        break;
    case TAlignType::ALIGN_BACK:
        ptOffset.x = ptDatum.x + pDatum->GetLayerBounds().right - pt.x - GetLayerBounds().left;
        break;
    default:
        break;
    }
    switch (yAlign) {
    case TAlignType::ALIGN_NEAR:
        ptOffset.y = ptDatum.y + pDatum->GetLayerBounds().top - pt.y - GetLayerBounds().top;
        break;
    case TAlignType::ALIGN_CENTER:
        ptOffset.y = ptDatum.y + pDatum->GetLayerBounds().CenterPoint().y - pt.y -
                     GetLayerBounds().CenterPoint().y;
        break;
    case TAlignType::ALIGN_FAR:
        ptOffset.y = ptDatum.y + pDatum->GetLayerBounds().bottom - pt.y - GetLayerBounds().bottom;
        break;
    case TAlignType::ALIGN_FRONT:
        ptOffset.y = ptDatum.y + pDatum->GetLayerBounds().top - pt.y - GetLayerBounds().bottom;
        break;
    case TAlignType::ALIGN_BACK:
        ptOffset.y = ptDatum.y + pDatum->GetLayerBounds().bottom - pt.y - GetLayerBounds().top;
        break;
    default:
        break;
    }

    OffsetOrigin(ptOffset, bRedraw);
    return true;
}

const CRect &GraphicLayer::GetLayerBounds() const
{
    return m_bounds;
}

void GraphicLayer::SetLayerBounds(const CRect &rcBounds, bool bRedraw /*= true*/)
{
    if (bRedraw) {
        CRect rcDraw;
        rcDraw.UnionRect(&m_bounds, &rcBounds);
        SchedulePaint(&rcDraw);
    }
    m_bounds = rcBounds;
}

void GraphicLayer::SetLayoutLua(const std::wstring &layoutLua)
{
    m_layoutLua = layoutLua;
}

std::wstring GraphicLayer::GetLayoutLua() const
{
    return m_layoutLua;
}

bool GraphicLayer::HitTest(const CPoint &pt) const
{
    return !!m_bounds.PtInRect(pt);
}

void GraphicLayer::Relayout()
{
    traverse_tree_node_t2b(this, [this](GraphicLayer *pLayer, int /*nDepth*/) -> int {
        if (pLayer->m_bitConfig[TBitConfig::BIT_ROOT_LAYER]) {
            return 1;
        }
        bool bLayoutChildren = true;
        pLayer->SendLayerMsg(&LayerMsgCallback::OnLayerMsgLayout, bLayoutChildren);
        return bLayoutChildren ? 1 : 0;
    });
}

bool shr::GraphicLayer::OnLayerMsgLayout(GraphicLayer *pLayer, bool &bLayoutChildren)
{
    (void)pLayer;
    (void)bLayoutChildren;
    assert(pLayer == this);
    if (!m_layoutLua.empty()) {
        auto pRoot = GetRootGraphicLayer();
        if (pRoot) {
            auto pLua = pRoot->FindLuaByName(m_layoutLua);
            if (pLua) {
                pLua->set_variable("this", this);
                pLua->run();
            }
        }
    }
    return true;
}

/** 计算子结点的坐标
@param[in] pChild 待计算的子结点
@param[in] pParent 父辈结点
@param[in] ptSrc 父辈结点坐标
@return 子结点坐标
*/
static CPoint CalculateChildPoint(GraphicLayer *pChild, GraphicLayer *pParent, const CPoint &ptSrc)
{
    CPoint ptOffset;
    while (pChild != pParent) {
        ptOffset += pChild->GetOrigin();
        pChild = pChild->parent();
    }
    return CPoint(ptSrc.x - ptOffset.x, ptSrc.y - ptOffset.y);
}

GraphicLayer *GraphicLayer::FindChildLayerFromPoint(GraphicLayer *pParent, CPoint &pt)
{
    if (pParent->child_count() == 0) {
        return nullptr;
    }

    GraphicLayer *pChildToFind = nullptr;
    int nDepthFinded = 0;
    traverse_tree_node_reverse_t2b(
        pParent,
        [pParent, &pt, &pChildToFind, &nDepthFinded](GraphicLayer *pLayer, int nDepth) -> int {
            if (nDepth == 0) {
                return 1;
            }
            if (pChildToFind && (nDepthFinded >= nDepth)) {
                //已找到
                return -1;
            }

            //坐标映射到当前结点pLayer
            CPoint ptCur = CalculateChildPoint(pLayer, (pChildToFind ? pChildToFind : pParent), pt);

            if (pLayer->IsEnable() && pLayer->IsVisible() && pLayer->HitTest(ptCur)) {
                if (pLayer->IsTransparent()) {
                    //不记录,继续子结点
                    return 1;
                } else {
                    //符合要求,记录下来
                    pChildToFind = pLayer;
                    nDepthFinded = nDepth;
                    pt = ptCur;
                    return (pLayer->IsInterceptChildren() ? -1 : 1);
                }
            } else {
                return 0;
            }
        });
    return pChildToFind;
}

void GraphicLayer::MapPointToRoot(const GraphicLayer *pLayer, POINT *pPt, size_t nCount /* = 1*/)
{
    assert(pLayer);
    assert(pLayer->GetRootGraphicLayer());
    assert(pPt);
    assert(nCount > 0);
    const GraphicLayer *pCur = pLayer;
    while (pCur && !pCur->is_root()) {
        for (size_t i = 0; i < nCount; ++i) {
            ((CPoint *)pPt)[i] += pCur->GetOrigin();
        }
        pCur = pCur->parent();
    }
}

void GraphicLayer::MapRootPointToLayer(const GraphicLayer *pLayer,
                                       POINT *pPtRoot,
                                       size_t nCount /*= 1*/)
{
    assert(pLayer);
    assert(pLayer->GetRootGraphicLayer());
    assert(pPtRoot);
    assert(nCount > 0);
    const GraphicLayer *pCur = pLayer;
    CPoint ptOffset;
    while (pCur && !pCur->is_root()) {
        ptOffset += pCur->GetOrigin();
        pCur = pCur->parent();
    }
    for (size_t i = 0; i < nCount; ++i) {
        ((CPoint *)pPtRoot)[i] -= ptOffset;
    }
}

void GraphicLayer::MapPointToLayer(const GraphicLayer *pLayerSrc,
                                   const GraphicLayer *pLayerDst,
                                   POINT *pPt,
                                   size_t nCount /*= 1*/)
{
    assert(pLayerSrc && pLayerDst);
    assert(pPt);
    assert(nCount > 0);
    if ((pLayerSrc == pLayerDst) || !pLayerSrc || !pLayerDst) {
        return;
    }
    assert(pLayerSrc->GetRootGraphicLayer());
    assert(pLayerSrc->GetRootGraphicLayer() == pLayerDst->GetRootGraphicLayer());
    const GraphicLayer *pCur = pLayerSrc;
    while (pCur && !pCur->is_root()) {
        for (size_t i = 0; i < nCount; ++i) {
            ((CPoint *)pPt)[i] += pCur->GetOrigin();
        }
        pCur = pCur->parent();
        if (pCur == pLayerDst) {
            return;
        }
    }
    MapRootPointToLayer(pLayerDst, pPt, nCount);
}

void GraphicLayer::SetMouseCapture(bool bCapture)
{
    auto pRoot = GetRootGraphicLayer();
    assert(pRoot);
    if (pRoot) {
        pRoot->_RegisterCapturedLayer(this, bCapture);
    }
}

void GraphicLayer::SetEnableMouseDragMove(bool bEnableDragMouve)
{
    auto pRoot = GetRootGraphicLayer();
    assert(pRoot);
    if (pRoot) {
        m_bitConfig[TBitConfig::BIT_MOUSE_DRAG_MOVE] = bEnableDragMouve;
        pRoot->_RegisterMouseDragMove(this, bEnableDragMouve);
    }
}

void GraphicLayer::SetEnableDragWindow(bool bEnableMouveWindow)
{
    auto pRoot = GetRootGraphicLayer();
    assert(pRoot);
    if (pRoot) {
        m_bitConfig[TBitConfig::BIT_DRAG_WINDOW] = bEnableMouveWindow;
        pRoot->_RegisterDragMoveWindow(this, bEnableMouveWindow);
    }
}

void GraphicLayer::SetMouseInOutLinkParent(bool bLink)
{
    m_bitConfig[TBitConfig::BIT_MOUSE_INOUT_LINK_PARENT] = bLink;
}

bool GraphicLayer::IsMouseInOutLinkParent() const
{
    return m_bitConfig[TBitConfig::BIT_MOUSE_INOUT_LINK_PARENT];
}

void GraphicLayer::SetTransparent(bool isEnable)
{
    if (m_bitConfig[TBitConfig::BIT_TRANSPARENT] != isEnable) {
        m_bitConfig[TBitConfig::BIT_TRANSPARENT] = isEnable;
        SchedulePaint();
    }
}

bool GraphicLayer::IsTransparent() const
{
    return m_bitConfig[TBitConfig::BIT_TRANSPARENT];
}

void GraphicLayer::SetInterceptChildren(bool bIntercept)
{
    m_bitConfig[TBitConfig::BIT_INTERCEPT_CHILDREN] = bIntercept;
}

bool GraphicLayer::IsInterceptChildren() const
{
    return m_bitConfig[TBitConfig::BIT_INTERCEPT_CHILDREN];
}

void GraphicLayer::SetEnableFocus(bool isEnableFocus)
{
    m_bitConfig[TBitConfig::BIT_ENABLE_FOCUS] = isEnableFocus;
}

bool GraphicLayer::IsEnableFocus()
{
    return m_bitConfig[TBitConfig::BIT_ENABLE_FOCUS];
}

void GraphicLayer::SetFocus()
{
    m_bitConfig[TBitConfig::BIT_ENABLE_FOCUS] = true;
    auto pRoot = GetRootGraphicLayer();
    if (pRoot) {
        pRoot->_RegisterFocusLayer(this, true);
    }
}

void GraphicLayer::ClearFocus()
{
    auto pRoot = GetRootGraphicLayer();
    if (pRoot) {
        pRoot->_RegisterFocusLayer(this, false);
    }
}

bool GraphicLayer::IsFocus()
{
    auto pRoot = GetRootGraphicLayer();
    if (pRoot) {
        return this == pRoot->GetFocusLayer();
    }
    return false;
}

bool GraphicLayer::IsVisible() const
{
    return m_bitConfig[TBitConfig::BIT_VISIBLE];
}

bool GraphicLayer::IsEnable() const
{
    return m_bitConfig[TBitConfig::BIT_ENABLE];
}

void GraphicLayer::SetVisible(bool isVisible)
{
    if (m_bitConfig[TBitConfig::BIT_VISIBLE] != isVisible) {
        m_bitConfig[TBitConfig::BIT_VISIBLE] = isVisible;
        if (!m_bitConfig[TBitConfig::BIT_VISIBLE]) {
            if (parent()) {
                parent()->SchedulePaint();
            }
        } else {
            SchedulePaint();
        }
    }
}

void GraphicLayer::SetEnable(bool isEnable)
{
    if (m_bitConfig[TBitConfig::BIT_ENABLE] != isEnable) {
        m_bitConfig[TBitConfig::BIT_ENABLE] = isEnable;
        SchedulePaint();
    }
}

void GraphicLayer::SetBgColor(COLOR32 crbg)
{
    if (m_crbg != crbg) {
        m_crbg = VERIFY_COLOR32(crbg);
        SchedulePaint();
    }
}

COLOR32 GraphicLayer::GetBgColor() const
{
    return m_crbg;
}

//指针所指的对象是否改写了BeforePaintChildren虚函数
static bool IsRewriteBeforePaintChildren(GraphicLayer *pLayer)
{
#if defined(WIN32) && defined(_MSC_VER)
    //此段代码不是可移植的，只有sisual studio的C++编译器，类的虚表才符合下面的规律

    static ptrdiff_t s_progAddr = 0;
    static std::once_flag s_funcAddrInitFlag;
    if (s_progAddr == 0) {
        std::call_once(s_funcAddrInitFlag, []() {
            GraphicLayer temp;
            ptrdiff_t *pVtbl = *((ptrdiff_t **)static_cast<IFastPaintHelper *>(&temp));
            //BeforePaintChildren虚函数在IFastPaintHelper的虚表中的第一个位置，
            //因此虚表pVtbl[0]就是GraphicLayer中BeforePaintChildren的实际地址，如果派生类
            //中改写此虚函数，虚表中对应位置填写的地址就会改变
            s_progAddr = pVtbl[0];
#    ifdef _M_IX86
            //用32位内联汇编取虚函数地址进行验证，64位不允许使用内联汇编
            ptrdiff_t asmProc = 0;
            __asm
            {
                mov eax, GraphicLayer::BeforePaintChildren
                mov asmProc, eax
            }
            if (asmProc != s_progAddr)
            {
                s_progAddr = 0;
                assert(!"获取虚函数地址错误");
            }
#    endif
        });
    }
    if (s_progAddr != 0) {
        return s_progAddr != (*(ptrdiff_t **)static_cast<IFastPaintHelper *>(pLayer))[0];
    }
#endif
    return true;
}

void GraphicLayer::SchedulePaint(const CRect *pRcDraw /* = nullptr*/)
{
    if (!IsVisible()) {
        return;
    }
    auto pRoot = GetRootGraphicLayer();
    if (!pRoot) {
        return;
    }
    ATL::CWindow *pWnd = pRoot->GetMSWindow();
    if (!pWnd || !pWnd->IsWindow() || !pWnd->IsWindowVisible()) {
        return;
    }

    GraphicLayer *pPaintStart = nullptr;

    CRect rcDraw = pRcDraw ? *pRcDraw : GetLayerBounds();
    if ((rcDraw == GetLayerBounds()) && MayItBePaintStart(this)) {
        //自身要作为绘制起点，必须附加一个条件：重绘区域等于自身区域。
        //像平移这种操作只重绘自身区域是不够的。当转到父Layer时，不必要这个条件，
        //因为超出父Layer的区域会被裁剪
        pPaintStart = this;
    }

    rcDraw.OffsetRect(m_origin); //转到父Layer坐标
    GraphicLayer *pLayer = parent();
    while (pLayer && (pLayer != pRoot)) {
        //因为裁剪，重绘的最大区域不会超出父Layer的区域
        rcDraw.IntersectRect(&rcDraw, &pLayer->GetLayerBounds());
        if (!pLayer->IsVisible() || rcDraw.IsRectEmpty()) {
            return;
        }

        if (!pPaintStart && MayItBePaintStart(pLayer)) {
            //找到最近的绘制起点
            pPaintStart = pLayer;
        }
        if (pPaintStart && (pPaintStart != pLayer)) {
            //如果记录下了绘制起点,向上遍历的过程中发现有父级Layer改写了BeforePaintChildren,
            //这个函数里面做的事情可能会影响到所有子Layer绘制,具体的影响是由实现决定的,不可预测.
            //比如,如果它在该虚函数里设置所有子Layer的透明度,这时候再直接从子级Layer开始绘制的话,
            //原有透明度效果就会被破坏,因此这种绘制起点需要清除
            //有的Layer虽然改写了BeforePaintChildren,但如果它当前的这次绘制并没有什么特殊处理,
            //这时候也是可以跳过的,因此在是否改写"BeforePaintChildren"的基础上增加IsPaintChildrenTrivial
            //的判断条件
            if (IsRewriteBeforePaintChildren(pLayer) && !pLayer->IsPaintChildrenTrivial()) {
                pPaintStart = nullptr;
            }
        }

        rcDraw.OffsetRect(pLayer->m_origin);
        pLayer = pLayer->parent();
    }
    pRoot->_RegisterLayeredWndClipRect(rcDraw);
    pRoot->_RegisterPaintStart(pPaintStart);
    pWnd->InvalidateRect(&rcDraw);
}

bool GraphicLayer::MayItBePaintStart(const GraphicLayer *pLayer)
{
    //背景不透明
    return (pLayer && (COLOR32_GetA(pLayer->m_crbg) == 255));
}

GraphicRootLayer *GraphicLayer::GetRootGraphicLayer() const
{
    GraphicLayer *pRoot = root();
    if (pRoot->m_bitConfig[TBitConfig::BIT_ROOT_LAYER]) {
        return ((GraphicRootLayer *)pRoot);
    } else {
        return nullptr;
    }
}

ATL::CWindow *GraphicLayer::GetMSWindow() const
{
    GraphicRootLayer *pRoot = GetRootGraphicLayer();
    if (pRoot) {
        return pRoot->GetMSWindow();
    } else {
        return nullptr;
    }
}

void GraphicLayer::ChangeMyPosInTheTree(GraphicLayer *pTarget, TInsertPos insertType)
{
    if (pTarget && pTarget != this && !is_root() &&
        (GetRootGraphicLayer() == pTarget->GetRootGraphicLayer())) {
        if (pTarget->is_root() &&
            (insertType == TInsertPos::AsPrevSibling || insertType == TInsertPos::AsNextSibling)) {
            return;
        }
        CPoint ptOrigin = m_origin;
        GraphicLayer *pSrc = parent();
#pragma warning(push)
#pragma warning(disable : 4995)
        remove_tree_node(this);
#pragma warning(pop)
        pTarget->insert_tree_node(this, insertType);
        GraphicLayer::MapPointToLayer(pSrc, parent(), &ptOrigin);
        SetOrigin(ptOrigin, false);
        pSrc->SchedulePaint();
        parent()->SchedulePaint();
    }
}

void GraphicLayer::BringToTop()
{
    ChangeMyPosInTheTree(parent(), TInsertPos::AsLastChild);
}

void GraphicLayer::BringToTopMost()
{
    ChangeMyPosInTheTree(root(), TInsertPos::AsLastChild);
}

HANDLE GraphicLayer::CreateLayerTimer(uint32_t nPeriod)
{
    GraphicRootLayer *pRootLayer = GetRootGraphicLayer();
    assert(pRootLayer);
    if (pRootLayer) {
        return pRootLayer->_AllocTimer(this, nPeriod);
    }
    return nullptr;
}

void GraphicLayer::DestroyLayerTimer(HANDLE hTimer)
{
    if (!hTimer) {
        return;
    }
    GraphicRootLayer *pRootLayer = GetRootGraphicLayer();
    assert(pRootLayer);
    if (pRootLayer) {
        pRootLayer->_FreeTimer(this, hTimer);
    }
}

void GraphicLayer::DestroyAllLayerTimers()
{
    GraphicRootLayer *pRootLayer = GetRootGraphicLayer();
    assert(pRootLayer);
    if (pRootLayer) {
        pRootLayer->_FreeTimer(this, nullptr);
    }
}

void GraphicLayer::AddLayerMsgHook(LayerMsgCallback *pHook)
{
    if (pHook) {
        if (!m_spHookers) {
            m_spHookers.reset(new std::list<LayerMsgCallback *>);
        }
        assert(std::find(m_spHookers->begin(), m_spHookers->end(), pHook) == m_spHookers->end());
        m_spHookers->push_front(pHook);
    }
}

void GraphicLayer::RemoveLayerMsgHook(LayerMsgCallback *pHook)
{
    if (pHook && m_spHookers) {
        auto it = std::find(m_spHookers->begin(), m_spHookers->end(), pHook);
        if (it != m_spHookers->end()) {
            //只标记不删除,避免在回调中自己删除自己导致迭代器失效
            *it = nullptr;
        }
    }
}

void GraphicLayer::RemoveAllLayerMsgHook()
{
    if (m_spHookers) {
        for (auto &item : *m_spHookers) {
            //只标记不删除,避免在回调中自己删除自己导致迭代器失效
            item = nullptr;
        }
    }
}

void GraphicLayer::AddLayerMsgObserver(LayerMsgCallback *pObserver)
{
    if (pObserver) {
        if (!m_spObservers) {
            m_spObservers.reset(new std::list<LayerMsgCallback *>);
        }
        assert(std::find(m_spObservers->begin(), m_spObservers->end(), pObserver) ==
               m_spObservers->end());
        m_spObservers->push_back(pObserver);
    }
}

void GraphicLayer::RemoveLayerMsgObserver(LayerMsgCallback *pObserver)
{
    if (pObserver && m_spObservers) {
        auto it = std::find(m_spObservers->begin(), m_spObservers->end(), pObserver);
        if (it != m_spObservers->end()) {
            //只标记不删除,避免在回调中自己删除自己导致迭代器失效
            *it = nullptr;
        }
    }
}

void GraphicLayer::RemoveAllLayerMsgObserver()
{
    if (m_spObservers) {
        for (auto &item : *m_spObservers) {
            //只标记不删除,避免在回调中自己删除自己导致迭代器失效
            item = nullptr;
        }
    }
}

GraphicLayer *GraphicLayer::CreateGraphicLayerByName(const wchar_t *pCreateKay)
{
    if (!pCreateKay || !*pCreateKay) {
        return nullptr;
    }
    auto it = GetLayersMap().find(pCreateKay);
    if (it == GetLayersMap().end()) {
        assert(!"未注册的类信息");
        return nullptr;
    }
    return (GraphicLayer *)it->second();
}

bool GraphicLayer::LoadFromXml(const wchar_t *pFilePath,
                               pugi::xml_encoding encoding /*= pugi::xml_encoding::encoding_auto*/)
{
    ATL::CAtlFile file;
    HRESULT hr = file.Create(pFilePath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
    if (FAILED(hr)) {
        assert(!"文件打开失败");
        return false;
    }
    ATL::CAtlFileMapping<uint8_t> fileMap;
    hr = fileMap.MapFile(file);
    if (FAILED(hr)) {
        assert(!"文件映射失败");
        return false;
    }
    return LoadFromXml(fileMap, fileMap.GetMappingSize(), encoding);
}

bool GraphicLayer::LoadFromXml(const void *pBuffer,
                               size_t nSize,
                               pugi::xml_encoding encoding /*= pugi::xml_encoding::encoding_auto*/)
{
    pugi::xml_document xmlDoc;
    if (!xmlDoc.load_buffer(pBuffer, nSize, pugi::parse_default, encoding)) {
        assert(!"xml解析失败");
        return false;
    }
    pugi::xml_node nodeRoot = xmlDoc.first_child();
    destroy_children(this);
    int nLastDepth = 0;
    GraphicLayer *pLastNode = this;
    traverse_tree_node_t2b(
        nodeRoot, [this, &nLastDepth, &pLastNode](pugi::xml_node &node, int nDepth) -> int {
            if ((node.type() == pugi::xml_node_type::node_pcdata) ||
                (node.type() == pugi::xml_node_type::node_cdata) ||
                (node.type() == pugi::xml_node_type::node_comment)) {
                return 0;
            }
            if (nDepth == 0) {
                if (ReadAllXmlAttributes(node)) {
                    return 1;
                } else {
                    return 0;
                }
            }
            GraphicLayer *pNew = CreateGraphicLayerByName(node.name());
            if (!pNew) {
                return 0;
            }

            if (nDepth > nLastDepth) {
                pLastNode->insert_tree_node(pNew, TInsertPos::AsLastChild);
                pLastNode = pLastNode->first_child();
            } else {
                for (int i = 0; i < nLastDepth - nDepth; ++i) {
                    pLastNode = pLastNode->parent();
                }
                pLastNode->insert_tree_node(pNew, TInsertPos::AsNextSibling);
                pLastNode = pLastNode->next_sibling();
            }
            nLastDepth = nDepth;

            if (pNew->ReadAllXmlAttributes(node)) {
                return 1;
            } else {
                return 0;
            }
        });
    return true;
}

bool GraphicLayer::ReadAllXmlAttributes(const pugi::xml_node &node)
{
    for (auto itAttr = node.attributes_begin(); itAttr != node.attributes_end(); ++itAttr) {
        OnReadingXmlAttribute(*itAttr);
    }
    return OnReadXmlAttributeEnded(node);
}

bool GraphicLayer::ReadAllXmlAttributes(const std::wstring &xmlText)
{
    pugi::xml_document xmlDoc;
    if (!xmlDoc.load_string(xmlText.c_str())) {
        assert(!"xml解析失败");
        return false;
    }
    return ReadAllXmlAttributes(xmlDoc.first_child());
}

void GraphicLayer::OnReadingXmlAttribute(const pugi::xml_attribute &attr)
{
    if (std::wcscmp(L"lrID", attr.name()) == 0) {
        m_nID = (size_t)attr.as_ullong(0);
    } else if (std::wcscmp(L"lrColor", attr.name()) == 0) {
        XmlAttributeUtility::ReadXmlValueColor(attr.as_string(), m_crbg);
    } else if (std::wcscmp(L"lrOrigin", attr.name()) == 0) {
        XmlAttributeUtility::ReadXmlValue(attr.as_string(), m_origin);
    } else if (std::wcscmp(L"lrBounds", attr.name()) == 0) {
        XmlAttributeUtility::ReadXmlValue(attr.as_string(), m_bounds);
    } else if (std::wcscmp(L"lrVisible", attr.name()) == 0) {
        m_bitConfig[TBitConfig::BIT_VISIBLE] = attr.as_bool(true);
    } else if (std::wcscmp(L"lrEnable", attr.name()) == 0) {
        m_bitConfig[TBitConfig::BIT_ENABLE] = attr.as_bool(true);
    } else if (std::wcscmp(L"lrTransparent", attr.name()) == 0) {
        m_bitConfig[TBitConfig::BIT_TRANSPARENT] = attr.as_bool(false);
    } else if (std::wcscmp(L"lrInterceptChildren", attr.name()) == 0) {
        m_bitConfig[TBitConfig::BIT_INTERCEPT_CHILDREN] = attr.as_bool(false);
    } else if (std::wcscmp(L"lrMouseLinkParent", attr.name()) == 0) {
        SetMouseInOutLinkParent(attr.as_bool(false));
    } else if (std::wcscmp(L"lrDragMove", attr.name()) == 0) {
        SetEnableMouseDragMove(attr.as_bool(false));
    } else if (std::wcscmp(L"lrDragWindow", attr.name()) == 0) {
        SetEnableDragWindow(attr.as_bool(false));
    } else if (std::wcscmp(L"lrEnableFocus", attr.name()) == 0) {
        SetEnableFocus(attr.as_bool(false));
    } else if (std::wcscmp(L"lrLayoutLua", attr.name()) == 0) {
        m_layoutLua = attr.as_string();
    }
}

bool GraphicLayer::OnReadXmlAttributeEnded(const pugi::xml_node & /*node*/)
{
    return true;
}

bool GraphicLayer::SaveToXml(const wchar_t *pFilePath,
                             pugi::xml_encoding encoding /*= pugi::xml_encoding::encoding_auto*/)
{
    pugi::xml_document xmlDoc;
    int nLastDepth = 0;
    pugi::xml_node node = xmlDoc;
    traverse_tree_node_t2b(this,
                           [this, &nLastDepth, &node](GraphicLayer *pLayer, int nDepth) -> int {
                               if (nDepth == 0) {
                                   if (pLayer->m_bitConfig[TBitConfig::BIT_ROOT_LAYER]) {
                                       node = node.append_child(L"root");
                                   } else {
                                       node = node.append_child(pLayer->GetCreateKey());
                                   }
                                   OnWritingAttributeToXml(node);
                                   return OnWritingAttributeToXmlEnded(node) ? 1 : 0;
                               }

                               if (nDepth > nLastDepth) {
                                   node = node.append_child(pLayer->GetCreateKey());
                               } else {
                                   for (int i = 0; i < nLastDepth - nDepth; ++i) {
                                       node = node.parent();
                                   }
                                   node = node.parent().append_child(pLayer->GetCreateKey());
                               }
                               nLastDepth = nDepth;

                               pLayer->OnWritingAttributeToXml(node);
                               return pLayer->OnWritingAttributeToXmlEnded(node) ? 1 : 0;
                           });
    return xmlDoc.save_file(
        pFilePath, L"    ", pugi::format_default | pugi::format_write_bom, encoding);
}

void GraphicLayer::OnWritingAttributeToXml(pugi::xml_node node)
{
    if (m_nID != 0) {
        auto attr = node.append_attribute(L"lrID");
        attr = m_nID;
    }

    auto attr = node.append_attribute(L"lrColor");
    XmlAttributeUtility::WriteValueColorToXml(attr, m_crbg);

    attr = node.append_attribute(L"lrOrigin");
    XmlAttributeUtility::WriteValueToXml(attr, m_origin);

    attr = node.append_attribute(L"lrBounds");
    XmlAttributeUtility::WriteValueToXml(attr, m_bounds);

    if (!m_bitConfig[TBitConfig::BIT_VISIBLE]) {
        attr = node.append_attribute(L"lrVisible");
        attr = false;
    }

    if (!m_bitConfig[TBitConfig::BIT_ENABLE]) {
        attr = node.append_attribute(L"lrEnable");
        attr = false;
    }

    if (m_bitConfig[TBitConfig::BIT_TRANSPARENT]) {
        attr = node.append_attribute(L"lrTransparent");
        attr = true;
    }

    if (m_bitConfig[TBitConfig::BIT_INTERCEPT_CHILDREN]) {
        attr = node.append_attribute(L"lrInterceptChildren");
        attr = true;
    }

    if (m_bitConfig[TBitConfig::BIT_MOUSE_DRAG_MOVE]) {
        attr = node.append_attribute(L"lrDragMove");
        attr = true;
    }

    if (m_bitConfig[TBitConfig::BIT_DRAG_WINDOW]) {
        attr = node.append_attribute(L"lrDragWindow");
        attr = true;
    }

    if (m_bitConfig[TBitConfig::BIT_ENABLE_FOCUS]) {
        attr = node.append_attribute(L"lrEnableFocus");
        attr = true;
    }

    if (!m_layoutLua.empty()) {
        attr = node.append_attribute(L"lrLayoutLua");
        attr = m_layoutLua.c_str();
    }
}

bool GraphicLayer::OnWritingAttributeToXmlEnded(pugi::xml_node node)
{
    (void)node;
    return true;
}

GraphicLayer *GraphicLayer::FindChildByID(size_t nID)
{
    GraphicLayer *pChild = nullptr;
    traverse_tree_node_t2b(this, [&pChild, nID](GraphicLayer *pNode, int /*nDepth*/) -> int {
        if (pNode && pNode->m_nID == nID) {
            pChild = pNode;
            return -1;
        } else {
            return 1;
        }
    });
    return pChild;
}

void GraphicLayer::RegisterLayerClassesInfo(const wchar_t *pCreateKay,
                                            TRuntimeCreateFunction pfnCreateFunc)
{
    assert(pCreateKay && std::wcslen(pCreateKay) > 0);
    assert(pfnCreateFunc);
    assert("重复注册" && (GetLayersMap().find(pCreateKay) == GetLayersMap().end()));
    GetLayersMap()[pCreateKay] = pfnCreateFunc;
}

std::unordered_map<std::wstring, TRuntimeCreateFunction> &GraphicLayer::GetLayersMap()
{
    static std::unordered_map<std::wstring, TRuntimeCreateFunction> s_layersMapInfo;
    return s_layersMapInfo;
}

SHARELIB_END_NAMESPACE

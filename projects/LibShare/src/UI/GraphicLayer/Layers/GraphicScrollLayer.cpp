#include "targetver.h"
#include "UI/GraphicLayer/Layers/GraphicScrollLayer.h"
#include "UI/GraphicLayer/Layers/GraphicRootLayer.h"

SHARELIB_BEGIN_NAMESPACE

IMPLEMENT_RUNTIME_DYNAMIC_CREATE(GraphicScrollLayer, GraphicLayer::RegisterLayerClassesInfo)

GraphicScrollLayer::GraphicScrollLayer()
    : m_bUseSysSetting(true)
    , m_nLinesSys(0)
    , m_nCharsSys(0)
{}

GraphicScrollLayer::~GraphicScrollLayer() {}

void GraphicScrollLayer::SetVerticalScrollUnit(int32_t nUnit)
{
    m_bUseSysSetting = (nUnit <= 0);
    m_nLinesSys = nUnit;
}

int32_t GraphicScrollLayer::GetVerticalScrollUnit()
{
    if (m_bUseSysSetting || (m_nLinesSys == 0)) {
        auto p = GetRootGraphicLayer();
        if (p) {
            return p->_GetSystemWheelSetting(true);
        }
    }
    return m_nLinesSys;
}

void GraphicScrollLayer::SetHorizontalScrollUnit(int32_t nUnit)
{
    m_bUseSysSetting = (nUnit <= 0);
    m_nCharsSys = nUnit;
}

int32_t GraphicScrollLayer::GetHorizontalScrollUnit()
{
    if (m_bUseSysSetting || (m_nCharsSys == 0)) {
        auto p = GetRootGraphicLayer();
        if (p) {
            return p->_GetSystemWheelSetting(false);
        }
    }
    return m_nCharsSys;
}

bool GraphicScrollLayer::OnLayerMsgMouse(GraphicLayer *pLayer,
                                         UINT uMsg,
                                         WPARAM wParam,
                                         LPARAM lParam,
                                         LRESULT &lResult)
{
    if (WM_MOUSEWHEEL == uMsg) {
        if (m_bUseSysSetting || (m_nLinesSys == 0)) {
            m_nLinesSys = GetRootGraphicLayer()->_GetSystemWheelSetting(true);
        }
        short zDelta = (short)HIWORD(wParam);
        int32_t nYOffset = zDelta / WHEEL_DELTA * m_nLinesSys;
        OffsetOrigin(CPoint(0, nYOffset), false);
        CRect rcBound = GetLayerBounds();
        rcBound.OffsetRect(0, -nYOffset);
        SetLayerBounds(rcBound, false);
        //本身映射到窗口上的坐标实际没有任何改变, 所以前两个OffsetOrigin和SetLayerBounds都不绘制,最后再整体重绘
        SchedulePaint();
    }
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    else if (WM_MOUSEHWHEEL == uMsg) {
        if (m_bUseSysSetting || (m_nCharsSys == 0)) {
            m_nCharsSys = GetRootGraphicLayer()->_GetSystemWheelSetting(false);
        }

        short zDelta = (short)HIWORD(wParam);
        int32_t nXOffset = zDelta / WHEEL_DELTA * m_nCharsSys;
        OffsetOrigin(CPoint(nXOffset, 0), false);
        CRect rcBound = GetLayerBounds();
        rcBound.OffsetRect(-nXOffset, 0);
        SetLayerBounds(rcBound, false);
        SchedulePaint();
    }
#endif
    else {
        return __super::OnLayerMsgMouse(pLayer, uMsg, wParam, lParam, lResult);
    }
    return true;
}

SHARELIB_END_NAMESPACE

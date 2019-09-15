#pragma once
#include "MacroDefBase.h"
#include "UI/GraphicLayer/Layers/GraphicLayer.h"

SHARELIB_BEGIN_NAMESPACE

class GraphicScrollLayer : public GraphicLayer
{
    DECLARE_RUNTIME_DYNAMIC_CREATE(GraphicScrollLayer, L"scroll")

public:
    GraphicScrollLayer();
    ~GraphicScrollLayer();

    //nUnit小于等于0表示使用系统设置
    void SetVerticalScrollUnit(int32_t nUnit);
    int32_t GetVerticalScrollUnit();
    //nUnit小于等于0表示使用系统设置
    void SetHorizontalScrollUnit(int32_t nUnit);
    int32_t GetHorizontalScrollUnit();

protected:
    /** 复用系统消息WM_MOUSEFIRST~WM_MOUSELAST,各参数含义不作任何改变,但坐标已经转换为相对pLayer自身的坐标
    */
    virtual bool OnLayerMsgMouse(GraphicLayer *pLayer,
                                 UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam,
                                 LRESULT &lResult);

private:
    bool m_bUseSysSetting;
    int32_t m_nLinesSys;
    int32_t m_nCharsSys;
};

SHARELIB_END_NAMESPACE

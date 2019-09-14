#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include "MacroDefBase.h"
#include "GraphicLayer.h"
#include "UI/DirectX/DWriteUtility.h"
#include "UI/Utility/WindowUtility.h"
#include "UI/GraphicLayer/ConfigEngine/XmlResourceMgr.h"
#include "lua/lua_wrapper.h"

SHARELIB_BEGIN_NAMESPACE

/*!
* \class GraphicRootLayer
* \brief 管理所有绘制单元，并作为windows消息与绘制单位之间的转接桥梁，一个窗口只能有一个该类的实例, 
*        该窗口是无边框窗口(全部都是客户区)
*/
class GraphicRootLayer :
    public GraphicLayer,
    public BorderlessWindowHandler
{
    BEGIN_MSG_MAP_EX(GraphicRootLayer)
        assert(hWnd == m_pWindow->m_hWnd);
        CHAIN_MSG_MAP(BorderlessWindowHandler)
        MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, OnMouse) //内部有处理则拦截
        MSG_WM_MOUSELEAVE(OnMouseLeave) //不拦截
        MESSAGE_RANGE_HANDLER(WM_KEYFIRST, WM_KEYLAST, OnKey) //内部有处理则拦截
        MSG_WM_SETFOCUS(OnSetFocus) //不拦截
        MSG_WM_KILLFOCUS(OnKillFocus) //不拦截
        MSG_WM_SIZE(OnSize) //不拦截
        MSG_WM_PAINT(OnPaint) //拦截
        MESSAGE_HANDLER_EX(LAYER_WM_TIMER_NOTIFY, OnLayerTimerMsg) //拦截
        MESSAGE_HANDLER_EX(LAYER_WM_INTERNAL_USE, OnLayerInternalUse) //拦截
        MSG_WM_NCDESTROY(OnNcDestroy) //不拦截
        MSG_WM_SETTINGCHANGE(OnSettingChange) //不拦截
        END_MSG_MAP()

public:
    explicit GraphicRootLayer(ATL::CWindow* pWnd);

    /*如果子控件不是new出来插入到RootLayer中的, 可以在此析构之前, 调用基类的
       template<class Deletor>static void destroy_children(tree_node *pNode, Deletor && d);
      而后再执行此析构函数
    */
    virtual ~GraphicRootLayer() final;

    //设置皮肤
    void SetSkinMap(std::shared_ptr<std::unordered_map<std::wstring, TBitmapInfo> > spSkinMap);
    TBitmapInfo * FindSkinByName(const std::wstring & key);

    /*从xml中加载lua脚本,xml结点名字为键,值为lua脚本. 如名字相同,后加载的覆盖先加载的.
    lua内部是用char*解析的,因此xml加载后的lua脚本会转码后给lua解析,要注意当前编译器的char*编码,
    及注意避免乱码.
    */
    bool LoadLuaFromXml(const wchar_t * pXmlFilePath);
    bool LoadLuaFromXml(const void * pData, size_t nLength);
    /** 根据名字查找lua脚本
    @param[in] key 名字
    @return lua_state_wrapper指针,找不到返回nullptr
    */
    lua_state_wrapper* FindLuaByName(const std::wstring & key);
    void ClearLua();

    ATL::CWindow* GetMSWindow() const;
    bool IsLayeredWindow();
    D2DRenderPack & D2DRender();
    DWriteUtility & DWritePack();
    void SetD2DRenderType(D2DRenderPack::TRenderType type);

    GraphicLayer * GetFocusLayer();

//----辅助函数，供基类使用，外部不要使用-------------------------------------------------

    /** 子Layer销毁时调用, 清理它在RootLayer中的注册信息，避免出现野指针崩溃
    */
    void _OnChildRemoved(GraphicLayer* pChild);

    /** 分配定时器
    @param[in] pLayer 定时器属于哪个Layer
    @param[in] nPeriod 定时器间隔, 毫秒
    @return 定时器句柄
    */
    HANDLE _AllocTimer(GraphicLayer * pLayer, uint32_t nPeriod);

    /** 销毁定时器
    @param[in] pLayer 定时器属于哪个Layer
    @param[in] hTimer 传nullptr表示删除pLayer的所有定时器
    */
    void _FreeTimer(GraphicLayer * pLayer, HANDLE hTimer);

    /** 注册/反注册Capture的Layer
    @param[in] pLayer 目标Layer
    @param[in] bCapture true:注册,false:反注册
    */
    void _RegisterCapturedLayer(GraphicLayer * pLayer, bool bCapture);

    /** 注册/反注册是否允许鼠标拖动图层
    @param[in] pLayer 目标Layer
    @param[in] bCapture true:注册,false:反注册
    */
    void _RegisterMouseDragMove(GraphicLayer * pLayer, bool bEnableDragMove);

    /** 注册/反注册是否允许拖动移动窗口
    @param[in] pLayer 目标Layer
    @param[in] bCapture true:注册,false:反注册
    */
    void _RegisterDragMoveWindow(GraphicLayer * pLayer, bool bEnableMoveWindow);

    /** 注册/反注册焦点layer
    @param[in] pLayer 目标Layer
    @param[in] bFocus true:注册,false:反注册, false时pLayer可以为空，表示清空焦点
    */
    void _RegisterFocusLayer(GraphicLayer * pLayer, bool bFocus);

    /** 注册层窗口的无效区域
    @param[in] rcClip无效区域
    */
    void _RegisterLayeredWndClipRect(const CRect & rcClip);

    /** 注册绘制起点
    @param[in] pPaintStart 注册/反注册的绘制起点
    */
    void _RegisterPaintStart(GraphicLayer * pPaintStart);

    /** 清空绘制起点
    */
    void _ClearPaintStart();

    /** 获取系统滚轮设置
    @param[in] bVertical true表示垂直，false表示水平
    @return 根据系统滚轮及字体大小计算出的滚动单位：像素
    */
    int32_t _GetSystemWheelSetting(bool bVertical);

private:
    //只有背景色的设置项起作用
    virtual void OnReadingXmlAttribute(const pugi::xml_attribute & attr) override;
    //只有背景色的设置项起作用
    virtual void OnWritingAttributeToXml(pugi::xml_node node) override;

//----消息转接-----------------------------------------------------

    LRESULT OnMouse(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void OnMouseLeave();
    LRESULT OnKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void OnSetFocus(ATL::CWindow /*wndOld*/);
    void OnKillFocus(ATL::CWindow /*wndFocus*/);
    void OnSize(UINT nType, CSize size);
    void OnPaint(WTL::CDCHandle /*dc*/);
    void DrawAllLayers(const CSize & szRender, const CRect & rcDraw, bool bLayeredWnd);
    
    /** 递归绘制
    @param[in] d2dRender D2D绘制包
    @param[in] pDefaultPath 默认的绘制路径
    @param[in] fastPath 快速绘制路径
    */
    void RecursiveDraw(D2DRenderPack & d2dRender, GraphicLayer * pDefaultPath, std::vector<GraphicLayer*> & fastPath);
    
    LRESULT OnLayerTimerMsg(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam);
    LRESULT OnLayerInternalUse(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam);
    void OnNcDestroy();
    void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);

//----工具函数------------------------------------------------------

    /** 发送MouseInOut消息
    @param[in] pMouseIn 当前鼠标进入的Layer,nullptr则表示没有进入任何的Layer
    */
    void NotifyMouseInOut(GraphicLayer * pMouseIn);
    void NotifyFocus(GraphicLayer * pMouse, UINT uMsg);

    /** 定时器回调
    */
    static void __stdcall TimerCallbackFunction(void * lpParameter, BOOLEAN /*TimerOrWaitFired*/);

private:
    D2DRenderPack m_d2dRenderPack;
    DWriteUtility m_dwrite;
    ATL::CWindow* m_pWindow;

//----绘制辅助相关--------------------------

    //D2DRender类型
    D2DRenderPack::TRenderType m_renderType;

    //层窗口需要绘制的无效区域
    CRect m_rcLayeredWndClip;

    //绘制起点
    GraphicLayer * m_pPaintStart;

    //快速绘制通道
    std::vector<GraphicLayer*> m_fastPath;

//----鼠标消息辅助------------------------------
    GraphicLayer* m_pLastMouseHandler;
    bool m_bMouseCapture;
    class MouseDragMoveHandler;
    std::unique_ptr<MouseDragMoveHandler> m_spMouseDragMove;
    class DragMoveWindowHandler;
    std::unique_ptr<DragMoveWindowHandler> m_spDragMoveWindow;

//----键盘消息------------------------------
    GraphicLayer * m_pFocusLayer;
    GraphicLayer * m_pFocusRetore;

//----系统滚轮设置------------------------------
    int32_t m_nVerticalPixel;
    int32_t m_nHorizontalPixel;

//----定时器相关-----------------------------

    //定时器数据
    struct LayerTimerData
    {
        ATL::CWindow * m_pWindow = nullptr;
        GraphicLayer * m_pLayer = nullptr;
        HANDLE m_hTimer = nullptr;
    };
    HANDLE m_hTimerQueue;
    ATL::CWin32Heap m_timerDataHeap;
    std::unordered_multimap<GraphicLayer*, LayerTimerData*> m_timerData;

//----lua脚本----------------------------------------------
    //lua_state_wrapper不能多线程使用,因此一个窗口一个,多窗口共享易出错
    std::unordered_map<std::wstring, lua_state_wrapper> m_luaPool;

//----皮肤----------------------------------------------------
    std::shared_ptr<std::unordered_map<std::wstring, TBitmapInfo> > m_spSkinMap;

};

SHARELIB_END_NAMESPACE

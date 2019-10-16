#pragma once
#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <unordered_map>
#include <atlbase.h>
#include <atltypes.h>
#include <d2d1helper.h>
#include "DataStructure/tree_node.h"
#include "GraphicLayerMsgDef.h"
#include "GraphicLayerTypeDef.h"
#include "Other/RuntimeDynamicCreate.h"
#include "UI/DirectX/D2D1Misc.h"
#include "UI/DirectX/D2DRenderPack.h"
#include "WTL/atlapp.h"
#include "WTL/atlcrack.h"
#include "pugixml/pugixml.hpp"

//不能提供给外部使用，因为每个Layer都可能注册自己到RootLayer中，一旦切断联系，会导致野指针。
//除非先析构，反注册完毕之后才能切断联系。
#pragma deprecated(remove_tree_node)

SHARELIB_BEGIN_NAMESPACE

class GraphicRootLayer;

struct __declspec(novtable) IFastPaintHelper
{
    //此接口必须放在类中第一个位置
    virtual void BeforePaintChildren(D2DRenderPack & /*d2dRender*/) = 0;
};

/* 
 配置Layer属性时,要先把它加入到树中,否则可能不生效!
 */
class GraphicLayer
    : public tree_node<GraphicLayer>
    , public LayerMsgCallback
    , public IFastPaintHelper
{
    DECLARE_RUNTIME_DYNAMIC_CREATE(GraphicLayer, L"layer")

public:
    GraphicLayer();
    virtual ~GraphicLayer();

    //----位置、区域--------------------------------------------

    // 自己的坐标原点在父对象中的坐标, 注意理解：父Layer中的坐标!!
    const CPoint &GetOrigin() const;

    // 设置自己的坐标原点在父对象中的坐标
    void SetOrigin(const CPoint &pt, bool bRedraw = true);

    // 相对偏移自己的原点
    void OffsetOrigin(const CPoint &pt, bool bRedraw = true);

    // 对齐方式
    enum TAlignType
    {
        NO_ALIGN = -1,  //不执行对齐
        ALIGN_NEAR = 0, //左（上）边对齐
        ALIGN_CENTER,   //居中对齐
        ALIGN_FAR,      //右（下）边对齐
        ALIGN_FRONT,    //右（下）边与基准对象的左（上）边对齐
        ALIGN_BACK,     //左（上）边与基准对象的右（下）边对齐
    };

    /** 对齐设置自己的位置, 如果xAlign或yAlign不在TAlignType预定义的值当中, 则不对齐
    @param[in] xAlign 水平对齐方式
    @param[in] yAlign 垂直对齐方式
    @param[in] pDatum 基准对象
    @param[in] bRedraw 是否重绘
    @return 操作是否成功
    */
    bool AlignXY(TAlignType xAlign, TAlignType yAlign, GraphicLayer *pDatum, bool bRedraw = true);

    // Layer的矩形区域,默认情况下点击响应及绘制都限制在此范围内,相对自己的坐标
    const CRect &GetLayerBounds() const;

    // 设置Layer的矩形区域,默认情况下点击响应及绘制都限制在此范围内,相对自己的坐标
    void SetLayerBounds(const CRect &rcBounds, bool bRedraw = true);

    //设置排布的lua脚本的名字
    void SetLayoutLua(const std::wstring &layoutLua);
    std::wstring GetLayoutLua() const;

    //点击测试,相对自己的坐标
    virtual bool HitTest(const CPoint &pt) const;

    //重新排布自身及所有子Layer,窗口大小改变时,会调用根Layer的Relayout
    void Relayout();

    /** 重新排布消息响应, 默认是的行为是调用 lrLayoutLua 中指定的lua脚本
    @param[in] bLayoutChildren 是否排布子Layer,默认为true
    */
    virtual bool OnLayerMsgLayout(GraphicLayer *pLayer, bool &bLayoutChildren) override;

    //----坐标转换-------------------------------------
    /** \static
    */

    /** 根据点坐标查找子孙Layer,找不到返回nullptr.
    查找逻辑：1. IsVisible && IsEnable && HitTest 为true时，才会被考虑在内；
            2. IsTransparent 为true的跳过，继续查找子Layer；
            3. 遇到 IsInterceptChildren 为true时，停止，不再继续查找子Layer；
    @param[in] pParent 父Layer,查找时不包含在内
    @param[in,out] pt in,父Layer中的坐标值;out,转换为找到的子孙Layer中的坐标值
    */
    static GraphicLayer *FindChildLayerFromPoint(GraphicLayer *pParent, CPoint &pt);

    // 把pLayer中的坐标值[in,out]pt转换到根窗口中的坐标（等同于window的客户区坐标）
    static void MapPointToRoot(const GraphicLayer *pLayer, POINT *pPt, size_t nCount = 1);

    // 把根窗口中的坐标（等同于window的客户区坐标）[in,out]ptRoot转换到目标Layer中的坐标
    static void MapRootPointToLayer(const GraphicLayer *pLayer, POINT *pPtRoot, size_t nCount = 1);

    // 把pLayerSrc中的坐标[in,out]pt转换到pLayerDst中的坐标
    static void MapPointToLayer(const GraphicLayer *pLayerSrc,
                                const GraphicLayer *pLayerDst,
                                POINT *pPt,
                                size_t nCount = 1);

    //----鼠标消息---------------------------------------

    // 设置是否捕获该Layer，设置之后，鼠标消息全都发到这个窗口。
    void SetMouseCapture(bool bCapture);

    // 配置是否允许鼠标拖动Layer，默认不允许
    void SetEnableMouseDragMove(bool bEnableDragMouve);

    // 配置是否允许拖动窗口，默认不允许，当它和SetEnableMouseDragMove都打开时，本属性覆盖SetEnableMouseDragMove
    void SetEnableDragWindow(bool bEnableMouveWindow);

    /** 设置自己的MouseInOut状态是否关联到父Layer，关联的情况下，如果鼠标进入自己，
        则认为同时也进入了父Layer；
    */
    void SetMouseInOutLinkParent(bool bLink);
    bool IsMouseInOutLinkParent() const;

    /* transparent为true时，自己不响应鼠标键盘事件，但子结点不受影响；
    与之对比，enale为false，自己和所有子结点都不再影响鼠标、键盘事件；
    另外，visible,enable优先级高于transparent
    */
    void SetTransparent(bool isEnable);
    bool IsTransparent() const;

    /*配置是否拦截子Layer的鼠标消息,为true时, 如果鼠标落在该Layer里面,消息发给该Layer,
      不会继续查找它的子Layer.
    */
    void SetInterceptChildren(bool bIntercept);

    // 是否拦截子Layer的鼠标消息
    bool IsInterceptChildren() const;

    //----键盘消息---------------------------------------

    //设置是否允许被设置焦点,Layer被设为焦点时,才能接收到键盘消息
    void SetEnableFocus(bool isEnableFocus);
    bool IsEnableFocus();

    //设置焦点,会自动将EnableFocus属性置true
    void SetFocus();
    void ClearFocus();
    bool IsFocus();

    //----绘制-------------------------------------------

    bool IsVisible() const;
    bool IsEnable() const;

    //visible为false的Layer，不绘制也不响应鼠标键盘事件
    void SetVisible(bool isVisible);

    //enable为false的Layer绘制，但不响应鼠标键盘事件
    void SetEnable(bool isEnable);

    //背景色，会在调用Paint前先用背景色填充整个区域，如果背景色alpha分量为0，则不填充
    void SetBgColor(COLOR32 crbg); //b8g8r8a8颜色值
    COLOR32 GetBgColor() const;

    virtual void Paint(D2DRenderPack & /*d2dRender*/){};
    virtual void BeforePaintChildren(D2DRenderPack & /*d2dRender*/) override{};
    virtual void AfterPaintChildren(D2DRenderPack & /*d2dRender*/){};
    virtual bool IsPaintChildrenTrivial() { return false; }; //改写的BeforePaintChildren是否无关紧要

    //默认使用GetLayerBounds作为重绘区域, 这里是相对自身的坐标
    void SchedulePaint(const CRect *pRcDraw = nullptr);

    // 是否可以作为绘制起点，窗口重绘的时候，优先从绘制起点开始绘制，而不是从根Layer
    static bool MayItBePaintStart(const GraphicLayer *pLayer);

    //----窗口--------------------------------------------------------

    //获取根结点GraphicRootLayer,如果结点没有加入到整个树中,那么root()获取到的与GetRootGraphicLayer()便不相同
    GraphicRootLayer *GetRootGraphicLayer() const;

    //获取绑定的窗口
    ATL::CWindow *GetMSWindow() const;

    // 修改自己在结点树中的位置，自己相对根窗口客户区的坐标保持不变
    void ChangeMyPosInTheTree(GraphicLayer *pTarget, TInsertPos insertType);

    // 把自己置于兄弟Layer中的顶层位置
    void BringToTop();

    // 把自己置于所有Layer的顶层位置
    void BringToTopMost();

    //----定时器, 定时器消息: OnLayerMsgTimer------------------------------------------------------------

    HANDLE CreateLayerTimer(uint32_t nPeriod);
    void DestroyLayerTimer(HANDLE hTimer);
    void DestroyAllLayerTimers();

    //----消息、钩子、监控（时序：钩子————正常处理函数————监控）----------------------

    /** 向控件发消息
    @param[in] pFn LayerMsgCallback中的消息响应函数指针
    @param[in] ...param 响应函数参数
    @return 消息是否处理了
    */
    template<class TFn, class... TParam>
    bool SendLayerMsg(TFn &&pFn, TParam &&... param)
    {
        //hooker
        if (m_spHookers && !m_spHookers->empty()) {
            for (auto it = m_spHookers->begin(); it != m_spHookers->end();) {
                if (*it) {
                    if (((*it)->*pFn)(this, std::forward<TParam>(param)...)) {
                        return true;
                    }
                    ++it;
                } else {
                    it = m_spHookers->erase(it);
                }
            }
        }

        //本身处理
        bool bHandled = (this->*pFn)(this, std::forward<TParam>(param)...);

        //observer
        if (m_spObservers && !m_spObservers->empty()) {
            for (auto it = m_spObservers->begin(); it != m_spObservers->end();) {
                if (*it) {
                    bHandled |= ((*it)->*pFn)(this, std::forward<TParam>(param)...);
                    ++it;
                } else {
                    it = m_spObservers->erase(it);
                }
            }
        }
        return bHandled;
    }

    //钩子:处理过此消息则拦截,优先级最高,会导致后面的hook,正常消息处理及所有监控全部失效.后添加的钩子先被调用.
    void AddLayerMsgHook(LayerMsgCallback *pHook);
    void RemoveLayerMsgHook(LayerMsgCallback *pHook);
    void RemoveAllLayerMsgHook();

    //监控:正常消息处理之后,再执行所有的监控.先添加的监控者先被调用.
    void AddLayerMsgObserver(LayerMsgCallback *pObserver);
    void RemoveLayerMsgObserver(LayerMsgCallback *pObserver);
    void RemoveAllLayerMsgObserver();

    //----xml属性配置-----------------------------------------------------

    //注册类信息的辅助函数
    static GraphicLayer *CreateGraphicLayerByName(const wchar_t *pCreateKay);

    /* 从xml加载
    1. 加载之前会删除所有的子结点；
    2. 重新构建所有子结点，并且读取自己及所有子结点属性；
    3. 如果是新创建的子结点，先加入到结点树中，再Load，否则有些属性设置不能生效；
    */
    bool LoadFromXml(const wchar_t *pFilePath,
                     pugi::xml_encoding encoding = pugi::xml_encoding::encoding_auto);
    bool LoadFromXml(const void *pBuffer,
                     size_t nSize,
                     pugi::xml_encoding encoding = pugi::xml_encoding::encoding_auto);

    /* 读取所有属性
    只读取自身的所有属性，不创建子结点也不会加载子结点的属性。实际通过调用 OnReadingXmlAttribute 和
    OnReadXmlAttributeEnded 来实现
    */
    bool ReadAllXmlAttributes(const pugi::xml_node &node);
    bool ReadAllXmlAttributes(const std::wstring &xmlText);

    /* 读取单项属性值
ReadAllXmlAttributes时的回调，各个子类具体实现。注意，如果是自己不需要的属性，必须调用基类的实现，避免属性读取遗漏。
属性注释格式, 绰号内表示属性名，GraphicLayer体系的属性名全都以lr(取Layer的发音简写)开头，
小写，后面的单词首字母大写，全拼，不用简写，并且区分大小写。
""                    : <格式><类型><含义><默认值>
"lrID"                : <无符号整型><size_t><用户自定义ID><0>
"lrColor"             : <16进制COLOR32,argb,alpha通道不写则为255><uint32_t><背景颜色><0>
"lrOrigin"            : <x,y><LONG><坐标原点，相对父Layer的坐标><0,0>
"lrBounds"            : <left,top,right,bottom><LONG><区域，相对自身坐标><0,0,0,0>
"lrVisible"           : <0或1><bool，写0或1，true或false都可以，其它bool相同，不再缀述><是否可见><1>
"lrEnable"            : <0或1><bool><enable属性><1>
"lrTransparent"       : <0或1><bool><Transparent属性><0>
"lrInterceptChildren" : <0或1><bool><InterceptChildren属性><0>
"lrMouseLinkParent"   : <0或1><bool><SetMouseInOutLinkParent><0>
"lrDragMove"          : <0或1><bool><SetEnableMouseDragMove><0>
"lrDragWindow"        : <0或1><bool><SetEnableDragWindow><0>
"lrEnableFocus"       : <0或1><bool><SetEnableFocus><0>
"lrLayoutLua"         : <字符串><string><lua脚本的名字><>
*/
    virtual void OnReadingXmlAttribute(const pugi::xml_attribute &attr);

    //ReadAllXmlAttributes时的回调，表示属性读取完毕。返回true表示继续遍历子结点，否则不再遍历子结点
    virtual bool OnReadXmlAttributeEnded(const pugi::xml_node &node);

    //写入属性到xml文件,包括自己及所有的子孙结点
    bool SaveToXml(const wchar_t *pFilePath,
                   pugi::xml_encoding encoding = pugi::xml_encoding::encoding_auto);

    //写入属性到xml结点中，SaveToXml时的回调，各个子类具体实现，注意写完自己的要调用基类的函数。
    virtual void OnWritingAttributeToXml(pugi::xml_node node);

    //SaveToXml时的回调，表示属性写入完毕。返回true表示继续遍历子结点，否则不再遍历子结点
    virtual bool OnWritingAttributeToXmlEnded(pugi::xml_node node);

    //动态创建辅助函数: 类信息注册
    static void RegisterLayerClassesInfo(const wchar_t *pCreateKay,
                                         TRuntimeCreateFunction pfnCreateFunc);

    //----UserData-----------------------------------------------------------------

    void SetUserData(void *pUserData) { m_pUserData = pUserData; }
    void *GetUserData() { return m_pUserData; }

    void SetID(size_t nID) { m_nID = nID; };
    size_t GetID() { return m_nID; }

    // 用ID查找子Layer,深度优先递归遍历查找
    GraphicLayer *FindChildByID(size_t nID);

protected:
    //用LAYER_WM_INTERNAL_USE给自己发消息的响应函数,该消息内部使用,可以用来实现一些异步处理
    virtual void OnLayerInternalMsg(LPARAM /*lParam*/){};

private:
    static std::unordered_map<std::wstring, TRuntimeCreateFunction> &GetLayersMap();

    friend class GraphicRootLayer;

    CPoint m_origin;
    CRect m_bounds;

    enum TBitConfig
    {
        BIT_VISIBLE = 0,
        BIT_ENABLE,
        BIT_TRANSPARENT,
        BIT_INTERCEPT_CHILDREN,
        BIT_MOUSE_INOUT_LINK_PARENT,
        BIT_MOUSE_DRAG_MOVE,
        BIT_DRAG_WINDOW,
        BIT_ENABLE_FOCUS,
        BIT_ROOT_LAYER,

        BIT_MAX_COUNT
    };
    bool m_bitConfig[TBitConfig::BIT_MAX_COUNT];

    COLOR32 m_crbg;

    std::unique_ptr<std::list<LayerMsgCallback *>> m_spHookers;
    std::unique_ptr<std::list<LayerMsgCallback *>> m_spObservers;

    //lua排布脚本
    std::wstring m_layoutLua;

    size_t m_nID = 0;
    void *m_pUserData = nullptr;
};

SHARELIB_END_NAMESPACE

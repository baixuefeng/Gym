#pragma once
#include <windows.h>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

/*!
 * \class GestureRecognize
 * \brief 鼠标手势识别算法
 */
class GestureRecognize
{
public:
    enum Direction
    {
        UNRECOGNIZABLE = -1, //无法判断
        Y_NEGATIVE,          //y轴负向
        Y_POSITIVE,          //y轴正向
        X_NEGATIVE,          //x轴负向
        X_POSITIVE           //x轴正向
    };

    GestureRecognize();
    ~GestureRecognize();

    // 设置识别灵敏度，默认是15
    void SetRecognizeSensitivity(int nSensityvity);

    // 清除原来的历史数据，设置识别起点，开始分析
    void BeginParse(const POINT &beginPoint);

    // 输入当前点，识别当前的方向
    Direction ParseDirection(const POINT &point);

    // 方向是否发生了改变
    bool IsDirectionChanged();

    // 获取上一个点
    POINT GetLastPoint();

private:
    // 清空
    void Clear();

    // 结合当前方向和历史数据识别方向
    Direction ParseHistory(Direction curDirect);

private:
    //识别灵敏度
    int m_iSensitivity;

    //上一个点
    POINT m_lastPoint;

    //四个方向上的移动增量
    int m_iDirectIncrement[4];

    //最后一个方向
    Direction m_lastDirect;

    //方向是否改变
    bool m_bDirectionChanged;
};

SHARELIB_END_NAMESPACE

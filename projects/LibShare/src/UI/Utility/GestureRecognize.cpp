#include "targetver.h"
#include "UI/Utility/GestureRecognize.h"

#include <cstring>
#include <cmath>

SHARELIB_BEGIN_NAMESPACE

GestureRecognize::GestureRecognize()
{
	m_iSensitivity = 15;//识别灵敏度
	Clear();
}

GestureRecognize::~GestureRecognize()
{
}

void GestureRecognize::Clear()
{
	m_lastPoint.x = 0;//上一个点
	m_lastPoint.y = 0;
	std::memset(m_iDirectIncrement, 0, sizeof(m_iDirectIncrement));//四个方向上的移动增量
    m_lastDirect = Direction::UNRECOGNIZABLE;//最后一个方向
	m_bDirectionChanged = false;//方向是否改变
}

void GestureRecognize::SetRecognizeSensitivity(int nSensityvity)
{
	if (nSensityvity > 1)
	{
		m_iSensitivity = nSensityvity;
	}
}

void GestureRecognize::BeginParse(const POINT & beginPoint)
{
	Clear();
    m_lastPoint = beginPoint;
}

GestureRecognize::Direction GestureRecognize::ParseDirection(const POINT & point)
{
    if (m_lastPoint.x == 0 && m_lastPoint.y == 0)
    {
        return Direction::UNRECOGNIZABLE;
    }
    //计算增量方向，只计算增量最大的方向，其他的都过滤掉
	Direction incrementDirection = Direction::UNRECOGNIZABLE;
	int iDx = point.x - m_lastPoint.x;
	int iDy = point.y - m_lastPoint.y;
	if (std::abs(iDx) > std::abs(iDy))
    {
        if (iDx < 0)
        {
			incrementDirection = Direction::X_NEGATIVE;
			m_iDirectIncrement[Direction::X_NEGATIVE] += -iDx;
        }
        else
        {
			incrementDirection = Direction::X_POSITIVE;
			m_iDirectIncrement[Direction::X_POSITIVE] += iDx;
        }
    }
    else
    {
        if (iDy < 0)
        {
			incrementDirection = Direction::Y_NEGATIVE;
			m_iDirectIncrement[Direction::Y_NEGATIVE] += -iDy;
        }
        else
        {
			incrementDirection = Direction::Y_POSITIVE;
			m_iDirectIncrement[Direction::Y_POSITIVE] += iDy;
        }
    }
    m_lastPoint = point;

	Direction result = ParseHistory(incrementDirection);

    if (result >= 0 && result != m_lastDirect)
    {
        m_bDirectionChanged = true;
    }
    else
    {
        m_bDirectionChanged = false;
    }
    m_lastDirect = result;

    return result;
}

GestureRecognize::Direction GestureRecognize::ParseHistory(Direction curDirect)
{
	if (m_iDirectIncrement[curDirect] > m_iSensitivity)
    {
        //该方向上的增量已经超过识别灵敏度，方向改变，清空其它方向上的增量
        for (int i = 0; i < 4; ++i)
        {
			if (i != curDirect)
            {
                m_iDirectIncrement[i] = 0;
            }
        }
		return curDirect;
    }
    else
    {
        //该方向上的增量还不够，维持原来的方向
        for (int i = 0; i < 4; ++i)
        {
			if (m_iDirectIncrement[i] > m_iSensitivity)
            {
                return Direction(i);
            }
        }
        return Direction::UNRECOGNIZABLE;
    }
}

POINT GestureRecognize::GetLastPoint()
{
    return m_lastPoint;
}

bool GestureRecognize::IsDirectionChanged()
{
    return m_bDirectionChanged;
}

SHARELIB_END_NAMESPACE

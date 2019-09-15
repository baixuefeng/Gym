#include "targetver.h"
#include "TestUtility/TestLRValue.h"
#include "log/TempLog.h"

SHARELIB_BEGIN_NAMESPACE

LRValue::LRValue(int n)
    : m_n(n)
{
    tcoutex << m_n << L"\n";
}

LRValue::~LRValue()
{
    tcoutex << m_n << L"\n";
}

LRValue::LRValue(const LRValue &other)
{
    m_n = other.m_n + 1;
    tcoutex << m_n << L"\n";
}

LRValue::LRValue(LRValue &&other)
{
    m_n = other.m_n;
    other.m_n = 0;
    tcoutex << m_n << L"\n";
}

LRValue &LRValue::operator=(const LRValue &other)
{
    m_n = other.m_n + 1;
    tcoutex << m_n << L"\n";
    return *this;
}

LRValue &LRValue::operator=(LRValue &&other)
{
    m_n = other.m_n;
    other.m_n = 0;
    tcoutex << m_n << L"\n";
    return *this;
}

SHARELIB_END_NAMESPACE

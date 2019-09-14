#pragma once

#include <ostream>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

struct FakeOstream
{
    template<class T> FakeOstream & operator <<(T &&)
    {
        return *this;
    }
    FakeOstream & operator<<(std::wostream & (*)(std::wostream &))
    {
        return *this;
    }
    FakeOstream & Printf(...)
    {
        return *this;
    }
};

template<class T>
FakeOstream & operator << (FakeOstream && os, T &&)
{
    return os;
}

SHARELIB_END_NAMESPACE

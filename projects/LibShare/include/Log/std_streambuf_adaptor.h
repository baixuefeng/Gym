#pragma once
#include <cassert>
#include <iosfwd>
#include <streambuf>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

template<class _CharType, class _Traits = std::char_traits<_CharType>>
class fix_length_buf_adaptor : public std::basic_streambuf<_CharType, _Traits>
{
    using _BaseType = std::basic_streambuf<_CharType, _Traits>;

public:
    fix_length_buf_adaptor() {}

    fix_length_buf_adaptor(char_type *pBuf, std::streamsize nSize)
    {
        _BaseType::pubsetbuf(pBuf, nSize);
    }

    char_type *get_buffer() const { return pbase(); }

    std::streamsize get_buffer_size() const { return epptr() - pbase(); }

protected:
    /** offer buffer to external agent
    @param[in] pBuf 缓冲区指针
    @param[in] nSize 缓冲区大小(字符数, count of char_type)
    */
    virtual _BaseType *setbuf(char_type *pBuf, std::streamsize nSize) override
    {
        assert(pBuf);
        assert(nSize > 0);
        setg(pBuf, pBuf, pBuf + nSize);
        setp(pBuf, pBuf, pBuf + nSize);
        return (this);
    }

    // change position by offset, according to way and mode
    virtual pos_type seekoff(off_type nOff,
                             std::ios_base::seekdir di,
                             std::ios_base::openmode mode) override
    {
        pos_type pos = -1;
        if ((mode & std::ios_base::in) && (_BaseType::gptr() != 0)) {
            assert(!(mode & std::ios_base::out));
            switch (di) {
            case std::ios_base::beg:
                pos = nOff;
                break;
            case std::ios_base::cur:
                pos = (off_type)(_BaseType::gptr() - _BaseType::eback() + nOff);
                break;
            case std::ios_base::end:
                pos = (off_type)(_BaseType::egptr() - _BaseType::eback() + nOff);
                break;
            default:
                assert(0);
            }
            if ((pos >= 0) && (_BaseType::eback() + (ptrdiff_t)pos <= _BaseType::egptr())) {
                _BaseType::gbump((int)(_BaseType::eback() - _BaseType::gptr() + pos));
            } else {
                pos = -1;
            }
        } else if ((mode & std::ios_base::out) && (_BaseType::pptr() != 0)) {
            assert(!(mode & std::ios_base::in));
            switch (di) {
            case std::ios_base::beg:
                pos = nOff;
                break;
            case std::ios_base::cur:
                pos = (off_type)(_BaseType::pptr() - _BaseType::pbase() + nOff);
                break;
            case std::ios_base::end:
                pos = (off_type)(_BaseType::epptr() - _BaseType::pbase() + nOff);
                break;
            default:
                assert(0);
            }
            if ((pos >= 0) && (_BaseType::pbase() + (ptrdiff_t)pos <= _BaseType::epptr())) {
                _BaseType::pbump((int)(_BaseType::pbase() - _BaseType::pptr() + pos));
            } else {
                pos = -1;
            }
        }
        return pos;
    }

    // change to specified position, according to mode
    virtual pos_type seekpos(pos_type nPos, std::ios_base::openmode mode) override
    {
        return seekoff((off_type)nPos, std::ios_base::beg, mode);
    }
};

using fix_buf = fix_length_buf_adaptor<char>;
using wfix_buf = fix_length_buf_adaptor<wchar_t>;

SHARELIB_END_NAMESPACE

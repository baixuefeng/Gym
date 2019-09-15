#include "Memory/compress_utility.h"
#include <cassert>
#include <memory>
#include "zlib.h"

SHARELIB_BEGIN_NAMESPACE

#define CACHE_BUFFER_SIZE 8192

namespace {
bool compress_data_impl(const void *pInput,
                        uint32_t nLength,
                        std::vector<uint8_t> &output,
                        int nLevel,
                        int windowBits)
{
    output.clear();
    if (!pInput || nLength == 0)
    {
        return true;
    }
    z_stream strm{0};
    int err =
        ::deflateInit2(&strm, nLevel, Z_DEFLATED, windowBits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (err != Z_OK)
    {
        return false;
    }

    std::unique_ptr<uint8_t[]> spBuffer(new uint8_t[CACHE_BUFFER_SIZE]{});
    strm.avail_in = nLength;
    strm.next_in = (Bytef *)pInput;
    strm.avail_out = CACHE_BUFFER_SIZE;
    strm.next_out = spBuffer.get();

    int flushFlag = Z_NO_FLUSH;
    do
    {
        err = ::deflate(&strm, flushFlag);
        if (err == Z_OK || (flushFlag == Z_FINISH && err == Z_BUF_ERROR))
        {
            if (strm.avail_out == 0)
            {
                output.insert(output.end(), spBuffer.get(), spBuffer.get() + CACHE_BUFFER_SIZE);
                strm.avail_out = CACHE_BUFFER_SIZE;
                strm.next_out = spBuffer.get();
            }
            else if (flushFlag == Z_NO_FLUSH && strm.avail_in == 0)
            {
                flushFlag = Z_FINISH;
            }
        }
        else if (flushFlag == Z_FINISH && err == Z_STREAM_END)
        {
            output.insert(
                output.end(), spBuffer.get(), spBuffer.get() + CACHE_BUFFER_SIZE - strm.avail_out);
            break;
        }
    } while (err == Z_OK);

    if (::deflateEnd(&strm) == Z_OK && err == Z_STREAM_END)
    {
        return true;
    }
    else
    {
        output.clear();
        return false;
    }
}

bool compress_data_impl(const void *pInput,
                        uint32_t nLength,
                        void *pOutput,
                        uint32_t &nOutLength,
                        int nLevel,
                        int windowBits)
{
    if (!pInput || nLength == 0)
    {
        return true;
    }
    assert(pOutput && nOutLength > 6);
    if (!pOutput || nOutLength <= 6)
    {
        return false;
    }
    z_stream strm{0};
    int err =
        ::deflateInit2(&strm, nLevel, Z_DEFLATED, windowBits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (err != Z_OK)
    {
        return false;
    }

    strm.avail_in = nLength;
    strm.next_in = (Bytef *)pInput;
    strm.avail_out = nOutLength;
    strm.next_out = (Bytef *)pOutput;

    err = ::deflate(&strm, Z_FINISH);
    if (::deflateEnd(&strm) == Z_OK && err == Z_STREAM_END)
    {
        nOutLength = strm.total_out;
        return true;
    }
    else
    {
        return false;
    }
}

} // namespace

bool zlib_compress_data(const void *pInput,
                        uint32_t nLength,
                        std::vector<uint8_t> &output,
                        int nLevel /* = 6*/)
{
    return compress_data_impl(pInput, nLength, output, nLevel, MAX_WBITS);
}

bool zlib_compress_data(const void *pInput,
                        uint32_t nLength,
                        void *pOutput,
                        uint32_t &nOutLength,
                        int nLevel /*= 6*/)
{
    return compress_data_impl(pInput, nLength, pOutput, nOutLength, nLevel, MAX_WBITS);
}

bool gzip_compress_data(const void *pInput,
                        uint32_t nLength,
                        std::vector<uint8_t> &output,
                        int nLevel /* = 6*/)
{
    //MAX_WBITS + 16, gz压缩方式
    return compress_data_impl(pInput, nLength, output, nLevel, MAX_WBITS + 16);
}

bool gzip_compress_data(const void *pInput,
                        uint32_t nLength,
                        void *pOutput,
                        uint32_t &nOutLength,
                        int nLevel /*= 6*/)
{
    //MAX_WBITS + 16, gz压缩方式
    return compress_data_impl(pInput, nLength, pOutput, nOutLength, nLevel, MAX_WBITS + 16);
}

bool decompress_data(const void *pInput, uint32_t nLength, std::vector<uint8_t> &output)
{
    output.clear();
    if (!pInput || (nLength == 0))
    {
        return true;
    }
    z_stream strm{0};
    //Add 32 to windowBits to enable zlib and gzip decoding with automatic header detection.
    int err = ::inflateInit2(&strm, MAX_WBITS + 32);
    if (err != Z_OK)
    {
        return false;
    }
    std::unique_ptr<uint8_t[]> spBuffer(new uint8_t[CACHE_BUFFER_SIZE]{});
    strm.avail_in = nLength;
    strm.next_in = (Bytef *)pInput;
    strm.avail_out = CACHE_BUFFER_SIZE;
    strm.next_out = spBuffer.get();
    do
    {
        err = ::inflate(&strm, Z_NO_FLUSH);
        if (err == Z_OK && strm.avail_out == 0)
        {
            output.insert(output.end(), spBuffer.get(), spBuffer.get() + CACHE_BUFFER_SIZE);
            strm.avail_out = CACHE_BUFFER_SIZE;
            strm.next_out = spBuffer.get();
        }
        else if (err == Z_STREAM_END)
        {
            output.insert(
                output.end(), spBuffer.get(), spBuffer.get() + CACHE_BUFFER_SIZE - strm.avail_out);
            break;
        }
    } while (err == Z_OK);

    if (::inflateEnd(&strm) == Z_OK && err == Z_STREAM_END)
    {
        return true;
    }
    else
    {
        output.clear();
        return false;
    }
}

bool decompress_data(const void *pInput, uint32_t nLength, void *pOutput, uint32_t &nOutLength)
{
    if (!pInput || (nLength == 0))
    {
        return true;
    }
    assert(pOutput && nOutLength > 6);
    if (!pOutput || nOutLength <= 6)
    {
        return false;
    }
    z_stream strm{0};
    //Add 32 to windowBits to enable zlib and gzip decoding with automatic header detection.
    int err = ::inflateInit2(&strm, MAX_WBITS + 32);
    if (err != Z_OK)
    {
        return false;
    }
    strm.avail_in = nLength;
    strm.next_in = (Bytef *)pInput;
    strm.avail_out = nOutLength;
    strm.next_out = (Bytef *)pOutput;
    err = ::inflate(&strm, Z_FINISH);

    if (::inflateEnd(&strm) == Z_OK && err == Z_STREAM_END)
    {
        nOutLength = strm.total_out;
        return true;
    }
    else
    {
        return false;
    }
}

SHARELIB_END_NAMESPACE

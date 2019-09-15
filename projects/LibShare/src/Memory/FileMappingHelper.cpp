#include "Memory/FileMappingHelper.h"
#include <cassert>
#include <cstdint>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

SHARELIB_BEGIN_NAMESPACE

FileMappingHelper::FileMappingHelper(const wchar_t *pFile)
    : m_fileSize(0)
    , m_writedSize(0)
    , m_readSize(0)
{
    boost::filesystem::path filePath{pFile};
    Init(filePath);
}

FileMappingHelper::FileMappingHelper(const char *pFile)
    : m_fileSize(0)
    , m_writedSize(0)
    , m_readSize(0)
{
    boost::filesystem::path filePath{pFile};
    Init(filePath);
}

FileMappingHelper::~FileMappingHelper()
{
    m_spMappedRegion.reset();
    m_spFileMapping.reset();
}

uint64_t FileMappingHelper::GetFileSize()
{
    return m_fileSize;
}

void FileMappingHelper::SeekWrite(int64_t offset, TSeekType type /*= TSeekType::TSEEK_CURRENT*/)
{
    SeekImpl(m_writedSize, offset, type);
}

void FileMappingHelper::SeekRead(int64_t offset, TSeekType type /*= TSeekType::TSEEK_CURRENT*/)
{
    SeekImpl(m_readSize, offset, type);
}

uint64_t FileMappingHelper::GetWritePos()
{
    return m_writedSize;
}

uint64_t FileMappingHelper::GetReadPos()
{
    return m_readSize;
}

bool FileMappingHelper::WriteBytes(const void *pBits, size_t nBytes)
{
    if (!pBits || !nBytes)
    {
        return true;
    }
    if (m_writedSize + nBytes > m_fileSize)
    {
        return false;
    }
    try
    {
        m_spMappedRegion.reset();
        boost::interprocess::mapped_region region{*m_spFileMapping,
                                                  boost::interprocess::mode_t::read_write,
                                                  (boost::interprocess::offset_t)m_writedSize,
                                                  nBytes};
        std::memcpy(region.get_address(), pBits, nBytes);
        m_writedSize += nBytes;
        return true;
    }
    catch (const std::exception &)
    {
        assert(!"WriteBytes Failed");
        return false;
    }
}

size_t FileMappingHelper::ReadBytes(void *pBuffer, size_t bufferSize)
{
    if (!pBuffer || (bufferSize == 0))
    {
        return 0;
    }
    size_t copySize = (size_t)(std::min)(m_fileSize - m_readSize, (uint64_t)bufferSize);
    if (copySize == 0)
    {
        return 0;
    }
    try
    {
        m_spMappedRegion.reset();
        boost::interprocess::mapped_region region{*m_spFileMapping,
                                                  boost::interprocess::mode_t::read_write,
                                                  (boost::interprocess::offset_t)m_readSize,
                                                  copySize};
        std::memcpy(pBuffer, region.get_address(), copySize);
        m_readSize += copySize;
        return copySize;
    }
    catch (const std::exception &)
    {
        assert(!"ReadBytes Failed");
        return 0;
    }
}

void *FileMappingHelper::LockBits(uint64_t offset, size_t nLockBytes)
{
    if (m_fileSize < offset + (uint64_t)nLockBytes)
    {
        return nullptr;
    }
    try
    {
        m_spMappedRegion = std::make_unique<boost::interprocess::mapped_region>(
            *m_spFileMapping,
            boost::interprocess::mode_t::read_write,
            (boost::interprocess::offset_t)offset,
            nLockBytes);
        return m_spMappedRegion->get_address();
    }
    catch (const std::exception &)
    {
        assert(!"LockBits Failed");
        return nullptr;
    }
}

void FileMappingHelper::UnLockBits()
{
    m_spMappedRegion.reset();
}

void FileMappingHelper::Init(const boost::filesystem::path &filePath)
{
    m_spFileMapping = std::make_unique<boost::interprocess::file_mapping>(
        filePath.string().c_str(), boost::interprocess::mode_t::read_write);
    m_fileSize = boost::filesystem::file_size(filePath);
}

void FileMappingHelper::SeekImpl(uint64_t &prevSize, int64_t offset, TSeekType type)
{
    switch (type)
    {
    case FileMappingHelper::TSeekType::SEEK_TYPE_BEGIN:
        if (offset <= 0)
        {
            prevSize = 0;
        }
        else if ((uint64_t)offset >= m_fileSize)
        {
            prevSize = m_fileSize;
        }
        else
        {
            prevSize = offset;
        }
        break;
    case FileMappingHelper::TSeekType::SEEK_TYPE_CURRENT:
        if (offset >= 0)
        {
            if (prevSize + offset >= m_fileSize)
            {
                prevSize = m_fileSize;
            }
            else
            {
                prevSize += offset;
            }
        }
        else
        {
            if ((uint64_t)std::abs(offset) >= prevSize)
            {
                prevSize = 0;
            }
            else
            {
                prevSize += offset;
            }
        }
        break;
    case FileMappingHelper::TSeekType::SEEK_TYPE_END:
        if (offset >= 0)
        {
            prevSize = m_fileSize;
        }
        else if ((uint64_t)std::abs(offset) >= m_fileSize)
        {
            prevSize = 0;
        }
        else
        {
            prevSize = m_fileSize + offset;
        }
        break;
    default:
        break;
    }
}

SHARELIB_END_NAMESPACE

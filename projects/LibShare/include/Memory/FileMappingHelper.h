#pragma once
#include <cstdint>
#include <memory>
#include <boost/core/noncopyable.hpp>
#include "MacroDefBase.h"

namespace boost {
namespace interprocess {
class file_mapping;
class mapped_region;
} // namespace interprocess

namespace filesystem {
class path;
}
} // namespace boost

SHARELIB_BEGIN_NAMESPACE

/** 该类内部使用FileMapping进行读写文件, 相比直接对文件读写, 速度快得多!
并且每次读写只映射文件的一部分, 而不是整个的文件, 因此内存占用也很小;
限制: 文件大小固定, 在传给该类之前已经固定好, 读、写都不能越界
*/
class FileMappingHelper : private boost::noncopyable
{
public:
    /** 构造函数
    @param[in] pFile 文件，如果映射失败抛出异常
    */
    explicit FileMappingHelper(const wchar_t *pFile);
    explicit FileMappingHelper(const char *pFile);

    ~FileMappingHelper();

    /** 获取文件大小
    */
    uint64_t GetFileSize();

    //----定位操作-----------------------------------------------------

    enum TSeekType
    {
        SEEK_TYPE_BEGIN,   //文件开头
        SEEK_TYPE_CURRENT, //当前位置
        SEEK_TYPE_END,     //文件结尾
    };

    /** 定位,这里的位置是内部自己实现的,和文件句柄中由系统记录的位置互不影响
    @param[in] offset 偏移
    @param[in] type 位置类型
    */
    void SeekWrite(int64_t offset, TSeekType type = TSeekType::SEEK_TYPE_CURRENT);
    void SeekRead(int64_t offset, TSeekType type = TSeekType::SEEK_TYPE_CURRENT);

    /** 获取当前位置
    */
    uint64_t GetWritePos();
    uint64_t GetReadPos();

    //----读写操作-----------------------------------------------------

    /** 写入数据,会自动更新写入位置.文件需要有 GENERIC_READ | GENERIC_WRITE 属性
    @param[in] pBits 数据指针
    @param[in] nBytes 数据大小
    @return 是否成功，如果失败，一个字节也不会写入
    */
    bool WriteBytes(const void *pBits, size_t nBytes);

    /** 读取数据,会自动更新读取位置,文件需要有 GENERIC_READ 属性
    @param[out] pBuffer 外部缓存
    @param[in] bufferSize 缓存大小
    @return 返回实际读取的数据大小
    */
    size_t ReadBytes(void *pBuffer, size_t bufferSize);

    /** 获取内存指针用于读写, 文件需要有 GENERIC_READ | GENERIC_WRITE 属性.
    注意: LockBits之后不要再次LockBits、WriteBytes或者ReadBytes,他们都会导致前一次获取的内存指针失效.
    @param[in] offset 偏移,从文件开头算起
    @param[in] nLockBytes 内存块大小,如果实际可操作内存小于该值，返回nullptr
    @return 内存指针,大小等于nLockBytes, 失败返回nullptr
    */
    void *LockBits(uint64_t offset, size_t nLockBytes);

    /** 释放通过LockBits所获取的内存,释放后内存指针失效
    */
    void UnLockBits();

private:
    /** 初始化
    @param[in] filePath 文件路径
    */
    void Init(const boost::filesystem::path &filePath);

    /** 定位
    @param[in,out] prevSize 更新之前的大小
    @param[in] offset 偏移值
    @param[in] type 位置类型
    */
    void SeekImpl(uint64_t &prevSize, int64_t offset, TSeekType type);

    /** 文件
    */
    std::unique_ptr<boost::interprocess::file_mapping> m_spFileMapping;

    /** 文件映射
    */
    std::unique_ptr<boost::interprocess::mapped_region> m_spMappedRegion;

    /** 文件大小
    */
    uint64_t m_fileSize;

    /** 已写入的大小
    */
    uint64_t m_writedSize;

    /** 已读取的大小
    */
    uint64_t m_readSize;
};

SHARELIB_END_NAMESPACE

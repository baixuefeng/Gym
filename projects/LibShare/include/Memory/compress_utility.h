#pragma once
#include <cstdint>
#include <vector>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE

/** zlib方式压缩数据
@param[in] pInput 原数据
@param[in] nLength 原数据长度
@param[in] output 压缩后数据
@param[in] nLevel 0~9,0表示不压缩,1表示速度最快,9表示压缩率最高
@return 是否成功
*/
bool zlib_compress_data(const void *pInput,
                        uint32_t nLength,
                        std::vector<uint8_t> &output,
                        int nLevel = 6);

/** zlib方式压缩数据
@param[in] pInput 数据
@param[in] nLength 原数据长度
@param[out] pOutput 输出缓冲区
@param[in,out] nOutLength in:输出缓冲区的大小，out:返回true时表示解压后数据大小，否则保持不变
@param[in] nLevel 0~9,0表示不压缩,1表示速度最快,9表示压缩率最高
@return 是否成功,输出空间不够也返回false
*/
bool zlib_compress_data(const void *pInput,
                        uint32_t nLength,
                        void *pOutput,
                        uint32_t &nOutLength,
                        int nLevel = 6);

/** gzip方式压缩数据
@param[in] pInput 原数据
@param[in] nLength 原数据长度
@param[out] output 压缩后数据
@param[in] nLevel 0~9,0表示不压缩,1表示速度最快,9表示压缩率最高
@return 是否成功
*/
bool gzip_compress_data(const void *pInput,
                        uint32_t nLength,
                        std::vector<uint8_t> &output,
                        int nLevel = 6);

/** gzip方式压缩数据
@param[in] pInput 数据
@param[in] nLength 原数据长度
@param[out] pOutput 输出缓冲区
@param[in,out] nOutLength in:输出缓冲区的大小，out:返回true时表示解压后数据大小，否则保持不变
@param[in] nLevel 0~9,0表示不压缩,1表示速度最快,9表示压缩率最高
@return 是否成功,输出空间不够也返回false
*/
bool gzip_compress_data(const void *pInput,
                        uint32_t nLength,
                        void *pOutput,
                        uint32_t &nOutLength,
                        int nLevel = 6);

/** 解压数据，支持zlib 和 gzip
@param[in] pInput 数据
@param[in] nLength 原数据长度
@param[out] output 压缩后数据
@return 是否成功
*/
bool decompress_data(const void *pInput, uint32_t nLength, std::vector<uint8_t> &output);

/** 解压数据，支持zlib 和 gzip
@param[in] pInput 数据
@param[in] nLength 原数据长度
@param[out] pOutput 输出缓冲区
@param[in,out] nOutLength in:输出缓冲区的大小，out:返回true时表示解压后数据大小，否则保持不变
@return 是否成功,输出空间不够也返回false
*/
bool decompress_data(const void *pInput, uint32_t nLength, void *pOutput, uint32_t &nOutLength);

SHARELIB_END_NAMESPACE

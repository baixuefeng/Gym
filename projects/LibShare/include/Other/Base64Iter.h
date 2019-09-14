#pragma once
#include "MacroDefBase.h"
#include <boost/iterator/iterator_traits.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>

SHARELIB_BEGIN_NAMESPACE

namespace base64{

/* 注意：boost::archive库的base64编码没有追加'='；也不能正解解密追加'='的密文。
   它原本只是用于程序序列化使用的。
*/

//base64编码迭代器
template<class InputIter> using Base64EncodeIter =
    boost::archive::iterators::base64_from_binary<
        boost::archive::iterators::transform_width<
            InputIter,
            6,
            sizeof(typename boost::iterators::iterator_value<InputIter>::type) * 8
        >
    >;

/** 输入一个迭代器，生成一个base64编码迭代器
@param[in] InputIter it 原始输入迭代器
@return base64编码迭代器
*/
template<class InputIter>
Base64EncodeIter<InputIter> make_encode_iter(InputIter it)
{
    return Base64EncodeIter<InputIter>(it);
}



//base64解码迭代器
template<class InputIter> using Base64DecodeIter =
    boost::archive::iterators::transform_width<
        boost::archive::iterators::binary_from_base64<InputIter>,
        sizeof(typename boost::iterators::iterator_value<InputIter>::type) * 8, 
        6
    >;

/** 输入一个迭代器，生成一个base64解码迭代器
@param[in] InputIter it 原始输入迭代器
@return base64解码迭代器
*/
template<class InputIter>
Base64DecodeIter<InputIter> make_decode_iter(InputIter it)
{
    return Base64DecodeIter<InputIter>(it);
}

}

SHARELIB_END_NAMESPACE

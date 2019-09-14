#pragma once
#include "MacroDefBase.h"
#include <boost/iterator/iterator_traits.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>

SHARELIB_BEGIN_NAMESPACE

namespace base64{

/* ע�⣺boost::archive���base64����û��׷��'='��Ҳ�����������׷��'='�����ġ�
   ��ԭ��ֻ�����ڳ������л�ʹ�õġ�
*/

//base64���������
template<class InputIter> using Base64EncodeIter =
    boost::archive::iterators::base64_from_binary<
        boost::archive::iterators::transform_width<
            InputIter,
            6,
            sizeof(typename boost::iterators::iterator_value<InputIter>::type) * 8
        >
    >;

/** ����һ��������������һ��base64���������
@param[in] InputIter it ԭʼ���������
@return base64���������
*/
template<class InputIter>
Base64EncodeIter<InputIter> make_encode_iter(InputIter it)
{
    return Base64EncodeIter<InputIter>(it);
}



//base64���������
template<class InputIter> using Base64DecodeIter =
    boost::archive::iterators::transform_width<
        boost::archive::iterators::binary_from_base64<InputIter>,
        sizeof(typename boost::iterators::iterator_value<InputIter>::type) * 8, 
        6
    >;

/** ����һ��������������һ��base64���������
@param[in] InputIter it ԭʼ���������
@return base64���������
*/
template<class InputIter>
Base64DecodeIter<InputIter> make_decode_iter(InputIter it)
{
    return Base64DecodeIter<InputIter>(it);
}

}

SHARELIB_END_NAMESPACE

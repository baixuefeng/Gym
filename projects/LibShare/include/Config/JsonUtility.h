#pragma once

#include <type_traits>
#include <boost/filesystem/fstream.hpp>
#include <rapidjson/document.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/encodings.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/memorybuffer.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "MacroDefBase.h"

SHARELIB_BEGIN_NAMESPACE
namespace JsonUtility {

/** 获取Object中的值
@param[in] object GenericObject
@param[in] key 子键
@param[out] value key对应的值
@param[in] defaultValue 如果获取不到，value则赋值为该默认值
@return 获取是否成功
*/
template<bool Const, typename TJsonValue, typename TSourceAllocator, typename TValue>
bool GetObjectSubValue(
    const RAPIDJSON_NAMESPACE::GenericObject<Const, TJsonValue> &object,
    const RAPIDJSON_NAMESPACE::GenericValue<
        typename RAPIDJSON_NAMESPACE::GenericObject<Const, TJsonValue>::EncodingType,
        TSourceAllocator> &key,
    TValue &value,
    const TValue &defaultValue = TValue{})
{
    auto it = object.FindMember(key);
    if (it == object.MemberEnd() || it->value.IsNull() || !it->value.template Is<TValue>()) {
        value = defaultValue;
        return false;
    } else {
        value = std::move(it->value.template Get<TValue>());
        return true;
    }
}

template<bool Const, typename TJsonValue, typename TValue>
bool GetObjectSubValue(
    const RAPIDJSON_NAMESPACE::GenericObject<Const, TJsonValue> &object,
    const typename RAPIDJSON_NAMESPACE::GenericObject<Const, TJsonValue>::Ch *pKey,
    TValue &value,
    const TValue &defaultValue = TValue{})
{
    typename RAPIDJSON_NAMESPACE::GenericObject<Const, TJsonValue>::ValueType key(
        RAPIDJSON_NAMESPACE::StringRef(pKey));
    return GetObjectSubValue(object, key, value, defaultValue);
}

template<bool Const, typename TJsonValue, typename TValue>
bool GetObjectSubValue(
    const RAPIDJSON_NAMESPACE::GenericObject<Const, TJsonValue> &object,
    const std::basic_string<typename RAPIDJSON_NAMESPACE::GenericObject<Const, TJsonValue>::Ch>
        &key,
    TValue &value,
    const TValue &defaultValue = TValue{})
{
    return GetObjectSubValue(object, key.c_str(), value, defaultValue);
}

namespace detail {
template<typename TDocType, typename TEncoding, typename TInputByteStream>
bool ParseImpl(
    TDocType &doc,
    RAPIDJSON_NAMESPACE::EncodedInputStream<TEncoding, TInputByteStream> &encodedInStream)
{
    doc.template ParseStream<RAPIDJSON_NAMESPACE::ParseFlag::kParseDefaultFlags, TEncoding>(
        encodedInStream);
    return !doc.HasParseError();
}

template<typename TDocType, typename TEncoding, typename TOutputByteStream>
bool WriteImpl(
    TDocType &doc,
    RAPIDJSON_NAMESPACE::EncodedOutputStream<TEncoding, TOutputByteStream> &encodedOutStream,
    bool prettyWrite)
{
    if (prettyWrite) {
        RAPIDJSON_NAMESPACE::PrettyWriter<std::remove_reference_t<decltype(encodedOutStream)>,
                                          typename TDocType::EncodingType,
                                          TEncoding>
            jsonWriter{encodedOutStream};
        return doc.Accept(jsonWriter);
    } else {
        RAPIDJSON_NAMESPACE::Writer<std::remove_reference_t<decltype(encodedOutStream)>,
                                    typename TDocType::EncodingType,
                                    TEncoding>
            jsonWriter{encodedOutStream};
        return doc.Accept(jsonWriter);
    }
}
} // namespace detail

/** 解析utf8的json文件
@param[out] doc GenericDocument
@param[in] file 文件
@return 解析是否成功
*/
template<typename TDocEncoding, typename TAllocator, typename TStackAllocator>
bool ParseUtf8File(
    RAPIDJSON_NAMESPACE::GenericDocument<TDocEncoding, TAllocator, TStackAllocator> &doc,
    const boost::filesystem::path &file)
{
    boost::filesystem::ifstream in{file, std::ios::in | std::ios::binary};
    if (!in) {
        return false;
    }
    RAPIDJSON_NAMESPACE::BasicIStreamWrapper<decltype(in)> inStream{in};
    RAPIDJSON_NAMESPACE::EncodedInputStream<RAPIDJSON_NAMESPACE::UTF8<>, decltype(inStream)>
        encodedInStream{inStream};
    return detail::ParseImpl(doc, encodedInStream);
}

/** 解析utf8的json内存
@param[out] doc GenericDocument
@param[in] pData 内存指针
@param[in] nLength 内存长度
@return 解析是否成功
*/
template<typename TDocEncoding, typename TAllocator, typename TStackAllocator>
bool ParseUtf8Memory(
    RAPIDJSON_NAMESPACE::GenericDocument<TDocEncoding, TAllocator, TStackAllocator> &doc,
    const void *pData,
    size_t nLength)
{
    RAPIDJSON_NAMESPACE::MemoryStream inStream{(const RAPIDJSON_NAMESPACE::MemoryStream::Ch *)pData,
                                               nLength};
    RAPIDJSON_NAMESPACE::EncodedInputStream<RAPIDJSON_NAMESPACE::UTF8<>, decltype(inStream)>
        encodedInStream{inStream};
    return detail::ParseImpl(doc, encodedInStream);
}

/** 解析字符串
@param[out] doc GenericDocument
@param[in] inputStrStream 字符串输入流
@return 解析是否成功
warning: template template parameter using 'typename' is a C++17 extension
*/
template<typename TDocEncoding,
         typename TAllocator,
         typename TStackAllocator,
         template<typename>
         class TInputStringStream,
         typename TStringEncoding>
bool ParseString(
    RAPIDJSON_NAMESPACE::GenericDocument<TDocEncoding, TAllocator, TStackAllocator> &doc,
    TInputStringStream<TStringEncoding> &inputStrStream)
{
    doc.template ParseStream<RAPIDJSON_NAMESPACE::ParseFlag::kParseDefaultFlags, TStringEncoding>(
        inputStrStream);
    return !doc.HasParseError();
}

/** 写入GenericDocument到json文件中
@param[in] doc GenericDocument
@param[in] file 文件
@param[in] prettyWrite 是否使用易读的方式写入
@param[in] putBOM 是否写入POM
@return 操作是否成功
*/
template<typename TDocEncoding, typename TAllocator, typename TStackAllocator>
bool WriteToUtf8File(
    const RAPIDJSON_NAMESPACE::GenericDocument<TDocEncoding, TAllocator, TStackAllocator> &doc,
    const boost::filesystem::path &file,
    bool prettyWrite = true,
    bool putBOM = true)
{
    boost::filesystem::ofstream out{file, std::ios::out | std::ios::binary | std::ios::trunc};
    if (!out) {
        return false;
    }

    RAPIDJSON_NAMESPACE::BasicOStreamWrapper<decltype(out)> outStream{out};
    RAPIDJSON_NAMESPACE::EncodedOutputStream<RAPIDJSON_NAMESPACE::UTF8<>, decltype(outStream)>
        encodedOutStream{outStream, putBOM};
    return detail::WriteImpl(doc, encodedOutStream, prettyWrite);
}

/** 写入GenericDocument到json内存块中
@param[in] doc GenericDocument
@param[out] outBuffer 内存块
@param[in] prettyWrite 是否使用易读的方式写入
@param[in] putBOM 是否写入POM
@return 操作是否成功
*/
template<typename TDocEncoding,
         typename TAllocator,
         typename TStackAllocator,
         typename TBufAllocator>
bool WriteToUtf8Memory(
    const RAPIDJSON_NAMESPACE::GenericDocument<TDocEncoding, TAllocator, TStackAllocator> &doc,
    RAPIDJSON_NAMESPACE::GenericMemoryBuffer<TBufAllocator> &outBuffer,
    bool prettyWrite = false,
    bool putBOM = false)
{
    outBuffer.Clear();
    RAPIDJSON_NAMESPACE::EncodedOutputStream<RAPIDJSON_NAMESPACE::UTF8<>,
                                             std::remove_reference_t<decltype(outBuffer)>>
        encodedOutStream{outBuffer, putBOM};
    return detail::WriteImpl(doc, encodedOutStream, prettyWrite);
}

/** 写入字符串流
@param[in] doc GenericDocument
@param[out] outBuffer 字符串buffer
@param[in] prettyWrite 是否使用易读的方式写入
@return 操作是否成功
*/
template<typename TDocEncoding,
         typename TAllocator,
         typename TStackAllocator,
         typename TStrEncoding,
         typename TStrAllocator>
bool WriteToString(
    const RAPIDJSON_NAMESPACE::GenericDocument<TDocEncoding, TAllocator, TStackAllocator> &doc,
    RAPIDJSON_NAMESPACE::GenericStringBuffer<TStrEncoding, TStrAllocator> &outString,
    bool prettyWrite = false)
{
    outString.Clear();
    if (prettyWrite) {
        RAPIDJSON_NAMESPACE::
            PrettyWriter<std::remove_reference_t<decltype(outString)>, TDocEncoding, TStrEncoding>
                jsonWriter{outString};
        return doc.Accept(jsonWriter);
    } else {
        RAPIDJSON_NAMESPACE::
            Writer<std::remove_reference_t<decltype(outString)>, TDocEncoding, TStrEncoding>
                jsonWriter{outString};
        return doc.Accept(jsonWriter);
    }
}

} // namespace JsonUtility
SHARELIB_END_NAMESPACE

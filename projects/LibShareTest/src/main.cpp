/*
*          Copyright Andrey Semashev 2007 - 2015.
* Distributed under the Boost Software License, Version 1.0.
*    (See accompanying file LICENSE_1_0.txt or copy at
*          http://www.boost.org/LICENSE_1_0.txt)
*/
/*!
* \file   main.cpp
* \author Andrey Semashev
* \date   11.11.2007
*
* \brief  An example of basic library usage. See the library tutorial for expanded
*         comments on this code. It may also be worthwhile reading the Wiki requirements page:
*         http://www.crystalclearsoftware.com/cgi-bin/boost_wiki/wiki.pl?Boost.Logging
*/

// #define BOOST_LOG_USE_CHAR
// #define BOOST_ALL_DYN_LINK 1
// #define BOOST_LOG_DYN_LINK 1
#include "stdafx.h"
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/back_inserter.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include "Memory/compress_utility.h"
using namespace boost::iostreams;

template<typename T>
std::ostream &operator <<(std::ostream &os, const std::vector<T> &v) {
    using namespace std;
    copy(v.begin(), v.end(), ostream_iterator<T>(os));
    return os;
}

void test1()
{
    std::string OriginalString = "hello world";

    std::vector<char> CompressedVector;
    {
        filtering_ostream fos;    //具有filter功能的输出流
        fos.push(zlib_compressor(gzip_params(gzip::best_compression)));  //gzip压缩功能
        fos.push(boost::iostreams::back_inserter(CompressedVector));     //输出流的数据的目的地
        fos << OriginalString;
        boost::iostreams::flush(fos);
        //boost::iostreams::close(fos);  //flush to CompressedVector 此函数调用后，CompressedVector存储的是gzip压缩后的数据
    }
    std::vector<uint8_t> in;
    shr::zlib_compress_data(OriginalString.c_str(), OriginalString.size(), in);
    std::string decompressedString;
        std::vector<uint8_t> out;
        shr::decompress_data(CompressedVector.data(), CompressedVector.size(), out);
    {
        filtering_ostream fos;
        fos.push(zlib_decompressor());  //gzip解压缩功能
        fos.push(boost::iostreams::back_inserter(decompressedString)); //存放流的数据的目的地
        fos.write(CompressedVector.data(), CompressedVector.size()); //把压缩的数据写入流
                                                                     // 或者用如下的形式。需要自定义(ostream<<vector)运算符 
                                                                     //out << compressedVector;
        boost::iostreams::flush(fos);
        //boost::iostreams::close(fos); //flush. decompressedString中出现解压后的数据
    }

    if (OriginalString == decompressedString) {
        std::cout << "test1() OK" << std::endl;
    }
}

void test2()
{
    std::string OriginalString = "hello world";

    std::string compressedString;
    {
        filtering_ostream fos;    //具有filter功能的输出流
        fos.push(gzip_compressor(gzip_params(gzip::best_compression)));
        fos.push(boost::iostreams::back_inserter(compressedString));
        fos << OriginalString;   //把压缩的数据写入流
        boost::iostreams::close(fos);  //flush. compressedString出现压缩后的数据
                                       //此时，不应当把string当做字符串理解，应该理解为一个char容器
    }

    std::string decompressedString;
    {
        filtering_ostream fos;
        fos.push(gzip_decompressor());
        fos.push(boost::iostreams::back_inserter(decompressedString));

        fos << compressedString; //把压缩数据写入流。此处，不会理会char=0的字节，只按照容器长度写入流。
                                 //strlen(compressedString.c_str() != compressedString.size())

        fos << std::flush;  //compressedString出现压缩后的数据

    }

    if (OriginalString == decompressedString) {
        std::cout << "test2() OK" << std::endl;
    }
}

int main()
{
    test1();
    test2();
    system("pause");
}

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
        filtering_ostream fos;    //����filter���ܵ������
        fos.push(zlib_compressor(gzip_params(gzip::best_compression)));  //gzipѹ������
        fos.push(boost::iostreams::back_inserter(CompressedVector));     //����������ݵ�Ŀ�ĵ�
        fos << OriginalString;
        boost::iostreams::flush(fos);
        //boost::iostreams::close(fos);  //flush to CompressedVector �˺������ú�CompressedVector�洢����gzipѹ���������
    }
    std::vector<uint8_t> in;
    shr::zlib_compress_data(OriginalString.c_str(), OriginalString.size(), in);
    std::string decompressedString;
        std::vector<uint8_t> out;
        shr::decompress_data(CompressedVector.data(), CompressedVector.size(), out);
    {
        filtering_ostream fos;
        fos.push(zlib_decompressor());  //gzip��ѹ������
        fos.push(boost::iostreams::back_inserter(decompressedString)); //����������ݵ�Ŀ�ĵ�
        fos.write(CompressedVector.data(), CompressedVector.size()); //��ѹ��������д����
                                                                     // ���������µ���ʽ����Ҫ�Զ���(ostream<<vector)����� 
                                                                     //out << compressedVector;
        boost::iostreams::flush(fos);
        //boost::iostreams::close(fos); //flush. decompressedString�г��ֽ�ѹ�������
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
        filtering_ostream fos;    //����filter���ܵ������
        fos.push(gzip_compressor(gzip_params(gzip::best_compression)));
        fos.push(boost::iostreams::back_inserter(compressedString));
        fos << OriginalString;   //��ѹ��������д����
        boost::iostreams::close(fos);  //flush. compressedString����ѹ���������
                                       //��ʱ����Ӧ����string�����ַ�����⣬Ӧ�����Ϊһ��char����
    }

    std::string decompressedString;
    {
        filtering_ostream fos;
        fos.push(gzip_decompressor());
        fos.push(boost::iostreams::back_inserter(decompressedString));

        fos << compressedString; //��ѹ������д�������˴����������char=0���ֽڣ�ֻ������������д������
                                 //strlen(compressedString.c_str() != compressedString.size())

        fos << std::flush;  //compressedString����ѹ���������

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

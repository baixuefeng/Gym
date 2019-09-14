include_guard(GLOBAL)

# zlib: ZLIB_LIBRARIES
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ZLIB_ROOT ${CMAKE_CURRENT_LIST_DIR}/zlib/win64)
else()
    set(ZLIB_ROOT ${CMAKE_CURRENT_LIST_DIR}/zlib/win32)
endif()
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
set(THIRD_PARTY_LIBRARIES_DIRS ${THIRD_PARTY_LIBRARIES_DIRS} ${ZLIB_ROOT}/lib)
add_compile_definitions(
    "ZLIB_DLL"
)

# brotli: BROTLI_LIBRARIES
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BROTLI_ROOT ${CMAKE_CURRENT_LIST_DIR}/brotli/win64)
else()
    set(BROTLI_ROOT ${CMAKE_CURRENT_LIST_DIR}/brotli/win32)
endif()
file(GLOB BROTLI_LIBRARIES
    LIST_DIRECTORIES false 
    ${BROTLI_ROOT}/lib/brotli*.lib)
file(GLOB BROTLI_STATIC_LIBRARIES
    LIST_DIRECTORIES false 
    ${BROTLI_ROOT}/lib/brotli*static.lib)
list(REMOVE_ITEM BROTLI_LIBRARIES ${BROTLI_STATIC_LIBRARIES})
list(SORT BROTLI_LIBRARIES 
    COMPARE FILE_BASENAME 
    ORDER DESCENDING)
if(BROTLI_LIBRARIES)
    message("-- Found brotli: " ${BROTLI_LIBRARIES})
else()
    message(FATAL_ERROR "brotli not found!")
endif()
include_directories(${BROTLI_ROOT}/include)
set(THIRD_PARTY_LIBRARIES_DIRS ${THIRD_PARTY_LIBRARIES_DIRS} ${BROTLI_ROOT}/bin)
add_compile_definitions(
    "BROTLI_SHARED_COMPILATION"
)

# openssl: OPENSSL_LIBRARIES
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OPENSSL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/OpenSSL/win64)
else()
    set(OPENSSL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/OpenSSL/win32)
endif()
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
set(THIRD_PARTY_LIBRARIES_DIRS ${THIRD_PARTY_LIBRARIES_DIRS} ${OPENSSL_ROOT_DIR}/bin)

# boost
set(BOOST_INCLUDEDIR ${CMAKE_CURRENT_LIST_DIR}/boost_1_71_0)
set(BOOST_LIBRARYDIR ${CMAKE_CURRENT_LIST_DIR}/boost_1_71_0/stage/lib)
set(Boost_NO_SYSTEM_PATHS ON)
include_directories(${BOOST_INCLUDEDIR})
link_directories(${BOOST_LIBRARYDIR})
add_compile_definitions(
    # boost 相关
    "BOOST_ASIO_NO_DEPRECATED"
    "BOOST_CHRONO_VERSION=2"
    "BOOST_FILESYSTEM_NO_DEPRECATED"
    "BOOST_THREAD_VERSION=5"
    "BOOST_ZLIB_BINARY=${ZLIB_LIBRARIES}"
)
if(WIN32)
# windows中ipc默认使用EventLog获取bootup time，基于此生成ipc的共享文件夹，但某些系统中可能获取失败。
# 比如可能被某些清理软件清除掉了EventLog，也可能EventLog满了自动清除了。因此添加该宏定义，使用注册表中
# HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management\PrefetchParameters
# 下的BootId来读取bootup time。 
# 不过最推荐的做法是自己定义下面两个宏之一。注意：目录必须自己创建好，ipc对象的名称实质上就是文件名，必须合法。
#   BOOST_INTERPROCESS_SHARED_DIR_PATH  //编译期可以固定下来的文件路径，末尾不要加'/'
#   BOOST_INTERPROCESS_SHARED_DIR_FUNC  //运行期决定的路径，如果定义该宏，需要实现下面的函数。
#   namespace boost {
#       namespace interprocess {
#           namespace ipcdetail {
#               void get_shared_dir(std::string &shared_dir);
#           }
#       }
#   }
# bug参考: https://svn.boost.org/trac10/ticket/12137#no1
# 文档参考: boost_1_69_0/doc/html/interprocess/acknowledgements_notes.html
add_compile_definitions(
    "BOOST_INTERPROCESS_BOOTSTAMP_IS_SESSION_MANAGER_BASED"
)
endif()

# curl: CURL_LIBRARY
set(CMAKE_INCLUDE_PATH ${CMAKE_CURRENT_LIST_DIR}/curl/include)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_LIBRARY_PATH ${CMAKE_CURRENT_LIST_DIR}/curl/lib_win64)
else()
    set(CMAKE_LIBRARY_PATH ${CMAKE_CURRENT_LIST_DIR}/curl/lib_win32)
endif()
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
set(THIRD_PARTY_LIBRARIES_DIRS ${THIRD_PARTY_LIBRARIES_DIRS} ${CMAKE_LIBRARY_PATH})

# glew: GLEW_LIBRARIES
set(CMAKE_INCLUDE_PATH ${CMAKE_CURRENT_LIST_DIR}/glew/include)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_LIBRARY_PATH ${CMAKE_CURRENT_LIST_DIR}/glew/lib_win64)
else()
    set(CMAKE_LIBRARY_PATH ${CMAKE_CURRENT_LIST_DIR}/glew/lib_win32)
endif()
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
set(THIRD_PARTY_LIBRARIES_DIRS ${THIRD_PARTY_LIBRARIES_DIRS} ${CMAKE_LIBRARY_PATH})

# lua: LUA_LIBRARIES
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ENV{LUA_DIR} ${CMAKE_CURRENT_LIST_DIR}/lua/win64)
else()
    set(ENV{LUA_DIR} ${CMAKE_CURRENT_LIST_DIR}/lua/win32)
endif()
find_package(Lua REQUIRED)
include_directories(${LUA_INCLUDE_DIR})
if(WIN32)
    add_compile_definitions(LUA_BUILD_AS_DLL)
endif()
set(THIRD_PARTY_LIBRARIES_DIRS ${THIRD_PARTY_LIBRARIES_DIRS} $ENV{LUA_DIR}/lib)

#----下面这些库以源代码方式使用--------------------------------

set(THIRD_PARTY_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/third_party_src)

# minhook: MINHOOK_SOURCE_CODE
include(${THIRD_PARTY_SRC_DIR}/minhook/minhook.cmake)
function(SRC_GRP_MINHOOK)
    source_group(TREE ${THIRD_PARTY_SRC_DIR} FILES ${MINHOOK_SOURCE_CODE})
endfunction()

#sqlite: SQLITE_SOURCE_CODE
include(${THIRD_PARTY_SRC_DIR}/sqlite/sqlite.cmake)
function(SRC_GRP_SQLITE)
    source_group(TREE ${THIRD_PARTY_SRC_DIR} FILES ${SQLITE_SOURCE_CODE})
endfunction()

# pugixml: PUGIXML_SOURCE_CODE
set(PUGIXML_SOURCE_CODE 
    ${THIRD_PARTY_SRC_DIR}/pugixml/pugiconfig.hpp
    ${THIRD_PARTY_SRC_DIR}/pugixml/pugixml.cpp
    ${THIRD_PARTY_SRC_DIR}/pugixml/pugixml.hpp)
add_compile_definitions(PUGIXML_WCHAR_MODE)
function(SRC_GRP_PUGIXML)
    source_group(TREE ${THIRD_PARTY_SRC_DIR} FILES ${PUGIXML_SOURCE_CODE})
endfunction()

#----纯头文件的库--------------------------------------------

# third_party_src: Eigen, glm, rapidjson, WTL
include_directories(${THIRD_PARTY_SRC_DIR})

#--------------------------------------------------------

# 正则匹配，拷贝库文件到目标目录，常用于将dll一起发布
function(RegexCopyThirdPartyLibs regex_search dest_dir)
    foreach(cur_dir ${THIRD_PARTY_LIBRARIES_DIRS})
        file(COPY ${cur_dir}/ DESTINATION ${dest_dir} 
            FILES_MATCHING REGEX ${regex_search})
    endforeach()
endfunction()

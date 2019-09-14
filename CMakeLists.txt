cmake_minimum_required(VERSION 3.12)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/out" CACHE PATH "install dir")
file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX})

project("Gym" 
    VERSION 1.0.0.1 
    DESCRIPTION "Gym"
    LANGUAGES C CXX ASM)

# compile config
include(vsconfig.cmake)
if(UNIX)
    add_compile_options(
        -fPIC
        -std=c++14
        -Wfatal-errors
        -Wall
        -Wextra)
endif()

#-------------------------------------------------------

macro(GATHER_SRC_FILES  DIR  FILE_LIST)
    file(GLOB  ${FILE_LIST}
        LIST_DIRECTORIES false
        ${DIR}/*.h
        ${DIR}/*.inl
        ${DIR}/*.hpp
        ${DIR}/*.c
        ${DIR}/*.cpp)
endmacro()

macro(GATHER_SRC_FILES_RECURSE  DIR  FILE_LIST)
    file(GLOB_RECURSE  ${FILE_LIST}
        LIST_DIRECTORIES false
        ${DIR}/*.h
        ${DIR}/*.inl
        ${DIR}/*.hpp
        ${DIR}/*.c
        ${DIR}/*.cpp)
endmacro()

#-------------------------------------------------------

# common config
include(${CMAKE_SOURCE_DIR}/ThirdParty/ThirdParty.cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
RegexCopyThirdPartyLibs([[.*\.dll]] ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
RegexCopyThirdPartyLibs([[.*\.dll]] ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release)
RegexCopyThirdPartyLibs([[.*\.dll]] ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/RelWithDebInfo)

#sub project

# LibShare有public属性的include文件夹，当其它工程链接它的时候，会自动把它的public include文件夹添加到自己的include目录中
add_subdirectory(${CMAKE_SOURCE_DIR}/projects/LibShare)

add_subdirectory(${CMAKE_SOURCE_DIR}/projects/LibShareTest)

add_subdirectory(${CMAKE_SOURCE_DIR}/projects/SimpleTest)

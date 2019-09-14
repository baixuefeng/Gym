GATHER_SRC_FILES_RECURSE(${CMAKE_CURRENT_LIST_DIR} srcfiles)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${srcfiles})
SRC_GRP_PUGIXML()
SRC_GRP_SQLITE()

add_library(LibShare STATIC ${srcfiles} ${PUGIXML_SOURCE_CODE} ${SQLITE_SOURCE_CODE})
target_compile_definitions(LibShare PRIVATE
    SHARELIB_NO_AUTO_LINK
    _ATL_NO_AUTOMATIC_NAMESPACE
    _WTL_NO_AUTOMATIC_NAMESPACE)
target_include_directories(LibShare 
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

# if(CMAKE_SIZEOF_VOID_P EQUAL 8)
#     set_target_properties(LibShare PROPERTIES 
#         OUTPUT_NAME_DEBUG "libShare_x64_MDd")
#     set_target_properties(LibShare PROPERTIES 
#         OUTPUT_NAME_RELEASE "libShare_x64_MD")
#     set_target_properties(LibShare PROPERTIES 
#         COMPILE_PDB_NAME_DEBUG "libShare_x64_MDd")
#     set_target_properties(LibShare PROPERTIES 
#         COMPILE_PDB_NAME_RELEASE "libShare_x64_MD")
# else()
#     set_target_properties(LibShare PROPERTIES 
#         OUTPUT_NAME_DEBUG "libShare_x86_MDd")
#     set_target_properties(LibShare PROPERTIES 
#         OUTPUT_NAME_RELEASE "libShare_x86_MD")
#     set_target_properties(LibShare PROPERTIES 
#         COMPILE_PDB_NAME_DEBUG "libShare_x86_MDd")
#     set_target_properties(LibShare PROPERTIES 
#         COMPILE_PDB_NAME_RELEASE "libShare_x86_MD")
# endif()

# install(TARGETS LibShare DESTINATION "${CMAKE_CURRENT_LIST_DIR}/lib")

GATHER_SRC_FILES_RECURSE(${CMAKE_CURRENT_LIST_DIR} srcfiles)
file(GLOB_RECURSE rcfiles
    LIST_DIRECTORIES false
    ${CMAKE_CURRENT_LIST_DIR}/resource/*.rc)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${srcfiles} ${rcfiles})

add_executable(SimpleTest ${srcfiles} ${rcfiles})
target_include_directories(SimpleTest 
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(SimpleTest 
    LibShare 
    ${ZLIB_LIBRARIES} 
    ${OPENSSL_LIBRARIES} 
    ${CURL_LIBRARIES} 
    ${GLEW_LIBRARIES} 
    ${LUA_LIBRARIES})

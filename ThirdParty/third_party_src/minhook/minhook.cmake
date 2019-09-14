set(MINHOOK_SOURCE_CODE 
    "${CMAKE_CURRENT_LIST_DIR}/include/MinHook.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/buffer.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/buffer.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/hook.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/trampoline.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/trampoline.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/hde/pstdint.h")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MINHOOK_SOURCE_CODE 
        ${MINHOOK_SOURCE_CODE}
        "${CMAKE_CURRENT_LIST_DIR}/src/hde/table64.h"
        "${CMAKE_CURRENT_LIST_DIR}/src/hde/hde64.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/hde/hde64.h")
else()
    set(MINHOOK_SOURCE_CODE 
        ${MINHOOK_SOURCE_CODE}
        "${CMAKE_CURRENT_LIST_DIR}/src/hde/table32.h"
        "${CMAKE_CURRENT_LIST_DIR}/src/hde/hde32.c"
        "${CMAKE_CURRENT_LIST_DIR}/src/hde/hde32.h")
endif()

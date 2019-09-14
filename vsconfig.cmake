add_compile_definitions(
    "UNICODE"
    "_UNICODE"                #UNICODE字符集
    "_DLL"                    #/MD or /MDd (Multithreaded DLL)
    "_CRT_SECURE_NO_WARNINGS" #禁用CRT警告
    "_SCL_SECURE_NO_WARNINGS" #禁用SCL警告
    "WIN32_LEAN_AND_MEAN"     #禁用一些过时的windows头文件
    "_WIN32_WINNT=0x0601"     #目标系统版本，必须和编译boost的目标系统版本相同，否则log库链接不上
)

add_compile_options(
    "/W4"                     #警告等级4
    "/wd4714"                 #标记为 __forceinline 的函数未内联
)

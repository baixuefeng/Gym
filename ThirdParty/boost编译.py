import os
import subprocess
import re
import glob
import time
import colorama

# 切换当前目录到boost目录
def ChangDirToBoost():
    if os.path.exists("boost") and os.path.isdir("boost"):
        return
    boostdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..\\boost_inuse")
    if not os.path.isdir(boostdir) or not os.path.exists(boostdir):
        print("未找到boost目录！！！")
        os._exit(0)

    os.chdir(boostdir)

# ---------------------------------------------------------------------

# 返回VS命令行的信息：(版本号，脚本地址)
def GetVSInfo():
    VSVer = []
    VSPath = []
    for item in os.environ:
        vsenvKey = re.match(re.compile(R"VS(\d+)COMNTOOLS", re.RegexFlag.I), item)
        if not (vsenvKey is None):
            temp = os.path.join(os.environ[item], R"..\..\VC\vcvarsall.bat")
            temp = os.path.normpath(temp)
            if os.path.exists(temp):
                VSVer.append(float(vsenvKey.group(1)) / 10)
                VSPath.append(temp)

    if len(VSPath) == 0:
        return None
    else:
        print("查找到以下编译器：")
        for index, item in enumerate(VSVer):
            print("{}. VS版本号:{:.1f}".format(index + 1, item))

        index = 0
        while (index <= 0) or (index > len(VSVer)) :
            index = int(input("请选择编译器(1~{})：".format(len(VSVer))))
        index -= 1
        return ("{:.1f}".format(VSVer[index]), VSPath[index])

# ---------------------------------------------------------------------

def GenerateB2(vscmd):
    if os.path.exists("b2.exe"):
        return
    if len(vscmd) == 0:
        print(colorama.Fore.RED + "命令错误！！！")
        os._exit(0)
    subProc = subprocess.Popen("cmd", stdin = subprocess.PIPE, encoding="gbk")
    subProc.stdin.write(vscmd + " x64\r\n")
    subProc.stdin.write("call bootstrap.bat\r\n")
    subProc.communicate()
    if not os.path.exists("b2.exe"):
        print(colorama.Fore.RED + "生成b2.exe失败，退出！！！")
        os._exit(0)

# ---------------------------------------------------------------------

# 选择要编译的boost子库
def GetBoostLibs():
    if not os.path.exists("b2.exe"):
        print(colorama.Fore.RED + "还未生成b2")
        os._exit(0)
    
    boostLibs = []
    index = 1
    for item in re.finditer(R"-\s+(\w+)\s?\r\n", subprocess.Popen("b2 --show-libraries", stdout=subprocess.PIPE).stdout.read().decode("gbk")):
        boostLibs.append(item.group(1))
        print("{:2}. {:<30}√".format(index, item.group(1)))
        index += 1
    
    isOk = False
    while not isOk:
        excludeLibs = []
        try:
            for item in re.split(",", input("选择不编译的库(英文逗号分隔，不要有空格，默认全部编译):")):
                excludeLibs.append(int(item))
        except ValueError as errMsg:
            errMsg
            excludeLibs = []
    
        index = 1
        libsStr = ""
        if len(excludeLibs) > 0:
            for item in boostLibs:
                if not index in excludeLibs:
                    libsStr += "--with-{} ".format(item)
                    print("{:2}. {:<30}√".format(index, item))
                else:
                    print("{:2}. {:<30}".format(index, item))
                index += 1
            temp = input("确认？(y or n)")
            isOk = (temp == "y" ) or (temp == "Y")
        else:
            isOk = True
    return libsStr

# ---------------------------------------------------------------------

# 编译
# address_model : 32 或 64
# variant ：debug 或 release
# runtimeLink : static 或 shared
def Compile(vscmd, compileCmd, address_model, variant, runtimeLink):
    if (len(vscmd) == 0) or (len(compileCmd) == 0):
        print(colorama.Fore.RED + "参数错误！！！")
        os._exit(0)

    if int(address_model) == 32:
        platform = "x86"
        zlibPath = os.getcwd() + R"\..\zlib\win32"
    elif int(address_model) == 64:
        platform = "x64"
        zlibPath = os.getcwd() + R"\..\zlib\win64"
    else:
        print(colorama.Fore.RED + "参数错误！！！")
        os._exit(0)

    # zlib
    zlibLibPath = os.path.realpath(zlibPath + R"\lib").replace('\\', '/')
    zlibIncludePath = os.path.realpath(zlibPath + R"\include").replace('\\', '/')
    if os.path.exists(zlibLibPath) and \
       os.path.exists(zlibIncludePath):
        print("build zlib with lib: {0}, include: {1}".format(zlibLibPath, zlibIncludePath))
        os.environ["ZLIB_LIBRARY_PATH"] = zlibLibPath
        os.environ["ZLIB_INCLUDE"] = zlibIncludePath
    else:
        print("no zlib support")

    if (variant != "debug") and (variant != "release"):
        print(colorama.Fore.RED + "参数错误！！！")
        os._exit(0)

    if (runtimeLink != "static") and (runtimeLink != "shared"):
        print(colorama.Fore.RED + "参数错误！！！")
        os._exit(0)

    logFile = "compile_{0}_{1}_{2}.log".format(platform, variant, runtimeLink)
    with open(logFile, "w") as fileOut:
        subProc = subprocess.Popen("cmd", stdin = subprocess.PIPE, stdout=fileOut, encoding="gbk")
    subProc.stdin.write((vscmd + " " + platform + "\r\n"))
    subProc.stdin.write(compileCmd.format(int(address_model), variant, runtimeLink))
    print("编译开始，查看boost文件夹下的编译日志： {}".format(logFile))
    try:
        subProc.communicate()
    except subprocess.TimeoutExpired as errMsg:
        errMsg
    return subProc

# ---------------------------------------------------------------------
# main entry

colorama.init(autoreset=True)
ChangDirToBoost()
VSInfo = GetVSInfo()
if VSInfo is None:
    print(colorama.Fore.RED + "未找到VS安装路径!!!")
    os._exit(0)

vsCmd = "call \"" + VSInfo[1] + "\" "
GenerateB2(vsCmd)

""" boost包含的子库
    - atomic
    - chrono
    - container
    - context
    - contract
    - coroutine
    - date_time
    - exception
    - fiber
    - filesystem
    - graph
    - graph_parallel
    - iostreams
    - locale
    - log
    - math
    - mpi
    - program_options
    - python
    - random
    - regex
    - serialization
    - signals
    - stacktrace
    - system
    - test
    - thread
    - timer
    - type_erasure
    - wave
"""
# boostLibs = GetBoostLibs()
# boostLibs = "--without-serialization --without-type_erasure --without-wave"
boostLibs = ""

compileCmd = "b2 toolset=msvc-{} --layout=versioned --build-type=complete ".format(VSInfo[0]) \
             + boostLibs \
             + " --hash threading=multi link=static address-model={0} variant={1} runtime-link={2} define=BOOST_ASIO_DISABLE_STD_CHRONO define=_CRT_SECURE_NO_WARNINGS \n"
    # define=_USING_V110_SDK71_ define=_WIN32_WINNT=0x0501 
    #如果定义了 BOOST_ASIO_NO_DEPRECATED ，会导致log编译不过


subProcs = []
# subProcs.append(Compile(vsCmd, compileCmd, 32, "debug", "static"))
subProcs.append(Compile(vsCmd, compileCmd, 32, "debug", "shared"))
# subProcs.append(Compile(vsCmd, compileCmd, 32, "release", "static"))
subProcs.append(Compile(vsCmd, compileCmd, 32, "release", "shared"))
# subProcs.append(Compile(vsCmd, compileCmd, 64, "debug", "static"))
subProcs.append(Compile(vsCmd, compileCmd, 64, "debug", "shared"))
# subProcs.append(Compile(vsCmd, compileCmd, 64, "release", "static"))
subProcs.append(Compile(vsCmd, compileCmd, 64, "release", "shared"))

print("编译中...")
for item in subProcs:
    item.communicate()
print("编译结束。")

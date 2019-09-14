import os
import subprocess
import shutil

cur_path = os.path.realpath(os.path.dirname(__file__))
work_dir = os.path.join(cur_path, "windows_build")
if not os.path.exists(work_dir):
    os.mkdir(work_dir)
else:
    for item in os.scandir(work_dir):
        if item.is_dir():
            shutil.rmtree(item.path, ignore_errors=True)
        else:
            os.remove(item.path)
os.chdir(work_dir)

if int(input("生成32位还是64位工程？")) == 32:
    bitParam = ""
else:
    bitParam = " Win64"

subMake = subprocess.Popen("cmd", stdin=subprocess.PIPE, encoding="gbk")
subMake.stdin.write('''cmake -G"Visual Studio 14 2015{}" ..
'''.format(bitParam))
subMake.communicate()

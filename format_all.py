import os
import re
import subprocess

RE_SRC = re.compile(R"[\w-]+\.(h|hpp|inl|c|cpp|cc)")

def FormatSubDir(dir):
    with os.scandir(dir) as it:
        for entry in it:
            if entry.is_dir():
                FormatSubDir(entry.path)
            elif entry.is_file() and RE_SRC.match(entry.name):
                # print("clang-format.exe -i -style=file {0}".format(entry.path))
                os.system("clang-format.exe -i -style=file {0}".format(entry.path))
                
#---------------------------------------

if __name__ == "__main__":
    cur_path = os.path.realpath(os.path.dirname(__file__))
    FormatSubDir(os.path.join(cur_path, "projects"))
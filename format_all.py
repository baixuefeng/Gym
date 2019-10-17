import os
import re
import sys
import subprocess

RE_SRC = re.compile(R"[\w-]+\.(h|hpp|inl|c|cpp|cc)")


def FormatSubDir(dir):
    with os.scandir(dir) as it:
        for entry in it:
            if entry.is_dir():
                FormatSubDir(entry.path)
            elif entry.is_file() and RE_SRC.match(entry.name):
                print("-- Formatting :", entry.path)
                os.system(
                    "clang-format.exe -i -style=file {0}".format(entry.path))

# ---------------------------------------


if __name__ == "__main__":
    for item in sys.argv[1:]:
        print("scandir :", item)
        FormatSubDir(os.path.realpath(item))

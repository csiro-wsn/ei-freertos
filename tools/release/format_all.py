#!/usr/bin/env python3

import os
import glob
import subprocess
from utility.general_repo_tools import get_repo_root


if __name__ == '__main__':
    root = get_repo_root()

    format_directories = [os.path.join(root, d) for d in ['apps', 'apps_test', 'core_csiro']]
    ignored_files = ["*gatt_efr32*", "*gatt_nrf52*", "*rpc_server*", "*rpc_client*"]

    for d in format_directories:
        c_finder_args = ["find", d, "-iname", "*.c"]
        h_finder_args = ["find", d, "-iname", "*.h"]
        for f in ignored_files:
            c_finder_args += ["-not", "(", "-name", f, ")"]
            h_finder_args += ["-not", "(", "-name", f, ")"]
        formatter_args = ["xargs", "clang-format", "-i", "-style=file"]

        print("Formatting source files in {:s}...".format(d))
        c_finder = subprocess.Popen(c_finder_args, stdout=subprocess.PIPE)
        result = subprocess.check_output(formatter_args, stdin=c_finder.stdout)

        print("Formatting header files in {:s}...".format(d))
        h_finder = subprocess.Popen(h_finder_args, stdout=subprocess.PIPE)
        result = subprocess.check_output(formatter_args, stdin=h_finder.stdout)

    print("Formatting complete")

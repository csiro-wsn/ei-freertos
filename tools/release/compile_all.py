#!/usr/bin/env python3

import os
import sys
import subprocess
import argparse
from utility.general_repo_tools import get_repo_root, get_build_root, get_path_for_application, get_all_applications, get_targets_for_application

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Verify that all applications compile')
    parser.add_argument('--verbose', dest='print_errors', action='store_true', help='Print stderr from build process')
    args = parser.parse_args()

    apps = [get_path_for_application(app) for app in get_all_applications()]
    success = True

    print("")
    print("Removing exisiting artifacts from {:s}".format(get_build_root()))
    subprocess.run(["rm", "-rf", "{:s}".format(get_build_root())], stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
    print("")

    print("{:>40s} |{:>20s} | {:80s}".format("APPLICATION", "TARGET", "STATUS"))

    for app_path in apps:
        app = os.path.basename(app_path)
        app_makefile = os.path.join(app_path, "Makefile")

        if not os.path.exists(app_makefile):
            print("{:>40s} |{:>20s} | {:40s}".format(app, "", "Does not have a Makefile"))
            success = False
            continue

        app_targets = get_targets_for_application(os.path.basename(os.path.normpath(app_path)))

        # Validate that supported platforms are provided
        if app_targets == []:
            print("{:>40s} |{:>20s} | {:40s}".format(app, "", "Does not list supported targets"))
            success = False
            continue

        # Change to the application directory
        os.chdir(app_path)

        # For each TARGET allegedly supported
        for target in app_targets:
            print("{:>40s} |{:>20s} | ".format(app, target), end='', flush=True)

            # Build the TARGET
            print("Building... ", end='', flush=True)
            build_output = subprocess.run(["make", "-j", "debug", "TARGET={:s}".format(target)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
            if build_output.returncode != 0:
                print("'make debug' FAILED")
                success = False
            else:
                print("'make debug' SUCCESS ")
            if args.print_errors:
                stdout = build_output.stdout
                stderr = build_output.stderr
                if len(stderr) > 0:
                    print("")
                    print(stdout)
                    print(stderr)

    sys.exit(0 if success else 1)

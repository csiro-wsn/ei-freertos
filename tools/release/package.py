#!/usr/bin/env python3

import argparse
import zipfile
import os
import json
from utility.general_repo_tools import get_repo_root

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Package predefined distributions into zip files')
    parser.add_argument('-d', '--distributions', dest='distributions', required=True, help='Distribution description json file')
    args = parser.parse_args()

    repo_root = get_repo_root()

    with open(args.distributions, 'r') as dists:
        distributions = json.load(dists)

    # Create a unique zip file for each distribution described in the dist file
    for dist in distributions:
        with zipfile.ZipFile('pacp_freertos-{:s}.zip'.format(dist), 'w') as zipFile:
            for path in distributions[dist]:
                if os.path.isdir(path):
                    # Walk every folder listed in the dist file
                    # Overlapping folder definitions will result in duplicated files (bad)
                    for dirName, subdirList, fileList in os.walk(os.path.join(repo_root, path)):
                        for fName in fileList:
                            # Add the file to the zip file under the relative path 'pacp-freertos/
                            absPath = os.path.join(dirName, fName)
                            relPath = os.path.join("pacp-freertos", os.path.relpath(dirName, repo_root), fName)
                            zipFile.write(absPath, relPath)
                else:
                    relPath = os.path.join("pacp-freertos", path)
                    zipFile.write(path, relPath)

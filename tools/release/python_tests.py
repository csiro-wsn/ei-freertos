#!/usr/bin/env python3

# Imports
from unittest import TestLoader, TextTestRunner
from utility.general_repo_tools import get_repo_root

if __name__ == "__main__":

    # Recursively find all test_*.py files in python packages in pacp-freertos
    # starting in the repo root, and only looking in packages. (Files with a
    # __init__.py file in them. This is the 'pyclasses' folder only.)
    testLoader = TestLoader()
    testSuite = testLoader.discover(get_repo_root())
    # Run the tests.
    textTestResult = TextTestRunner().run(testSuite)
    if (not textTestResult.wasSuccessful()):
        exit(1)
    exit(0)

#!/usr/bin/env python3

'''
This aim of this file is to test a few of the more important make rules in
the repo, so that this script can be ran by the continuous integration
server to ensure that changes to the repo don't break these rules.

Rules which get tested:
    clean
    vscode
    gdb
    coverage (single target)
'''

import subprocess
import os
from utility.general_repo_tools import (get_path_for_application, get_all_targets_in_repo, get_application_for_given_target)


def test_all_rules_for_all_targets(rule_list):
    '''
    Tests all rules in the rule_test for every target.
    '''
    targets = get_all_targets_in_repo()
    for target in targets:
        supported_app = get_application_for_given_target(target)
        for rule in rule_list:
            rule_return_value = test_rule(rule, target, supported_app)
            if (rule_return_value.returncode != 0):
                print(rule_return_value.stderr)
                exit(1)


def one_off_rule(rule, target, application, multithread=False, dir_change=""):
    rule_return_value = test_rule(rule, target, application, multithread=multithread, dir_change=dir_change)
    if (rule_return_value.returncode != 0):
        print(rule_return_value.stderr)
        print("Rule failed!")
        exit(1)


def test_rule(rule, target, application, multithread=False, dir_change=""):
    '''
    Tests a specified rule, on a specified target.
    Returns the result of executing that specific rule.
    '''
    os.chdir(get_path_for_application(application))

    if (multithread):
        multithread_string = " -j"
    else:
        multithread_string = ""

    print("Running rule: \"make {} TARGET={:<11}{}\" in application {}".format(rule.format(target), target, multithread_string, application))

    if (multithread):
        return subprocess.run(["make", rule.format(target), "TARGET={}".format(target), '-j'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')
    else:
        return subprocess.run(["make", rule.format(target), "TARGET={}".format(target)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8')


if __name__ == "__main__":
    # Test all general rules
    general_rule_list = ["clean", "vscode", "gdb"]
    test_all_rules_for_all_targets(general_rule_list)

    # Test one-off specific rules
    one_off_rule("coverage", "ceresig", "unifiedbase", multithread=True)

    exit(0)

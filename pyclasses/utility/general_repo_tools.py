#!/usr/bin/env python3

'''
This file contains a number of different functions to help other tools to do
their thing around the repo.
'''
import os


def get_repo_root():
    '''
    Return the root path of the repo.
    '''
    file_path = os.path.dirname(os.path.abspath(__file__))
    file_path = os.path.normpath(os.path.join(file_path, "../.."))
    return file_path


def get_build_root():
    return os.path.join(get_repo_root(), "build")


def get_path_for_application(application):
    '''
    Returns the path to an application given the name.
    '''
    path = None
    apps = get_apps_in_directory(os.path.join(get_repo_root(), "apps"))
    test_apps = get_apps_in_directory(os.path.join(get_repo_root(), "apps_test"))

    if (application in apps):
        path = os.path.join(get_repo_root(), "apps", application)
        return path
    if (application in test_apps):
        path = os.path.join(get_repo_root(), "apps_test", application)
        return path

    return path


def get_all_targets_in_repo():
    '''
    Returns a list of all unique targets for applications in the repo.
    '''
    apps = get_all_applications()
    targets = []

    for app in apps:
        targets = targets + get_targets_for_application(app)

    return list(set(targets))


def get_all_applications():
    '''
    Returns a list of the names of all apps, and test_apps in a repo.
    '''
    apps = get_apps_in_directory(os.path.join(get_repo_root(), "apps"))
    test_apps = get_apps_in_directory(os.path.join(get_repo_root(), "apps_test"))
    return list(set(apps + test_apps))


def get_targets_for_application(application):
    '''
    Returns a list of all targets for a given application.
    '''
    targets = []

    path = get_path_for_application(application)
    if (path is None):
        raise Exception("Application doesn't exist.")

    target_line = ""
    with open(os.path.join(path, "Makefile"), "r") as fd:
        for line in fd:
            if ("SUPPORTED_TARGETS" in line):
                target_line = line
                break

    if (target_line == ""):
        return targets

    # Split out all supported targets
    targets = target_line.split(":=")[1].strip('\n').split(" ")
    # Remove empty strings from list
    targets = list(filter(None, targets))

    return targets


def get_application_for_given_target(target):
    '''
    Returns the name of an application which can be built for a given target.
    '''
    targets = get_all_targets_in_repo()
    if (target not in targets):
        return None

    apps = get_all_applications()
    for app in apps:
        if (target in get_targets_for_application(app)):
            return app

    return None


def get_apps_in_directory(path):
    '''
    Return a list of all the application names in the given directory
    '''
    apps = os.listdir(path)
    apps = [app for app in apps if os.path.exists(os.path.join(path, app, "Makefile"))]
    return apps

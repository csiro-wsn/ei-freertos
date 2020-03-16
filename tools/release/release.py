#!/usr/bin/env python3

from git import Repo
import argparse
import re
import requests
import time
import sys
import os


def get_git_state(repo_path):
    repo = Repo(repo_path)
    # Update our knowledge of the remote
    for remote in repo.remotes:
        remote.fetch()
    # Validate no uncommited changes
    if repo.is_dirty():
        print('Repository has uncommited changes')
        return (None, None)
    # Check remote and local have same commits
    current_branch = repo.active_branch.name
    commits_ahead = sum(1 for c in repo.iter_commits('origin/{0:s}..{0:s}'.format(current_branch)))
    commits_behind = sum(1 for c in repo.iter_commits('{0:s}..origin/{0:s}'.format(current_branch)))
    if commits_ahead != 0:
        print('Repository has commits that have not been pushed')
        return (None, None)
    if commits_behind != 0:
        print('Repository has commits on the origin that do not exist on the local branch')
        return (None, None)
    return repo.head.commit.hexsha, repo.config_reader().get("user", "name")


def simple_macro_extractor(filename):
    regex = r"(?<=#define)[^\n]*"
    macros = {}
    with open(filename, 'r') as f:
        matches = re.finditer(regex, f.read(-1), re.MULTILINE)
        for match in matches:
            m = match.group().replace('\t', ' ').strip().split(' ', 1)
            if len(m) > 1:
                if(m[0].strip() != "NVM_APP_STRUCT"):
                    print(m[1].strip())
                    macros[m[0].strip()] = eval(m[1].strip())
    return macros


def get_major_minor(repo_path, application):
    config_file = os.path.join(repo_path, 'apps', application, 'inc', 'application.h')
    macros = simple_macro_extractor(config_file)
    if ('APP_MAJOR' not in macros) or ('APP_MINOR' not in macros):
        print('APP_MAJOR and APP_MINOR are not present in {:s}'.format(config_file))
        return None
    return (macros['APP_MAJOR'], macros['APP_MINOR'])


def get_rpcs(repo_path, application):
    app_rpc_file = os.path.join(repo_path, 'apps', application, 'rpc_list.txt')
    default_rpc_file = os.path.join(repo_path, 'core_csiro', 'rpc', 'default_rpcs.txt')
    # Read the requested rpcs
    with open(app_rpc_file, "r") as f:
        requested_rpcs = [l.strip() for l in f.readlines()]
    # Get the default rpc list
    with open(default_rpc_file, "r") as f:
        default_rpcs = [l.strip() for l in f.readlines()]
    rpcs = set(requested_rpcs + default_rpcs)
    rpcs = [l for l in rpcs if (l != "") and (not l.startswith("#"))]
    return rpcs


def valid_online_parameters(server, major, minor, target, commit, rpcs):
    try:
        existing_apps = requests.get(server + '/app/json').json()
    except Exception:
        print("Failed to retrieve applications from {:s}".format(server))
        return False
    # If the major or minor is new, valid
    if str(app_major) not in existing_apps:
        return True
    if str(app_minor) not in existing_apps[str(app_major)]:
        return True
    existing_app = existing_apps[str(app_major)][str(app_minor)]
    # If the target already exists for the application
    if target in existing_app['binaries']:
        print("Application {:d}.{:d} already has a binary for TARGET:{:s}".format(app_major, app_minor, target))
        return False
    # Check this MAJOR MINOR is built from the same commit
    if commit != existing_app['commit']:
        print("Application {:d}.{:d} was built from commit {:s} but we are on {:s}".format(app_major, app_minor, existing_app['commit'], commit))
        return False
    # For existing applications, check RPC list matches
    if set(existing_app['rpcs']) != set(rpcs):
        print("Application {:d}.{:d} on server has a different RPC list, please update APP_MINOR".format(app_major, app_minor, args.server))
        return False
    return True


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate and upload a release')
    parser.add_argument('--app', dest='application', required=True, help='Application to release')
    parser.add_argument('--repo', dest='repo', required=True, help='Repository location')
    parser.add_argument('--target', dest='target', required=True, help='Platform target')
    parser.add_argument('--server', dest='server', required=True, help='Remote server')
    args = parser.parse_args()
    # Check that git is in a commited, pushed state
    (commit_hash, user) = get_git_state(args.repo)
    if commit_hash is None:
        print("Commit hash is \'None\'")
        sys.exit(-1)
    # Get the application MAJOR and MINOR
    (app_major, app_minor) = get_major_minor(args.repo, args.application)
    # Get application RPC's
    rpcs = get_rpcs(args.repo, args.application)
    # Validate application parameters against existing applications
    if not valid_online_parameters(args.server, app_major, app_minor, args.target, commit_hash, rpcs):
        sys.exit(-1)
    # Check application binary exists
    app_binary_path = os.path.join(args.repo, 'build', 'REL', args.target, 'obj', args.application, '{:s}.bin'.format(args.application))
    if not os.path.exists(app_binary_path):
        print("Application binary {:s} does not exist")
        sys.exit(-1)

    t = int(time.time())
    app_info = {
        '{:d}.{:d}'.format(app_major, app_minor): {
            'name': args.application,
            'known_issues': '',
            'description': '',
            'rpcs': rpcs,
            'commit': commit_hash,
            'binaries': {
                '{:s}'.format(args.target): {
                    'built_by': user,
                    'uploaded': t
                }
            }
        }
    }

    try:
        requests.post(args.server + '/app/release', json=app_info)
        filename = '{:s}.{:d}.{:d}.{:s}.{:d}'.format(args.application, app_major, app_minor, args.target, t)
        requests.post(args.server + '/app/release/binary', files={filename: open(app_binary_path, 'rb')})
    except Exception as e:
        print('Failed to push {:d}.{:d} release to {:s}'.format(app_major, app_minor, args.server))
        sys.exit(-1)

#!/usr/bin/env python3
# Copyright 2021 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Release branch updater tool.

This is a tool to merge from the main branch into a release branch.

Inspired by the fingerprint release process:
http://go/cros-fingerprint-firmware-branching-and-signing and now used by other
boards.
"""
from __future__ import print_function
import argparse
import os
import re
import subprocess
import sys
import textwrap

BUG_NONE_PATTERN = re.compile('none', flags=re.IGNORECASE)


def git_commit_msg(branch, head, merge_head, rel_paths):
    """Generates a merge commit message based off of relevant changes.

    This function obtains the relevant commits from the given relative paths in
    order to extract the bug numbers. It constructs the git commit message
    showing the command used to find the relevant commits.

    Args:
        branch: String indicating the release branch name
        head: String indicating the HEAD refspec
        merge_head: String indicating the merge branch refspec.
        rel_paths: String containing all the relevant paths for this particular
                   baseboard or board.

    Returns:
        A String containing the git commit message with the exception of the
        Signed-Off-By field and Change-ID field.
    """
    relevant_commits_cmd, relevant_commits = get_relevant_commits(head,
                                                                  merge_head,
                                                                  '--oneline',
                                                                  rel_paths)

    _, relevant_bugs = get_relevant_commits(head, merge_head, '', rel_paths)
    relevant_bugs = set(re.findall('BUG=(.*)', relevant_bugs))
    # Filter out "none" from set of bugs
    filtered = []
    for bug_line in relevant_bugs:
        bug_line = bug_line.replace(',', ' ')
        bugs = bug_line.split(' ')
        for bug in bugs:
            if bug and not BUG_NONE_PATTERN.match(bug):
                filtered.append(bug)
    relevant_bugs = filtered

    # TODO(b/179509333): remove Cq-Include-Trybots line when regular CQ and
    # firmware CQ do not behave differently.
    COMMIT_MSG_TEMPLATE = """
Merge remote-tracking branch cros/main into {BRANCH}

Relevant changes:

{RELEVANT_COMMITS_CMD}

{RELEVANT_COMMITS}

BRANCH=None
{BUG_FIELD}
TEST=`make -j buildall`

Cq-Include-Trybots: chromeos/cq:cq-orchestrator
"""
    # Wrap the relevant commits command and bug field such that we don't exceed
    # 72 cols.
    relevant_commits_cmd = textwrap.fill(relevant_commits_cmd, width=72)
    # Wrap at 68 cols to save room for 'BUG='
    bugs = textwrap.wrap(' '.join(relevant_bugs), width=68)
    bug_field = ''
    for line in bugs:
        bug_field += 'BUG=' + line + '\n'
    # Remove the final newline since the template adds it for us.
    bug_field = bug_field[:-1]

    return COMMIT_MSG_TEMPLATE.format(BRANCH=branch,
                                      RELEVANT_COMMITS_CMD=relevant_commits_cmd,
                                      RELEVANT_COMMITS=relevant_commits,
                                      BUG_FIELD=bug_field)


def get_relevant_boards(baseboard):
    """Searches through the EC repo looking for boards that use the given
    baseboard.

    Args:
        baseboard: String containing the baseboard to consider

    Returns:
        A list of strings containing the boards based off of the baseboard.
    """
    proc = subprocess.run(['git', 'grep', 'BASEBOARD:=' + baseboard, '--',
                           'board/'],
                          stdout=subprocess.PIPE,
                          encoding='utf-8',
                          check=True)
    boards = []
    res = proc.stdout.splitlines()
    for line in res:
        boards.append(line.split('/')[1])
    return boards


def get_relevant_commits(head, merge_head, fmt, relevant_paths):
    """Searches git history to find changes since the last merge which modify
    files present in relevant_paths.

    Args:
        head: String indicating the HEAD refspec
        merge_head: String indicating the merge branch refspec.
        fmt: An optional string containing the format option for `git log`
        relevant_paths: String containing all the relevant paths for this
                        particular baseboard or board.

    Returns:
        A tuple containing the arguments passed to the git log command and
        stdout.
    """
    if fmt:
        cmd = ['git', 'log', fmt, head + '..' + merge_head, '--',
               relevant_paths]
    else:
        cmd = ['git', 'log', head + '..' + merge_head, '--', relevant_paths]

    # Pass cmd as a string to subprocess.run() since we need to run with shell
    # equal to True.  The reason we are using shell equal to True is to take
    # advantage of the glob expansion for the relevant paths.
    cmd = ' '.join(cmd)
    proc = subprocess.run(cmd,
                          stdout=subprocess.PIPE,
                          encoding='utf-8',
                          check=True,
                          shell=True)
    return ''.join(proc.args), proc.stdout


def main(argv):
    """Generates a merge commit from ToT to a desired release branch.

    For the commit message, it finds all the commits that have modified a
    relevant path. By default this is the baseboard or board directory.  The
    user may optionally specify a path to a text file which contains a longer
    list of relevant files.  The format should be in the glob syntax that git
    log expects.

    Args:
        argv: A list of the command line arguments passed to this script.
    """
    # Set up argument parser.
    parser = argparse.ArgumentParser(description=("A script that generates a "
                                                  "merge commit from cros/main"
                                                  " to a desired release "
                                                  "branch.  By default, the "
                                                  "'recursive' merge strategy "
                                                  "with the 'theirs' strategy "
                                                  "option is used."))
    parser.add_argument('--baseboard')
    parser.add_argument('--board')
    parser.add_argument('release_branch', help=('The name of the target release'
                                                ' branch'))
    parser.add_argument('--relevant_paths_file',
                        help=('A path to a text file which includes other '
                              'relevant paths of interest for this board '
                              'or baseboard'))
    parser.add_argument('--merge_strategy', '-s', default='recursive',
                        help='The merge strategy to pass to `git merge -s`')
    parser.add_argument('--strategy_option', '-X',
                        help=('The strategy option for the chosen merge '
                              'strategy'))

    opts = parser.parse_args(argv)

    baseboard_dir = ''
    board_dir = ''

    if opts.baseboard:
        # Dereference symlinks so "git log" works as expected.
        baseboard_dir = os.path.relpath('baseboard/' + opts.baseboard)
        baseboard_dir = os.path.relpath(os.path.realpath(baseboard_dir))

        boards = get_relevant_boards(opts.baseboard)
    elif opts.board:
        board_dir = os.path.relpath('board/' + opts.board)
        board_dir = os.path.relpath(os.path.realpath(board_dir))
        boards = [opts.board]
    else:
        parser.error('You must specify a board OR a baseboard')

    print("Gathering relevant paths...")
    relevant_paths = []
    if opts.baseboard:
        relevant_paths.append(baseboard_dir)
    else:
        relevant_paths.append(board_dir)

    for board in boards:
        relevant_paths.append('board/' + board)

    # Check for the existence of a file that has other paths of interest.
    if opts.relevant_paths_file and os.path.exists(opts.relevant_paths_file):
        with open(opts.relevant_paths_file, 'r') as relevant_paths_file:
            for line in relevant_paths_file:
                if not line.startswith('#'):
                    relevant_paths.append(line.rstrip())
    relevant_paths.append('util/getversion.sh')
    relevant_paths = ' '.join(relevant_paths)

    # Now that we have the paths of interest, let's perform the merge.
    print("Updating remote...")
    subprocess.run(['git', 'remote', 'update'], check=True)
    subprocess.run(['git', 'checkout', '-B', opts.release_branch, 'cros/' +
                    opts.release_branch], check=True)
    print("Attempting git merge...")
    if opts.merge_strategy == 'recursive' and not opts.strategy_option:
        opts.strategy_option = 'theirs'
    print("Using '%s' merge strategy" % opts.merge_strategy,
          ("with strategy option '%s'" % opts.strategy_option
           if opts.strategy_option else ''))
    arglist = ['git', 'merge', '--no-ff', '--no-commit', 'cros/main', '-s',
               opts.merge_strategy]
    if opts.strategy_option:
        arglist.append('-X' + opts.strategy_option)
    subprocess.run(arglist, check=True)

    print("Generating commit message...")
    branch = subprocess.run(['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                            stdout=subprocess.PIPE,
                            encoding='utf-8',
                            check=True).stdout.rstrip()
    head = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'],
                          stdout=subprocess.PIPE,
                          encoding='utf-8',
                          check=True).stdout.rstrip()
    merge_head = subprocess.run(['git', 'rev-parse', '--short',
                                 'MERGE_HEAD'],
                                stdout=subprocess.PIPE,
                                encoding='utf-8',
                                check=True).stdout.rstrip()

    print("Typing as fast as I can...")
    commit_msg = git_commit_msg(branch, head, merge_head, relevant_paths)
    subprocess.run(['git', 'commit', '--signoff', '-m', commit_msg], check=True)
    subprocess.run(['git', 'commit', '--amend'], check=True)
    print(("Finished! **Please review the commit to see if it's to your "
           "liking.**"))


if __name__ == '__main__':
    main(sys.argv[1:])
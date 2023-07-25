#!python
import sys
import pathlib
import argparse
import shutil

this_file_directory = pathlib.Path(__file__).parent.resolve()
sys.path.insert(0, str(this_file_directory))

import shared_build_code
from helpers import *


parser = argparse.ArgumentParser()
parser.add_argument('--debug', dest='release', default=True, action='store_false')
parser.add_argument('--build_tests', dest='build_tests', default=False, action='store_true')
parser.add_argument('--depth', dest='depth', default=3)
args = parser.parse_args()

shared_build_code.build_shumi_chess(args.release, args.build_tests)
shared_build_code.build_python_gui_module(args.release)


print_blue(f'Running perf test with depth {args.depth}')
run_command_blocking([
    'valgrind',
    '--tool=callgrind',
    str(shared_build_code.bin_dir.joinpath('run_minimax_depth')),
    str(args.depth),
], debug=True, stdout_pipe=None, stderr_pipe=None)

callgrind_files = []
for filename, filepath in get_all_paths(shared_build_code.root_of_project_directory):
    if filename.startswith('callgrind.out.'):
        number = int(filename.split('.')[-1])
        callgrind_files.append((number, filepath))
callgrind_files.sort(reverse=True)

for _, filepath in callgrind_files:
    latest_callgrind_filepath = shared_build_code.bin_dir.joinpath(filepath.name)
    print_blue(f'Moving {filepath} to {latest_callgrind_filepath}')
    shutil.move(filepath, str(latest_callgrind_filepath))

run_command_blocking([
    'kcachegrind',
    str(latest_callgrind_filepath),
], debug=True, stdout_pipe=None, stderr_pipe=None)
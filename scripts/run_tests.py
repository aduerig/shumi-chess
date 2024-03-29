#!python
import sys
import pathlib
import argparse

this_file_directory = pathlib.Path(__file__).parent.resolve()
sys.path.insert(0, str(this_file_directory))

import shared_build_code
from helpers import *


parser = argparse.ArgumentParser()
parser.add_argument('test_filter', nargs='?', default='*')
parser.add_argument('--debug', dest='release', default=True, action='store_false')
args = parser.parse_args()


shared_build_code.build_shumi_chess(args.release, build_tests=True)
shared_build_code.build_python_gui_module(args.release)

cmd = [
    str(shared_build_code.bin_dir.joinpath('unit_tests')),
    '--gtest_color=yes',
]
if args.test_filter != '*':
    cmd += ['--gtest_filter=' + args.test_filter]
run_command_blocking(cmd, debug=True, stdout_pipe=None, stderr_pipe=None)
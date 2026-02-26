#!python
import sys
import pathlib
import argparse
import os

this_file_directory = pathlib.Path(__file__).parent.resolve()
sys.path.insert(0, str(this_file_directory))

import shared_build_code
from helpers import *

root_dir = this_file_directory.parent
test_data_dir = root_dir / 'tests' / 'test_data'
generate_script = root_dir / 'tests' / 'generate_test_data' / 'generate_depth_test_data.py'


def check_test_data():
    has_data = test_data_dir.exists() and any(test_data_dir.glob('*.dat'))
    if not has_data:
        print_cyan("Test data is missing! Generating it now...")
        print_cyan(f"  Directory: {test_data_dir}")
        sys.path.insert(0, str(generate_script.parent))
        import generate_depth_test_data
        generate_depth_test_data.generate_all()
        print_cyan("Test data generation complete.")

    # Hard check: don't let tests run without data
    if not test_data_dir.exists() or not any(test_data_dir.glob('*.dat')):
        print_red("ERROR: Test data still missing after generation attempt. Cannot run tests.")
        print_red(f"  Expected .dat files in: {test_data_dir}")
        print_red(f"  Try running manually: python {generate_script}")
        sys.exit(1)


parser = argparse.ArgumentParser()
parser.add_argument('test_filter', nargs='?', default='*')
parser.add_argument('--debug', dest='release', default=True, action='store_false')

parser.add_argument('-asserts', '--asserts', dest='shumi_asserts', default=True, action='store_true')
parser.add_argument('-no-asserts', '--no-asserts', dest='shumi_asserts', action='store_false')

args = parser.parse_args()

check_test_data()

shared_build_code.build_shumi_chess(args.release, build_tests=True)
shared_build_code.build_shumi_chess(args.release, build_tests=True, shumi_asserts=args.shumi_asserts)
# shared_build_code.build_python_gui_module(args.release)


windows_exe = '.exe' if is_windows() else ''


release_mode = 'debug' if not args.release else 'release'
release_mode = release_mode.capitalize()

cmd = [
    str(shared_build_code.bin_dir.joinpath(f'{release_mode}/unit_tests{windows_exe}')),
    '--gtest_color=yes',
]
if args.test_filter != '*':
    cmd += ['--gtest_filter=' + args.test_filter]
run_command_blocking(cmd, debug=True, stdout_pipe=None, stderr_pipe=None)

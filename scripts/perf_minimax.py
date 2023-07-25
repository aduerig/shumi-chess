#!python
import sys
import pathlib
import argparse

this_file_directory = pathlib.Path(__file__).parent.resolve()
sys.path.insert(0, str(this_file_directory))

import shared_build_code
from helpers import *


parser = argparse.ArgumentParser()
parser.add_argument('--debug', dest='release', default=True, action='store_false')
parser.add_argument('--build_tests', dest='build_tests', default=False, action='store_true')
parser.add_argument('--depth', dest='depth', default=6)
args = parser.parse_args()

shared_build_code.build_shumi_chess(args.release, args.build_tests)
shared_build_code.build_python_gui_module(args.release)

print_blue(f'Running perf test with depth {args.depth}')
processcode, _stdout, _stderr = run_command_blocking([
    'perf',
    'record',
    '-o', 
    str(shared_build_code.bin_dir.joinpath('minimax_perf.data')),
    '-g', # measures callgraphs
    str(shared_build_code.bin_dir.joinpath('run_minimax_depth')),
    str(args.depth),
], debug=True, stdout_pipe=None, stderr_pipe=None)
if processcode:
    print_red('perf failed')
    sys.exit(1)



run_command_blocking([
    'perf',
    'report',
    '-g', # measures callgraphs
    '-i',
    str(shared_build_code.bin_dir.joinpath('minimax_perf.data')),
], debug=True, stdout_pipe=None, stderr_pipe=None)
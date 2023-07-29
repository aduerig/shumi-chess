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
parser.add_argument('--time_to_run', dest='time_to_run', default=1)
args = parser.parse_args()

shared_build_code.build_shumi_chess(args.release, build_tests=False)
shared_build_code.build_python_gui_module(args.release)

print_blue(f'Running perf test with time_to_run {args.time_to_run}')
processcode, _stdout, _stderr = run_command_blocking([
    'perf',
    'record',
    '-o', 
    str(shared_build_code.bin_dir.joinpath('minimax_perf.data')),
    '-g', # measures callgraphs
    str(shared_build_code.bin_dir.joinpath('run_minimax_time')),
    str(args.time_to_run),
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
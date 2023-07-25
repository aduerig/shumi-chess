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
args = parser.parse_args()

shared_build_code.build_shumi_chess(args.release, build_tests=False)
shared_build_code.build_python_gui_module(args.release)
run_command_blocking([
    str(shared_build_code.bin_dir.joinpath('measure_speed_random_games')),
], debug=True, stdout_pipe=None, stderr_pipe=None)
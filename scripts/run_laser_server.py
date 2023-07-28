#!python
import sys
import pathlib
import argparse

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent
sys.path.insert(0, str(this_file_directory))

import shared_build_code
from helpers import *


parser = argparse.ArgumentParser()
parser.add_argument('--debug', dest='release', default=True, action='store_false')
args = parser.parse_args()

shared_build_code.build_shumi_chess(args.release, build_tests=False)
shared_build_code.build_python_gui_module(args.release)

show_board_path = root_of_project_directory.joinpath('driver', 'laser_client.py')
return_code, _stdout, _stderr = run_command_blocking([
    'python',
    str(show_board_path),
], stdout_pipe=None, stderr_pipe=None)
print(f'return code was {return_code}')
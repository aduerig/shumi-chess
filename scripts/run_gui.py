#!python
import sys
import pathlib
import argparse

this_file_directory = pathlib.Path(__file__).parent.resolve()
sys.path.insert(0, str(this_file_directory))

import shared_build_code
from helpers import *

# Python arguments
parser = argparse.ArgumentParser()
parser.add_argument('--debug', dest='release', default=True, action='store_false')
parser.add_argument('--fen', default=None)
parser.add_argument('--human', default=False, action='store_true')
parser.add_argument('-d', '--depth', type=int, default=7, help='Maximum deepening')
parser.add_argument('-t', '--time', type=int, default=2000, help='Time per move in ms')


args = parser.parse_args()

print('Building for:', 'debug' if not args.release else 'release')

shared_build_code.build_shumi_chess(args.release, build_tests=False)
shared_build_code.build_python_gui_module(args.release)

# shared_build_code.run_python_gui(fen=args.fen, human=args.human)
shared_build_code.run_python_gui(fen=args.fen, human=args.human, depth=args.depth, time=args.time)


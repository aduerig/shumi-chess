#!python
import sys
import pathlib
import argparse

this_file_directory = pathlib.Path(__file__).parent.resolve()
sys.path.insert(0, str(this_file_directory))

import shared_build_code
from helpers import *

def parse_int_auto(s: str) -> int:
    # base=0 lets Python accept decimal (12), hex (0xC), binary (0b1100), etc.
    return int(s, 0)
    

# Python arguments
parser = argparse.ArgumentParser()

parser.add_argument('--debug', dest='release', default=True, action='store_false')
parser.add_argument('--fen', default=None)
parser.add_argument('--human', default=False, action='store_true')

# common
parser.add_argument('-d', '--depth', type=int, default=5, help='Max deepening')
parser.add_argument('-t', '--time',  type=int, default=99, help='Time per move in ms')
parser.add_argument('-r', '--rand',  type=int, default=0, help='Randomization')

# per-side
parser.add_argument('-wd', '--wd', type=int, default=None, help='white max deepening')
parser.add_argument('-wt', '--wt', type=int, default=None, help='white time ms')

parser.add_argument('-bd', '--bd', type=int, default=None, help='black max deepening')
parser.add_argument('-bt', '--bt', type=int, default=None, help='black time ms')

# parser.add_argument('-f', '--feat', type=int, default=0, help='Special argument')
# parser.add_argument('-wf', '--wf',  type=int, default=None, help='white Special argument')
# parser.add_argument('-bf', '--bf',  type=int, default=None, help='black Special argument')
parser.add_argument('-f', '--feat', type=parse_int_auto, default=0,    help='Special argument (decimal or hex)')
parser.add_argument('-wf', '--wf',  type=parse_int_auto, default=None, help='white Special argument')
parser.add_argument('-bf', '--bf',  type=parse_int_auto, default=None, help='black Special argument')


args = parser.parse_args()

print('Building for:', 'debug' if not args.release else 'release')

shared_build_code.build_shumi_chess(args.release, build_tests=False)
shared_build_code.build_python_gui_module(args.release)


shared_build_code.run_python_gui(
    fen=args.fen,
    human=args.human,
    depth=args.depth,
    time=args.time,
    rand=args.rand,
    feat=args.feat,
    wdepth=args.wd,
    wtime=args.wt,
    wargu=args.wf,
    bdepth=args.bd,
    btime=args.bt,
    bargu=args.bf
)



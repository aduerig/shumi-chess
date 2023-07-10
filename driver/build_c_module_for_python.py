import setuptools
import distutils.cygwinccompiler
from distutils.core import setup, Extension
import os
from os import path
import sys
import time
import pathlib

from helpers import *

this_file_directory = pathlib.Path(__file__).parent.resolve()

distutils.cygwinccompiler.get_msvcr = lambda: []

steps_to_root = 1

if '--steps_to_root' in sys.argv:
    index = sys.argv.index('--steps_to_root')
    steps_to_root = int(sys.argv[index + 1])
    del sys.argv[index + 1]
    del sys.argv[index]

script_dir, _ = path.split(path.abspath(__file__))
root_dir = script_dir
for i in range(steps_to_root):
    root_dir = path.join(root_dir, os.pardir)
root_dir = path.normpath(root_dir)

one_above = path.normpath(path.join(script_dir, os.pardir))

print_cyan('thinking the one_above is {}'.format(one_above))
print_cyan('thinking the script_dir is {}'.format(script_dir))
print_cyan('thinking root directory of project is {}'.format(root_dir))

link_args = [path.join(root_dir, 'lib', 'libShumiChess.a')]
is_windows = os.name == 'nt'
if is_windows:
    link_args += ['-static', '-static-libgcc', '-static-libstdc++']

the_module = Extension(
    'engine_communicator',
    sources = [path.join(script_dir, 'engine_communicatormodule.cpp')],
    include_dirs = [path.join(one_above, 'src')],
    library_dirs = [path.join(root_dir, 'lib')],
    # tries to do a .so (dynamic) build with this
    # libraries = ['ShumiChess'],
    extra_compile_args=['-std=c++17'],
    extra_link_args=link_args,
)

setup(
    name = 'engine_communicator',
    version = '1.0',
    description = 'To communicate with ShumiChess C++ backend',
    ext_modules = [the_module]
)


if is_windows:
    name_of_output = 'engine_communicator.cp310-win_amd64.pyd'
    output_path = os.path.join(root_dir, 'driver', 'build', 'lib.win-amd64-3.10', name_of_output)
    wanted_path = os.path.join(root_dir, 'driver', name_of_output)

    if os.path.exists(output_path):
        if os.path.exists(wanted_path):
            os.remove(wanted_path)
            time.sleep(.05)
        os.rename(output_path, wanted_path)
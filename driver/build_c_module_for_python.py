import setuptools
import distutils.cygwinccompiler
from distutils.core import setup, Extension
import sys
import time
import pathlib

from helpers import *

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent

distutils.cygwinccompiler.get_msvcr = lambda: []

steps_to_root = 1

if '--steps_to_root' in sys.argv:
    index = sys.argv.index('--steps_to_root')
    steps_to_root = int(sys.argv[index + 1])
    del sys.argv[index + 1]
    del sys.argv[index]


print_cyan(f'{root_of_project_directory=}, {this_file_directory=}')

extra_link_args = [str(root_of_project_directory.joinpath('lib', 'libShumiChess.a'))]
extra_compile_args=['-std=c++17']

# if is_MVCC():
if True:
    extra_compile_args=['/std:c++17']
    extra_link_args = [str(root_of_project_directory.joinpath('lib', 'ShumiChess.lib'))]

if is_windows():
    extra_link_args += ['-static', '-static-libgcc', '-static-libstdc++']


the_module = Extension(
    'engine_communicator',
    sources = [str(this_file_directory.joinpath('engine_communicatormodule.cpp'))],
    include_dirs = [str(root_of_project_directory.joinpath('src'))],
    library_dirs = [str(root_of_project_directory.joinpath('lib'))],
    # tries to do a .so (dynamic) build with this
    # libraries = ['ShumiChess'],
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
)

setup(
    name = 'engine_communicator',
    version = '1.0',
    description = 'To communicate with ShumiChess C++ backend',
    ext_modules = [the_module]
)


if is_windows():
    name_of_output = 'engine_communicator.cp310-win_amd64.pyd'
    output_path = os.path.join(root_of_project_directory, 'driver', 'build', 'lib.win-amd64-3.10', name_of_output)
    wanted_path = os.path.join(root_of_project_directory, 'driver', name_of_output)

    if os.path.exists(output_path):
        if os.path.exists(wanted_path):
            os.remove(wanted_path)
            time.sleep(.05)
        os.rename(output_path, wanted_path)
# called by: python .\driver\build_c_module_for_python.py build --compiler=mingw32

from setuptools import setup, Extension
import sys
import pathlib
import os

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent

sys.path.insert(0, str(root_of_project_directory))
from helpers import *

release_mode = 'release'
if '--release' in sys.argv:
    del sys.argv[sys.argv.index('--release')]
if '--debug' in sys.argv:
    release_mode = 'debug'
    del sys.argv[sys.argv.index('--debug')]

print_cyan(f'building with {release_mode=}, {root_of_project_directory=}, {this_file_directory=}')

lib_dir = root_of_project_directory.joinpath('lib')
extra_link_args = [str(lib_dir.joinpath('libShumiChess.a'))]
extra_compile_args=['-std=c++17']

if is_windows():
    extra_link_args = [str(lib_dir.joinpath('ShumiChess.lib')), '-static', '-static-libgcc', '-static-libstdc++']

if release_mode == 'debug':
    extra_compile_args += ['-g', '-O0']
else:
    extra_compile_args += ['-Ofast']

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
    last_modified = float('-inf')
    output_path = None
    for filename, filepath in get_all_paths(this_file_directory.joinpath('build'), recursive=True, allowed_extensions=set(['.pyd'])):
        if filepath.stat().st_mtime > last_modified:
            last_modified = filepath.stat().st_mtime
            output_path = filepath
    wanted_path = root_of_project_directory.joinpath('driver', filename)

    if output_path is None:
        print_red(f'Could not find {output_path}')
        sys.exit(1)
    print_green(f'Copying {output_path} to {wanted_path}')
    wanted_path.unlink(missing_ok=True)
    os.rename(output_path, wanted_path)

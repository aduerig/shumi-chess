from setuptools import setup, Extension
import sys
import pathlib
import os

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent

sys.path.insert(0, str(root_of_project_directory.joinpath('scripts')))
from helpers import *


if is_windows():
    # force a valid Windows platform name so Python 3.13 doesn't error
    if not any(arg.startswith("--plat-name") for arg in sys.argv):
        sys.argv.append("--plat-name=win-amd64")


release_mode = 'release'
if '--release' in sys.argv:
    del sys.argv[sys.argv.index('--release')]
if '--debug' in sys.argv:
    release_mode = 'debug'
    del sys.argv[sys.argv.index('--debug')]

print_cyan(f'building with {release_mode=}, {root_of_project_directory=}, {this_file_directory=}')

lib_dir = root_of_project_directory.joinpath('build', 'lib')

extra_compile_args=['-std=c++17']
if is_windows():
    extra_compile_args = ['/std:c++17']


config_subdir = 'Release' if release_mode == 'release' else 'Debug'
if is_windows():
    lib_path = lib_dir.joinpath(config_subdir, 'ShumiChess.lib')
    extra_link_args = [str(lib_path)]
else:
    lib_path = lib_dir.joinpath(config_subdir, 'libShumiChess.a')
    extra_link_args = [str(lib_path)]


if release_mode == 'debug':
    if is_windows():
        extra_compile_args += ['/Zi', '/Od', '/MDd', '/D_DEBUG']
    else:
        extra_compile_args += ['-g', '-O0']
else:
    if not is_windows():
        extra_compile_args += ['-O3', '-ffast-math']

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

# if is_windows():
#     last_modified = float('-inf')
#     output_path = None
    # for filename, filepath in get_all_paths(this_file_directory.joinpath('build'), recursive=True, allowed_extensions=set(['.pyd'])):
    #     print(filename)
    #     if filepath.stat().st_mtime > last_modified:
    #         last_modified = filepath.stat().st_mtime
    #         output_path = filepath
    # wanted_path = root_of_project_directory.joinpath('driver', filename)

    # if output_path is None:
    #     print_red(f'Could not find {output_path}')
    #     sys.exit(1)
    # print_green(f'Copying {output_path} to {wanted_path}')
    # wanted_path.unlink(missing_ok=True)
    # os.rename(output_path, wanted_path)

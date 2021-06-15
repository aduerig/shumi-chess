import setuptools
import distutils.cygwinccompiler
from distutils.core import setup, Extension
import os
from os import path
import sys

distutils.cygwinccompiler.get_msvcr = lambda: []

steps_to_root = 1

if '--steps_to_root' in sys.argv:
    index = sys.argv.index('--steps_to_root')
    steps_to_root = int(sys.argv[index + 1])
    del sys.argv[index + 1]
    del sys.argv[index]

script_dir, _ = path.split(path.abspath(__file__))
print(script_dir)
root_dir = script_dir
for i in range(steps_to_root):
    root_dir = path.join(root_dir, '..')

print('thinking root directory of project is {}'.format(root_dir))

the_module = Extension(
    'engine_communicator',
    sources = [path.join(script_dir, 'engine_communicatormodule.cpp')],
    include_dirs = [path.join(root_dir, 'src')],
    library_dirs = [path.join(root_dir, 'lib')],
    # tries to do a .so (dynamic) build with this
    # libraries = ['ShumiChess'],
    extra_compile_args=['-std=c++17'],
    extra_link_args=[path.join(root_dir, 'lib', 'libShumiChess.a')]
    # needed for windows, doesnt work on linux:
    # extra_link_args=['../lib/libShumiChess.a', '-static', '-static-libgcc', '-static-libstdc++']
)

setup(
    name = 'engine_communicator',
    version = '1.0',
    description = 'To communicate with ShumiChess C++ backend',
    ext_modules = [the_module]
)

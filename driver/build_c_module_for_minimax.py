# python build_c_module_for_minimax.py install

import setuptools
import distutils.cygwinccompiler
from distutils.core import setup, Extension
from os import path
import os

from helpers import *

distutils.cygwinccompiler.get_msvcr = lambda: []


script_dir, _ = path.split(path.abspath(__file__))
root_dir = path.normpath(path.join(script_dir, os.pardir))

print_cyan('thinking script_dir directory of project is {}'.format(script_dir))
print_cyan('thinking root directory of project is {}'.format(root_dir))

the_module = Extension(
    'minimax_ai',
    sources = [path.join(script_dir, 'minimax_aimodule.cpp'), path.join(root_dir, 'src', 'minimax.cpp')],
    include_dirs = [path.join(root_dir, 'src')],
    library_dirs = [path.join(root_dir, 'lib')],
    # tries to do a .so (dynamic) build with this
    # libraries = ['ShumiChess'],
    # libraries = ['rt'],
    extra_compile_args=['-std=c++17'],
    extra_link_args=[path.join(root_dir, 'lib', 'libShumiChess.a')]
    # needed for windows, doesnt work on linux:
    # extra_link_args=['../lib/libShumiChess.a', '-static', '-static-libgcc', '-static-libstdc++']
)

setup(
    name = 'minimax_ai',
    version = '1.0',
    description = 'Acts as a minimax AI for the ShumiChess AI',
    ext_modules = [the_module]
)

import setuptools
import distutils.cygwinccompiler
from distutils.core import setup, Extension
import os
from os import path

distutils.cygwinccompiler.get_msvcr = lambda: []

execution_dir = os.getcwd()

the_module = Extension(
    'engine_communicator',
    sources = [path.join('driver', 'engine_communicatormodule.cpp')],
    include_dirs = [path.join(execution_dir, 'src')],
    library_dirs = [path.join(execution_dir, 'lib')],
    # tries to do a .so (dynamic) build with this
    # libraries = ['ShumiChess'],
    extra_compile_args=['-std=c++17'],
    extra_link_args=[path.join(execution_dir, 'lib', 'libShumiChess.a')]
    # needed for windows, doesnt work on linux:
    # extra_link_args=['../lib/libShumiChess.a', '-static', '-static-libgcc', '-static-libstdc++']
)

setup(
    name = 'engine_communicator',
    version = '1.0',
    description = 'To communicate with ShumiChess C++ backend',
    ext_modules = [the_module]
)

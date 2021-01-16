import setuptools
from distutils.core import setup, Extension

the_module = Extension(
    'engine_communicator',
    sources = ['engine_communicatormodule.cpp'],
    include_dirs = ['../src'],
    library_dirs = ['../lib'],
    # tries to do a .so (dynamic) build with this
    # libraries = ['ShumiChess'],
    extra_compile_args=['-std=c++17'],
    extra_link_args=['../lib/libShumiChess.a', '-static', '-static-libgcc', '-static-libstdc++']
)

setup(
    name = 'engine_communicator',
    version = '1.0',
    description = 'To communicate with ShumiChess C++ backend',
    ext_modules = [the_module]
)

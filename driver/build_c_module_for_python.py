from distutils.core import setup, Extension

the_module = Extension('engine_communicator',
                    sources = ['engine_communicatormodule.cpp'])

setup(name = 'engine_communicator',
       version = '1.0',
       description = 'To communicate with ShumiChess C++ backend',
       ext_modules = [the_module])

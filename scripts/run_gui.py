import sys
import pathlib
import argparse

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent
sys.path.insert(0, str(root_of_project_directory))

from helpers import *


parser = argparse.ArgumentParser(description='Run the GUI')
parser.add_argument('--debug', dest='debug', default=False, action='store_true')
parser.add_argument('--build-tests', dest='build_tests', default=False, action='store_true')
args = parser.parse_args()


build_c_module_for_python_path = root_of_project_directory.joinpath('driver', 'build_c_module_for_python.py')
show_board_path = root_of_project_directory.joinpath('driver', 'show_board.py')
print_cyan(f'{show_board_path=}, {build_c_module_for_python_path=}')

build_type = 'Release'
if args.debug:
    build_type = 'Debug'

build_tests = 'OFF'
if args.build_tests:
    build_tests = 'ON'

print_cyan('==== STEP 1 =====')
return_code, stdout, stderr = run_command_blocking([
    'cmake',
    str(root_of_project_directory.joinpath('CMakeLists.txt')),
    '-Wno-dev',
    f'-DCMAKE_BUILD_TYPE={build_type}',
    f'-DBUILD_TESTS={build_tests}',
], stdout_pipe=None, stderr_pipe=None)
if return_code:
    print_red('cmake CMakeLists.txt FAILED, exiting')
    sys.exit(1)
print_green(f'cmake CMakeLists.txt SUCCEEDED, {return_code=}')

print_cyan('==== STEP 2 =====')
return_code, stdout, stderr = run_command_blocking([
    'cmake',
    '--build',
    str(root_of_project_directory.joinpath('.')),
], stdout_pipe=None, stderr_pipe=None)
if return_code:
    print_red('cmake build . FAILED')
    sys.exit(1)

print_green(f'cmake build . SUCCEEDED, {return_code=}')

print_cyan('==== STEP 3 =====')
cmd_options = [
    'python',
    str(build_c_module_for_python_path),
    'build',
    '--force',
    '--build-lib=driver', 
    '--build-temp=driver/build',
]
if is_windows():
    cmd_options.append('--compiler=mingw32')
return_code, stdout, stderr = run_command_blocking(cmd_options, stdout_pipe=None, stderr_pipe=None)
if return_code:
    print_red('driver/build_c_module_for_python.py FAILED, exiting')
    sys.exit(1)

print_green(f'python build_c_module_for_python.py SUCCEEDED, {return_code=}')

return_code, stdout, stderr = run_command_blocking([
    'python',
    str(show_board_path),
], stdout_pipe=None, stderr_pipe=None)


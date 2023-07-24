import sys
import pathlib

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent
sys.path.insert(0, str(root_of_project_directory))

from helpers import *


build_c_module_for_python_path = root_of_project_directory.joinpath('driver', 'build_c_module_for_python.py')
show_board_path = root_of_project_directory.joinpath('driver', 'show_board.py')
print_cyan(f'{show_board_path=}, {build_c_module_for_python_path=}')

build_type = 'debug'

return_code, stdout, stderr = run_command_blocking([
    'cmake',
    'CMakeLists.txt',
    '-Wno-dev',
    f'DCMAKE_BUILD_TYPE={build_type}',
])
if return_code:
    print_red('cmake CMakeLists.txt FAILED, exiting')
    sys.exit(1)

return_code, stdout, stderr = run_command_blocking([
    'cmake',
    '--build',
    '.',
])
if return_code:
    print_red('cmake --build . FAILED, exiting')
    sys.exit(1)


return_code, stdout, stderr = run_command_blocking([
    'python',
    str(build_c_module_for_python_path),
    'build',
    '--compiler=mingw32',
    '--force',
    '--build-lib=driver', 
    '--build-temp=driver/build',
])
if return_code:
    print_red('driver/build_c_module_for_python.py FAILED, exiting')
    sys.exit(1)


return_code, stdout, stderr = run_command_blocking([
    'python',
    str(show_board_path),
])


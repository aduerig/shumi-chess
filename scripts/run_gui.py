import sys
import pathlib

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent
sys.path.insert(0, str(root_of_project_directory))

from helpers import *


build_c_module_for_python_path = root_of_project_directory.joinpath('driver', 'build_c_module_for_python.py')
show_board_path = root_of_project_directory.joinpath('driver', 'show_board.py')
print_cyan(f'{show_board_path=}, {build_c_module_for_python_path=}')

build_type = 'Debug'

print_cyan('==== STEP 1 =====')
return_code, stdout, stderr = run_command_blocking([
    'cmake',
    str(root_of_project_directory.joinpath('CMakeLists.txt')),
    '-Wno-dev',
    f'-DCMAKE_BUILD_TYPE={build_type}',
])
if return_code:
    print_red('cmake CMakeLists.txt FAILED, exiting')
    sys.exit(1)

print_green(f'cmake CMakeLists.txt SUCCEEDED, {return_code=}, stdout:')
print(stdout)
print_yellow('stderr (probably warnings) was:')
print(stderr)

print_cyan('==== STEP 2 =====')
return_code, stdout, stderr = run_command_blocking([
    'cmake',
    '--build',
    str(root_of_project_directory.joinpath('.')),
])
if return_code:
    print_red('cmake build . FAILED')
    print(f'stdout was:')
    print_blue(stdout)
    print(f'stderr was:')
    print_red(stderr)
    sys.exit(1)


print_green(f'cmake build . SUCCEEDED, {return_code=}, stdout:')
print(stdout)
print_yellow('stderr:', stderr)

print_cyan('==== STEP 3 =====')
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


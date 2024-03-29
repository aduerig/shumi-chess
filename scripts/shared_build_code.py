import sys
import pathlib

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent
sys.path.insert(0, str(root_of_project_directory))
sys.path.insert(0, str(this_file_directory))

from helpers import *


build_c_module_for_python_path = root_of_project_directory.joinpath('driver', 'build_c_module_for_python.py')
show_board_path = root_of_project_directory.joinpath('driver', 'show_board.py')
bin_dir = root_of_project_directory.joinpath('bin')
print_cyan(f'{show_board_path=}, {build_c_module_for_python_path=}')
def build_shumi_chess(release, build_tests):
    build_tests_str = 'OFF'
    if build_tests:
        build_tests_str = 'ON'
    
    build_type_str = 'Debug'
    if release:
        build_type_str = 'Release'

    print_cyan('==== BUILDING SHUMICHESS STEP 1 =====')
    cmd = [
        'cmake',
        str(root_of_project_directory.joinpath('CMakeLists.txt')),
        '-Wno-dev',
        f'-DCMAKE_BUILD_TYPE={build_type_str}',
        f'-DBUILD_TESTS={build_tests_str}',
    ]
    # if is_windows():
    #     cmd.insert(0, 'CC=gcc')
    return_code, _stdout, _stderr = run_command_blocking(cmd, stdout_pipe=None, stderr_pipe=None, debug=True)
    if return_code:
        sys.exit(1)

    print_cyan('==== BUILDING SHUMICHESS STEP 2 =====')
    return_code, _stdout, _stderr = run_command_blocking([
        'cmake',
        '--build',
        str(root_of_project_directory.joinpath('.')),
    ], stdout_pipe=None, stderr_pipe=None, debug=True)
    if return_code:
        sys.exit(1)

def build_python_gui_module(release):
    build_type_str = 'Debug'
    if release:
        build_type_str = 'Release'

    print_cyan('==== BULIDING PYTHON GUI MODULE =====')
    cmd_options = [
        'python',
        str(build_c_module_for_python_path),
        'build',
        '--force',
        '--build-lib=driver', 
        '--build-temp=driver/build',
        f'--{build_type_str.lower()}',
    ]
    if is_windows():
        cmd_options.append('--compiler=mingw32')
    return_code, _stdout, _stderr = run_command_blocking(cmd_options, stdout_pipe=None, stderr_pipe=None, debug=True)
    if return_code:
        sys.exit(1)

def run_python_gui():
    _return_code, _stdout, _stderr = run_command_blocking([
        'python',
        str(show_board_path),
    ], stdout_pipe=None, stderr_pipe=None)
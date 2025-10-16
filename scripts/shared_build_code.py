import sys
import pathlib

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent
sys.path.insert(0, str(root_of_project_directory))
sys.path.insert(0, str(this_file_directory))

from helpers import *


build_c_module_for_python_path = root_of_project_directory.joinpath('driver', 'build_c_module_for_python.py')
show_board_path = root_of_project_directory.joinpath('driver', 'show_board.py')
bin_dir = root_of_project_directory.joinpath('build').joinpath('bin')


print_cyan(f'{show_board_path=}, {build_c_module_for_python_path=}')
def build_shumi_chess(release, build_tests):
    build_tests_str = 'OFF'
    if build_tests:
        build_tests_str = 'ON'
    
    build_type_str = 'Debug'
    if release:
        build_type_str = 'Release'

    print_cyan('==== BUILDING SHUMICHESS STEP 1 (CONFIGURE) =====')
    cmd = [
        'cmake',
        '-S', str(root_of_project_directory),
        '-B', str(root_of_project_directory.joinpath('build')), # Use a dedicated build folder
        '-Wno-dev',
        f'-DCMAKE_BUILD_TYPE={build_type_str}',
        f'-DBUILD_TESTS={build_tests_str}',
    ]
    return_code, _stdout, _stderr = run_command_blocking(cmd, stdout_pipe=None, stderr_pipe=None, debug=True)
    if return_code:
        sys.exit(1)

    print_cyan('==== BUILDING SHUMICHESS STEP 2 (BUILD) =====')
    cmd_build = [
        'cmake',
        '--build',
        str(root_of_project_directory.joinpath('build')),
        '--config',
        build_type_str
    ]
    return_code, _stdout, _stderr = run_command_blocking(cmd_build, stdout_pipe=None, stderr_pipe=None, debug=True)
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
        # cmd_options.append('--compiler=mingw32')
        cmd_options.append('--compiler=msvc')
    return_code, _stdout, _stderr = run_command_blocking(cmd_options, stdout_pipe=None, stderr_pipe=None, debug=True)
    if return_code:
        sys.exit(1)

def run_python_gui(fen, human):
    cmd_line = [
        'python',
        str(show_board_path)
    ]
    if fen is not None:
        cmd_line.extend(['--fen', fen])
    if human:
        cmd_line.extend(['--human'])
    _return_code, _stdout, _stderr = run_command_blocking(cmd_line, stdout_pipe=None, stderr_pipe=None)
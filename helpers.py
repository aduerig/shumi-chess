import os
import pathlib
import subprocess


def is_windows():
    import platform
    plt = platform.system()
    if plt == "Windows":
        return True
    return False


def is_linux():
    import platform
    plt = platform.system()
    if plt == "Linux":
        return True
    return False


def is_macos():
    import platform
    plt = platform.system()
    if plt == "Darwin":
        return True
    return False


def is_marias_computer():
    import socket
    return socket.gethostname() in ['DESKTOP-IKO6828']


def is_image_path(path):
    return path.endswith('.jpg') or path.endswith('.png') or path.endswith('.webp')


def is_ray():
    import socket
    return socket.gethostname() in ['ray']


def is_erics_laptop():
    import socket
    return socket.gethostname() in ['LAPTOP-ERIC']


def is_andrews_main_computer():
    import socket
    return socket.gethostname() in ['zetai']


def is_andrews_laptop():
    import socket
    return socket.gethostname() in ['DESKTOP-754BOFE']


def is_doorbell():
    import socket
    return socket.gethostname() in ['doorbell']


ray_is_active_andrew = False
def get_ray_directory():
    global ray_is_active_andrew
    if is_ray():
        return pathlib.Path('T:/')
    elif is_andrews_main_computer():
        mount_path = pathlib.Path('/mnt/ray_network_share')
        if ray_is_active_andrew:
            return mount_path
        _, stdout, _ = run_command_blocking([
            'ls',
            str(mount_path),
        ])
        if stdout:
            return mount_path
        else:
            print_red('Cannot find any files in {mount_path}, you probably need to run "sudo mount -t cifs -o username=${USER},password=${PASSWORD},uid=$(id -u),gid=$(id -g) //192.168.86.210/T /mnt/ray_network_share/"')
            exit()
    else:
        print_red('doesnt know how contact ray_directory')

nas_is_active_andrew = False
def get_nas_directory():
    global nas_is_active_andrew
    if is_andrews_main_computer():
        mount_path = pathlib.Path('/mnt/nas')
        if nas_is_active_andrew:
            return mount_path
        _, stdout, _ = run_command_blocking([
            'ls',
            str(mount_path),
        ])
        if stdout:
            return mount_path
        else:
            print_red('Cannot find any files in {mount_path}, you probably need to run "sudo mount -t cifs -o username=crammem,password=#Cumbr1dge,uid=$(id -u),gid=$(id -g) //192.168.86.75/Raymond /mnt/nas/"')
            exit()
    else:
        print_red('doesnt know how contact nas_directory')


def get_stack_trace() -> str:
    import traceback
    return traceback.format_exc()


def print_stacktrace() -> None:
    print_red(get_stack_trace())


video_extensions = set(map(lambda x: x.lower(), ['.WEBM', '.MPG', '.MP2', '.MPEG', '.MPE', '.MPV', '.OGG', '.MP4', '.M4P', '.M4V', '.AVI', '.WMV', '.MOV', '.QT', '.FLV', '.SWF', '.AVCHD', '.mkv']))


doorbell_ip = '192.168.86.55'
ssh_connection = None
def maybe_open_ssh_connection_doorbell():
    global ssh_connection
    import paramiko

    if ssh_connection is not None and ssh_connection.get_transport() and ssh_connection.get_transport().is_active:
        return ssh_connection
    
    print_cyan('opening ssh_connection to doorbell')
    ssh_connection = paramiko.client.SSHClient()
    ssh_connection.load_system_host_keys()
    ssh_connection.connect(hostname=doorbell_ip,
                port = 22,
                username='pi')


# looks like most people use https://www.fabfile.org/ for the higher level library
scp_connection = None
def maybe_open_scp_connection_doorbell():
    global scp_connection
    from scp import SCPClient

    if scp_connection is not None and scp_connection.transport and scp_connection.transport.is_active:
        return scp_connection

    print_cyan('opening scp_connection to doorbell')    
    maybe_open_ssh_connection_doorbell()
    scp_connection = SCPClient(ssh_connection.get_transport())


def close_connections_to_doorbell():
    if scp_connection is not None and scp_connection.transport and scp_connection.transport.is_active:
        print_cyan('closing ssh_connection')
        scp_connection.close()
        return
    
    if ssh_connection is not None and ssh_connection.get_transport() and ssh_connection.get_transport().is_active:
        print_cyan('closing ssh_connection')
        ssh_connection.close()


def run_command_on_doorbell_via_ssh(command, keep_open=False):
    print_yellow('andrew: trying this new extra scp step on error (assuming rekord_box folder doesnt exist)')

    maybe_open_ssh_connection_doorbell()
    _stdin, _stdout, _stderr = ssh_connection.exec_command(command)
    if not keep_open:
        close_connections_to_doorbell()
    return _stdin, _stdout, _stderr


def scp_to_doorbell(local_filepath, remote_folder, keep_open=False):
    remote_filepath = remote_folder.joinpath(local_filepath.name)

    if is_windows():
        remote_filepath = str(remote_filepath).replace('\\\\', '/').replace('\\', '/')

    print(f'{bcolors.OKBLUE}Moving from "{local_filepath}", to remote "{doorbell_ip}:{remote_filepath}"{bcolors.ENDC}')
    maybe_open_scp_connection_doorbell()
    scp_connection.put(str(local_filepath), remote_filepath)

    if not keep_open:
        close_connections_to_doorbell()


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def yellow(s):
    return f'{bcolors.WARNING}{s}{bcolors.ENDC}'

def green(s):
    return f'{bcolors.OKGREEN}{s}{bcolors.ENDC}'

def cyan(s):
    return f'{bcolors.OKCYAN}{s}{bcolors.ENDC}'

def bold(s):
    return f'{bcolors.BOLD}{s}{bcolors.ENDC}'

def blue(s):
    return f'{bcolors.OKBLUE}{s}{bcolors.ENDC}'

def red(s):
    return f'{bcolors.FAIL}{s}{bcolors.ENDC}'


disable_color_flag = False
# I think this is called "true color" which is 24 bit color
def rgb_ansi(text, rgb_tuple):
    if disable_color_flag:
        return text
    rgb_style = f'38;2;{rgb_tuple[0]};{rgb_tuple[1]};{rgb_tuple[2]}'
    return f'\033[{rgb_style}m{text}\033[0m'


def disable_color():
    global disable_color_flag
    disable_color_flag = True
    global bcolors
    bcolors.HEADER = ''
    bcolors.OKBLUE = ''
    bcolors.OKCYAN = ''
    bcolors.OKGREEN = ''
    bcolors.WARNING = ''
    bcolors.FAIL = ''
    bcolors.ENDC = ''
    bcolors.BOLD = ''
    bcolors.UNDERLINE = ''


def print_yellow(*args, **kwargs):
    args = map(str, args)
    print(yellow(' '.join(args)), **kwargs)

def print_green(*args, **kwargs):
    args = map(str, args)
    print(green(' '.join(args)), **kwargs)

def print_cyan(*args, **kwargs):
    args = map(str, args)
    print(cyan(' '.join(args)), **kwargs)

def print_bold(*args, **kwargs):
    args = map(str, args)
    print(bold(' '.join(args)), **kwargs)

def print_blue(*args, **kwargs):
    args = map(str, args)
    print(blue(' '.join(args)), **kwargs)

def print_red(*args, **kwargs):
    args = map(str, args)
    print(red(' '.join(args)), **kwargs)


# !TODO straight up i think this might be right lol
def get_clean_filesystem_string(s):
    # cleaned = ''.join(char for char in s if char.isalnum() or char in ' -_.()[],')
    # print_blue('get_clean_filesystem_string():', s, 'cleaned:', cleaned)
    # return cleaned
    return s

def get_no_duplicate_spaces(s):
    import re
    return re.sub(r"\s+", " ", s)


def random_letters(num_chars: int) -> str:
    import random
    letters = [chr(ord('a') + a) for a in range(26)]
    return ''.join(random.sample(letters, num_chars))


def get_all_paths(directory, only_files=False, exclude_names=None, recursive=False, allowed_extensions=None, quiet=False):
    if not isinstance(directory, pathlib.Path):
        directory = pathlib.Path(directory)
    if not directory.exists():
        if not quiet:
            print_yellow(f'{directory} does not exist, returning [] for paths')
        return []
    paths = []
    for filename in os.listdir(directory):
        if exclude_names is not None and filename in exclude_names:
            continue
        filepath = pathlib.Path(directory).joinpath(filename)
        
        if filepath.is_file():
            if allowed_extensions is not None and filepath.suffix not in allowed_extensions:
                continue
            paths.append((filename, filepath))
        # print(f'{filepath=}', filepath.is_dir())
        if filepath.is_dir() and recursive:
            # !TODO add this
            if only_files:
                pass
            
            paths += get_all_paths(filepath, only_files=only_files, exclude_names=exclude_names, recursive=recursive, allowed_extensions=allowed_extensions, quiet=quiet)
    return paths

def start_video_in_mpv_async(video_path, volume=70):
    print(f'start_video_in_mpv: starting "{video_path}"')
    run_command_async([
        'mpv', 
        str(video_path), 
        '--no-resume-playback',
        '--sid=no',
        '--title=mpv_vtuber_window',
        '--x11-name=mpv_vtuber_window',
        f'--volume={volume}',
    ], debug=True)

def is_linux_root():
    return is_linux() and os.geteuid() == 0

def run_command_blocking(full_command_arr, timeout=None, debug=False, stdin_pipe=None, stdout_pipe=subprocess.PIPE, stderr_pipe=subprocess.PIPE):
    for index in range(len(full_command_arr)):
        cmd = full_command_arr[index]
        if type(cmd) != str:
            print_yellow(f'WARNING: the parameter "cmd" was not a str, casting and continuing')
            full_command_arr[index] = str(full_command_arr[index])

    if is_windows():
        if full_command_arr[0] in ['ffmpeg', 'ffplay', 'ffprobe']:
            full_command_arr[0] += '.exe'

    full_call = full_command_arr[0] + ' ' + ' '.join(map(lambda x: f'"{x}"', full_command_arr[1:]))
    if debug:
        print(f'going to run "{full_call}"')
    
    # env = os.environ.copy() # env['SSH_AUTH_SOCK'] = os.
    process = subprocess.Popen(full_command_arr, stdout=stdout_pipe, stderr=stderr_pipe, stdin=stdin_pipe)
    stdout, stderr = process.communicate(timeout=timeout)

    if debug:
        print(f'Finished execution, return code was {process.returncode}')

    if stdout is not None:
        stdout = stdout.decode("utf-8")
    if stderr is not None:
        stderr = stderr.decode("utf-8")

    if process.returncode:
        print_red(f'FAILURE executing "{full_call}"')
        if stdout:
            print('stdout', stdout)
        if stderr:
            print_red('stderr', stderr)
    elif debug:
        print_green(f'SUCCESS executing "{full_call}"')
        if stdout:
            print('stdout', stdout)
        if stderr:
            print_red('stderr', stderr)
        
    return process.returncode, stdout, stderr


def make_if_not_exist(output_dir, quiet=False):
    if not os.path.exists(output_dir):
        if not quiet:
            print_yellow(f'Creating {output_dir} since it didn\'t exist')
        os.mkdir(output_dir)
    return output_dir


def get_temp_dir():
    return make_if_not_exist(pathlib.Path(__file__).parent.joinpath('temp'))


def run_command_async(full_command_arr, debug=False, stdin=None):
    import subprocess

    for index in range(len(full_command_arr)):
        cmd = full_command_arr[index]
        if type(cmd) != str:
            print_yellow(f'the parameter "cmd" was not a str, casting and continuing')
            full_command_arr[index] = str(full_command_arr[index])

    if is_windows():
        if full_command_arr[0] in ['ffmpeg', 'ffplay']:
            full_command_arr[0] += '.exe'

    full_call = full_command_arr[0] + ' ' + ' '.join(map(lambda x: f'"{x}"', full_command_arr[1:]))
    process = subprocess.Popen(full_command_arr, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=stdin)
    
    if debug:
        print(f'started process with "{full_call}"')
    return process


# -ss '120534ms'
def seconds_to_fmmpeg_ms_string(seconds):
    return str(int(seconds * 1000)) + 'ms'

# 63 -> '00:06:03'
def seconds_to_hmsm_string(seconds) -> str:
    seconds = float(seconds)
    milliseconds = seconds % 1
    seconds = int(seconds)
    minutes = seconds // 60
    hours = str(minutes // 60).zfill(2)
    minutes = str(minutes % 60).zfill(2)
    seconds = str(seconds % 60).zfill(2)
    milliseconds = str(milliseconds)[1:4].ljust(4, '0')

    agged = f'{hours}:{minutes}:{seconds}{milliseconds}'
    return agged


# argument stuff
    # global args
    # parser = argparse.ArgumentParser()
    # parser.add_argument('--testing', action='store_true', default=False,
    #                help='To run without rasberry pi support')
    # args = parser.parse_args()






# make directory above importable
# import sys
# import pathlib

# def make_directory_above_importable():
#     path_above_file = pathlib.Path(__file__).parent.joinpath('..').resolve()
#     sys.path.insert(0, str(path_above_file))


# go up a line: '\033[A'
# up a line and begining: '\033[F'


# disabling std out
# if is_windows():
#     null_std_out = open('nul', 'w')
# elif is_linux() or is_macos():
#     null_std_out = open('/dev/null', 'w')


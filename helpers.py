import os
import pathlib
import subprocess
import sys
import platform
import time

# 
try:
    profile
except NameError:
    profile = lambda x: x

helpers_directory = pathlib.Path(__file__).parent.resolve()


def get_datetime_str(ms=False):
    import datetime
    if ms:
        return datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')
    return datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

def unix_to_human_readable(unix_time):
    import datetime
    return datetime.datetime.utcfromtimestamp(unix_time).strftime('%Y-%m-%d %H:%M:%S')

def is_image_path(path):
    return path.endswith('.jpg') or path.endswith('.png') or path.endswith('.webp')

def get_hostname():
    import socket
    return socket.gethostname()

is_windows = lambda: platform.system() == "Windows"
is_linux = lambda: platform.system() == "Linux"
is_macos = lambda: platform.system() == "Darwin"

is_raspberry_pi = lambda: is_linux() and pathlib.Path('/boot/config.txt').exists()

is_moneymaker = lambda: get_hostname() == 'moneymaker'
is_jesse = lambda: get_hostname() == 'jesse'
is_andrewpi = lambda: get_hostname() == 'andrewpi'
is_meschpi = lambda: get_hostname() == 'meschpi'
is_marias_computer = lambda: get_hostname() == 'DESKTOP-IKO6828'
is_ray = lambda: get_hostname() == 'ray'
is_erics_laptop = lambda: get_hostname() == 'Eric-Laptop'
is_andrews_main_computer = lambda: get_hostname() == 'zetai'
is_andrews_laptop = lambda: get_hostname() == 'DESKTOP-754BOFE'
is_doorbell = lambda: get_hostname() == 'doorbell'
is_dj = lambda: get_hostname() == 'DESKTOP-6HUD9AG'

def is_screensaver_running():
    if not is_ray():
        print_yellow('is_screensaver_running() was called, but on a computer that isnt ray, returning False')
        return False

    import psutil
    for process in psutil.process_iter():
        try:
            if 'python.exe' in process.name():
                args = process.cmdline()
                if len(args) > 1:
                    if args[1].endswith('screensaver.py'):
                        if len(args) > 2 and args[2].lower() == '/s':
                            return True
        except psutil.NoSuchProcess:
            pass
    return False


def get_eric_directory():
    if is_andrews_main_computer():
        mount_path = pathlib.Path('/mnt/eric_network_share')
        if not run_command_blocking(['ls', str(mount_path)])[1]:
            return print_red(f'Cannot find any files in {mount_path}')
        return mount_path
    elif is_windows(): 
        return pathlib.Path(r'\\ERIC-DESKTOP\Network')
    print_red('dont know how contact ray_directory')


def get_ray_directory():
    if is_ray() or is_erics_laptop() or is_dj():
        return pathlib.Path('T:/')
    elif is_andrews_main_computer():
        mount_path = pathlib.Path('/mnt/ray_network_share')
        # hangs on vpm?
        # if not run_command_blocking(['ls', str(mount_path)])[1]:
        #     return print_red(f'Cannot find any files in {mount_path}')
        return mount_path
    elif is_windows():
        return pathlib.Path(r'\\Ray\T')
    elif is_macos():
        return pathlib.Path('/Volumes/ray/')
    print_red('dont know how contact ray_directory')


def get_nas_directory():
    if is_andrews_main_computer() or is_andrews_laptop():
        mount_path = pathlib.Path('/mnt/nas')
        if not run_command_blocking(['ls', str(mount_path)])[1]:
            return print_red(f'Cannot find any files in {mount_path}')
        return mount_path
    print_red('dont know how contact nas_directory')


def get_stack_trace():
    import traceback
    return traceback.format_exc()


def print_stacktrace():
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
    return scp_connection


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

def rgb_ansi(text, rgb): # I think this is called "true color" which is 24 bit color
    rgb_style = f'38;2;{rgb[0]};{rgb[1]};{rgb[2]}'
    return f'\033[{rgb_style}m{text}\033[0m'

def disable_color():
    global bcolors, rgb_ansi
    bcolors.HEADER = ''
    bcolors.OKBLUE = ''
    bcolors.OKCYAN = ''
    bcolors.OKGREEN = ''
    bcolors.WARNING = ''
    bcolors.FAIL = ''
    bcolors.ENDC = ''
    bcolors.BOLD = ''
    bcolors.UNDERLINE = ''
    rgb_ansi = lambda text, rgb: text

if not sys.stdout.isatty(): # if outputting to file
    disable_color()

yellow = lambda s: f'{bcolors.WARNING}{s}{bcolors.ENDC}'
green = lambda s: f'{bcolors.OKGREEN}{s}{bcolors.ENDC}'
cyan = lambda s: f'{bcolors.OKCYAN}{s}{bcolors.ENDC}'
bold = lambda s: f'{bcolors.BOLD}{s}{bcolors.ENDC}'
blue = lambda s: f'{bcolors.OKBLUE}{s}{bcolors.ENDC}'
red = lambda s: f'{bcolors.FAIL}{s}{bcolors.ENDC}'


color_tracker = 0
avail_colors = [(yellow, 'yellow'), (green, 'green'), (cyan, 'cyan'), (blue, 'blue'), (red, 'red')]
def next_color_func(skip=None):
    global color_tracker
    while True:
        color_tracker = (color_tracker + 1) % len(avail_colors)
        if skip is None or avail_colors[color_tracker][1] not in skip:
            break
    return avail_colors[color_tracker][0]

def next_color(s, skip=None):
    return next_color_func(skip)(s)


def print_yellow(*args, **kwargs):
    print(yellow(' '.join(map(str, args))), **kwargs)

def print_green(*args, **kwargs):
    print(green(' '.join(map(str, args))), **kwargs)

def print_cyan(*args, **kwargs):
    print(cyan(' '.join(map(str, args))), **kwargs)

def print_bold(*args, **kwargs):
    print(bold(' '.join(map(str, args))), **kwargs)

def print_blue(*args, **kwargs):
    print(blue(' '.join(map(str, args))), **kwargs)

def print_red(*args, **kwargs):
    print(red(' '.join(map(str, args))), **kwargs)


def get_no_duplicate_spaces(s):
    import re
    return re.sub(r"\s+", " ", s)


def random_letters(num_chars):
    import random
    letters = [chr(ord('a') + a) for a in range(26)]
    return ''.join(random.sample(letters, num_chars))


def get_all_paths(directory, only_files=False, exclude_names=None, recursive=False, allowed_extensions=None, quiet=False):
    directory = pathlib.Path(directory)

    if not directory.exists():
        if not quiet:
            print(f'{directory} does not exist, returning [] for paths')
        return

    exclude_names = set(exclude_names or [])

    for entry in directory.iterdir():
        if entry.name in exclude_names:
            continue
        
        if entry.is_file():
            if allowed_extensions and entry.suffix not in allowed_extensions:
                continue
            yield entry.name, entry
        elif entry.is_dir() and recursive:
            yield from get_all_paths(entry, only_files, exclude_names, recursive, allowed_extensions, quiet)
        elif not only_files:
            yield entry.name, entry


def get_local_ip():
    import socket
    try:
        info = socket.gethostbyname_ex(socket.gethostname())
        return info[2][0]
    except:
        return 'cant_resolve_hostbyname'


def length_without_ansi_codes(s):
    import re
    return len(re.sub('\x1b\[\d+(;\d+)*m', '', s))

def http_server_blocking(port, directory_to_serve, for_printing=['']):
    import http.server
    class CustomHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
        def translate_path(self, path):
            return os.path.join(str(directory_to_serve), path.lstrip("/"))
    for service in for_printing:
        print_green(f'http://{get_local_ip()}:{port}/{service}', flush=True)
    http.server.ThreadingHTTPServer(('', port), CustomHTTPRequestHandler).serve_forever()
    

def http_server_async(port, filepath_to_serve, for_printing=['']):
    import threading
    threading.Thread(target=http_server_blocking, args=(port, filepath_to_serve, for_printing)).start()


def is_python_32_bit():
    return sys.maxsize > 2**32

def is_linux_root():
    return is_linux() and os.geteuid() == 0


def make_if_not_exist(output_dir, quiet=False):
    output_dir = pathlib.Path(output_dir)
    if not output_dir.exists():
        if not quiet:
            print_yellow(f'Creating {output_dir} since it didn\'t exist')
        os.makedirs(output_dir)
    return output_dir


def get_temp_dir(inner_dir_name=None):
    temp_dir = make_if_not_exist(pathlib.Path(__file__).parent.joinpath('temp'))
    if inner_dir_name is None:
        return temp_dir
    return make_if_not_exist(temp_dir.joinpath(inner_dir_name)) 

def get_sub_temp_dir(name=None):
    if name is None:
        name = random_letters(15)
    return make_if_not_exist(get_temp_dir().joinpath(name))

def get_temp_file(name=None, suffix=None):
    if name is None:
        name = random_letters(15)
    if suffix is not None:
        return get_temp_dir().joinpath(name).with_suffix(suffix)
    return get_temp_dir().joinpath(name)


def run_command_blocking(full_command_arr, timeout=None, debug=False, stdin_pipe=None, stdout_pipe=subprocess.PIPE, stderr_pipe=subprocess.PIPE, timing=False, shell=False):
    start_time = time.time()
    for index in range(len(full_command_arr)):
        cmd = full_command_arr[index]
        if type(cmd) != str:
            if not isinstance(cmd, pathlib.Path):
                print_yellow(f'WARNING: the parameter "{cmd}" was not a str, casting and continuing')
            full_command_arr[index] = str(full_command_arr[index])

    if is_windows():
        if full_command_arr[0] in ['ffmpeg', 'ffplay', 'ffprobe']:
            full_command_arr[0] += '.exe'

    full_call = full_command_arr[0] + ' ' + ' '.join(map(lambda x: f'"{x}"', full_command_arr[1:]))
    if debug:
        print(f'going to run "{full_call}"')
    
    # env = os.environ.copy() # env['SSH_AUTH_SOCK'] = os.
    process = subprocess.Popen(full_command_arr, stdout=stdout_pipe, stderr=stderr_pipe, stdin=stdin_pipe, shell=shell)
    stdout, stderr = process.communicate(timeout=timeout)

    if debug:
        print(f'Finished execution, return code was {process.returncode}')

    if stdout is not None:
        # !TODO this might be bad, i read somewhere online that this is more encompassing than utf8 idk tho probably not lol
        stdout = stdout.decode("ISO-8859-1") 
    if stderr is not None:
        stderr = stderr.decode("ISO-8859-1")
        

    if debug:
        if process.returncode:
            print_red(f'FAILURE executing "{full_call}"')
            if stdout:
                print('stdout', stdout)
            if stderr:
                print_red('stderr', stderr)
        else:
            print_green(f'SUCCESS executing "{full_call}"')
            if stdout:
                print('stdout', stdout)
            if stderr:
                print_red('stderr', stderr)
        
    if timing:
        print_blue(f'Took {time.time() - start_time:.2f} seconds, command was "{full_call}"')
    return process.returncode, stdout, stderr


def run_command_async(full_command_arr, debug=False, stdin=None, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=False):
    for index in range(len(full_command_arr)):
        cmd = full_command_arr[index]
        if type(cmd) != str:
            if not isinstance(cmd, pathlib.Path):
                print_yellow(f'WARNING: the parameter "{cmd}" was not a str, casting and continuing')
            full_command_arr[index] = str(full_command_arr[index])

    if is_windows():
        if full_command_arr[0] in ['ffmpeg', 'ffplay']:
            full_command_arr[0] += '.exe'

    full_call = full_command_arr[0] + ' ' + ' '.join(map(lambda x: f'"{x}"', full_command_arr[1:]))
    process = subprocess.Popen(full_command_arr, stdout=stdout, stderr=stderr, stdin=stdin, shell=shell)
    
    if debug:
        print(f'started process with "{full_call}"')
    return process


def executable_running(executable_name):
    import psutil
    for proc in psutil.process_iter(['pid', 'name']):
        if proc.info['name'] == executable_name:
            return True
    return False


def kill_process_by_name(executable_name):
    import psutil
    for proc in psutil.process_iter(['pid', 'name']):
        if proc.info['name'] == executable_name:
            try:
                proc.kill()
            except psutil.AccessDenied:
                print_red(f'Access denied for {executable_name=}')
            except psutil.NoSuchProcess:
                pass


# -ss '120534ms'
def seconds_to_fmmpeg_ms_string(seconds):
    if seconds < 0:
        print_red(f'WARNING: seconds_to_fmmpeg_ms_string was called with {seconds=}, returning 0')
        return '0ms'
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

def hmsm_string_to_seconds(string):
    parts = string.split(":")
    hours = int(parts[0])
    minutes = int(parts[1])
    seconds_parts = parts[2].split(".")
    seconds = int(seconds_parts[0])
    milliseconds = int(seconds_parts[1])
    return hours * 3600 + minutes * 60 + seconds + milliseconds / 100


def play_file_mpv(video_path, volume=None, subtitles=False, run_async=False, quit_on_end=False):
    cmd = [
        'mpv', video_path, 
        '--no-resume-playback',
        '--no-pause',
        '--load-scripts=no',
    ]
    if not subtitles:
        cmd.append('--sid=no')
    if volume is not None:
        cmd.append(f'--volume={volume}')
    if run_async or quit_on_end:
        cmd.append(f'--keep-open=no') # prevents zombie processes if you have that flag up
    if run_async:
        return run_command_async(cmd)
    run_command_blocking(cmd, stdout_pipe=None, stderr_pipe=None)



def kill_self(seconds=0, tries=10):

    import time
    if seconds:
        print(f'Sleeping for {seconds} before killing self')
        time.sleep(seconds)
    import atexit
    atexit._run_exitfuncs()

    if is_windows():
        kill_string = f'taskkill /PID {os.getpid()} /f'
    else:
        kill_string = f'kill -9 {os.getpid()}'
    for i in range(tries):
        print(f'{i}: running to kill: "{kill_string}"')
        os.system(kill_string)
    print_red('THIS SHOULD BE UNREACHABLE')


def path_is_local(path):
    if is_ray():
        if path.resolve().parts[0] in ['C:\\', 'T:\\']:
            return True
    elif is_linux():
        if ''.join(path.resolve().parts[0:2]) != '/mnt':
            return True
    elif is_windows():
        if not is_andrews_laptop():
            print(f'path_is_local called, assuming that only the C:\\ drive is local. update this code if you dont want it to print every time')
        if path.resolve().parts[0] == 'C:\\':
            return True
    return False

# Going over network is quite slow with processing, copy things locally to speed things up on successive runs.
def copy_file_locally(file_path, output_directory=get_temp_dir()):
    import shutil
    if path_is_local(file_path):
        return file_path

    dest_path = output_directory.joinpath(file_path.name)
    if not dest_path.exists() and file_path.exists():
        print_yellow(f'WARNING: Copying {file_path.name} locally. This is for speedup because networking is slow. Copying {file_path} to {dest_path}')
        shutil.copy(file_path, dest_path)
        print_green('Finished copying file')
    return dest_path


def dump_text_to_file(text, output_directory=get_temp_dir()):
    filepath = output_directory.joinpath(random_letters(15) + '.txt')
    filepath.write_text(text)
    return filepath

def bytes_to_human_readable_string(size, decimal_places=2):
    for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
        if size < 1024.0 or unit == 'TB':  # stop at TB for simplicity
            break
        size /= 1024.0
    return f"{size:.{decimal_places}f} {unit}"



def ns_to_friendly(ns):
    converted = time.localtime(ns / 1_000_000_000)
    return time.strftime('%Y-%m-%d %H:%M:%S', converted)


def kill_existing_port_listener(port=5000):
    import platform
    if is_windows():
        try:
            find_pid_command = f'netstat -a -n -o | findstr ":{port}"'
            result = subprocess.run(find_pid_command, capture_output=True, text=True, shell=True)
            
            pids_to_kill = set()
            
            if result.stdout:
                for line in result.stdout.strip().split('\n'):
                    if 'LISTENING' in line:
                        pid = line.strip().split()[-1]
                        if pid.isdigit():
                            pids_to_kill.add(pid)
            
            if not pids_to_kill:
                return

            for pid in pids_to_kill:
                print(f"Process with PID {pid} is listening on port {port}. Terminating it...")
                kill_command = f'taskkill /F /PID {pid}'
                subprocess.run(kill_command, capture_output=True, text=True, shell=True, check=True)
                
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
            print(f"An error occurred on Windows: {e}")

    elif is_linux() or is_macos():
        try:
            find_pid_command = f'lsof -t -i:{port}'
            print(f'Running command to find PIDs listening on port {port}: {find_pid_command}')
            result = subprocess.run(find_pid_command, shell=True, capture_output=True, text=True)
            
            if not result.stdout:
                print(f'No process is listening on port {port}.')
                return

            pids = result.stdout.strip().split('\n')
            for pid in pids:
                if pid.isdigit():
                    print(f"Process with PID {pid} is listening on port {port}. Terminating it...")
                    kill_command = f'kill -9 {pid}'
                    subprocess.run(kill_command, shell=True, check=True)
        
        except (subprocess.CalledProcessError, FileNotFoundError) as e:
             print(f"An error occurred on {platform.system()}: {e}. Please ensure 'lsof' is installed.")

    else:
        raise NotImplementedError(f"Platform not supported: {platform.system()}")


# go up a line: '\033[A'
# up a line and begining: '\033[F'

# disabling std out
# if is_windows():
#     null_std_out = open('nul', 'w')
# elif is_linux() or is_macos():
#     null_std_out = open('/dev/null', 'w')
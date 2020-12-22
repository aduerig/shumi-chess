# TODO change this up when adding build scripts
import sys
sys.path.append('build/lib.linux-x86_64-3.9')

import engine_communicator

print('this is from python')
status = engine_communicator.systemcall('echo "this is from the shell"')
status = engine_communicator.print_from_c()

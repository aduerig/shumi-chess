import sys
import os

# code to just find the non-temp build folder
############################################################
abs_real_filepath = os.path.realpath(__file__)
just_dir, _ = os.path.split(abs_real_filepath)
module_build_dir = os.path.join(just_dir, 'build')

for filepath in os.listdir(module_build_dir):
    first = os.path.join(module_build_dir, filepath)
    if 'temp' not in filepath:
        final = os.path.join(module_build_dir, filepath)
        sys.path.append(final)
        break
############################################################

import engine_communicator

print('this is from python')
status = engine_communicator.systemcall('echo "this is from the shell"')
status = engine_communicator.print_from_c()

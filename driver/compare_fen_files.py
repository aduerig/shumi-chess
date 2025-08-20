import sys
import pathlib


this_file_directory = pathlib.Path(__file__).parent
temp_dir = this_file_directory / 'temp'

test_dir = this_file_directory.parent.joinpath('tests', 'test_data')




test_name = 'rooks_depth_1.dat'
# shumi_name = 'temp_file_1755531121.7167237.txt'
shumi_name = 'temp_file_1755532212.6155126.txt'




test_filepath = test_dir / test_name
shumi_filepath = temp_dir / shumi_name
if not shumi_filepath.exists():
    print(f"Shumi file does not exist: {shumi_filepath}")
    exit()

if not test_filepath.exists():
    print(f"Test file does not exist: {test_filepath}")
    exit()



# go through and skip line 1 of the test file. Read the fens and strip them from each. Print 2 sections, what boards are
# in the tests but NOT in shumi and ones that are in shumi but NOT in the tests

with open(test_filepath, 'r') as f:
    f.readline()  # skip line 1
    test_fens = {line.strip() for line in f}


with open(shumi_filepath, 'r') as f:
    f.readline()  # skip line 1
    shumi_fens = {line.strip() for line in f}


print(f'\nTest FENs not in Shumi:')
for fen in test_fens - shumi_fens:
    print(f' - {fen}')

print(f'\n\nShumi FENs not in Test:')
for fen in shumi_fens - test_fens:
    print(f' - {fen}')
import sys
import os
from graphics import *

# code to just find the non-temp build folder for the C extension
############################################################
abs_real_filepath = os.path.realpath(__file__)
actual_file_dir, _ = os.path.split(abs_real_filepath)
module_build_dir = os.path.join(actual_file_dir, 'build')

for filepath in os.listdir(module_build_dir):
    first = os.path.join(module_build_dir, filepath)
    if 'temp' not in filepath:
        final = os.path.join(module_build_dir, filepath)
        sys.path.append(final)
        break
############################################################


import engine_communicator

# print('this is from python')
# engine_communicator.systemcall('echo "this is from the shell"')
# engine_communicator.print_from_c()


# ! drawing chess board
win = GraphWin(width = 800, height = 800)

# set the coordinates of the window; bottom left is (0, 0) and top right is (1, 1)
win.setCoords(0, 0, 1, 1)
square_size = 1.0 / 8

# draws the other 32 squares of other colors
every_other = 1
for y in range(8):
    for x in range(8):
        location_x = square_size * x
        location_y = square_size * y
        square_to_draw = Rectangle(
            Point(location_x, location_y),
            Point(location_x + square_size, location_y + square_size)
        )
        if every_other:
            square_to_draw.setFill(color_rgb(65, 65, 65))
        else:
            square_to_draw.setFill(color_rgb(125, 125, 125))
        square_to_draw.draw(win)
        every_other = 1 - every_other
    every_other = 1 - every_other

# getting chess_image_filepaths of chess images
chess_image_filepaths = {}
for file in os.listdir(os.path.join(actual_file_dir, 'images')):
    filepath = os.path.join(actual_file_dir, 'images', file)
    if os.path.isfile(filepath):
        just_piece = os.path.splitext(file)[0]
        chess_image_filepaths[just_piece] = filepath
print(chess_image_filepaths)

# precompute chess coordinate notation to x, y values
coord_to_x_y = {
    (str(chr(ord('a') + x)) + str(y+1)): (x, y) for y in range(8) for x in range(8)
}

# rendering pieces on board
positions = engine_communicator.get_piece_positions()
print(positions)
for piece_name, piece_coords in positions.items():
    for coord in piece_coords:
        x, y = coord_to_x_y[coord]
        location_of_image = Point(
            (x / 8) + (square_size / 2), 
            (y / 8) + (square_size / 2)
        )
        image_to_draw = Image(location_of_image, chess_image_filepaths[piece_name])
        image_to_draw.draw(win)


# ! chess logic
# positions = engine_communicator.get_piece_positions()
# print(positions)
# legal_moves = engine_communicator.get_legal_moves()

# print(legal_moves)

is_focused = False
while True:
    win.getMouse()

    # square_clicked_on
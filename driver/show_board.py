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
square_size = 1.0 / 10
potential_move_circle_radius = square_size / 6

# draws the background color
background = Rectangle(
    Point(0, 0),
    Point(1, 1)
)
background.setFill(color_rgb(50, 50, 50))
background.draw(win)

# draws the pop button
pop_button = Rectangle(
    Point(square_size * 8, square_size * 8),
    Point(1, square_size * 7)
)
pop_button.setFill(color_rgb(59, 48, 32))
pop_button.draw(win)

pop_text = Text(
    Point(square_size * 9, square_size * 7.5), 
    "pop!"
)
pop_text.setFill(color_rgb(200, 200, 200))
pop_text.draw(win)

# draws the squares of colors
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

# precompute chess coordinate notation to x, y values
coord_to_x_y = {
    (str(chr(ord('a') + x)) + str(y+1)): (x, y) for y in range(8) for x in range(8)
}

x_y_to_coord = {
    (x, y): (str(chr(ord('a') + x)) + str(y+1)) for y in range(8) for x in range(8)
}

# pythons representation of the board
board = {
    (str(chr(ord('a') + x)) + str(y+1)):None for y in range(8) for x in range(8)
}

# rendering pieces on boards

def undraw_pieces():
    for graphic in board.values():
        if graphic:
            graphic.undraw()
    for i in range(len(board)):
        board[i] = None

def render_pieces():
    positions = engine_communicator.get_piece_positions()
    for piece_name, piece_coords in positions.items():
        for coord in piece_coords:
            x, y = coord_to_x_y[coord]
            location_of_image = Point(
                (x / 10) + (square_size / 2), 
                (y / 10) + (square_size / 2)
            )
            image_to_draw = Image(location_of_image, chess_image_filepaths[piece_name])
            image_to_draw.draw(win)
            board[coord] = image_to_draw
render_pieces()

# ! playing logic
coord_focused = None
drawn_potential = []
avail_moves = []
while True:
    # get raw positions
    raw_position = win.getMouse()
    raw_x, raw_y = raw_position.x, raw_position.y

    # undraw potentials no matter what
    for i in drawn_potential:
        i.undraw()
    drawn_potential = []

    # get square_clicked_on
    x, y = int(raw_x * 10), int(raw_y * 10)
    if (x, y) not in x_y_to_coord:
        coord_focused = None

        # if click the pop button
        if raw_x > square_size * 8 and raw_x < 1 and raw_y < square_size * 8 and raw_y > square_size * 7:
            engine_communicator.pop()
            undraw_pieces()
            render_pieces()
            coord_focused = None
            avail_moves = []
        continue
    coord_clicked = x_y_to_coord[(x, y)]
    legal_moves = engine_communicator.get_legal_moves()
    
    # diferent actions
    if coord_clicked in avail_moves: # if user inputs a valid move
        engine_communicator.make_move(coord_focused, coord_clicked)
        undraw_pieces()
        render_pieces()
        coord_focused = None
        avail_moves = []
        continue
    elif not board[coord_clicked]:
        coord_focused = None
    elif coord_clicked == coord_focused:
        coord_focused = None
    else:
        coord_focused = coord_clicked
    
    avail_moves = []
    for move in legal_moves:
        from_square, to_square = move[:2], move[2:]
        if coord_focused == from_square:
            focused_x, focused_y = coord_to_x_y[to_square]
            avail_moves.append(to_square)
            render_x = (focused_x * square_size) + square_size / 2
            render_y = (focused_y * square_size) + square_size / 2
            potential_move = Circle(
                Point(render_x, render_y),
                potential_move_circle_radius
            )
            potential_move.setFill(color_rgb(170, 170, 170))
            potential_move.draw(win)
            drawn_potential.append(potential_move)
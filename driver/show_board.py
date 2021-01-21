import sys
import os
from graphics import *
import threading
import random

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


# ! button stuff
class Button:
    rectangle_graphics_object = None
    text_graphics_object = None

    def __init__(self, function_to_call, function_to_get_text, button_color, text_color):
        self.function_to_call = function_to_call
        self.function_to_get_text = function_to_get_text
        self.button_color = button_color
        self.text_color = text_color

    def get_text(self):
        return self.function_to_get_text()

    def clicked(self):
        return self.function_to_call(self)

    def update_text(self):
        self.text_graphics_object.setText(self.get_text())

def clicked_pop_button(button_obj):
    engine_communicator.pop()
    undraw_pieces()
    render_all_pieces_and_assign(board)
    coord_focused = None
    avail_moves = []

def clicked_white_button(button_obj):
    if type(both_players[0]) == Human:
        both_players[0] = Random()
    elif type(both_players[0]) == Random:
        both_players[0] = Human()
    button_obj.update_text()

def clicked_black_button(button_obj):
    if type(both_players[1]) == Human:
        both_players[1] = Random()
    elif type(both_players[1]) == Random:
        both_players[1] = Human()
    button_obj.update_text()

def reset_board():
    engine_communicator.reset_engine();
    undraw_pieces()
    render_all_pieces_and_assign(board)

def clicked_reset_button(button_obj):
    reset_board()

global autoreset_toggle; autoreset_toggle = False
def clicked_autoreset(button_obj):
    global autoreset_toggle
    if autoreset_toggle == False:
        autoreset_toggle = True
    else:
        autoreset_toggle = False
    button_obj.update_text()


def gui_click_choices():
    curr_y_cell = 8
    for button in button_holder:
        if raw_x > square_size * 8 and raw_x < 1 and raw_y < square_size * curr_y_cell and raw_y > square_size * (curr_y_cell - 1):
            button.clicked()
        curr_y_cell -= 1


# ! some object types
class PlayerType:
    name = ""
    
    def __init__(self):
        pass

    def get_name(self):
        return self.__class__.__name__


class Human(PlayerType):
    def get_move(self):
        raise Exception('not handled here, move code to here')


class AI(PlayerType):
    def get_move(self):
        raise Exception('cant call AI class directly')


class Random(AI):
    def get_move(self):
        moves = engine_communicator.get_legal_moves()
        # ! if this line errors it is because random.choice(moves) returns 0, which shouldn't really be possible in a completed engine
        choice = random.choice(moves)
        return choice[0:2], choice[2:4]

# variables for holding types
both_players = [Human(), Random()]

# ! drawing GUI elements
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


# argument list: function to run on click, text, button_color, text color
button_holder = [
    Button(clicked_pop_button, lambda: "pop!", color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(clicked_white_button, lambda: "Black\n{}".format(both_players[0].get_name()), color_rgb(100, 100, 100), color_rgb(200, 200, 200)),
    Button(clicked_black_button, lambda: "White\n{}".format(both_players[1].get_name()), color_rgb(20, 20, 20), color_rgb(200, 200, 200)),
    Button(clicked_reset_button, lambda: "Reset", color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(clicked_autoreset, lambda: "autoreset: {}".format(autoreset_toggle), color_rgb(59, 48, 32), color_rgb(200, 200, 200))
]

curr_y_cell = 8
for button_obj in button_holder:
    new_button = Rectangle(
        Point(square_size * 8, square_size * curr_y_cell),
        Point(1, square_size * (curr_y_cell - 1))
    )
    new_button.setFill(button_obj.button_color)
    new_button.draw(win)

    new_button_text = Text(
        Point(square_size * 9, square_size * (curr_y_cell - .5)), 
        button_obj.get_text()
    )
    new_button_text.setFill(button_obj.text_color)
    new_button_text.draw(win)

    button_obj.rectangle_graphics_object = new_button
    button_obj.text_graphics_object = new_button_text
    curr_y_cell -= 1

# current turn text
turn_text_values = {0: "White's turn", 1: "Black's turn"}
current_turn_text = Text(
    Point(square_size * .5, square_size * 9), 
    turn_text_values[0]
)
current_turn_text.setFill(color_rgb(200, 200, 200))
current_turn_text.draw(win)

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
acn_to_x_y = {
    (str(chr(ord('a') + x)) + str(y+1)): (x, y) for y in range(8) for x in range(8)
}


x_y_to_acn = {}
# reversing dict
for acn, coord in acn_to_x_y.items():
    x_y_to_acn[coord] = acn


# pythons representation of the board
board = {
    (str(chr(ord('a') + x)) + str(y+1)):None for y in range(8) for x in range(8)
}


# rendering pieces on boards
def undraw_pieces():
    for value in board.values():
        if value:
            value[2].undraw()
    for i in range(len(board)):
        board[i] = None


def render_all_pieces_and_assign(board):
    positions = engine_communicator.get_piece_positions()
    for piece_name, piece_acn in positions.items():
        for acn in piece_acn:
            x, y = acn_to_x_y[acn]
            location_of_image = Point(
                (x / 10) + (square_size / 2), 
                (y / 10) + (square_size / 2)
            )
            image_to_draw = Image(location_of_image, chess_image_filepaths[piece_name])
            image_to_draw.draw(win)
            board[acn] = (piece_name, acn, image_to_draw)


def graphics_update_only_moved_pieces():
    positions = engine_communicator.get_piece_positions()
    still_taken_acns = set()
    for piece_name, piece_acn in positions.items():
        for acn in piece_acn:
            if board[acn] == None:
                x, y = acn_to_x_y[acn]
                location_of_image = Point(
                    (x / 10) + (square_size / 2), 
                    (y / 10) + (square_size / 2)
                )
                image_to_draw = Image(location_of_image, chess_image_filepaths[piece_name])
                image_to_draw.draw(win)
                board[acn] = (piece_name, acn, image_to_draw)
            elif board[acn][0] != piece_name:
                board[acn][2].undraw()
                x, y = acn_to_x_y[acn]
                location_of_image = Point(
                    (x / 10) + (square_size / 2), 
                    (y / 10) + (square_size / 2)
                )
                image_to_draw = Image(location_of_image, chess_image_filepaths[piece_name])
                image_to_draw.draw(win)
                board[acn] = (piece_name, acn, image_to_draw)
            still_taken_acns.add(acn)
            

    for value in board.values():
        if value:
            piece_name, acn, graphics_object = value
            if acn not in still_taken_acns:
                graphics_object.undraw()
                board[acn] = None


render_all_pieces_and_assign(board)


# ! playing loop for players
player_index = 0
coord_focused = None
drawn_potential = []
avail_moves = []
while True:
    # 1 game iteration
    while engine_communicator.game_over() == -1:
        # sets turn text
        current_turn_text.setText(turn_text_values[player_index])


        # diferent chess board actions if players turn
        curr_player = both_players[player_index]
        curr_player_type = type(curr_player)
        if isinstance(curr_player, AI):
            from_acn, to_acn = curr_player.get_move()
            engine_communicator.make_move_two_acn(from_acn, to_acn)
            graphics_update_only_moved_pieces()
            player_index = 1 - player_index
            continue

        # get raw positions
        raw_position = win.getMouse()
        raw_x, raw_y = raw_position.x, raw_position.y
        x, y = int(raw_x * 10), int(raw_y * 10)

        # undraw potentials no matter what
        for i in drawn_potential:
            i.undraw()
        drawn_potential = []

        # get square_clicked_on in the gui
        if (x, y) not in x_y_to_acn:
            gui_click_choices()
            continue
        coord_clicked = x_y_to_acn[(x, y)]

        if curr_player_type == Human:
            if coord_clicked in avail_moves: # if user inputs a valid move
                engine_communicator.make_move_two_acn(coord_focused, coord_clicked)
                graphics_update_only_moved_pieces()
                coord_focused = None
                avail_moves = []
                player_index = 1 - player_index
                continue
            # if clicking on a piece that cannot be moved to
            elif not board[coord_clicked]:
                coord_focused = None
            # if clicking on the same square, defocus
            elif coord_clicked == coord_focused:
                coord_focused = None
            # else, it focuses in on the square clicked
            else:
                coord_focused = coord_clicked
            
            # refresh and draw draw potential moves
            legal_moves = engine_communicator.get_legal_moves()
            avail_moves = []
            for move in legal_moves:
                from_square, to_square = move[:2], move[2:]
                if coord_focused == from_square:
                    focused_x, focused_y = acn_to_x_y[to_square]
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
    
    if autoreset_toggle:
        print("automatically resetting board")
        reset_board()
        continue

    # get raw positions
    raw_position = win.getMouse()
    raw_x, raw_y = raw_position.x, raw_position.y
    x, y = int(raw_x * 10), int(raw_y * 10)

    gui_click_choices()
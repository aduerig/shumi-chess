import importlib
import os
import random
from tkinter import filedialog
import time

# same directory
from modified_graphics import *
import engine_communicator

script_file_dir, _ = os.path.split(os.path.realpath(__file__))


# trying to import any avaliable AIs
imported_ais = {}
for filepath in os.listdir(script_file_dir):
    if filepath.endswith('.so') and 'ai' in filepath.lower():
        ai_friendly_module_name = filepath.split('.')[0]
        print('trying to import', ai_friendly_module_name)
        i = importlib.import_module(ai_friendly_module_name)
        imported_ais[ai_friendly_module_name] = i
for key, val in imported_ais.items():
    print('AI: {} imported, module is: {}'.format(key, val))


def reset_board():
    global curr_game, legal_moves
    engine_communicator.reset_engine();
    undraw_pieces()
    render_all_pieces_and_assign(board)
    curr_game += 1
    legal_moves = engine_communicator.get_legal_moves()


def get_random_move(legal_moves):
    # ! if this line errors it is because random.choice(moves) returns 0, which shouldn't really be possible in a completed engine
    choice = random.choice(legal_moves)
    return choice[0:2], choice[2:4]


def get_ai_move(legal_moves: list[str], name_of_ai: str) -> None:
    if name_of_ai.lower() == 'random_ai':
        return get_random_move(legal_moves)
    # move = engine_communicator.minimax_ai_get_move()
    seconds = 1.5
    move = engine_communicator.minimax_ai_get_move_iterative_deepening(seconds)
    return move[0:2], move[2:4]


ai_default = 'minimax_ai'
both_players = ['human', ai_default]

# ! drawing GUI elements
screen_width = 800
screen_height = 800
win = GraphWin(width = screen_width, height = screen_height)

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
    acn_focused = None
    avail_moves = []


def get_next_player(player_name: str) -> str:
    if player_name == 'human':
        return 'random_ai'
    elif player_name == 'random_ai':
        return 'minimax_ai'
    elif player_name == 'minimax_ai':
        return 'human'

def clicked_white_button(button_obj):
    both_players[0] = get_next_player(both_players[0])
    button_obj.update_text()

def clicked_black_button(button_obj):
    both_players[1] = get_next_player(both_players[1])
    button_obj.update_text()

global autoreset_toggle; autoreset_toggle = False
def clicked_autoreset(button_obj):
    global autoreset_toggle
    if autoreset_toggle == False:
        autoreset_toggle = True
    else:
        autoreset_toggle = False
    button_obj.update_text()


def clicked_reset_button(button_obj):
    reset_board()


def load_game(button_obj):
    file_path = filedialog.askopenfilename()
    print('you selected this path:', file_path)


# argument list: function to run on click, text, button_color, text color
button_holder = [
    Button(clicked_pop_button, lambda: "pop!", color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(clicked_white_button, lambda: "White\n{}".format(both_players[0]), color_rgb(100, 100, 100), color_rgb(200, 200, 200)),
    Button(clicked_black_button, lambda: "Black\n{}".format(both_players[1]), color_rgb(20, 20, 20), color_rgb(200, 200, 200)),
    Button(clicked_reset_button, lambda: "Reset", color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(clicked_autoreset, lambda: "Autoreset board on\ndraw: {}".format(autoreset_toggle), color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(load_game, lambda: "Load game\n(doesn't work)", color_rgb(59, 48, 32), color_rgb(200, 200, 200))
]

def gui_click_choices():
    curr_y_cell = 8
    for button in button_holder:
        if raw_left_clicked_x > square_size * 8 and raw_left_clicked_x < 1 and raw_left_clicked_y < square_size * curr_y_cell and raw_left_clicked_y > square_size * (curr_y_cell - 1):
            button.clicked()
        curr_y_cell -= 1

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

# curr game text
global curr_game; curr_game = 1
curr_game_text = Text(
    Point(square_size * 2.5, square_size * 9), 
    'Game {}'.format(curr_game)
)
curr_game_text.setFill(color_rgb(200, 200, 200))
curr_game_text.draw(win)

curr_move_text = Text(
    Point(square_size * 4.5, square_size * 9), 
    'Move {}'.format(engine_communicator.get_move_number())
)
curr_move_text.setFill(color_rgb(200, 200, 200))
curr_move_text.draw(win)


winner_text = 'GAME OVER: {} wins'
game_over_text = Text(
    Point(square_size * 4.5, square_size * 9.5), 
    winner_text
)
game_over_text.setFill(color_rgb(255, 160, 122))

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
for file in os.listdir(os.path.join(script_file_dir, 'images')):
    filepath = os.path.join(script_file_dir, 'images', file)
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


def unfocus_and_stop_dragging():
    global acn_focused, is_dragging, drawn_potential, avail_moves
    if not acn_focused and not is_dragging:
        return
    
    # if we didn't just click and release on the same square
    for i in drawn_potential:
        i.undraw()
    drawn_potential = []
    avail_moves = []

    # move dragged piece back to starting square
    dragged_piece = board[acn_focused][2]
    dragged_piece.anchor.x
    dragged_piece.anchor.y

    x_coord_to_move_to, y_coord_to_move_to = acn_to_x_y[acn_focused]

    x_pos_to_move_to = (x_coord_to_move_to / 10) + (square_size / 2)
    y_pos_to_move_to = (y_coord_to_move_to / 10) + (square_size / 2)

    dragged_piece.move(x_pos_to_move_to - dragged_piece.anchor.x, y_pos_to_move_to - dragged_piece.anchor.y)
    acn_focused = None
    is_dragging = False


def make_move(from_acn, to_acn):
    global legal_moves, player_index
    engine_communicator.make_move_two_acn(from_acn, to_acn)
    legal_moves = engine_communicator.get_legal_moves()
    graphics_update_only_moved_pieces()
    player_index = 1 - player_index


# ! playing loop for players
global legal_moves, player_index, acn_focused, is_dragging, drawn_potential
player_index = 0
acn_focused = None
drawn_potential = []
avail_moves = []
fps = 60.0
last_frame = 0
legal_moves = engine_communicator.get_legal_moves()
is_dragging = False
while True:
    game_over_text.undraw()
    # 1 game iteration
    while engine_communicator.game_over() == -1:
        # stuff to do every frame no matter what
        # sets text
        current_turn_text.setText(turn_text_values[player_index])
        curr_game_text.setText('Game {}'.format(curr_game))
        curr_move_text.setText('Move {}'.format(engine_communicator.get_move_number()))

        raw_position_left_click = win.checkMouse()
        user_left_clicked = False
        if raw_position_left_click:
            user_left_clicked = True
            raw_left_clicked_x, raw_left_clicked_y = raw_position_left_click.x, raw_position_left_click.y
            left_clicked_x, left_clicked_y = int(raw_left_clicked_x * 10), int(raw_left_clicked_y * 10)

        # if is AIs turn and the user hasn't clicked
        if not user_left_clicked:
            curr_player = both_players[player_index]
            if 'ai' in curr_player:
                from_acn, to_acn = get_ai_move(legal_moves, curr_player)
                # print('AI has come up with move: {} to {}'.format(from_acn, to_acn))
                make_move(from_acn, to_acn)
                continue
        
        # if human
        if curr_player == 'human':
            if time.time() < (last_frame + 1/fps):
                time.sleep(time.time() - (last_frame + 1/fps))
                last_frame = time.time()

            # code for releasing dragging if right click
            raw_position_right_click = win.checkRightClick()
            if acn_focused and raw_position_right_click:
                unfocus_and_stop_dragging()

            # code for releasing dragging if left click release
            raw_position_left_release = win.checkLeftRelease() 
            if acn_focused and raw_position_left_release:
                raw_left_released_x, raw_left_released_y = raw_position_left_release.x, raw_position_left_release.y
                left_released_x, left_released_y = int(raw_left_released_x * 10), int(raw_left_released_y * 10)
                # mouse was released on an avaliable square, make move there
                if (left_released_x, left_released_y) in list(map(lambda x: acn_to_x_y[x], avail_moves)):
                    temp = acn_focused
                    unfocus_and_stop_dragging()
                    make_move(temp, x_y_to_acn[(left_released_x, left_released_y)])
                    avail_moves = []
                # mouse was released on the same square as it was clicked on
                elif (left_released_x, left_released_y) != acn_to_x_y[acn_focused]:
                    unfocus_and_stop_dragging()

            # code for dragging
            elif is_dragging or acn_focused:
                dragged_piece = board[acn_focused][2]
                scaledMouseX, scaledMouseY = win.toWorld(win.newmouseX, win.newmouseY)

                deltX = scaledMouseX - dragged_piece.anchor.x
                deltY = scaledMouseY - dragged_piece.anchor.y
                dragged_piece.move(deltX, deltY)


        if user_left_clicked:
            # undraw potentials no matter what
            for i in drawn_potential:
                i.undraw()
            drawn_potential = []

            # get square_clicked_on in the gui
            if (left_clicked_x, left_clicked_y) not in x_y_to_acn:
                gui_click_choices()
                continue
            acn_clicked = x_y_to_acn[(left_clicked_x, left_clicked_y)]
            
            if curr_player == 'human':
                if acn_clicked in avail_moves: # if user inputs a valid move
                    temp = acn_focused
                    unfocus_and_stop_dragging()
                    make_move(temp, acn_clicked)
                    avail_moves = []
                    continue
                # if clicking on a piece that cannot be moved to
                elif not board[acn_clicked]:
                    unfocus_and_stop_dragging()
                # if clicking on the same square, defocus
                elif acn_clicked == acn_focused:
                    unfocus_and_stop_dragging()
                # else, it focuses in on the square clicked
                else:
                    unfocus_and_stop_dragging()
                    is_dragging = True
                    acn_focused = acn_clicked
                
                # draw draw potential moves
                if acn_focused:
                    avail_moves = []
                    for move in legal_moves:
                        from_square, to_square = move[:2], move[2:]
                        if acn_focused == from_square:
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
    
    # winner determinations
    winner = 'draw'
    if engine_communicator.game_over() == 0:
        winner = 'white'
    elif engine_communicator.game_over() == 2:
        winner = 'black'
    game_over_text.setText(winner_text.format(winner))
    game_over_text.draw(win)
    
    if winner == 'draw' and autoreset_toggle:
        reset_board()
        continue

    # get raw positions
    raw_position_left_click = win.getMouse()
    raw_left_clicked_x, raw_left_clicked_y = raw_position_left_click.x, raw_position_left_click.y
    left_clicked_x, left_clicked_y = int(raw_left_clicked_x * 10), int(raw_left_clicked_y * 10)

    gui_click_choices()
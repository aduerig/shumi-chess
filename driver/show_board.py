import importlib
import os
import random
from tkinter import filedialog
import time
import pathlib
import argparse
import threading
import queue

# same directory
from modified_graphics import *
import engine_communicator



# --- Game counters (global state) ---
curr_game = 1
curr_game_white = 0
curr_game_black = 0
curr_game_draw = 0
# ------------------------------------



# Python arguments
parser = argparse.ArgumentParser()
parser.add_argument('--fen', default=None)
parser.add_argument('--human', default=False, action='store_true')
parser.add_argument('-d', '--depth', type=int, default=7, help='Maximum deepening')
parser.add_argument('-t', '--time', type=int, default=2000, help='Time per move in ms')


args = parser.parse_args()

script_file_dir = pathlib.Path(os.path.split(os.path.realpath(__file__))[0])

temp_folder = script_file_dir.joinpath('temp')
temp_folder.mkdir(exist_ok=True)

# Globals for multithreaded AI
ai_move_queue = queue.Queue()
ai_is_thinking = False

def get_temp_file(suffix=''):
    return temp_folder.joinpath(f'temp_file_{time.time()}{suffix}')


# trying to import any available AIs
imported_ais = {}
for filepath in os.listdir(script_file_dir):
    if filepath.endswith('.so') and 'ai' in filepath.lower():
        ai_friendly_module_name = filepath.split('.')[0]
        print('trying to import', ai_friendly_module_name)
        i = importlib.import_module(ai_friendly_module_name)
        imported_ais[ai_friendly_module_name] = i
for key, val in imported_ais.items():
    print('AI: {} imported, module is: {}'.format(key, val))


##def reset_board(fen=""):
def reset_board(fen="", winner="????"):

    global curr_game_white, curr_game_black, curr_game_draw, curr_game
    global legal_moves, game_state_might_change, last_move_indicator, ai_is_thinking, player_index

    #print('reset_board called from python')
    
    # If AI is thinking, cancel it
    if ai_is_thinking:
        print("Board reset during AI turn, cancelling computation.")
        ai_is_thinking = False
        # Clear the queue of any potential stale moves
        while not ai_move_queue.empty():
            try:
                ai_move_queue.get_nowait()
            except queue.Empty:
                break

    # Remove the last move indicator on board reset
    if last_move_indicator:
        last_move_indicator.undraw()
        last_move_indicator = None

    if fen:
        engine_communicator.reset_engine(fen)
    else:
        print('Resetting to basic position cause FEN string is empty')
        engine_communicator.reset_engine()
    
    # Resetting the board always sets the turn to white
    player_index = 0
    undraw_pieces()
    render_all_pieces_and_assign(board)


    # Update the match counters
    curr_game += 1
    if winner == 'white':
        curr_game_white += 1
    elif winner == 'black':
        curr_game_black += 1
    elif winner == 'draw':
        curr_game_draw += 1

    curr_game_text.setText(f'Game {curr_game}')
    white_wins_text.setText(f'W {curr_game_white}')
    black_wins_text.setText(f'B {curr_game_black}')
    draw_wins_text.setText(f'D {curr_game_draw}')



    legal_moves = engine_communicator.get_legal_moves()
    game_state_might_change = True


def get_random_move(legal_moves):
    # ! if this line errors it is cause random.choice(moves) returns 0, which shouldn't really be possible in a completed engine
    choice = random.choice(legal_moves)
    return choice[0:2], choice[2:4]

def get_ai_move_threaded(legal_moves: list[str], name_of_ai: str):
    try:
        if name_of_ai.lower() == 'random_ai':
            from_acn, to_acn = get_random_move(legal_moves)
        else:
            # Run engine with CLI knobs (time & max depth)
            milliseconds   = args.time  if args.time  is not None else 2000
            max_deepening  = args.depth if args.depth is not None else 7
            move = engine_communicator.minimax_ai_get_move_iterative_deepening(milliseconds, max_deepening)
            from_acn, to_acn = move[0:2], move[2:4]

        # Only post if the result is still relevant
        if ai_is_thinking:
            ai_move_queue.put((from_acn, to_acn))
    except Exception:
        # Keep the main loop resilient if the worker hits an error
        if ai_is_thinking:
            ai_move_queue.put(None)


ai_default = 'minimax_ai'
both_players = ['human', ai_default]
if args.human:
    both_players = ['human', 'human']

# ! drawing GUI elements
screen_width = 700
screen_height = 800
win = GraphWin(width = screen_width, height = screen_height)

# 2. Start the GUI in the middle of the screen
# win.master.withdraw()
# win.master.update_idletasks()  # Update "requested size"
# x = (win.master.winfo_screenwidth() - win.master.winfo_reqwidth()) / 2
# y = (win.master.winfo_screenheight() - win.master.winfo_reqheight()) / 2
# win.master.geometry(f"+{int(x)}+{int(y)}")
# win.master.deiconify()


# 2. Start the GUI in the upper right corner of the screen
win.master.withdraw()
win.master.update_idletasks()  # ensure reqwidth/reqheight are set
margin = 20  # pixels from screen edges
x = win.master.winfo_screenwidth() - win.master.winfo_reqwidth() - margin
y = margin
if x < 0: x = 0
if y < 0: y = 0
win.master.geometry(f"+{int(x)}+{int(y)}")
win.master.deiconify()



def on_esc_key_hit(event):
    ##print('Trying to escape program!')
    win.close()
    os._exit(0)

# 1. Pressing escape closes out of the program
win.master.bind("<Escape>", on_esc_key_hit)


def on_one_key_hit(event):
    #print("hello world from 1 key")       # pop!
    engine_communicator.pop()

win.master.bind("1", on_one_key_hit)



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


def clicked_flip_button(button_obj):
    global game_state_might_change, last_move_indicator, ai_is_thinking, player_index, acn_focused, avail_moves
    
    # If AI is thinking, cancel it.
    if ai_is_thinking:
       print("Flip Board called during AI turn, cancelling computation.")
       ai_is_thinking = False
       # Clear the queue
       while not ai_move_queue.empty():
           try:
               ai_move_queue.get_nowait()
           except queue.Empty:
               break
    
    global whiteOnBottom, acn_to_x_y, x_y_to_acn
    # flip the flag
    whiteOnBottom = not whiteOnBottom
    # rebuild the two dicts based on new orientation
    acn_to_x_y, x_y_to_acn = build_maps(whiteOnBottom)

    # now force redraw
    undraw_pieces()
    render_all_pieces_and_assign(board)

    #engine_communicator.pop()
    # A pop reverts the turn, so we must revert our player index tracker
    # player_index = 1 - player_index
    
    # game_state_might_change = True
    # undraw_pieces()
    # render_all_pieces_and_assign(board)
    # acn_focused = None
    # avail_moves = []
    # # Remove the last move indicator since the move was undone
    # if last_move_indicator:
    #     last_move_indicator.undraw()
    #     last_move_indicator = None


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
    # get stripped entry text
    fen_string = set_fen_text.getText().strip()
    #print(f'Got FEN string: {fen_string}')
    reset_board(fen_string)


def wake_up(button_obj):
    engine_communicator.wakeup()
    


def get_fen(button_obj):
    fen = engine_communicator.get_fen()
    print(f'Current FEN: {fen}')
    set_fen_text.setText(fen)      # <-- put FEN into the entry box


# engine_communicator.make_move_two_acn(from_acn, to_acn)
# legal_moves = engine_communicator.get_legal_moves()

def output_fens_depth_1(button_obj):
    global legal_moves
    import chess

    # print(f'Output_fens_depth_1 called')

    legal_moves = engine_communicator.get_legal_moves()
    before_fen = engine_communicator.get_fen()
    python_chess_engine_board = chess.Board(before_fen)

    shumi_fens = []
    python_fens = []

    for choice in reversed(legal_moves):
        from_acn, to_acn = choice[0:2], choice[2:4]
        engine_communicator.make_move_two_acn(from_acn, to_acn)
        fen = engine_communicator.get_fen()
        shumi_fens.append(fen)
        engine_communicator.pop()
        after_fen = engine_communicator.get_fen()

        if before_fen != after_fen:
            print(f'FENS DID NOT MATCH AFTER POP, INTERRUPTING PRINT OUT')
            print(f'Before: {before_fen}, After: {after_fen}')
            print(f'Tried to make move: {from_acn} to {to_acn}')
            return
        python_chess_engine_board.push_san(from_acn + to_acn)
        python_fens.append(python_chess_engine_board.fen(en_passant_rights="fen"))
        python_chess_engine_board.pop()

    legal_moves = engine_communicator.get_legal_moves()


    python_fens_set = set(python_fens)
    shumi_fens_set = set(shumi_fens)

    temp = shumi_fens_set - python_fens_set
    if temp:
        print(f'Fens in SHUMI that are NOT in other engines:')
        for i in temp:
            print(f' - {i}')

    temp = python_fens_set - shumi_fens_set
    if temp:
        print(f'Fens in other engines that are NOT in SHUMI:')
        for i in temp:
            print(f' - {i}')

    filepath = "shumi_fens.txt"   # always the same file in the current working dir
    # filepath = get_temp_file(suffix='.txt')

    with open(filepath, 'w') as f:
        for fen in shumi_fens:
            f.write(fen + '\n')
    print(f'Output {len(shumi_fens)} FENs to {filepath}')


# argument list: function to run on click, text, button_color, text color
button_holder = [
    Button(clicked_flip_button, lambda: "Flip Board", color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(clicked_white_button, lambda: "White\n{}".format(both_players[0]), color_rgb(100, 100, 100), color_rgb(200, 200, 200)),
    Button(clicked_black_button, lambda: "Black\n{}".format(both_players[1]), color_rgb(20, 20, 20), color_rgb(200, 200, 200)),
    Button(clicked_reset_button, lambda: "Reset\n(to FEN above)", color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(clicked_autoreset, lambda: "Autoreset board\n{}".format(autoreset_toggle), color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(wake_up, lambda: "Wake up", color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(get_fen, lambda: "Get FEN", color_rgb(59, 48, 32), color_rgb(200, 200, 200)),
    Button(output_fens_depth_1, lambda: "Depth 1 FENs + test", color_rgb(59, 48, 32), color_rgb(200, 200, 200))
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

# small field left of the turn label
material_text = Text(Point(square_size * 0.30, square_size * 9), '1234')
material_text.setFill(color_rgb(200, 200, 200))
material_text.setSize(8)
material_text.draw(win)

score = 4321 
material_text.setText(str(score))


# current turn text
turn_text_values = {0: "White's turn", 1: "Black's turn"}
current_turn_text = Text(
    Point(square_size * 1.25, square_size * 9),
    turn_text_values[0]
)
current_turn_text.setFill(color_rgb(200, 200, 200))
current_turn_text.draw(win)


# current game text (plus small W/B/D counters to the left)
# white_wins_text = Text(Point(square_size * 5.9, square_size * 9), f'W {curr_game_white}')
# black_wins_text = Text(Point(square_size * 6.5, square_size * 9), f'B {curr_game_black}')
# draw_wins_text  = Text(Point(square_size * 7.1, square_size * 9), f'D {curr_game_draw}')
white_wins_text = Text(Point(square_size * 7.0, square_size * 9), f'W {curr_game_white}')
black_wins_text = Text(Point(square_size * 7.5, square_size * 9), f'B {curr_game_black}')
draw_wins_text  = Text(Point(square_size * 8.1, square_size * 9), f'D {curr_game_draw}')




for t in (white_wins_text, black_wins_text, draw_wins_text):
    t.setFill(color_rgb(200, 200, 200))
    t.setSize(8)
    t.draw(win)

curr_game_text = Text(
    Point(square_size * 2.5, square_size * 9),
    f'Game {curr_game}'
)
curr_game_text.setFill(color_rgb(200, 200, 200))
curr_game_text.draw(win)


# set current move text
curr_move_text = Text(
    Point(square_size * 4.5, square_size * 9),
    'Move {}'.format(engine_communicator.get_move_number())
)
curr_move_text.setFill(color_rgb(200, 200, 200))
curr_move_text.draw(win)


# small static label "FEN:" to the left of the FEN box
fen_label = Text(Point(square_size * 0.25, square_size * 8.5), "FEN:")
fen_label.setFill(color_rgb(200, 200, 200))
fen_label.setSize(8)   # small
fen_label.draw(win)

# set fen entry text box 
set_fen_text = Entry(
    Point(square_size * 5.0, square_size * 8.5),
    70
)
set_fen_text.setFill(color_rgb(200, 200, 200))
set_fen_text.draw(win)


# set the winner text
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
            square_to_draw.setFill(color_rgb(60, 80, 60))      # black squares
        else:
            square_to_draw.setFill(color_rgb(145, 145, 145))   # white squares
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
whiteOnBottom = True

def build_maps(whiteOnBottom: bool):
    if whiteOnBottom:
        acn_to_x_y = {
            (chr(ord('a') + x) + str(y+1)): (x, y)
            for y in range(8) for x in range(8)
        }
    else:
        acn_to_x_y = {
            (chr(ord('a') + x) + str(y+1)): (7 - x, 7 - y)
            for y in range(8) for x in range(8)
        }
    x_y_to_acn = {v: k for k, v in acn_to_x_y.items()}
    return acn_to_x_y, x_y_to_acn

# initial build
acn_to_x_y, x_y_to_acn = build_maps(whiteOnBottom)



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
    ##print('Unfocusing from', acn_focused)
    acn_focused = None
    is_dragging = False






def make_move(from_acn, to_acn):
    global legal_moves, player_index, last_move_indicator

    # Remove the previous move's indicator
    if last_move_indicator:
        last_move_indicator.undraw()

    engine_communicator.make_move_two_acn(from_acn, to_acn)
    legal_moves = engine_communicator.get_legal_moves()
    fen = engine_communicator.get_fen()
    print(f'Fen is now {fen}')
    graphics_update_only_moved_pieces()
    player_index = 1 - player_index

    # Draw a new arrow for the current move
    from_x, from_y = acn_to_x_y[from_acn]
    to_x, to_y = acn_to_x_y[to_acn]
    p1 = Point((from_x + 0.5) * square_size, (from_y + 0.5) * square_size)
    p2 = Point((to_x + 0.5) * square_size, (to_y + 0.5) * square_size)
    arrow = Line(p1, p2)
    arrow.setArrow("last")
    arrow.setWidth(4)
    arrow.setFill(color_rgb(200, 200, 50))
    arrow.draw(win)
    last_move_indicator = arrow



game_state_might_change = True
game_state_result = -1
def game_over_cache():
    global game_state_might_change, game_state_result
    if game_state_might_change:
        game_state_result = engine_communicator.game_over()
        game_state_might_change = False
    return game_state_result


# we need to do this for some strange reason, I think cause there's been a click since the last run of the program?
win.checkLeftRelease()
win.checkRightClick()
# ! playing loop for players
global legal_moves, player_index, acn_focused, is_dragging, drawn_potential
player_index = 0
acn_focused = None
drawn_potential = []
avail_moves = []
fps = 5.0               # Make pretty samlsmalll, the display dont need to do anything that fast

last_move_indicator = None
if args.fen is not None:
    reset_board(args.fen)
legal_moves = engine_communicator.get_legal_moves()
is_dragging = False
ai_is_thinking = False
try:
    while True:

        game_over_text.undraw()
        while game_over_cache() == -1: # stuff to do every frame no matter what
            # print(f'Loop: {acn_focused}')

            current_turn_text.setText(turn_text_values[player_index])
            curr_game_text.setText('Gamee {}'.format(curr_game))
            curr_move_text.setText('Movee {}'.format(engine_communicator.get_move_number()))

            material_text.setText(str(engine_communicator.get_draw_status()))

            curr_player = both_players[player_index]

            if 'ai' in curr_player and not ai_is_thinking:
                # start thread
                ai_thread = threading.Thread(target=get_ai_move_threaded, args=(legal_moves, curr_player), daemon=True)
                ai_thread.start()
                ai_is_thinking = True

            if ai_is_thinking:
                try:
                    from_acn, to_acn = ai_move_queue.get_nowait()
                    make_move(from_acn, to_acn)
                    game_state_might_change = True
                    ai_is_thinking = False
                    continue
                except queue.Empty:
                    pass

            raw_position_left_click = win.checkMouse()
            if raw_position_left_click:
                ##print('clicked')
                raw_left_clicked_x, raw_left_clicked_y = raw_position_left_click.x, raw_position_left_click.y
                left_clicked_x, left_clicked_y = int(raw_left_clicked_x * 10), int(raw_left_clicked_y * 10)

                if raw_left_clicked_x > square_size * 8:
                    gui_click_choices()
                    continue

                if curr_player == 'human':
                    for i in drawn_potential: i.undraw()
                    drawn_potential = []

                    if (left_clicked_x, left_clicked_y) in x_y_to_acn:
                        acn_clicked = x_y_to_acn[(left_clicked_x, left_clicked_y)]
                        ##print(f'clicked on {acn_clicked}')

                        if acn_clicked in avail_moves:
                            temp = acn_focused
                            print('making move', temp, 'to', acn_clicked)
                            unfocus_and_stop_dragging()
                            make_move(temp, acn_clicked)
                            game_state_might_change = True
                            avail_moves = []
                            continue
                        elif not board[acn_clicked]:
                            ##print('clicked on empty square, unfocusing')
                            unfocus_and_stop_dragging()
                        else:
                            unfocus_and_stop_dragging()
                            is_dragging = True
                            acn_focused = acn_clicked
                            ##print('focusing on', acn_focused)

                        # draw potential moves
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
                                    ##print('Potential move at', to_square, 'which is', (render_x, render_y))
                                    potential_move.setFill(color_rgb(170, 170, 170))
                                    potential_move.draw(win)
                                    drawn_potential.append(potential_move)

        
            # if curr_player == 'human' and acn_focused:
            #     if win.checkRightClick():
            #         print('right clicked, unfocusing')
            #         unfocus_and_stop_dragging()

            #     raw_position_left_release = win.checkLeftRelease()
            #     if raw_position_left_release:
            #         print('released')
            #         raw_left_released_x, raw_left_released_y = raw_position_left_release.x, raw_position_left_release.y
            #         left_released_x, left_released_y = int(raw_left_released_x * 10), int(raw_left_released_y * 10)
            #         if (left_released_x, left_released_y) in list(map(lambda x: acn_to_x_y[x], avail_moves)):
            #             temp = acn_focused
            #             print('making move', temp, 'to', x_y_to_acn[(left_released_x, left_released_y)])
            #             unfocus_and_stop_dragging()
            #             make_move(temp, x_y_to_acn[(left_released_x, left_released_y)])
            #             game_state_might_change = True
            #             avail_moves = []
            #         elif acn_focused and (left_released_x, left_released_y) != acn_to_x_y[acn_focused]:
            #             print('released on invalid square, unfocusing')
            #             unfocus_and_stop_dragging()

            #     elif is_dragging:
            #         dragged_piece = board[acn_focused][2]
            #         scaledMouseX, scaledMouseY = win.toWorld(win.newmouseX, win.newmouseY)
            #         deltX = scaledMouseX - dragged_piece.anchor.x
            #         deltY = scaledMouseY - dragged_piece.anchor.y
            #         dragged_piece.move(deltX, deltY)

            win.update()
            time.sleep(1/fps)

        winner = '????'
        gamover = engine_communicator.game_over()
        if gamover == 0:    # GameState::WHITEWIN    
            winner = 'white'
        elif gamover == 2:  # GameState::BLACKWIN      
            winner = 'black'
        elif gamover == 1:  # GameState::DRAW
            winner = 'draw'
        #print("dog",gamover, winner)

        curr_game_text.setText(f'Game {curr_game}')
        white_wins_text.setText(f'W {curr_game_white}')
        black_wins_text.setText(f'B {curr_game_black}')
        draw_wins_text.setText(f'D {curr_game_draw}')

        # Only auto-reset if the game did NOT end in a draw. Otherwise we just stop and die and show the board
        #if autoreset_toggle and winner != 'draw':
        if autoreset_toggle:
            print('RESETTING BOARD TOGGLE')
            reset_board("", winner)
            game_state_might_change = True
            continue

        # ----- autoreset is OFF: update counters right here -----
        curr_game += 1
        if winner == 'white':
            curr_game_white += 1
        elif winner == 'black':
            curr_game_black += 1
        elif winner == 'draw':
            curr_game_draw += 1

        curr_game_text.setText(f'Game {curr_game}')
        white_wins_text.setText(f'W {curr_game_white}')
        black_wins_text.setText(f'B {curr_game_black}')
        draw_wins_text.setText(f'D {curr_game_draw}')


        # skip the blocking getMouse()

        # get raw positions
        raw_position_left_click = win.getMouse()
        raw_left_clicked_x, raw_left_clicked_y = raw_position_left_click.x, raw_position_left_click.y
        left_clicked_x, left_clicked_y = int(raw_left_clicked_x * 10), int(raw_left_clicked_y * 10)

        gui_click_choices()
except GraphicsError:
    pass

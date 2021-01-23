import sys
import os
import time
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


# ! change this for a different number of seconds to run
length_of_run = 10
# !

import engine_communicator


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
    def get_move(self, legal_moves):
        # ! if this line errors it is because random.choice(moves) returns 0, which shouldn't really be possible in a completed engine
        choice = random.choice(legal_moves)
        return choice[0:2], choice[2:4]

# variables for holding types
both_players = [Random(), Random()]


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





# ! playing loop for players
player_index = 0
legal_moves = engine_communicator.get_legal_moves()
start_time = time.time()
white_wins, black_wins, draws = 0, 0, 0
total_moves = 0
curr_game = 0

print('running for {} seconds'.format(length_of_run))
while time.time() < (start_time + length_of_run):
    winner = 'draw'
    while engine_communicator.game_over() == -1:
        if len(legal_moves) == 0:
            break

        curr_player = both_players[player_index]
        if isinstance(curr_player, AI):
            from_acn, to_acn = curr_player.get_move(legal_moves)
            total_moves += 1
            engine_communicator.make_move_two_acn(from_acn, to_acn)
            legal_moves = engine_communicator.get_legal_moves()
            player_index = 1 - player_index
            continue
        else:
            raise Exception("doesn't support this player type {}".format(str(type(both_players[player_index]))))
    
    # winner determinations
    if engine_communicator.game_over() == 0:
        white_wins += 1
    elif engine_communicator.game_over() == 2:
        black_wins += 1
    else:
        draws += 1
    engine_communicator.reset_engine();
    curr_game += 1
    legal_moves = engine_communicator.get_legal_moves()

real_total_time = time.time() - start_time
print('seconds_run: {}\ntotal {}\nwhite wins: {}\nblack wins: {}\ndraws: {}'.format(real_total_time, curr_game, white_wins, black_wins, draws))
print('games per second: {:0.2f}'.format(curr_game / real_total_time))
print('moves per second (nps): {:0.2f}'.format(total_moves / real_total_time))

stockfish_nps = 2 * pow(10, 6)
print('{:0.2f}% of stockfishes nps'.format(
    ((total_moves / real_total_time) / stockfish_nps) * 100
))
    
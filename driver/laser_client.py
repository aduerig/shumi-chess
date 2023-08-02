import sys
import pathlib
import asyncio
import multiprocessing
import json
import time
import traceback
from copy import deepcopy
import datetime

import websockets

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent

sys.path.insert(0, str(root_of_project_directory))
from helpers import *
import engine_communicator


def get_datetime_string():
    now = datetime.datetime.now()
    return now.strftime("%d/%m/%Y %H:%M:%S") # dd/mm/YY H:M:S# dd/mm/YY H:M:S



uri = 'wss://api.playlaser.xyz/'

get_games_json = {
    "messageType": "games",
}


minutes_for_us = 2
minutes_for_them = 5
time_to_engine = .3
seconds_for_us = int(minutes_for_us * 60 * 1000)
seconds_for_them = int(minutes_for_them * 60 * 1000)
our_name = 'mr. robot'
create_game_json = {
    "messageType": "create",
    "data": {
        "fen": "7q/4pnk1/4prn1/5pp1/1PP5/1NRP4/1KNP4/Q7 b - - 0 1",
        "turn": "b",
        "times": [seconds_for_us, seconds_for_them],
        "increments": [0, 0],
        "color": "w",
        "name": our_name,
    }
}


def invert_square(square):
    letter = square[0]
    number = square[1]
    inverted_letter = chr(ord('h') - ord(letter) + ord('a'))
    inverted_number = chr(ord('8') - ord(number) + ord('1'))
    return inverted_letter + inverted_number


def invert_move(move):
    from_square = move[0]
    to_square = move[1]
    return [invert_square(from_square), invert_square(to_square)]





curr_process = None
in_queue, out_queue = None, None
async def init_new_game():
    global in_queue, out_queue, curr_process

    if curr_process is not None and curr_process.is_alive():
        print_red('curr_process is not None, meaning there was another process, how kill? maybe just send message')
        exit(1)

    manager = multiprocessing.Manager()
    out_queue, in_queue = manager.Queue(), manager.Queue()
    curr_process = multiprocessing.Process(target=wait_for_engine_instructions, args=(in_queue, out_queue))
    curr_process.start()


def wait_for_engine_instructions(in_queue, out_queue):
    print(f'Starting wait_for_engine_instructions')    
    while True:
        if in_queue.qsize():
            instruction, data = in_queue.get_nowait()

            if instruction == 'get_fen':
                fen_str = engine_communicator.get_fen()
                out_queue.put(fen_str)
            elif instruction == 'get_move':
                move_str = engine_communicator.minimax_ai_get_move_iterative_deepening(data)
                move = [move_str[0:2], move_str[2:4]]
                inverted_move = invert_move(move)
                out_queue.put(inverted_move)
            elif instruction == 'make_move':
                inverted_move = invert_move(data)
                engine_communicator.get_legal_moves()
                engine_communicator.make_move_two_acn(inverted_move[0], inverted_move[1])
                print_cyan('Current boardstate')
                engine_communicator.print_gameboard()
                out_queue.put(None)
            elif instruction == 'kill_engine':
                out_queue.put(None)
                sys.exit(0)
        time.sleep(.01)


async def close_engine_if_open():
    if curr_process is not None and curr_process.is_alive():
        print_yellow('Closing engine')
        await communicate_with_engine('kill_engine')
        curr_process.join()


async def communicate_with_engine(instruction, data=None):
    in_queue.put((instruction, data))
    while out_queue.qsize() == 0:
        await asyncio.sleep(.01)
    return_val = out_queue.get_nowait()
    print(f'Got "{return_val}" from engine')
    return return_val

class CustomError(Exception):
    pass


async def host_and_play_games(websocket):
    total_games = 0
    won_games = 0
    drawn_games = 0
    engine_last_move = None

    await close_engine_if_open()

    games_log_path = this_file_directory.joinpath('games_log')
    max_existing_game_num = -1
    for filename, filepath in get_all_paths(games_log_path):
        if '_' in filename:
            number_part = filename.split('_')[0]
            if number_part.isnumeric():
                max_existing_game_num = max(max_existing_game_num, int(number_part))
        
        try:
            with open(filepath, 'r') as f:
                for line in f:
                    if line.startswith('Winner:'):
                        if line.startswith('Winner: w'):
                            won_games += 1
                        elif line.startswith('Winner: d'):
                            drawn_games += 1
                        total_games += 1
        except:
            print_stacktrace()
            print(f'Couldnt load game data from {filepath}')

    for game_num in range(max_existing_game_num + 1, 10000000):
        print(f'Hosting {game_num=}, {won_games=}, {drawn_games=}, {total_games=}')
        await init_new_game()

        create_game_json['data']['name'] = f'mr. robot - w: {won_games}, d: {drawn_games}, l: {total_games - (won_games + drawn_games)}'
        our_name = create_game_json['data']['name']
        await websocket.send(json.dumps(create_game_json))

        response_data = json.loads(await websocket.recv())
        if response_data['status'] == 'nameTaken':
            print_red('Raising exception because name taken')
            raise CustomError('Name taken')

        while True:
            response_data = json.loads(await websocket.recv())
            if response_data['status'] == 'games':
                pass
                # print('Got games lisit, but ignoring it')
            elif response_data['status'] == 'joined':
                break
        
        players = response_data['data']['names']
        game_key = response_data['data']['key']
        game_id = response_data['data']['id']
        seconds_left_for_us = seconds_for_us
        seconds_left_for_them = seconds_for_them
        print_cyan(f'Someone Joined! Playing game {players=}, {game_id=}, {game_key=}, {seconds_left_for_us=}, {seconds_left_for_them=}')


        with open(games_log_path.joinpath(f'{game_num}_{players[0]}_{players[1]}.dat'), 'w') as f:
            f.write(f'Game started at: {get_datetime_string()}\n')
            f.write(f'Player 0: {players[0]}, Player 1: {players[1]}\n')
            f.write(f'Seconds for player 0: {seconds_left_for_us}, Seconds for player 0: {seconds_left_for_them}\n')
            move_num = 1
            while True:
                response_data = json.loads(await websocket.recv())
                if response_data['status'] == 'games':
                    print('Got games lisit, but ignoring it')
                    continue
            
                if response_data['status'] == 'gameOver':
                    print_red(f'Game over, {winner=}')
                    winner = response_data['data']['winner']
                    if winner == 'w':
                        won_games += 1
                    if winner == 'd':
                        drawn_games += 1
                    total_games += 1
                    f.write(f'Winner: {winner}\n')
                    break
                
                if response_data['status'] == 'moved':
                    opponent_move = response_data['data']['move']

                    winner = response_data['data']['winner']
                    if winner is not None:
                        print_red(f'Game over, {winner=}')

                        if winner == 'w':
                            won_games += 1
                        if winner == 'd':
                            drawn_games += 1
                        total_games += 1
                        f.write(f'Winner: {winner}\n')
                        break

                    if opponent_move == engine_last_move:
                        continue

                    seconds_left_for_us = response_data['data']['times'][0] / 1000
                    seconds_left_for_them = response_data['data']['times'][1] / 1000
                    print_green(f'Got move from opponent {opponent_move=}, {winner=}, {seconds_left_for_us=}, {seconds_left_for_them=}')

                    # makes the opponents move
                    await communicate_with_engine('make_move', opponent_move)
                    f.write(f'Move {move_num}: {" ".join(invert_move(opponent_move))}, Player 0 seconds: {seconds_left_for_us}, Player 1 seconds: {seconds_left_for_them}\n')
                    move_num += 1

                    # gets move from engine
                    engine_move = await communicate_with_engine('get_move', time_to_engine)
                    engine_last_move = deepcopy(engine_move)

                    # updates engine with our move
                    await communicate_with_engine('make_move', engine_move)
                    f.write(f'Move {move_num}: {" ".join(invert_move(engine_move))}\n')
                    move_num += 1

                    fen = await communicate_with_engine('get_fen')

                    new_string = ''
                    for char in fen:
                        if char in 'acefghijklmnopqrstuvxyz' or char in 'acefghijklmnopqrstuvxyz'.upper():
                            if char.isupper():
                                new_string += char.lower()
                            else:
                                new_string += char.upper()
                        else:
                            new_string += char
                    fen = new_string
                    fen = fen.replace('b', 'w')
                    print(f'sending fen {fen}')
                    await websocket.send(json.dumps({
                        'messageType': 'move',
                        'data': {
                            'key': game_key,
                            'id': game_id, 
                            'name': our_name,
                            'fen': fen,
                            'move': engine_move,
                        }
                    }))
        await close_engine_if_open()

async def send_ping(websocket):
    while True:
        ping_message = json.dumps({
            "messageType": "ping",
        })
        await websocket.send(ping_message)
        print(f'{get_datetime_string()}: Sent ping')
        await asyncio.sleep(8)


async def main():
    times = 0
    while True:
        print(f'{get_datetime_string()}: Going to start websockets {times=}')
        try:
            async with websockets.connect(uri) as websocket:
                print_green(f'Connected to {uri}')
                await asyncio.gather(send_ping(websocket), host_and_play_games(websocket))
        except websockets.exceptions.WebSocketException as e:
            print_red(f'WebSocketException in main: {e}')
            traceback.print_exc()
        except CustomError as e:
            print_red(f'Name was taken in main, sleeping for 30')
            time.sleep(30)
        except TimeoutError as e:
            print_red(f'Timeout error in main, sleeping for 30')
            time.sleep(30)
            
        times += 1
        time.sleep(.5)
asyncio.run(main())

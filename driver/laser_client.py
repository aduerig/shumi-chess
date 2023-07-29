import sys
import pathlib
import asyncio
import multiprocessing
import json
import time
from copy import deepcopy

import websockets

this_file_directory = pathlib.Path(__file__).parent.resolve()
root_of_project_directory = this_file_directory.parent

sys.path.insert(0, str(root_of_project_directory))
from helpers import *
import engine_communicator



uri = 'wss://api.playlaser.xyz/'

get_games_json = {
    "messageType": "games",
}


minutes_for_us = 2
minutes_for_them = 3
time_to_engine = .5
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


def wait_for_engine_instructions(in_queue, out_queue):
    print(f'Starting wait_for_engine_instructions')    
    legal_moves = engine_communicator.get_legal_moves()
    print(f'Called engine_communicator.get_legal_moves() successfully, {len(legal_moves)}')
    while True:
        if in_queue.qsize():
            # maybe need try catch here?
            instruction, data = in_queue.get_nowait()

            if instruction == 'get_fen':
                # !TODO
                fen_str = engine_communicator.get_fen()
                out_queue.put(fen_str)
            elif instruction == 'get_move':
                move_str = engine_communicator.minimax_ai_get_move_iterative_deepening(data)
                move = [move_str[0:2], move_str[2:4]]
                inverted_move = invert_move(move)
                print(f'Got {move=} out of engine, {inverted_move=}')
                out_queue.put(inverted_move)
            elif instruction == 'make_move':
                inverted_move = invert_move(data)
                print(f'Making {data=} move in engine, {inverted_move=}')
                engine_communicator.get_legal_moves()
                print('might hang here')
                engine_communicator.make_move_two_acn(inverted_move[0], inverted_move[1])
                print('current state:')
                engine_communicator.print_gameboard()
                out_queue.put(None)
        time.sleep(.01)


curr_process = None
in_queue, out_queue = None, None
async def init_new_game():
    global in_queue, out_queue, curr_process

    if curr_process is not None:
        print_red('curr_process is not None, meaning there was another process, how kill? maybe just send message')
        exit(1)
        # curr_process.terminate()
        # curr_process.join()

    manager = multiprocessing.Manager()
    out_queue, in_queue = manager.Queue(), manager.Queue()
    curr_process = multiprocessing.Process(target=wait_for_engine_instructions, args=(in_queue, out_queue))
    curr_process.start()


async def host_and_play_games(websocket):
    for game_num in range(10000000):
        print(f'Hosting {game_num=}')
        await init_new_game()
        await websocket.send(json.dumps(create_game_json))


        response_data = json.loads(await websocket.recv())
        print_cyan('Response: ' + json.dumps(response_data, indent=4))

        if response_data['status'] == 'nameTaken':
            print_red('Quitting because name taken')
            exit(1)


        while True:
            response_data = json.loads(await websocket.recv())
            print_cyan('Response: ' + json.dumps(response_data, indent=4))

            if response_data['status'] == 'games':
                print('Got games lisit, but ignoring it')
            elif response_data['status'] == 'joined':
                print_green('SOMEONE JOINED')
                break
        
        players = response_data['data']['names']
        game_key = response_data['data']['key']
        game_id = response_data['data']['id']
        seconds_left_for_us = seconds_for_us
        seconds_left_for_them = seconds_for_them
        print(f'Playing game {players=}, {game_id=}, {game_key=}, {seconds_left_for_us=}, {seconds_left_for_them=}')

        while True:
            print('Waiting for response...')
            response_data = json.loads(await websocket.recv())
            print('Got response!')
            if response_data['status'] == 'games':
                print('Got games lisit, but ignoring it')
                continue
            # print_blue('Response: ' + json.dumps(response_data, indent=4))
            if response_data['status'] == 'moved':
                move = response_data['data']['move']
                winner = response_data['data']['winner']
                if winner == 'null':
                    print_red(f'Game over, {winner=}')
                    break

                seconds_left_for_us = response_data['data']['times'][0]
                seconds_left_for_them = response_data['data']['times'][1]
                print_green(f'Got move from opponent {move=}, {winner=}, {seconds_left_for_us=}, {seconds_left_for_them=}')

                print_cyan('updating engine with their move')
                in_queue.put(('make_move', move))
                while out_queue.qsize() == 0:
                    time.sleep(.01)
                return_val = out_queue.get_nowait()
                print(f'Got "{return_val}" from engine, expected None')

                print_cyan(f'querying engine for our move with {time_to_engine} seconds')
                in_queue.put(('get_move', time_to_engine))
                while out_queue.qsize() == 0:
                    time.sleep(.01)
                engine_move = out_queue.get_nowait()
                print(f'Got "{engine_move}" from engine, expected a move')

                print_cyan('updating engine with our move')
                in_queue.put(('make_move', engine_move))
                while out_queue.qsize() == 0:
                    time.sleep(.01)
                return_val = out_queue.get_nowait()
                print(f'Got "{return_val}" from engine, expected None')

                # gets new fen
                in_queue.put(('get_fen', time_to_engine))
                while out_queue.qsize() == 0:
                    time.sleep(.01)
                current_fen = out_queue.get_nowait()
                print(f'Got "{current_fen}" from engine, expected a fen')

                # need to send
                move_to_send = {
                    'messageType': 'move',
                    'data': {
                        'key': game_key,
                        'id': game_id, 
                        'name': our_name,
                        'fen': current_fen,
                        'move': engine_move,
                    }
                }
                await websocket.send(json.dumps(move_to_send))

                # check if this hangs
                print('eating a reponse here that is ourselves...')
                response_data = json.loads(await websocket.recv())
                # print_blue('Response: ' + json.dumps(response_data, indent=4))

        await asyncio.sleep(10000)


async def send_ping(websocket):
    while True:
        ping_message = json.dumps({
            "messageType": "ping",
        })
        await websocket.send(ping_message)
        await asyncio.sleep(9)


async def main():
    print('starting main')
    async with websockets.connect(uri) as websocket:
        print_green(f'Connected to {uri}')
        await asyncio.gather(send_ping(websocket), host_and_play_games(websocket))

asyncio.run(main())




# move = engine_communicator.minimax_ai_get_move_iterative_deepening(seconds)


# while engine_communicator.game_over() == -1:
# winner = 'draw'
# if engine_communicator.game_over() == 0:
#     winner = 'white'
# elif engine_communicator.game_over() == 2:
#     winner = 'black'




# legal_moves = engine_communicator.get_legal_moves()
# engine_communicator.reset_engine()

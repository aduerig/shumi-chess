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


# async def update_engine_with_move(websocket, move):
#     print('Updating engine!')
#     timeout_seconds = 10

#     while True:
#         if time.time() - start_time > timeout_seconds:
#             print_yellow(f'Solve timed out after {timeout_seconds} seconds')
#             process.terminate()
#             process.join()
#             await send_msg(websocket, json.dumps({'type': 'timeout', 'time': timeout_seconds}))
#             return
#         elif not process.is_alive():
#             solved_board, seconds_taken = final_queue.get()
#             break
#         else:
#             intermediary_grid = None
#             while intermediary_queue.qsize() > 0:
#                 try:
#                     intermediary_grid = intermediary_queue.get_nowait()
#                 except:
#                     pass

#             if intermediary_grid is not None:
#                 grid_back = precompute.grid_to_js_grid(intermediary_grid)
#                 msg = {
#                     'type': 'whole_board',
#                     'grid': grid_back,
#                 }
#                 await send_msg(websocket, json.dumps(msg))

#             await asyncio.sleep(0.1)

#     if solved_board is None:
#         print_yellow('Solve was impossible, sending impossible message back')
#         await send_msg(websocket, json.dumps({'type': 'impossible', 'time': seconds_taken}))
#         return

#     print_green('Solved board successfully and sending back')
#     msg = {
#         'type': 'clues',
#         'clues': crossword.get_clues(solved_board),
#     }
#     await send_msg(websocket, json.dumps(msg))

#     grid_back = precompute.grid_to_js_grid(solved_board.grid)
#     msg = {
#         'type': 'whole_board',
#         'grid': grid_back,
#     }

#     await send_msg(websocket, json.dumps(msg))
#     await send_msg(websocket, json.dumps({'type': 'solved', 'time': seconds_taken}))



def wait_for_engine_instructions(in_queue, out_queue):
    print(f'Starting wait_for_engine_instructions')    
    legal_moves = engine_communicator.get_legal_moves()
    print(f'Called engine_communicator.get_legal_moves() successfully, {len(legal_moves)}')
    while True:
        if in_queue.qsize():
            # maybe need try catch here?
            instruction, data = in_queue.get_nowait()

        if instruction == 'get_fen':
            fen_str = engine_communicator.get_fen()
            out_queue.put(fen_str)
        elif instruction == 'get_move':
            move_str = engine_communicator.get_move_iterative_deepening(data)
            out_queue.put(move_str)
        elif instruction == 'make_move':
            engine_communicator.make_move_two_acn(data[0:2], data[2:4])
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
        init_new_game()
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
        print(f'Playing game {players=}, {game_id=}, {game_key=}')

        seconds_left_for_us = seconds_for_us
        seconds_left_for_them = seconds_for_them
        while True:
            response_data = json.loads(await websocket.recv())
            print_blue('Response: ' + json.dumps(response_data, indent=4))

            if response_data['status'] == 'games':
                print('Got games lisit, but ignoring it')
            elif response_data['status'] == 'move':
                print_green('Got move')

                move_str = ''.join(response_data['data']['move'])
                winner = response_data['data']['winner']
                if winner == 'null':
                    print_red(f'Game over, {winner=}')
                    break

                seconds_left_for_us = response_data['data']['times'][0]
                seconds_left_for_them = response_data['data']['times'][1]

                # updates engine
                in_queue.put(('make_move', move_str))
                if in_queue.qsize():
                    return_val = out_queue.get_nowait()
                    print(f'Got "{return_val}" from engine, expected None')
                # asyncio.create_task(update_engine_with_move(websocket, move))

    
                # gets our move
                # asyncio.create_taskfgt(get_move_from_engine(websocket))

                chosen_move = 'a1a2'
                current_fen = '7q/4pnk1/4prn1/3P2p1/1P2p3/1NRP4/1KNP4/Q7 b - - 0 1'

                # need to send
                move_to_send = {
                    'messageType': 'move',
                    'data': {
                        'key': game_key,
                        'id': game_id, 
                        'name': our_name,
                        'fen': current_fen,
                        'move': [chosen_move[0:2], chosen_move[2:4]]
                    }
                }
                await websocket.send(json.dumps(move_to_send))

                # check if this hangs
                print('going to wait for a response lets see what happens here...')
                response_data = json.loads(await websocket.recv())
                print_blue('Response: ' + json.dumps(response_data, indent=4))

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

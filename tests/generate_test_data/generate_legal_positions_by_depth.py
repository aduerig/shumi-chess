
# uses https://python-chess.readthedocs.io/en/latest/_modules/chess.html 
# to generate correct paths and legal moves
import chess


# diff tests/test_data/legal_positions_by_depth.dat tests/test_data/legal_positions_by_depth_saved.dat
def fix_thing(board, popper, depth):
    stop = False
    while not stop:
        first = False
        popper.pop()
        if not popper:
            return False
        board.pop()
        last = popper[-1]
        try:
            the_move = next(last)
            board.push(the_move)
            popper.append(iter(list(board.legal_moves)))
            stop = True
        except:
            if not popper:
                return False
    while len(popper) < depth:
        the_move = next(popper[-1])
        board.push(the_move)
        popper.append(iter(list(board.legal_moves)))
    return True

def get_all_boards_at_depth(board, depth):
    if depth <= 0:
        yield 1
    else:
        popper = [iter(list(board.legal_moves))]

        while len(popper) < depth:
            the_move = next(popper[-1])
            board.push(the_move)
            popper.append(iter(list(board.legal_moves)))

        while popper:
            the_move = None
            while the_move == None:
                last = popper[-1]
                try:
                    the_move = next(last)
                except:
                    if not fix_thing(board, popper, depth):
                        return
            board.push(the_move)
            yield 1
            board.pop()

board = chess.Board()
total_move_counter = 0
tracker = {}
levels_to_search = 2

get_num_dups = False
if get_num_dups:
    boards_set = set()
    all_boards = []

file_name = '../test_data/legal_positions_by_depth.dat'
with open(file_name, 'w') as file:
    for depth in range(1, levels_to_search):
        depth_counter = 0
        file.write('DEPTH: ' + str(depth) + '\n')
        for _ in get_all_boards_at_depth(board, depth - 1):
            all_legals = list(board.legal_moves)
            for move in all_legals:
                board.push(move)
                if get_num_dups:
                    all_boards.append(board.fen())
                    boards_set.add(all_boards[-1])
                file.write(board.fen() + '\n')
                print(board.shredder_fen())
                board.pop()
                total_move_counter += 1
                depth_counter += 1
        tracker[depth] = depth_counter

print(tracker)
if get_num_dups:
    print('total duplicates: {0:,}'.format(len(all_boards) - len(boards_set)))
print('printed board fens out to', file_name)
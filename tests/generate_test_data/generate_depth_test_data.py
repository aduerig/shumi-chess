import chess
import cProfile
import itertools
import os
import time

# Treat as BFS problem
def generateMoveTestDataFileSpeedEfficient(base_board, depth, out_file):
    with open(out_file, 'w+') as file:
        file.write('Starting Fen: {}\n'.format(base_board.fen(en_passant="fen")))

        curr_depth = 0
        curr_board_queue = [base_board]
        while curr_depth < depth:
            # new depth here
            child_board_queue = []
            for curr_board in curr_board_queue:
                for move in curr_board.legal_moves:
                    curr_board.push(move)
                    if curr_depth == depth - 1:
                        file.write(curr_board.fen(en_passant="fen") + '\n')
                    if curr_depth + 1 < depth:
                        child_board_queue.append(curr_board.copy(stack=0))
                    curr_board.pop()
            curr_board_queue = child_board_queue
            curr_depth += 1


# The memory efficient code is nearly unusably slow
def generateMoveTestDataFileMemEfficient(board, depth, out_file):
    def generateChildrenBoards(working_board):
        for move in working_board.legal_moves:
            working_board.push(move)
            yield working_board
            working_board.pop()


    def appendBoardsUntilMaxDepth(board, curr_depth, max_depth):
        if curr_depth < max_depth:
            new_boards = generateChildrenBoards(board)
            if legal_board_generators_by_depth[curr_depth]:
                legal_board_generators_by_depth[curr_depth] = itertools.chain(legal_board_generators_by_depth[curr_depth], new_boards)
            else:
                legal_board_generators_by_depth[curr_depth] = new_boards
            for child_board in generateChildrenBoards(board):
                appendBoardsUntilMaxDepth(child_board, curr_depth+1, max_depth)


    legal_board_generators_by_depth = [None for d in range(depth)]
    appendBoardsUntilMaxDepth(board, 0, depth)

    with open(out_file, 'w+') as file:
        curr_depth = 1
        for board_generator in legal_board_generators_by_depth:
            file.write('DEPTH: ' + str(curr_depth) + '\n')
            for board in board_generator:
                file.write(board.fen(en_passant="fen") + '\n')
            curr_depth+=1


def generate_all_files_in_depth(start_fen: str, depth: int, board_name: str = 'unknown') -> None:
    global test_data_out_dir;
    for curr_depth in range(1, depth + 1):
        tic = time.perf_counter()
        out_path = os.path.join(test_data_out_dir, "{}_depth_{}.dat".format(board_name, curr_depth))
        if not os.path.exists(out_path):
            generateMoveTestDataFileSpeedEfficient(chess.Board(start_fen), curr_depth, out_path)
    # ? is this seconds?
    print(f"{board_name} to depth {depth} generation complete. Seconds taken: {time.perf_counter() - tic:0.2f}")


if __name__ == "__main__":
    test_data_path = os.path.join('..', 'test_data')
    if not os.path.exists(test_data_path):
        print('Did not find directory at {}, creating it'.format(test_data_path))
        os.mkdir(test_data_path)

    global test_data_out_dir;
    test_data_out_dir = os.path.relpath(test_data_path)

    generate_all_files_in_depth('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1', 4, 'normal')
    generate_all_files_in_depth('4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1', 4, 'pawns')
    # TODO: something is wrong with rooks
    generate_all_files_in_depth('r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1', 4, 'rooks')
    generate_all_files_in_depth('1n2k1n1/8/8/8/8/8/8/1N2K1N1 w - - 0 1', 5, 'knights')
    generate_all_files_in_depth('2b1kb2/8/8/8/8/8/8/2B1KB2 w - - 0 1', 4, 'bishops')
    generate_all_files_in_depth('3qk3/8/8/8/8/8/8/3QK3 w - - 0 1', 4, 'queens')
    generate_all_files_in_depth('4k3/8/8/8/8/8/8/4K3 w - - 0 1', 5, 'kings')

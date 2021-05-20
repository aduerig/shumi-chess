import chess
import cProfile
import itertools
import os
import time

# Treat as BFS problem
def generateMoveTestDataFileSpeedEfficient(base_board, depth, out_file):
    with open(out_file, 'w+') as file:
        curr_depth = 0
        curr_board_queue = [base_board]
        while curr_depth < depth:
            file.write('DEPTH: ' + str(curr_depth+1) + '\n')

            child_board_queue = []
            for curr_board in curr_board_queue:
                for move in curr_board.legal_moves:
                    curr_board.push(move)
                    file.write(curr_board.fen(en_passant="fen") + '\n')
                    if curr_depth+1 < depth:
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


if __name__ == "__main__":
    test_data_out_dir = os.path.relpath("../test_data/")

    tic = time.perf_counter()
    generateMoveTestDataFileSpeedEfficient(chess.Board(), 5, os.path.join(test_data_out_dir, "legal_positions_by_depth.dat"))
    print(f"Initial Board complete. Generation Time: {time.perf_counter() - tic:0.2f}")
    tic = time.perf_counter()
    generateMoveTestDataFileSpeedEfficient(chess.Board("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1"), 5, os.path.join(test_data_out_dir, "legal_positions_by_depth_pawns.dat"))
    print(f"Pawn Board complete. Generation Time: {time.perf_counter() - tic:0.2f}")
    tic = time.perf_counter()
    generateMoveTestDataFileSpeedEfficient(chess.Board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"), 5, os.path.join(test_data_out_dir, "legal_positions_by_depth_rooks.dat"))
    print(f"Rook Board complete. Generation Time: {time.perf_counter() - tic:0.2f}")
    tic = time.perf_counter()
    generateMoveTestDataFileSpeedEfficient(chess.Board("1n2k1n1/8/8/8/8/8/8/1N2K1N1 w - - 0 1"), 6, os.path.join(test_data_out_dir, "legal_positions_by_depth_knights.dat"))
    print(f"Knight Board complete. Generation Time: {time.perf_counter() - tic:0.2f}")
    tic = time.perf_counter()
    generateMoveTestDataFileSpeedEfficient(chess.Board("2b1kb2/8/8/8/8/8/8/2B1KB2 w - - 0 1"), 5, os.path.join(test_data_out_dir, "legal_positions_by_depth_bishops.dat"))
    print(f"Bishop Board complete. Generation Time: {time.perf_counter() - tic:0.2f}")
    tic = time.perf_counter()
    generateMoveTestDataFileSpeedEfficient(chess.Board("3qk3/8/8/8/8/8/8/3QK3 w - - 0 1"), 5, os.path.join(test_data_out_dir, "legal_positions_by_depth_queens.dat"))
    print(f"Queen Board complete. Generation Time: {time.perf_counter() - tic:0.2f}")
    tic = time.perf_counter()
    generateMoveTestDataFileSpeedEfficient(chess.Board("4k3/8/8/8/8/8/8/4K3 w - - 0 1"), 8, os.path.join(test_data_out_dir, "legal_positions_by_depth_kings.dat"))
    print(f"Kings Board complete. Generation Time: {time.perf_counter() - tic:0.2f}")
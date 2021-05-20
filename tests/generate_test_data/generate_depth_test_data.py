import chess
import itertools
import os

def generateMoveTestDataFile(board, depth, out_file):
    def generateChildrenBoards(base_board):
        for move in base_board.legal_moves:
            temp_board = base_board.copy(stack=False)
            temp_board.push(move)
            yield temp_board


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
            file.write('DEPTH: ' + str(depth+1) + '\n')
            for board in board_generator:
                file.write(board.fen(en_passant="fen") + '\n')
            curr_depth+=1


if __name__ == "__main__":
    test_data_out_dir = os.path.relpath("../test_data/")
    generateMoveTestDataFile(chess.Board(), 4, os.path.join(test_data_out_dir, "legal_positions_by_depth.dat"))
    print("Starting Board complete.")
    generateMoveTestDataFile(chess.Board("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1"), 4, os.path.join(test_data_out_dir, "legal_positions_by_depth_pawns.dat"))
    print("Pawn Board complete.")
    generateMoveTestDataFile(chess.Board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"), 4, os.path.join(test_data_out_dir, "legal_positions_by_depth_rooks.dat"))
    print("Rook Board complete.")
    generateMoveTestDataFile(chess.Board("1n2k1n1/8/8/8/8/8/8/1N2K1N1 w - - 0 1"), 5, os.path.join(test_data_out_dir, "legal_positions_by_depth_knights.dat"))
    print("Knight Board complete.")
    generateMoveTestDataFile(chess.Board("2b1kb2/8/8/8/8/8/8/2B1KB2 w - - 0 1"), 4, os.path.join(test_data_out_dir, "legal_positions_by_depth_bishops.dat"))
    print("Bishop Board complete.")
    generateMoveTestDataFile(chess.Board("3qk3/8/8/8/8/8/8/3QK3 w - - 0 1"), 4, os.path.join(test_data_out_dir, "legal_positions_by_depth_queens.dat"))
    print("Queen Board complete.")
    generateMoveTestDataFile(chess.Board("4k3/8/8/8/8/8/8/4K3 w - - 0 1"), 5, os.path.join(test_data_out_dir, "legal_positions_by_depth_kings.dat"))
    print("Kings Board complete.")
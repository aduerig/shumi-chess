#include <float.h>
#include <iomanip>
#include <sstream>
#include <locale>

#include "globals.hpp"
#include "minimax.hpp"
#include "utility.hpp"

using namespace std;
using namespace ShumiChess;
using namespace utility;
using namespace utility::representation;
using namespace utility::bit;

MinimaxAI::MinimaxAI(Engine& e) : engine(e) {
    // std::array<std::array<double, 8>, 8> pawn_values = {{
    //     {5, 5, 7, 7, 0, 0, 0, 0},
    //     {4, 6, 6, 8, 8, 8, 8, 0},
    //     {5, 5, 7, 7, 7, 7, 8, 0},
    //     {4, 6, 6, 6, 6, 6, 8, 0},
    //     {5, 5, 5, 5, 5, 5, 8, 7},
    //     {4, 4, 4, 4, 4, 7, 6, 7},
    //     {3, 3, 3, 3, 6, 5, 6, 5},
    //     {2, 2, 2, 5, 4, 5, 4, 5},
    // }};

    // std::array<double, 64> black_pawn_value_lookup;
    // std::array<double, 64> white_pawn_value_lookup;

    for (int i = 0; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;
        black_pawn_value_lookup[i] = .1 * pawn_values[7 - row][7 - col];
        white_pawn_value_lookup[63 - i] = .1 * pawn_values[7 - row][7 - col];
    }
}


template<class T>
string format_with_commas(T value) {
    stringstream ss;
    ss.imbue(locale(""));
    ss << fixed << value;
    return ss.str();
}


double MinimaxAI::Quiesce(int depth, int starting_depth, double alpha, double beta) {
    nodes_visited++;
    max_depth = max(max_depth, starting_depth - depth);

    // this is double calling legal_moves on the initial call
    LegalMoves legal_moves = engine.get_legal_moves();
    LegalMoves capture_moves = get_capture_moves(legal_moves);
    GameState state = engine.game_over(legal_moves);
    
    double stand_pat;
    if (state != GameState::INPROGRESS) {
        stand_pat = game_over_value(state, engine.game_board.turn);
    }
    else {
        stand_pat = evaluate_board(engine.game_board.turn, capture_moves);
    }

    if (stand_pat >= beta) {
        return beta;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }

    vector<Move> temp_moves;
    temp_moves.reserve(capture_moves.num_moves);
    for (int i = 0; i < capture_moves.num_moves; i++) {
        temp_moves.push_back(capture_moves.moves[i]);
    }

    for (auto move : temp_moves) {
        engine.push(move);
        
        double score = -Quiesce(depth - 1, starting_depth, -beta, -alpha);
        engine.pop();

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
           alpha = score;
        }
    }
    return alpha;
}



MoveAndBoardValue MinimaxAI::store_board_values_negamax(int depth, int starting_depth, double alpha, double beta, spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<Move, double, MoveHash>> &board_values, spp::sparse_hash_map<int, MoveBoardValueDepth> &transposition_table, bool debug) {
    nodes_visited++;
    max_depth = max(max_depth, starting_depth - depth);
    LegalMoves legal_moves = engine.get_legal_moves();
    GameState state = engine.game_over(legal_moves);
    
    if (state != GameState::INPROGRESS) {
        return {Move{}, game_over_value(state, engine.game_board.turn)};
    }

    if (depth == 0) {
        LegalMoves capture_moves = get_capture_moves(legal_moves);
        if (capture_moves.num_moves != 0) {
            return {Move{}, Quiesce(depth, starting_depth, alpha, beta)};
        }
        return {Move{}, evaluate_board(engine.game_board.turn, legal_moves)};
    }

    // !TODO the interplay of this and capture moves could be an issue, gating behind else right now but its kinda nonsensicle idk
    if (transposition_table.find(engine.game_board.zobrist_key) != transposition_table.end()) {
        MoveBoardValueDepth mbvd = transposition_table[engine.game_board.zobrist_key];
        if (mbvd.depth >= depth) {
            return {mbvd.move, mbvd.board_value};
        }
    }

    std::vector<MoveAndBoardValue> moves_ordered_search;
    moves_ordered_search.reserve(legal_moves.num_moves);
    if (board_values.find(engine.game_board.zobrist_key) != board_values.end()) {
        spp::sparse_hash_map<Move, double, MoveHash> moves_with_values = board_values[engine.game_board.zobrist_key];

        for (int i = 0; i < legal_moves.num_moves; i++) {
            // This shouldnt be filling -dbl_max probably, should be sorting like the else below (capture sorting)
            if (moves_with_values.find(legal_moves.moves[i]) == moves_with_values.end()) {
                moves_ordered_search.push_back({legal_moves.moves[i], -DBL_MAX + 1});
            }
            else {
                moves_ordered_search.push_back({legal_moves.moves[i], moves_with_values[legal_moves.moves[i]]});
            }
        }

    } else {
        for (int i = 0; i < legal_moves.num_moves; i++) {
            
            double capture_bonus = 0;
            if (legal_moves.moves[i].is_capture != Piece::NONE) {
                capture_bonus = 10000;
            }
            // capture_bonus = min((5 - (int) legal_moves.moves[i].is_capture), 1) * 100;
            double capture_piece = legal_moves.moves[i].is_capture * 100;
            double capturing_piece = -legal_moves.moves[i].piece_type;
            double capture_val = capture_bonus + capture_piece + capturing_piece;

            moves_ordered_search.push_back({legal_moves.moves[i], capture_val});
        }
    }

    std::sort(moves_ordered_search.begin(), moves_ordered_search.end(), 
        [](const MoveAndBoardValue& a, const MoveAndBoardValue& b) {
            return a.board_value > b.board_value;
        });

    double max_move_value = -DBL_MAX;
    Move best_move = moves_ordered_search[0].move;
    for (MoveAndBoardValue &m : moves_ordered_search) {
        Move move = m.move;
        engine.push(move);
        MoveAndBoardValue move_and_board_value = store_board_values_negamax(depth - 1, starting_depth, -beta, -alpha, board_values, transposition_table, debug);
        double board_value = -move_and_board_value.board_value;

        engine.pop();

        // !TODO same as above, this feels bogus. need to think about quissense and caching stuff
        board_values[engine.game_board.zobrist_key][move] = board_value;

        if (board_value > max_move_value) {
            max_move_value = board_value;
            best_move = move;
        }
        alpha = max(alpha, max_move_value);
        if (alpha >= beta) {
            break;
        }
    }

    if (transposition_table.find(engine.game_board.zobrist_key) == transposition_table.end() || 
        transposition_table[engine.game_board.zobrist_key].depth <= depth) {
        transposition_table[engine.game_board.zobrist_key] = {best_move, max_move_value, depth};
    }
    return {best_move, max_move_value};
}


Move MinimaxAI::get_move_iterative_deepening(double time) {
    seen_zobrist.clear();
    nodes_visited = 0;
    max_depth = 0;

    uint64_t zobrist_key_start = engine.game_board.zobrist_key;

    auto start_time = chrono::high_resolution_clock::now();
    auto required_end_time = start_time + chrono::duration<double>(time);

    spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<Move, double, MoveHash>> board_values;
    spp::sparse_hash_map<int, MoveBoardValueDepth> transposition_table;
    MoveAndBoardValue best_move;

    int depth = 1;
    while (chrono::high_resolution_clock::now() <= required_end_time) {
        best_move = store_board_values_negamax(depth, depth, -DBL_MAX, DBL_MAX, board_values, transposition_table, false);
        if (best_move.board_value == (DBL_MAX - 1) || best_move.board_value == (-DBL_MAX + 1)) {
            break;
        }
        depth++;
    }

    LegalMoves legal_moves = engine.get_legal_moves();
    Move move_chosen = legal_moves.moves[0];

    cout << "Went to iterative depth " << depth - 1 << ", max depth was: " << max_depth << endl;
    cout << "Found " << format_with_commas(board_values.size()) << " items inside of board_values" << endl;

    if (board_values.find(engine.game_board.zobrist_key) == board_values.end()) {
        cout << "Did not find zobrist key in board_values, this is bad" << endl;
        exit(1);
    }

    // auto stored_move_values = board_values[engine.game_board.zobrist_key];
    // for (int i = 0; i < legal_moves.num_moves; i++) {
    //     Move m = legal_moves.moves[i];
    //     if (stored_move_values.find(m) == stored_move_values.end()) {
    //         cout << "did not find move " << move_to_string(m) << " in the stored moves. this is probably bad" << endl;
    //         exit(1);
    //     }
    // }
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << colorize(AColor::BRIGHT_CYAN, "Minimax AI get_move_iterative_deepening chose move: for " + color + " player with score of " + to_string(best_move.board_value)) << endl;
    cout << colorize(AColor::BRIGHT_MAGENTA, "Nodes per second: " + format_with_commas((int)(nodes_visited / chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count()))) << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;
    cout << colorize(AColor::BRIGHT_BLUE, "Time it was supposed to take: " + to_string(time) + "s, Actual time taken: " + to_string(chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count()) + "s") << endl;
    if (engine.game_board.zobrist_key != zobrist_key_start) {
        cout << "get_move_iterative_deepening zobrist_key at begining: " << zobrist_key_start << ", at end: " << engine.game_board.zobrist_key << endl;
        exit(1);
    }
    return best_move.move;
}



// if (debug == true) {
//     cout << colorize(AColor::BRIGHT_GREEN, "On depth " + to_string(depth) + ", move is: " + move_to_string(m) + ", board_value below is: " + to_string(board_value) + ", color perspective: " + color_str(engine.game_board.turn)) << endl;
//     print_gameboard(engine.game_board);
// }




// uint64_t starting_zobrist_key = engine.game_board.zobrist_key;

// if (seen_zobrist.find(starting_zobrist_key) != seen_zobrist.end()) {
//     if (seen_zobrist[starting_zobrist_key] != gameboard_to_string(engine.game_board)) {
//         cout << "Zobrist collision!" << endl;
//         cout << "Zobrist key: " << starting_zobrist_key << endl;
//         cout << "Seen zobrist:\n" << seen_zobrist[starting_zobrist_key] << endl;
//         cout << "Current zobrist:\n" << gameboard_to_string(engine.game_board) << endl;
//         exit(1);
//     }
// }
// else {
//     seen_zobrist[starting_zobrist_key] = gameboard_to_string(engine.game_board);
// }


// if (engine.game_board.zobrist_key != starting_zobrist_key) {
//     print_gameboard(engine.game_board);
//     cout << "zobrist key doesn't match for: " << move_to_string(m) << endl;
//     exit(1);
// }
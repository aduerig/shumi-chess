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


double MinimaxAI::Quiesce(LegalMoves capture_moves, int depth, int max_depth, double alpha, double beta) {
    double stand_pat = evaluate_board<Piece::PAWN>(engine.game_board.turn, capture_moves) + evaluate_board<Piece::ROOK>(engine.game_board.turn, capture_moves) + evaluate_board<Piece::KNIGHT>(engine.game_board.turn, capture_moves) + evaluate_board<Piece::QUEEN>(engine.game_board.turn, capture_moves) + evaluate_board<Piece::NONE>(engine.game_board.turn, capture_moves);
    if( stand_pat >= beta )
        return beta;
    if( alpha < stand_pat )
        alpha = stand_pat;

    vector<Move> temp_moves;
    temp_moves.reserve(capture_moves.num_moves);
    for (int i = 0; i < capture_moves.num_moves; i++) {
        temp_moves.push_back(capture_moves.moves[i]);
    }

    for (auto move : temp_moves) {
        engine.push(move);

        LegalMoves legal_moves = engine.get_legal_moves();

        int move_counter = 0;
        for (int i = 0; i < legal_moves.num_moves; i++) {
            Move m = legal_moves.moves[i];
            if (m.capture != Piece::NONE) {
                capture_moves_internal[move_counter] = m;
                move_counter += 1;
            }
        }

        LegalMoves new_capture_moves {capture_moves_internal, move_counter};
        
        double score = -Quiesce(new_capture_moves, depth - 1, max_depth, -beta, -alpha);
        engine.pop();

        if( score >= beta )
            return beta;
        if( score > alpha )
           alpha = score;
    }
    return alpha;
}

MoveAndBoardValue MinimaxAI::store_board_values_negamax(int depth, int starting_depth, double alpha, double beta, spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<Move, double, MoveHash>> &board_values, spp::sparse_hash_map<int, MoveBoardValueDepth> &transposition_table, bool debug) {
    nodes_visited++;
    max_depth = max(max_depth, starting_depth - depth);
    LegalMoves consider_moves = engine.get_legal_moves();
    GameState state = engine.game_over(consider_moves);
    
    if (state != GameState::INPROGRESS) {
        double end_value = 0;
        if (state == GameState::BLACKWIN) {
            end_value = engine.game_board.turn == ShumiChess::WHITE ? -DBL_MAX + 1 : DBL_MAX - 1;
        }
        else if (state == GameState::WHITEWIN) {
            end_value = engine.game_board.turn == ShumiChess::BLACK ? -DBL_MAX + 1 : DBL_MAX - 1;
        }
        return {Move{}, end_value};
    }


    if (depth == 0) {
        int move_counter = 0;
        for (int i = 0; i < consider_moves.num_moves; i++) {
            Move m = consider_moves.moves[i];
            if (m.capture != Piece::NONE) {
                capture_moves_internal[move_counter] = m;
                move_counter += 1;
            }
        }

        LegalMoves capture_moves {capture_moves_internal, move_counter};
        if (capture_moves.num_moves == 0) {
            double eval = evaluate_board<Piece::PAWN>(engine.game_board.turn, consider_moves) + evaluate_board<Piece::ROOK>(engine.game_board.turn, consider_moves) + evaluate_board<Piece::KNIGHT>(engine.game_board.turn, consider_moves) + evaluate_board<Piece::QUEEN>(engine.game_board.turn, consider_moves) + evaluate_board<Piece::NONE>(engine.game_board.turn, consider_moves);
            return {Move{}, eval};
        }
        else {
            return {Move{}, Quiesce(capture_moves, depth, max_depth, alpha, beta)};
        }
    }

    // !TODO the interplay of this and capture moves could be an issue, gating behind else right now but its kinda nonsensicle idk
    if (transposition_table.find(engine.game_board.zobrist_key) != transposition_table.end()) {
        MoveBoardValueDepth mbvd = transposition_table[engine.game_board.zobrist_key];
        if (mbvd.depth >= depth) {
            return {mbvd.move, mbvd.board_value};
        }
    }


    std::vector<Move> moves_ordered_search;
    moves_ordered_search.reserve(consider_moves.num_moves);

    if (board_values.find(engine.game_board.zobrist_key) != board_values.end()) {
        spp::sparse_hash_map<Move, double, MoveHash> moves_with_values = board_values[engine.game_board.zobrist_key];

        std::vector<MoveAndBoardValue> temp_sorting_vec;
        temp_sorting_vec.reserve(consider_moves.num_moves);


        for (int i = 0; i < consider_moves.num_moves; i++) {
            if (moves_with_values.find(consider_moves.moves[i]) == moves_with_values.end()) {
                temp_sorting_vec.push_back({consider_moves.moves[i], -DBL_MAX + 1});
            }
            else {
                temp_sorting_vec.push_back({consider_moves.moves[i], moves_with_values[consider_moves.moves[i]]});
            }
        }

        std::sort(temp_sorting_vec.begin(), temp_sorting_vec.end(), 
            [](const MoveAndBoardValue& a, const MoveAndBoardValue& b) {
                return a.board_value > b.board_value;
            });
        
        for (const auto& move_and_board_value : temp_sorting_vec) {
            moves_ordered_search.push_back(move_and_board_value.move);
        }
    } else {
        for (int i = 0; i < consider_moves.num_moves; i++) {
            moves_ordered_search.push_back(consider_moves.moves[i]);
        }
    }

    double max_move_value = -DBL_MAX;
    Move best_move = moves_ordered_search[0];
    for (Move &m : moves_ordered_search) {
        engine.push(m);
        MoveAndBoardValue move_and_board_value = store_board_values_negamax(depth - 1, starting_depth, -beta, -alpha, board_values, transposition_table, debug);
        double board_value = -move_and_board_value.board_value;

        engine.pop();

        // !TODO same as above, this feels bogus. need to think about quissense and caching stuff
        if (depth > 0) {
            board_values[engine.game_board.zobrist_key][m] = board_value;
        }

        if (board_value > max_move_value) {
            max_move_value = board_value;
            best_move = m;
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

// if (debug == true) {
//     cout << colorize(AColor::BRIGHT_BLUE, "===== DEPTH 0 EVAL: " + to_string(eval) + ", color is: " + color_str(engine.game_board.turn)) << endl;
//     print_gameboard(engine.game_board);
// }

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
        cout << "deepening to depth " << depth << endl;
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
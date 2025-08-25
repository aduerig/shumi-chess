#include <float.h>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <locale>
#include <string>

//#define NDEBUG         // Define (uncomment) this to disable asserts
#undef NDEBUG
#include <assert.h>

#include <globals.hpp>
#include "utility.hpp"
#include "minimax.hpp"


using namespace std;
using namespace ShumiChess;
using namespace utility;
using namespace utility::representation;
using namespace utility::bit;

// Debug
//#define _DEBUGGING_PUSH_POP
//#define _DEBUGGING_MOVE_TREE
#ifdef _DEBUGGING_MOVE_TREE
    FILE *fpStatistics = NULL;
    char szDebug[256];
#endif

//////////////////////////////////////////////////////////////////////////////////////

MinimaxAI::MinimaxAI(Engine& e) : engine(e) { 
     // Open a file for debug writing

    #ifdef _DEBUGGING_MOVE_TREE 
        fpStatistics = fopen("C:\\programming\\shumi-chess\\Statistics.txt", "w");
        if(fpStatistics == NULL)    // Check if file was opened successfully
        {
            printf("Error opening statistics file!");
            assert(0);
        }    
    #endif
}

MinimaxAI::~MinimaxAI() { 
    #ifdef _DEBUGGING_MOVE_TREE
        if (fpStatistics != NULL) fclose(fpStatistics);
    #endif
}

int MinimaxAI::bits_in(ull bitboard) {
    auto bs = bitset<64>(bitboard);
    return (int) bs.count();
}

template<class T>
string format_with_commas(T value) {
    stringstream ss;
    ss.imbue(locale(""));
    ss << fixed << value;
    return ss.str();
}

double MinimaxAI::evaluate_board(Color for_color, vector<ShumiChess::Move>& moves) {
    double d_board_val_adjusted = 0.0;

    for (const auto& color : array<Color, 2>{Color::WHITE, Color::BLACK}) {
        double d_board_val = 0.0;

        // Add up the values for each piece
        for (const auto& i : piece_values) {
            
            // Obtain piece type and value
            Piece piece_type;
            double piece_value;
            tie(piece_type, piece_value) = i;
            
            // Since the king can never leave the board (engine does not allow), and there is an infinite score checked earlier we can just skip kings
            if (piece_type == Piece::KING) continue;

            // Get bitboard of all pieces on board of this type and color
            // NOTE: whatabout pairs of knights etc. We handle these all toegether? I think we do!
            ull pieces_bitboard = engine.game_board.get_pieces(color, piece_type);

            // Adds for the piece value mutiplied by how many of that piece there is.
            d_board_val += (piece_value * bits_in(pieces_bitboard));

            // hack improvements Favors ??
            // double dMultiplier = 1.0;
            // if (piece_type == Piece::QUEEN) dMultiplier = 0.5;
            // if (piece_type == Piece::KNIGHT) dMultiplier = 7.0;
            // if (piece_type == Piece::BISHOP) dMultiplier = 5.0;
            // d_board_val += (moves.size() * dMultiplier / 100);
            
            // Add a small bonus for pawns and knights in the middle of the board
            // if ( (piece_type == Piece::PAWN) || (piece_type == Piece::KNIGHT)) {
            //     ull center_squares = (0b00000000'00000000'00000000'00011000'00011000'00000000'00000000'00000000);
            //     ull middle_place = pieces_bitboard & center_squares;
            //     d_board_val +=  0.1 * bits_in(middle_place);
            //     // cout << "adding up " << color_str(color) << endl;
            // }
        }
        // Negate score for color
        if (color != for_color) {
            d_board_val *= -1;
        }
        d_board_val_adjusted += d_board_val;
    }

    // Favor for most moves, divided by arbitrary constant "80" 
    // NOTE: But we already adusted for color,  didnt we just above?  
    //     HAS THIS BEEN "play tested", it seems to do nothing.
    //d_board_val_adjusted += (moves.size() / 80);

    return d_board_val_adjusted;
}

//
// Choose the "minimax" AI move.
//
tuple<double, Move> MinimaxAI::store_board_values_negamax(int depth, double alpha, double beta
                                    , unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> &board_values
                                    , ShumiChess::Move& moveLast, bool debug) {

    nodes_visited++;
    
    // Get all legal moves from this position
    vector<Move> moves = engine.get_legal_moves();

    GameState state = engine.game_over(moves);
    
    if (state != GameState::INPROGRESS) {
        
        // Game is over
        double d_end_value;
        if (state == GameState::BLACKWIN) {
            d_end_value = engine.game_board.turn == ShumiChess::WHITE ? (-DBL_MAX + 1) : (DBL_MAX - 1);
        }
        else if (state == GameState::WHITEWIN) {
            d_end_value = engine.game_board.turn == ShumiChess::BLACK ? (-DBL_MAX + 1) : (DBL_MAX - 1);
        }
        return make_tuple(d_end_value, Move{});
    }
    
    // depth == 0 is the deepest level of analysis
    if (depth == 0) {

        // Evaluate end node.
        double d_eval = evaluate_board(engine.game_board.turn, moves);

        #ifdef _DEBUGGING_MOVE_TREE
            // Write to debug file 
            int nChars;
            strcpy(szDebug, "\n\n");
            nChars = fputs(szDebug, fpStatistics);
            if (nChars == EOF) assert(0);

            std::string stemp = gameboard_to_string(engine.game_board);
            const char* psz = stemp.c_str();
            nChars = fputs(psz, fpStatistics);
            if (nChars == EOF) assert(0);


            sprintf(szDebug, "      depth= %ld  score= %f", depth, d_eval);
            nChars = fputs(szDebug, fpStatistics);
            if (nChars == EOF) assert(0);
        #endif

        // Debug   NOTE: this if check reduces speed
        if (debug == true) {
            cout << colorize(AColor::BRIGHT_BLUE, "===== DEPTH 0 EVAL: " + to_string(d_eval) + ", color is: " + color_str(engine.game_board.turn)) << endl;
            print_gameboard(engine.game_board);

        }
        return make_tuple(d_eval, Move{});
    }

    unordered_map<Move, double, MoveHash> moves_with_values;
    std::vector<Move> sorted_moves;

    // NOTE: disabled so I can stll keep playing.
    if (0) {
    //if (board_values.find(engine.game_board.zobrist_key) != board_values.end()) {
        // NOTE: do someting with zobrist key? This is probably broken.
        moves_with_values = board_values[engine.game_board.zobrist_key];

        // NOTE: Commented out so I can stll keep playing.
        // for (auto& something : moves_with_values) {
        //     Move looking = something.first;
        //     if (std::find(moves.begin(), moves.end(), looking) == moves.end()) {
        //         print_gameboard(engine.game_board);
        //         cout << "Move shouldnt be legal: " << move_to_string(looking) << " at depth 1" << endl;
        //         exit(1);
        //     }
        // }

        std::vector<std::pair<Move, double>> vec(moves_with_values.begin(), moves_with_values.end());

        // Sort moves by best to worst
        std::sort(vec.begin(), vec.end(), 
            [](const std::pair<Move, double>& a, const std::pair<Move, double>& b) {
                return (a.second > b.second);
            });

        for (const auto& pair : vec) {
            sorted_moves.push_back(pair.first);
        }
        for (Move& m : moves) {
            if (moves_with_values.find(m) == moves_with_values.end()) {
                sorted_moves.push_back(m);
            }
        }
    } else {   // NOTE: Is this the normal path?
        sorted_moves = moves;
    }

    double dMax_move_value = -DBL_MAX;
    Move best_move = sorted_moves[0];
    for (Move &m : sorted_moves) {
        
        #ifdef _DEBUGGING_PUSH_POP
            string temp_fen_before = engine.game_board.to_fen();
        #endif
        
        // Push data
        engine.push(m);

        #ifdef _DEBUGGING_PUSH_POP
            string temp_fen_between = engine.game_board.to_fen();
        #endif

        // Recursive call down another level.
        auto ret_val = store_board_values_negamax((depth - 1), -beta, -alpha, board_values, m, debug);
        
        // ret_val is a tuple of the score and the move.
        double d_score_value = -get<0>(ret_val);

        // Debug   NOTE: this if check reduces speed
        // if (debug == true) {
        //     cout << colorize(AColor::BRIGHT_GREEN, "On depth " + to_string(depth) + ", move is: " + move_to_string(m) + ", score_value below is: " + to_string(d_score_value) + ", color perspective: " + color_str(engine.game_board.turn)) << endl;
        //     print_gameboard(engine.game_board);
        // }

        // Pop data
        engine.pop();

        #ifdef _DEBUGGING_PUSH_POP
            string temp_fen_after = engine.game_board.to_fen();
            if (temp_fen_before != temp_fen_after) {
                cout << "PROBLEM WITH PUSH POP!!!!!" << endl;
                cout_move_info(m);
                cout << "FEN before  push/pop: " << temp_fen_before  << endl;
                cout << "FEN between push/pop: " << temp_fen_between << endl;
                cout << "FEN after   push/pop: " << temp_fen_after   << endl;
                assert(0);
            }
        #endif

        // Digest score result
        board_values[engine.game_board.zobrist_key][m] = d_score_value;

        if (d_score_value > dMax_move_value) {
            dMax_move_value = d_score_value;
            best_move = m;
        }
        alpha = max(alpha, dMax_move_value);
        if (alpha >= beta) {
            // Stop looking for new moves - 
            break;
        }

    }
    return make_tuple(dMax_move_value, best_move);
}


// NOTE: This the entry point into the C to get a AI move.

Move MinimaxAI::get_move_iterative_deepening(double time) {
    seen_zobrist.clear();
    nodes_visited = 0;

    uint64_t zobrist_key_start = engine.game_board.zobrist_key;
    //cout << "zobrist_key at start of get_move_iterative_deepening is: " << zobrist_key_start << endl;

    auto start_time = chrono::high_resolution_clock::now();
    auto required_end_time = start_time + chrono::duration<double>(time);

    // Zobrist stuff ?
    unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> board_values;

    Move best_move;
    double d_best_move_value;

    Move null_move;

    int depth = 1;
    while (chrono::high_resolution_clock::now() <= required_end_time) {
        
        cout << "Deepening to " << depth << " half moves" << endl;

        auto ret_val = store_board_values_negamax(depth, -DBL_MAX, DBL_MAX, board_values, null_move, false);

        // ret_val is a tuple of the score and the move.
        d_best_move_value = get<0>(ret_val);
        best_move = get<1>(ret_val);

        depth++;
    }

    vector<Move> top_level_moves = engine.get_legal_moves();
    Move move_chosen = top_level_moves[0];

    cout << "Went to depth " << (depth - 1) << endl;
    cout << "Found " << board_values.size() << " items inside of board_values" << endl;

    if (board_values.find(engine.game_board.zobrist_key) == board_values.end()) {
        cout << "Did not find zobrist key in board_values, this is bad" << endl;
        exit(1);
    }

    // auto stored_move_values = board_values[engine.game_board.zobrist_key];
    // for (Move& m : top_level_moves) {
    //     if (stored_move_values.find(m) == stored_move_values.end()) {
    //         cout << "did not find move " << move_to_string(m) << " in the stored moves. this is probably bad" << endl;
    //         exit(1);
    //     }
    // }
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << colorize(AColor::BRIGHT_CYAN, to_string(d_best_move_value) + "= score, Minimax AI get_move_iterative_deepening chose move: for " + color + " to move") << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;
    cout << colorize(AColor::BRIGHT_BLUE, "Time it was supposed to take: " + to_string(time) + " s") << endl;
    cout << colorize(AColor::BRIGHT_GREEN, "Actual time taken: " + to_string(chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count()) + " s") << endl;
    //cout << "get_move_iterative_deepening zobrist_key at begining: " << zobrist_key_start << ", at end: " << engine.game_board.zobrist_key << endl;
    return best_move;
}









// === straight forward minimax below ===
double MinimaxAI::get_value(int depth, int color_multiplier, double alpha, double beta) {

    assert(0);

    nodes_visited++;
    vector<Move> moves = engine.get_legal_moves();
    GameState state = engine.game_over(moves);
    
    if (state == GameState::BLACKWIN) {
        // Use of DBL_MAX + 1 not really valid, as DBL_MAX + 1 == DBL_MAX in doubles.
        return (-DBL_MAX + 1) * (color_multiplier);
    }
    else if (state == GameState::WHITEWIN) {
        return (DBL_MAX - 1) * (color_multiplier);
    }
    else if (state == GameState::DRAW) {
        return 0;
    }
    
    // Depth decreases, when it hits zero we stop looking.
    if (depth == 0) {
        Color color_perspective = Color::BLACK;
        if (color_multiplier) {
            color_perspective = Color::WHITE;
        }
        return evaluate_board(color_perspective, moves) * color_multiplier;
    }
    //
    // Otherwise dive down one more level.
    //
    if (color_multiplier == 1) {  // Maximizing player
        double dMax_move_value = -DBL_MAX;
        for (Move& m : moves) {

            engine.push(m);

            double score_value = -1 * get_value(depth - 1, color_multiplier * -1, alpha, beta);

            engine.pop();

            dMax_move_value = max(dMax_move_value, score_value);
            alpha = max(alpha, dMax_move_value);
            if (beta <= alpha) {
                break; // beta cut-off
            }
        }
        return dMax_move_value;

    } else {  // Minimizing player
        double dMin_move_value = DBL_MAX;
        for (Move& m : moves) {
            engine.push(m);
            double score_value = -1 * get_value(depth - 1, color_multiplier * -1, alpha, beta);
            engine.pop();
            dMin_move_value = min(dMin_move_value, score_value);
            beta = min(beta, dMin_move_value);
            if (beta <= alpha) {
                break; // alpha cut-off
            }
        }
        return dMin_move_value;
    }
}

//
// Get 
//

Move MinimaxAI::get_move(int depth) {
    auto start_time = chrono::high_resolution_clock::now();

    nodes_visited = 0;

    int color_multiplier = 1;
    if (engine.game_board.turn == Color::BLACK) {
        color_multiplier = -1;
    }

    Move move_chosen;
    double dMax_move_value = -DBL_MAX;
    vector<Move> moves = engine.get_legal_moves();
    for (Move& m : moves) {
        engine.push(m);
        double score_value = get_value(depth - 1, color_multiplier * -1, -DBL_MAX, DBL_MAX);
        if (score_value * -1 > dMax_move_value) {
            dMax_move_value = score_value * -1;
            move_chosen = m;
        }
        engine.pop();
    }
    
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";
    cout << colorize(AColor::BRIGHT_CYAN, "Minimax AI get_move chose move: for " + color + " player") << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;

    chrono::duration<double> total_time = chrono::high_resolution_clock::now() - start_time;
    cout << colorize(AColor::BRIGHT_GREEN, "Total time taken for get_move(): " + to_string(total_time.count()) + " s") << endl;

    return move_chosen;
}

Move MinimaxAI::get_move() {
    int default_depth = 7;
    cout << colorize(AColor::BRIGHT_GREEN, "Called MinimaxAI::get_move() with no arguments, using default depth: " + to_string(default_depth)) << endl;
    return get_move(default_depth);
}


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
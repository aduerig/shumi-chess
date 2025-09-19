#include <float.h>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <locale>
#include <string>

//#define NDEBUG         // Define (uncomment) this to disable asserts
#undef NDEBUG
#include <assert.h>

#define DADS_CRAZY_EVALUATION_CHANGES


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
    char szValue[128];
    char szScore[128];
#endif

//#define RANDOMIZING_EQUAL_MOVES

static void printMoveToFile(const char* p_move_text, Color turn, int level, bool precede_with_CR);

//////////////////////////////////////////////////////////////////////////////////////

MinimaxAI::MinimaxAI(Engine& e) : engine(e) { 
     // Open a file for debug writing

    #ifdef _DEBUGGING_MOVE_TREE 
        fpStatistics = fopen("C:\\programming\\shumi-chess\\debug.dat", "w");
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



template<class T>
string format_with_commas(T value) {
    stringstream ss;
    ss.imbue(locale(""));
    ss << fixed << value;
    return ss.str();
}

//
// Returns " relative (negamax) score". Relative score is positive for great positions for the specified player. 
// Absolute score is always positive for great positions for white.
//
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
            d_board_val += (piece_value * (double)engine.bits_in(pieces_bitboard));

            #ifdef DADS_CRAZY_EVALUATION_CHANGES
                  d_board_val += 0.0;
            #endif


            // Add a small bonus for pawns and knights in the middle of the board
            // if ( (piece_type == Piece::PAWN) || (piece_type == Piece::KNIGHT)) {
            //     ull center_squares = (0b00000000'00000000'00000000'00011000'00011000'00000000'00000000'00000000);
            //     ull middle_place = pieces_bitboard & center_squares;
            //     d_board_val +=  0.1 * bits_in(middle_place);
            //     // cout << "adding up " << color_str(color) << endl;
            // }
        }

        // Mske the "absolute score" an "relative (negamax) score".
        assert (d_board_val>=0);
        if (color != for_color) {
            d_board_val *= -1;
        }
        d_board_val_adjusted += d_board_val;
    }

    // Favor for most moves from this position, divided by arbitrary constant "80" 
    // NOTE: But we already adusted for color,  didnt we just above?  
    //     HAS THIS BEEN "play tested"
    //d_board_val_adjusted += (moves.size() / 80);

    return d_board_val_adjusted;
}

//
// Choose the "minimax" AI move.
// Returns a tuple of:
//    the score value
//    the "best move"
//
#define HUGE_SCORE 10000  // A million centipawns!       //  DBL_MAX    // A relative score

tuple<double, Move> MinimaxAI::store_board_values_negamax(int depth, double alpha, double beta
                                    //, unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> &move_scores
                                    , unordered_map<std::string, unordered_map<Move, double, MoveHash>> &move_scores
                                    , ShumiChess::Move& move_last
                                    , bool debug) {
    //assert(depth>=0);

    nodes_visited++;
    

    // These are are the final result, returned from here. A best score, and best move.
    // If I didnt look at any moves, then the best move is, default, or "none".
    double d_end_score = 0.0;
    Move the_best_move = {};

    // default-initializes the tuple: the double becomes 0.0 and Move is default-constructed.
    std::tuple<double, ShumiChess::Move> final_result;

    int level = (top_depth-depth);
    assert (level >= 0);
    double d_level = static_cast<double>(level);
    //d_level = 0.0;   // Stupid debug. Delete me.
     
    vector<Move> legal_moves = engine.get_legal_moves();

    std::vector<Move> moves_to_loop_over = legal_moves;     // By default



    GameState state = engine.game_over(legal_moves);


    std::vector<ShumiChess::Move> unquiet_moves;

    if (state != GameState::INPROGRESS) {
        
        // Game is over.  Here recursive analysis must stop.

        // Relative (negamax): score is from the side-to-move’s perspective.
        // NOTE: I added "mate-distance bookkeeping":
        // Use a large mate score reduced by d_level so shorter mates have larger magnitude.
        if (state == GameState::WHITEWIN) {
            d_end_score = (engine.game_board.turn == ShumiChess::WHITE)
                            ? (+HUGE_SCORE - d_level)   // (edge case, normally loser to move)
                            : (-HUGE_SCORE + d_level);  // side to move is mated
        }
        else if (state == GameState::BLACKWIN) {
            d_end_score = (engine.game_board.turn == ShumiChess::BLACK)
                            ? (+HUGE_SCORE - d_level)   // (edge case, normally loser to move)
                            : (-HUGE_SCORE + d_level);  // side to move is mated
        }
        else if (state == GameState::DRAW) { 
            // This is Stalemate.
            //print_gameboard(engine.game_board);
            d_end_score = 0.0;    // I am not needed because of initilazation
        }
        else
        {
            assert(0);
        }

        //Move defaultMove = Move{};       // Use a default move", as there is no "best move" in this position. Its the end.
        final_result = make_tuple(d_end_score, the_best_move);

        return final_result;


    } else if (depth <= 0) {

        // depth == 0 is the deepest level of analysis. Here recursive analysis must stop.
        //
        // Evaluate end node "relative score". Relative score is always positive for great positions for the specified player. 
        // Absolute score is always positive for great positions for white.

        d_end_score = evaluate_board(engine.game_board.turn, legal_moves);

        //Move defaultMove = Move{};       // Use a default move", as there is no "best move" in this position. Its the end.
        final_result = make_tuple(d_end_score, the_best_move);

    } else {
        //
        // Keep analyzing moves (recurse another level)
        //

        #ifdef _DEBUGGING_MOVE_TREE
            engine.bitboards_to_algebraic(engine.game_board.turn, move_last, state, engine.move_string);         
            
            int level = (top_depth-depth);
            printMoveToFile(engine.move_string.c_str(), engine.game_board.turn, level, true);
        #endif


        std::vector<Move> sorted_moves;              // Local array to store sorted moves
        
        
        
        //sorted_moves = legal_moves;
        sorted_moves = moves_to_loop_over;


        // Sort the moves by relative score (a descending sort over doubles)
        // "iterative deepening"
        sort_moves_by_score(sorted_moves, move_scores, true);

        //
        // Loop over all moves
        //
        d_end_score = -HUGE_SCORE;
        // Note: by default pick the first move? Or best move from last time? Does it matter?
        // I guess note, if we are not sorted.
        the_best_move =  sorted_moves[0]; 

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
            auto ret_val = store_board_values_negamax((depth - 1), -beta, -alpha
                                                    , move_scores
                                                    , m, debug);
            
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

            // Digest (store) score result

            string temp_fen_now = engine.game_board.to_fen();
            move_scores[temp_fen_now][m] = d_score_value;

            // uint64_t someInt = engine.game_board.zobrist_key;
            // move_scores[someInt][m] = d_score_value;

            // Note: Should this comparison be done with a fuzz?
            bool b_use_this_move = (d_score_value > d_end_score);
            
            // Note: this should be done as a real random choice. (random over the moves possible). 
            // This dumb approach favors moves near the end of the list
            #ifdef RANDOMIZING_EQUAL_MOVES
                // Hey, randomize the choice (sort of).
                if (d_score_value == d_end_score) {
                    b_use_this_move = engine.flip_a_coin();
                } else {
                    b_use_this_move = (d_score_value > d_end_score);
            }
            #endif

            if (b_use_this_move) {
                d_end_score = d_score_value;
                the_best_move = m;
            }

            #define VERY_SMALL 1.0e-9
            alpha = max(alpha, d_end_score);
            // NOTE: Is this the best way to do this comparison?
            //if ((alpha) >= beta) {
            if ((alpha) >= (beta+VERY_SMALL)) {   
                // Stop looking for new moves - (break out of the for loop over all legal moves)
                #ifdef _DEBUGGING_MOVE_TREE
                    // Append to the move tree
                    snprintf(szDebug, sizeof(szDebug), "   alpha= %f,  beta %f      %f", alpha, beta, d_end_score);
                    int ierr = fputs(szDebug, fpStatistics);
                    assert (ierr!=EOF);
                #endif
                break;
            }

        }

        // Assemble return
        final_result = make_tuple(d_end_score, the_best_move);
    }




    #ifdef _DEBUGGING_MOVE_TREE

        // int level = (top_depth-depth);   // Increasing positive with analysis
        assert (level>=0);
        Add_to_print_tree(fpStatistics
                            , move_last
                            , final_result
                            , state, level);
   
    #endif



    #ifdef _DEBUGGING_MOVE_TREE
        if (depth==0) {


            // vector<ShumiChess::Move> unquiet_moves;
            // unquiet_moves = engine.reduce_to_unquiet_moves(legal_moves);


            // for (const ShumiChess::Move& mv : legal_moves) {
            //     // ...
            //     //engine.bitboards_to_algebraic(engine.game_board.turn, mv, (GameState::INPROGRESS), engine.move_string);       
            //     print_move
            // }

            bool in_check = engine.is_king_in_check(engine.game_board.turn);

            sprintf(szDebug, "EVAL:%i %i\n", in_check, unquiet_moves.size());
            int nChars = fputs(szDebug, fpStatistics);
            if (nChars == EOF) assert(0);
            engine.print_moves_to_file(unquiet_moves, (4+level), fpStatistics);

            
        }
    #endif





    return final_result;
}




//////////////////////////////////////////////////////////////////////////////////
//
// NOTE: This the entry point into the C to get a minimax AI move.
//   It does "Iterative deepening".
//
//////////////////////////////////////////////////////////////////////////////////

Move MinimaxAI::get_move_iterative_deepening(double time) {

    seen_zobrist.clear();
    nodes_visited = 0;

    uint64_t zobrist_key_start = engine.game_board.zobrist_key;
    //cout << "zobrist_key at start of get_move_iterative_deepening is: " << zobrist_key_start << endl;

    auto start_time = chrono::high_resolution_clock::now();
    auto required_end_time = start_time + chrono::duration<double>(time);

    // Results of previous searches
    //unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> move_scores;
    unordered_map<std::string, unordered_map<Move, double, MoveHash>> move_scores;

    size_t nFENS = move_scores.size();    // This returns the number of FEN rows.
    assert(nFENS == 0);

    Move best_move = {};
    double d_best_move_value;

    //Move null_move = Move{};
    Move null_move = engine.users_last_move;

    int maximum_depth = 2;        // Note: because i said so.
    assert(maximum_depth>=1);

    // NOTE: this should be an option: depth .vs. time.
    int depth = 1;
    do {
        
        cout << "Deepening to " << depth << " half moves" << endl;

        top_depth = depth;
        auto ret_val = store_board_values_negamax(depth, -HUGE_SCORE, HUGE_SCORE
                                                , move_scores
                                                , null_move, false);

        // ret_val is a tuple of the score and the move.
        d_best_move_value = get<0>(ret_val);
        best_move = get<1>(ret_val);

        #ifdef _DEBUGGING_MOVE_TREE

            //print_move_scores_to_file(fpStatistics, move_scores);

        #endif


        depth++;
    //} while (chrono::high_resolution_clock::now() <= required_end_time);
    } while (depth < (maximum_depth+1));

    cout << "Went to depth " << (depth - 1) << endl;

    //vector<Move> top_level_moves = engine.get_legal_moves();
    //Move move_chosen = top_level_moves[0];     // Note: Assumes the moves are sorted?

    //cout << "Found " << move_scores.size() << " items inside of move_scores" << endl;

    // if (move_scores.find(engine.game_board.zobrist_key) == move_scores.end()) {
    //     cout << "Did not find zobrist key in move_scores, this is bad" << endl;
    //     exit(1);
    // }

    // auto stored_move_values = move_scores[engine.game_board.zobrist_key];
    // for (Move& m : top_level_moves) {
    //     if (stored_move_values.find(m) == stored_move_values.end()) {
    //         cout << "did not find move " << move_to_string(m) << " in the stored moves. this is probably bad" << endl;
    //         exit(1);
    //     }
    // }
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";

    // Convert to absolute score
    double d_best_move_value_abs = d_best_move_value;
    if (engine.game_board.turn == Color::BLACK) d_best_move_value_abs = -d_best_move_value_abs;
    
    string abs_score_string = to_string(d_best_move_value_abs);

    engine.bitboards_to_algebraic(engine.game_board.turn, best_move, (GameState::INPROGRESS)
                //, NULL
                , engine.move_string);

    cout << colorize(AColor::BRIGHT_CYAN,engine.move_string) << "   ";

    cout << colorize(AColor::BRIGHT_CYAN, abs_score_string + "= score, Minimax AI chose move: for " + color + " to move") << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;
    cout << colorize(AColor::BRIGHT_GREEN, "Time it was supposed to take: " + to_string(time) + " s") << endl;
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

    assert(0);      // exploratory assert


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

// Sort the moves by relative score (a descending sort over doubles)
void MinimaxAI::sort_moves_by_score(
    std::vector<ShumiChess::Move>& moves,  // reorder this in place
    const std::unordered_map<std::string,
        std::unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>
    >& move_scores,
    bool sort_descending  // true = highest score first
)

{
    const std::string fen = engine.game_board.to_fen();
    auto row_it = move_scores.find(fen);  // don't create if missing
    if (row_it != move_scores.end() && !row_it->second.empty()) {
        const auto& row = row_it->second; // unordered_map<Move,double,...>

        // Pair up each current legal move with its stored score (or a tiny fallback)
        std::vector<std::pair<Move,double>> a;
        a.reserve(moves.size());
        for (const Move& m : moves) {
            auto it = row.find(m);
            double s = (it != row.end()) ? it->second : std::numeric_limits<double>::lowest();
            a.emplace_back(m, s);  // unscored moves will end up last
        }

        // Insertion sort by score (descending)
        for (size_t i = 1; i < a.size(); ++i) {
            auto key = a[i];
            size_t j = i;
            while (j > 0 && a[j-1].second < key.second) { a[j] = a[j-1]; --j; }
            a[j] = key;
        }

        // Write back just the moves, to sorted_moves, in the new order
        for (size_t i = 0; i < a.size(); ++i) moves[i] = a[i].first;
    }
}

        //unordered_map<Move, double, MoveHash> moves_with_values;

        // NOTE: disabled so I can stll keep playing.
        // (causes "Magical rook appearence3 bug:", after Ke2)
        // if (0) {  // "temporary fix"
        // //if (move_scores.find(engine.game_board.zobrist_key) != move_scores.end()) {
        //     // NOTE: do someting with zobrist key? This is probably broken.
        //     moves_with_values = move_scores[engine.game_board.zobrist_key];

        //     // NOTE: Commented out so I can stll keep playing.
        //     // for (auto& something : moves_with_values) {
        //     //     Move looking = something.first;
        //     //     if (std::find(moves.begin(), moves.end(), looking) == moves.end()) {
        //     //         print_gameboard(engine.game_board);
        //     //         cout << "Move shouldnt be legal: " << move_to_string(looking) << " at depth 1" << endl;
        //     //         exit(1);
        //     //     }
        //     // }

        //     std::vector<std::pair<Move, double>> vec(moves_with_values.begin(), moves_with_values.end());

        //     // Sort moves by best to worst
        //     std::sort(vec.begin(), vec.end(), 
        //         [](const std::pair<Move, double>& a, const std::pair<Move, double>& b) {
        //             return (a.second > b.second);
        //         });

        //     for (const auto& pair : vec) {
        //         sorted_moves.push_back(pair.first);
        //     }
        //     for (Move& m : legal_moves) {
        //         if (moves_with_values.find(m) == moves_with_values.end()) {
        //             sorted_moves.push_back(m);
        //         }
        //     }
        // } else {   // NOTE: Is this the normal path?
        //     sorted_moves = legal_moves;
        // }






/////////////////////////////////////////////////////////////////////

#ifdef _DEBUGGING_MOVE_TREE

static void printMoveToFile(const char* p_move_text, Color turn, int level, bool precede_with_CR)
{

    //int level = (top_depth-depth);

    if (precede_with_CR) {
        int ierr = fputc('\n', fpStatistics);
        assert (ierr!=EOF);
    }

    int ierr = fputc('\n', fpStatistics);
    assert (ierr!=EOF);

    // Indent the whole thing over based on depth level
    int nTabs = level+2;
    
    if (nTabs<0) nTabs=0;

    int nSpaces = nTabs*4;
    int nChars = fprintf(fpStatistics, "%*s", nSpaces, "");

    // compose "..."+move (for Black) or just move (for White)
    if (turn == opposite_color(ShumiChess::BLACK)) snprintf(szValue, sizeof(szValue), "...%s", p_move_text);
    else                                           snprintf(szValue, sizeof(szValue), "%s",    p_move_text);

    // print as a single left-justified 8-char field: "...e4   " or "e4     "
    //                                                 12345678
    fprintf(fpStatistics, "%-12.8s", szValue);

}


///////////////////////////////////////////////////////////////////////////////////


void MinimaxAI::Add_to_print_tree(FILE* fpStatistics
                                    , ShumiChess::Move& move_last
                                    ,std::tuple<double, ShumiChess::Move> final_result
                                    , GameState state
                                    ,int level
                                    )
{

    assert(level>=0);
    const char* sz_move_text;

    double d_end_score = std::get<0>(final_result);                 // first element
    ShumiChess::Move best_move = std::get<1>(final_result);         // second element

    // Debug only
    //if ( (state != GameState::INPROGRESS) || (depth == 0) )
    if (1)   // (state != GameState::INPROGRESS) || (depth == 0) )
    {
        // Write to debug file 
        int nChars;

        // Get algebriac (SAN) text form of the last move.
        engine.bitboards_to_algebraic(engine.game_board.turn, move_last, state, engine.move_string);
        sz_move_text = engine.move_string.c_str();

        printMoveToFile(sz_move_text, engine.game_board.turn, level, false);

        // Print the rest of the crap
        strcpy(szValue, "");

        // Add the best move
        strcat(szValue, "  best= ");
        engine.bitboards_to_algebraic(engine.game_board.turn, best_move, state, engine.move_string);
        sz_move_text = engine.move_string.c_str();
        strcat(szValue, sz_move_text);


       // Show absolute score
        double d_eval_abs_score = d_end_score;
        if (engine.game_board.turn == ShumiChess::BLACK)  d_eval_abs_score = -d_eval_abs_score;
        if (fabs(d_eval_abs_score) < 0.000000005) d_eval_abs_score = 0.0;   // Note: Crazy trick to avoid "-0.00"

        if (d_eval_abs_score == 0.0) {
            // approximate centering in the same spot as val=
            snprintf(szScore, sizeof(szScore), "     ~   ");
        } else {
            snprintf(szScore, sizeof(szScore),
                    (fabs(d_eval_abs_score) >= 1e6) ? "%8.2e " : "%8.2f ",
                    d_eval_abs_score);
        }
        strcat(szValue, szScore);

        // Add the "level
        snprintf(szScore, sizeof(szScore), " lev= %-4d", level);
        strcat(szValue, szScore);

        // Print the mess.
        nChars = fputs(szValue, fpStatistics);
        if (nChars == EOF) assert(0);

    }

}


void MinimaxAI::print_move_scores_to_file(
    FILE* fpStatistics,
    const std::unordered_map<std::string,std::unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>>& move_scores
)
{

    // sprintf(szValue, "\n---------------------------------------------------------------------------");
    // fprintf(fpStatistics, "%s", szValue);
    // size_t iFENS = move_scores.size();    // This returns the number of FEN rows.
    // sprintf(szValue, "\n\n  nFENS = %i", (int)iFENS);
    // fprintf(fpStatistics, "%s", szValue);

    fputs("\n\n---------------------------------------------------------------------------", fpStatistics);
    fprintf(fpStatistics, "\n  nFENS = %zu\n", move_scores.size());  // %zu for size_t

    // cout << endl;
    // print_gameboard(engine.game_board);

    for (const auto& fen_row : move_scores) {
        const auto& moves_map = fen_row.second;

        // Optional: print the FEN once per block
        fprintf(fpStatistics, "\nFEN: %s\n", fen_row.first.c_str());

        for (const auto& kv : moves_map) {
            const Move& m = kv.first;
            double score  = kv.second;

            std::string san_string;
            engine.bitboards_to_algebraic(
                                        ShumiChess::BLACK
                                        //utility::representation::opposite_color(engine.game_board.turn)
                                        ,m
                                        ,ShumiChess::GameState::INPROGRESS
                                        ,san_string);

            fprintf(fpStatistics, "%s  %.6f\n", san_string.c_str(), score);
        }
    }
    fputs("\n\n---------------------------------------------------------------------------", fpStatistics);


}


#endif



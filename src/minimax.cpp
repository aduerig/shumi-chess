#include <float.h>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <locale>
#include <string>
#include <tuple>
#include <algorithm>  // at top
#include <set>
#include <cstdint>
#include <cmath>

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

#include <chrono>
#include <iostream>

#include <atomic>
static std::atomic<int> g_live_ply{0};   // value the callback prints

// Debug
//#define _DEBUGGING_PUSH_POP

//#define _DEBUGGING_TO_FILE         // I must be defined to use either of the below
//#define _DEBUGGING_MOVE_TREE
//#define _DEBUGGING_MOVE_CHAIN
//#define _DEBUGGING_PV_ORDERING

// extern bool bMoreDebug;
// extern string debugMove;

#ifdef _DEBUGGING_TO_FILE
    FILE *fpDebug = NULL;
    char szValue[256];
    char szScore[256];
#endif

#define DADS_CRAZY_EVALUATION_CHANGES   // Not so crazy.

#define RANDOMIZING_EQUAL_MOVES         // Uncomment to move equal moves randomly

#define IS_CALLBACK_THREAD              // Uncomment to enable the callback to show "nPly", real time.

#ifdef IS_CALLBACK_THREAD
    #include <thread>
    #include <cstdio>

    static std::atomic<bool> g_cb_running{false};
    static std::thread g_cb_thread;

    // forward decl if g_live_ply is defined below, or put both in same region
    extern std::atomic<int> g_live_ply;  // remove 'extern' if in same file above

    static void start_callback_thread() {
        g_cb_running.store(true, std::memory_order_relaxed);
        // immediate ping
        //std::fprintf(stderr, "[PLY] start cur=%d\n", g_live_ply.load(std::memory_order_relaxed));
        g_cb_thread = std::thread([]{
            using namespace std::chrono_literals;
            while (g_cb_running.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                if (!g_cb_running.load(std::memory_order_relaxed)) break;
                std::fprintf(stderr, "..%d", g_live_ply.load(std::memory_order_relaxed));
                std::fflush(stderr);
            }
        });
    }


    static void stop_callback_thread() {
        g_cb_running.store(false, std::memory_order_relaxed);
        if (g_cb_thread.joinable()) g_cb_thread.join();
    }
#else
    static void start_callback_thread() {}
    static void stop_callback_thread() {}
#endif



//////////////////////////////////////////////////////////////////////////////////////

MinimaxAI::MinimaxAI(Engine& e) : engine(e) { 
     // Open a file for debug writing

    #ifdef _DEBUGGING_TO_FILE 
        fpDebug = fopen("C:\\programming\\shumi-chess\\debug.dat", "w");
        if(fpDebug == NULL)    // Check if file was opened successfully
        {
            printf("Error opening debug.dat file!");
            assert(0);
        }    
        //fprintf(fpDebug, "opening debug.dat file!");

        
        e.setDebugFilePointer(fpDebug);

    #endif
}



MinimaxAI::~MinimaxAI() { 
    #ifdef _DEBUGGING_TO_FILE
        if (fpDebug != NULL) fclose(fpDebug);
    #endif
}



template<class T>
string format_with_commas(T value) {
    stringstream ss;
    ss.imbue(locale(""));
    ss << fixed << value;
    return ss.str();
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns " relative (negamax) score". Relative score is positive for great positions for the specified player. 
// Absolute score is always positive for great positions for white.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// returns a positive score, if "for_color" is ahead.
//
double MinimaxAI::evaluate_board(Color for_color, const vector<ShumiChess::Move>& legal_moves)
{
    // move_history
    #ifdef _DEBUGGING_MOVE_CHAIN
        if (engine.move_history.size() > 7) {
        //if (look_for_king_moves()) {
        //if (has_repeated_move()) {
        //if (alternating_repeat_prefix_exact(2)) {
            print_move_history_to_file(fpDebug);    // debug only
        }
        
    #endif

    evals_visited++;

    int cp_score_adjusted = 0;

    int cp_score_pieces_only = 0;        // Pieces only
    //
    // Pieces only
    //
    for (const auto& color1 : array<Color, 2>{Color::WHITE, Color::BLACK}) {

        int cp_score_pieces_only_temp = 0;

        // Add up the scores for each piece
        for (Piece piece_type = Piece::PAWN;
            piece_type <= Piece::QUEEN;
            piece_type = static_cast<Piece>(static_cast<int>(piece_type) + 1))
        {    
            
            // Since the king can never leave the board (engine does not allow), 
            // and there is a huge score checked earlier we can just skip kings.
            // NOTE: ineffecient. Should put king last in piece enum. King IS the last in the enum. This line is useless!
            //if (piece_type == Piece::KING) continue;


            // Get bitboard of all pieces on board of this type and color
            ull pieces_bitboard = engine.game_board.get_pieces(color1, piece_type);

            // Adds for the piece value multiplied by how many of that piece there is (using centipawns)
            int cp_board_score = engine.centipawn_score_of(piece_type);
            int nPieces = engine.bits_in(pieces_bitboard);
            cp_score_pieces_only_temp += (nPieces * cp_board_score);

            assert (cp_score_pieces_only_temp>=0);

        }

        if (color1 != for_color) cp_score_pieces_only_temp *= -1;
        cp_score_pieces_only += cp_score_pieces_only_temp;
    }
    //
    // positional considerations only
    //
    int cp_score_position = 0;

    #ifdef DADS_CRAZY_EVALUATION_CHANGES
   
        for (const auto& color : array<Color, 2>{Color::WHITE, Color::BLACK}) {

            int cp_score_position_temp = 0;        // positional considerations only

            // Add code to make king shy from center.
            double anti_centerness;     // larger when closer to edges, small when king in center
            bool isOK = engine.game_board.king_anti_centerness(color, anti_centerness);
            assert (isOK);     // isOK just means that there wasnt a king
            cp_score_position_temp += static_cast<int>(std::lround(anti_centerness*80));   // centipawns

            // Add code to encourage rook connections on back rank.
            double connectiveness;
            isOK = engine.game_board.rook_connectiveness(color, connectiveness);
            //assert (isOK);    // isOK just means that there werent two rooks to connect
            //cp_score_position_temp += std::lround(connectiveness*40);   // centipawns
            cp_score_position_temp += static_cast<int>(std::lround(connectiveness*50));

            // Add code to discourage isolated doubled or tripled pawns.
            int isolanis =  engine.game_board.count_isolated_doubled_pawns(color);
            cp_score_position_temp -= isolanis*20;   // centipawns


            // Add code to encourage minor piece mobility
            //int imobility = engine.get_minor_piece_move_number (legal_moves);


            // Add code to encourage knights in the center (average)
            double centerness;
            isOK = engine.game_board.knights_centerness(color, centerness);
            //assert (isOK); bOK of false just means no knights on board.
            cp_score_position += centerness*4;  // centipawns

            // // // Add code to encourage bishops in the center (average)
            // isOK = engine.game_board.bishops_centerness(color, centerness);
            // //assert (isOK); bOK of false just means no bishops on board.
            // cp_score_position += centerness*5;  // centipawns
            
            // Add positional stuff to score (score is positive at this point for for_color)
            if (color != for_color) cp_score_position_temp *= -1;
            cp_score_position += cp_score_position_temp;
        }

        // Add code to promote/discourage trading, depending on who is ahead.
        cp_score_position += cp_score_pieces_only*0.1;

    #endif

    // Both position and pieces are now signed properly to be positive for the "for_color" side.
    cp_score_adjusted = cp_score_pieces_only + cp_score_position;


    #ifdef _DEBUGGING_MOVE_TREE
        fprintf(fpDebug, " ev=%i", cp_score_adjusted);
    #endif

     // Convert sum (for both colors) from centipawns to double.
    return (cp_score_adjusted * 0.01);
}

//////////////////////////////////////////////////////////////////////////////////


int g_iMove = 0;
int g_this_depth = 6;

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

	#ifdef IS_CALLBACK_THREAD
    	start_callback_thread();
    #endif

    engine.move_history = stack<Move>();

    
    // Results of previous iterave searches
    //unordered_map<std::string, unordered_map<Move, double, MoveHash>> move_scores_table;

    // size_t nFENS = move_scores_table.size();    // This returns the number of FEN rows.
    // assert(nFENS == 0);
 
    //MoveScoreList move_and_scores_list;


    
    int this_depth;

    Move best_move = {};
    double d_best_move_value;

    g_iMove++;
    cout << "\nMove: " << g_iMove << endl;

    //Move null_move = Move{};
    Move null_move = engine.users_last_move;

    this_depth = 6;        // Note: because i said so.
    int maximum_depth = this_depth;

    // code to make it speed up (lower) depth as the game goes on. (never lower than 4 though)
    // maximum_depth = this_depth - (int)(g_iMove/5);
    // if (maximum_depth < 5) maximum_depth = 5;
    assert(maximum_depth>=1);

    // NOTE: there should be an option: depth .vs. time.
    int depth = 1;
    int nPly = 0;


    //print_move_history_to_file(fpDebug);    // debug only

    do {

        #ifdef _DEBUGGING_TO_FILE 
            //clear_stats_file(fpDebug, "C:\\programming\\shumi-chess\\debug.dat");
        #endif

        #ifdef _DEBUGGING_MOVE_TREE
            fputs("\n\n---------------------------------------------------------------------------", fpDebug);
            print_move_to_file(null_move, nPly, state, false, true, false, fpDebug);
        #endif

        cout << endl << "Deepening to " << depth << " half moves " << "out of " << maximum_depth;

        //move_scores_table.clear();
        //move_and_scores_list.clear();

        top_depth = depth;
        double alpha = -HUGE_SCORE;
        double beta = HUGE_SCORE;
        auto ret_val = store_board_values_negamax(depth, alpha, beta
                                                //, move_scores_table
                                                //, move_and_scores_list
                                                , null_move
                                                , (nPly+1)
                                               );

        // ret_val is a tuple of the score and the move.
        d_best_move_value = get<0>(ret_val);
        best_move = get<1>(ret_val);

        

        // publish PV for the *next* iteration:   PV PUSH
        prev_root_best_ = best_move;


        MoveScore mTemp;   
        mTemp.first  = best_move;
        mTemp.second = d_best_move_value;
                                
        engine.move_and_score_to_string(mTemp, true);

        cout << " Best is:" << engine.move_string;

        depth++;

    //} while (chrono::high_resolution_clock::now() <= required_end_time);
    } while (depth < (maximum_depth+1));

    cout << "\nWent to depth " << (depth - 1) << endl;

    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";

    // Convert to absolute score
    double d_best_move_value_abs = d_best_move_value;
    if (engine.game_board.turn == Color::BLACK) d_best_move_value_abs = -d_best_move_value_abs;
    
    if (std::fabs(d_best_move_value_abs) < VERY_SMALL_SCORE) d_best_move_value_abs = 0.0;        // avoid negative zero
    string abs_score_string = to_string(d_best_move_value_abs);


    engine.bitboards_to_algebraic(engine.game_board.turn, best_move
                , (GameState::INPROGRESS)
                //, NULL
                , false
                , false
                , engine.move_string);    // Output

    cout << colorize(AColor::BRIGHT_CYAN,engine.move_string) << "   ";


    //print_move_history_to_file(fpDebug);


    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", d_best_move_value_abs);
    abs_score_string = buf;

    cout << colorize(AColor::BRIGHT_CYAN, abs_score_string + " =score,  ");
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total" + " ---- " + format_with_commas(evals_visited) + " Evals") << endl;
    chrono::duration<double> total_time = chrono::high_resolution_clock::now() - start_time;
    cout << colorize(AColor::BRIGHT_GREEN, (static_cast<std::ostringstream&&>(std::ostringstream() << "Total time: " << std::fixed << std::setprecision(2) << total_time.count() << " sec")).str());

    assert (total_time.count() > 0);
    //cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total" + " ---- " + format_with_commas(evals_visited) + " Evals") << endl;
    double nodes_per_sec = nodes_visited / total_time.count();
    double evals_per_sec = evals_visited / total_time.count();
    cout << colorize(AColor::BRIGHT_GREEN, "   nodes/sec= " + format_with_commas(std::llround(nodes_per_sec)) + "   evals/sec= " + format_with_commas(std::llround(evals_per_sec))) << endl;

  
    // Debug only
    // int isolanis;
    bool isOK;
    double centerness;

    // isolanis =  engine.game_board.count_isolated_doubled_pawns(Color::WHITE);
    isOK = engine.game_board.knights_centerness(Color::WHITE, centerness);  // Gets smaller closer to center.
    cout << "wht " << centerness << endl;

    isOK = engine.game_board.knights_centerness(Color::BLACK, centerness);  // Gets smaller closer to center. 
    cout << "blk " << centerness << endl;   
    // isolanis =  engine.game_board.count_isolated_doubled_pawns(Color::BLACK);
    // // isOK = engine.game_board.king_anti_centerness(Color::WHITE, centerness);  // Gets smaller closer to center.
    // // assert (isOK);
    // cout << "blk " << isolanis << endl;

    // double connectiveness;
    // bool bStatus;
    // bStatus  = engine.game_board.rook_connectiveness(Color::WHITE, connectiveness);
    
    
    
    //int imobility = engine.get_minor_piece_move_number (legal_moves);
    // cout << "wht " << connectiveness << endl;
    // bStatus = engine.game_board.rook_connectiveness(Color::BLACK, connectiveness);
    // cout << "blk " << connectiveness << endl;


	#ifdef IS_CALLBACK_THREAD
    	stop_callback_thread();
    #endif

    return best_move;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Choose the "minimax" AI move.
// Returns a tuple of:
//    the score value
//
////////////////////////////////////////////////////////////////////////////////////////////////////

tuple<double, Move> MinimaxAI::store_board_values_negamax(
                    int depth, double alpha, double beta
                    //,unordered_map<std::string, unordered_map<Move, double, MoveHash>> &move_scores_table
                    //,MoveScoreList& move_and_scores_list
                    ,const ShumiChess::Move& move_last      // seems to be used for debug only...
                    ,int nPly)
{

    // Initialize return 
    double d_best_score = 0.0;
    Move the_best_move = {};
    //std::tuple<double, ShumiChess::Move> final_result;


    vector<Move> *p_moves_to_loop_over = 0;
    vector<Move> unquiet_moves;

    nodes_visited++;


    //bMoreDebug = true;
    //debugMove = engine.move_string;

    bool in_check = engine.is_king_in_check(engine.game_board.turn);


    // communications from 
    if (engine.bHurryUpGrampa) {
        g_this_depth--;
        if (g_this_depth <= 3) g_this_depth = 6;
        cout << "\n ...take it easy ..." << g_this_depth << endl;
        engine.bHurryUpGrampa = false;

    }


    //bMoreDebug = false;

    // =====================================================================
    // asserts
    // =====================================================================

    //if (alpha > beta) assert(0);


    // =====================================================================

    #ifdef _DEBUGGING_MOVE_CHAIN1
        engine.bitboards_to_algebraic(engine.game_board.turn, move_last
                                , (GameState::INPROGRESS)
                                , false
                                , false
                                , engine.move_string);

    #endif



    //  If mover is in check, this routine returns all check escapes and only the check escapes.
    std::vector<Move> legal_moves = engine.get_legal_moves();

    GameState state = engine.game_over(legal_moves);

    #ifdef _DEBUGGING_MOVE_CHAIN1
        print_move_to_file(move_last, nPly, state, in_check, false, false, fpDebug);
    #endif


    p_moves_to_loop_over = &legal_moves;
    assert(p_moves_to_loop_over);

        // Over analysis sentinal Sorry, I should not be this large <- NOTE:
    constexpr int MAX_PLY = 32;
    if (nPly > MAX_PLY) {

        assert(0);

        std::cout << "\n\x1b[31m! MAX PLY  trap#1 " << nPly << "\x1b[0m\n";
        #ifdef _DEBUGGING_TO_FILE 

            fprintf(fpDebug, "crap!");
            std::fflush(fpDebug);
        #endif

        assert(p_moves_to_loop_over);
        auto tup = best_move_static(engine.game_board.turn, *p_moves_to_loop_over, in_check);
        double scoreMe = std::get<0>(tup);
        ShumiChess::Move moveMe = std::get<1>(tup);
        return { scoreMe, moveMe };

    }


    //assert(nPly < 22); // runaway recursion stop



    // ✅ INSERT PV ordering (at the root) here — only if this is the root node
    if (depth == top_depth) {

        assert(p_moves_to_loop_over);
        #ifdef _DEBUGGING_PV_ORDERING
            std::vector<Move> moves_to_loop_overtemp = *p_moves_to_loop_over;  // snapshot current underlying list
        #endif

        // Start with the last deepenings best move first
         for (size_t i = 0; i < p_moves_to_loop_over->size(); ++i) {
            if ( (*p_moves_to_loop_over)[i] == prev_root_best_ ) {
                if (i != 0) {
                    std::swap( (*p_moves_to_loop_over)[0], (*p_moves_to_loop_over)[i] );
                }
                break;
            }
        }

        #ifdef _DEBUGGING_PV_ORDERING
            //if (moves_to_loop_overtemp != moves_to_loop_over) {
            if (moves_to_loop_overtemp != *p_moves_to_loop_over) {

                fputs("\n\n-1+++++++++++++\n", fpDebug);
                //engine.moves_and_scores_to_file(move_and_scores_listSave, false, fpDebug);
                engine.print_moves_to_file(moves_to_loop_overtemp, 0, fpDebug);
                fputs("-1---------------\n", fpDebug);


                fputs("-2+++++++++++++\n", fpDebug);
                //engine.moves_and_scores_to_file(move_and_scores_list, false, fpDebug);
                engine.print_moves_to_file(moves_to_loop_over, 0, fpDebug);
                fputs("-2---------------\n", fpDebug);
            }
        #endif

    }

    


    // =====================================================================
    // Terminal positions (game over)
    // =====================================================================
    if (state != GameState::INPROGRESS) {

        int level = (top_depth - depth);
        assert(level >= 0);
        double d_level = static_cast<double>(level);

        if (state == GameState::WHITEWIN) {
            d_best_score = (engine.game_board.turn == ShumiChess::WHITE)
                            ? (+HUGE_SCORE - d_level)
                            : (-HUGE_SCORE + d_level);
        } else if (state == GameState::BLACKWIN) {
            d_best_score = (engine.game_board.turn == ShumiChess::BLACK)
                            ? (+HUGE_SCORE - d_level)
                            : (-HUGE_SCORE + d_level);
        } else if (state == GameState::DRAW) {
            d_best_score = 0.0;          // Stalemate
        } else {
            assert(0);
        }

        // final_result = std::make_tuple(d_best_score, the_best_move);
        // return final_result;
        return {d_best_score, the_best_move};

    }

    // =====================================================================
    // Hard node-limit sentinel fuse colorize(AColor::BRIGHT_CYAN,
    // =====================================================================
    if (nodes_visited > 5.0e8) {    // 10,000,000 a good number here
        std::cout << "\x1b[31m! NODES VISITED trap#2 " << nodes_visited << "dep=" << depth << "  " << d_best_score << "\x1b[0m\n";

        // Static board evaluation
        d_best_score = evaluate_board(engine.game_board.turn, legal_moves);

        //final_result = std::make_tuple(d_best_score, the_best_move);
        //return final_result;
        return {d_best_score, the_best_move};
    }


    // =====================================================================
    // Quiescence entry when depth <= 0
    // =====================================================================
    assert (depth >= 0);
    if (depth == 0) {

        // Static board evaluation
        d_best_score = evaluate_board(engine.game_board.turn, legal_moves);

        unquiet_moves = engine.reduce_to_unquiet_moves_MVV_LVA(legal_moves);

        // If quiet (not in check & no tactics), just return stand-pat
        if (!in_check && unquiet_moves.empty()) {
            return { d_best_score, Move{} };
        }

        if (!in_check) {

            // Stand-pat cutoff
            if (d_best_score >= beta) {
                return { d_best_score, Move{} };
            }
            alpha = std::max(alpha, d_best_score);

            // Extend on captures/promotions only
            p_moves_to_loop_over = &unquiet_moves; 

            #ifdef _DEBUGGING_MOVE_TREE
                int ierr = sprintf( szValue, "\nonquiet=%zu ", unquiet_moves.size());
                assert (ierr!=EOF);

                print_moves_to_print_tree(unquiet_moves, depth, szValue, "\n");
            #endif

        } else {
            // In check: use all legal moves, since by definition (see get_legal_moves() the set of all legal moves is equivnelent 
            // to the set of all check escapes. By definition.
            //moves_to_loop_over = legal_moves;  // not needed as its done ealier above. Sorry.
        }

		// Pick best static evaluation among all legal moves if hit the over ply
        constexpr int MAX_QPLY = 24;        
        if (nPly >= MAX_QPLY) {
            //std::cout << "\x1b[31m! MAX_QPLY trap " << nPly << "\x1b[0m\n";
            //std::cout << "\x1b[31m!" << "\x1b[0m";
            auto tup = best_move_static(engine.game_board.turn, (*p_moves_to_loop_over), in_check);
            double scoreMe = std::get<0>(tup);
            ShumiChess::Move moveMe = std::get<1>(tup);
            return { scoreMe, moveMe };
        }

    } else {
        // depth > 0: already have moves_to_loop_over = legal_moves
        // nothing to change here

    }


    // =====================================================================
    // Recurse over selected move set "moves_to_loop_over"
    // =====================================================================
    
    if (!p_moves_to_loop_over->empty()) {

        const std::vector<Move>& sorted_moves = *p_moves_to_loop_over;

        bool b_use_this_move;

        // MoveScoreList move_and_scores_listSave = move_and_scores_list;

        // // (optional) move ordering
        //sort_moves_by_score(move_and_scores_list, true);

        // if (move_and_scores_list != move_and_scores_listSave) {

        //     #ifdef _DEBUGGING_PV_ORDERING
        //         fputs("\n\n-1+++++++++++++", fpDebug);
        //         engine.moves_and_scores_to_file(move_and_scores_listSave, false, fpDebug);
        //         fputs("\n-1---------------", fpDebug);
        //     #endif
        //     #ifdef _DEBUGGING_PV_ORDERING
        //         fputs("\n\n-2+++++++++++++", fpDebug);
        //         engine.moves_and_scores_to_file(move_and_scores_list, false, fpDebug);
        //         fputs("\n-2---------------", fpDebug);
        //     #endif
        // }

        d_best_score = -HUGE_SCORE;
        the_best_move = sorted_moves[0];

         for (const Move& m : sorted_moves) {

            #ifdef _DEBUGGING_PUSH_POP
                std::string temp_fen_before = engine.game_board.to_fen();
            #endif

            engine.pushMove(m);

            //GameState state_repeat =  draw_by_twofold();
            //GameState state_repeat =  draw_by_repetition();
            // if (state_repeat == GameState::DRAW) {
            //     cout << "!!!!!!!!!draw!!!!!!\n" << endl;
            // }


            #ifdef _DEBUGGING_MOVE_TREE
                print_move_to_file(m, nPly, state, false, false, fpDebug);
            #endif
               
            #ifdef IS_CALLBACK_THREAD
            	g_live_ply = nPly;
            #endif

            // Two parts 1. in negamax, (relative scores) the alpha betas are reversed in sign
            //           2. The beta and alpha arguments are staggered, or reversed. 
            auto ret_val = store_board_values_negamax(
                (depth > 0 ? depth - 1 : 0),                // Refuse to allow negative depth
                -beta, -alpha,      // reverse in sign and order at the same time
                //move_and_scores_list,
                //move_scores_table,
                m, (nPly+1)
            );

            // The third part of negamax: negate the score to keep it relative.
            double d_score_value = -std::get<0>(ret_val);

            engine.popMove();

            #ifdef _DEBUGGING_PUSH_POP
                std::string temp_fen_after = engine.game_board.to_fen();
                if (temp_fen_before != temp_fen_after) {
                    std::cout << "PROBLEM WITH PUSH POP!!!!!" << std::endl;
                    cout_move_info(m);
                    std::cout << "FEN before  push/pop: " << temp_fen_before  << std::endl;
                    std::cout << "FEN after   push/pop: " << temp_fen_after   << std::endl;
                    assert(0);
                }
            #endif




            // Store score
            //std::string temp_fen_now = engine.game_board.to_fen();
            //move_scores_table[temp_fen_now][m] = d_score_value;

            // record (move, score)
            //move_and_scores_list.emplace_back(m, d_score_value);

             // Note: this should be done as a real random choice. (random over the moves possible). 
            // This dumb approach favors moves near the end of the list
            #ifdef RANDOMIZING_EQUAL_MOVES
                // Hey, randomize the choice (sort of).
                if (d_score_value == d_best_score) {
                //if ( (d_score_value - d_best_score) < VERY_SMALL_SCORE) {
                    b_use_this_move = engine.flip_a_coin();
                } else {
                    b_use_this_move = (d_score_value > d_best_score);
            }
            #else
                b_use_this_move = (d_score_value > d_best_score);
            #endif


            if (b_use_this_move) {
                d_best_score = d_score_value;
                the_best_move = m;
            }

            // Think of alpha as “best score found so far at this node.”
            alpha = std::max(alpha, d_best_score);

            // Alpha/beta "cutoff", (fail high), break the analysis
            if (alpha >= beta + VERY_SMALL_SCORE) {

                #ifdef _DEBUGGING_MOVE_CHAIN3
                    char szTemp[64];
                    sprintf(szTemp, " Beta cutoff %f %f",  alpha, beta);
                    fputs(szTemp, fpDebug);
                #endif
                if (!engine.is_unquiet_move(m)){
                    #ifdef _DEBUGGING_MOVE_CHAIN1
                        char szTemp[64];
                        sprintf(szTemp, " Beta quiet cutoff %f %f",  alpha, beta);
                        fputs(szTemp, fpDebug);
                        print_move_to_file(m, nPly, state, false, false,false, fpDebug);
                    #endif
                }

                break;
            }

        }   // End loop over all moves to look at
    }

    //final_result = std::make_tuple(d_best_score, the_best_move);
    //return final_result;
    return {d_best_score, the_best_move};
}

ShumiChess::GameState MinimaxAI::draw_by_repetition() const
{
    const std::stack<ShumiChess::Move>& hist = engine.move_history;
    if (hist.size() < 6) return GameState::INPROGRESS;

    std::stack<ShumiChess::Move> tmp = hist;
    const ShumiChess::Move m6 = tmp.top(); tmp.pop();
    const ShumiChess::Move m5 = tmp.top(); tmp.pop();
    const ShumiChess::Move m4 = tmp.top(); tmp.pop();
    const ShumiChess::Move m3 = tmp.top(); tmp.pop();
    const ShumiChess::Move m2 = tmp.top(); tmp.pop();
    const ShumiChess::Move m1 = tmp.top();

    auto same_move = [](const ShumiChess::Move& a, const ShumiChess::Move& b) {
        return (a.from == b.from) && (a.to == b.to);
    };

    if (same_move(m1, m3) && same_move(m1, m5) &&
        same_move(m2, m4) && same_move(m2, m6))
        return GameState::DRAW;

    return GameState::INPROGRESS;
}

ShumiChess::GameState MinimaxAI::draw_by_twofold() const
{
    const std::stack<ShumiChess::Move>& hist = engine.move_history;
    if (hist.size() < 4) return GameState::INPROGRESS;

    std::stack<ShumiChess::Move> tmp = hist;
    const ShumiChess::Move m4 = tmp.top(); tmp.pop(); // newest
    const ShumiChess::Move m3 = tmp.top(); tmp.pop();
    const ShumiChess::Move m2 = tmp.top(); tmp.pop();
    const ShumiChess::Move m1 = tmp.top();            // oldest of these four

    auto same_move = [](const ShumiChess::Move& a, const ShumiChess::Move& b) {
        return (a.from == b.from) && (a.to == b.to);
    };

    // Detect A,B,A,B over the last four half-moves
    if (same_move(m1, m3) && same_move(m2, m4))
        return GameState::DRAW;

    return GameState::INPROGRESS;
}

bool MinimaxAI::look_for_king_moves() const
{
    int king_moves = 0;
    std::stack<ShumiChess::Move> tmp = engine.move_history; // copy; don't mutate engine

    while (!tmp.empty()) {
        ShumiChess::Move m = tmp.top(); tmp.pop();
        if (m.piece_type == ShumiChess::Piece::KING) {
            if (++king_moves >= 2) return true;
        }
    }
    return false;
}
bool MinimaxAI::has_repeated_move() const
{
    // Flatten stack → vector (order doesn’t matter for “any duplicate”)
    std::stack<ShumiChess::Move> tmp = engine.move_history;
    std::vector<ShumiChess::Move> seq;
    seq.reserve(tmp.size());
    while (!tmp.empty()) { seq.push_back(tmp.top()); tmp.pop(); }

    // O(n^2) duplicate check on (from,to,promotion); ignore null moves
    for (size_t i = 0; i < seq.size(); ++i) {
        const auto& a = seq[i];
        if (a.from == 0ULL && a.to == 0ULL) continue;
        for (size_t j = i + 1; j < seq.size(); ++j) {
            const auto& b = seq[j];
            
            if ( (a.from == b.from) && (a.to == b.to) && (a.piece_type == b.piece_type) && (a.promotion == b.promotion) )
                return true;
        }
    }
    return false;
}
bool MinimaxAI::alternating_repeat_prefix_exact(int pairs) const
{
    if (pairs < 2 || pairs > 3) return false;

    // stack -> vector oldest→newest
    std::stack<ShumiChess::Move> tmp = engine.move_history;
    std::vector<ShumiChess::Move> seq; seq.reserve(tmp.size());
    while (!tmp.empty()) { seq.push_back(tmp.top()); tmp.pop(); }
    std::reverse(seq.begin(), seq.end());

    const size_t need = static_cast<size_t>(2 * pairs);
    if (seq.size() < need) return false;

    const auto& A = seq[0];
    const auto& B = seq[1];

    auto eq_exact = [](const ShumiChess::Move& a, const ShumiChess::Move& b){
        return (a.from == b.from) &&
               (a.to == b.to) &&
               (a.piece_type == b.piece_type) &&
               (a.promotion == b.promotion);
    };

    // verify prefix A,B,A,B,(A,B) over the first `need` moves
    for (size_t i = 2; i < need; ++i) {
        if ((i % 2 == 0)) { // even index → must match A
            if (!eq_exact(seq[i], A)) return false;
        } else {            // odd index  → must match B
            if (!eq_exact(seq[i], B)) return false;
        }
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


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
        return 0.0;                       // stalemate
    }
    
    // Depth decreases, when it hits zero we stop looking.
    if (depth == 0) {
        Color color_perspective = Color::BLACK;
        if (color_multiplier) {
            color_perspective = Color::WHITE;
        }
        Move mvdefault = Move{};
        return evaluate_board(color_perspective, moves) * color_multiplier;

    }
    //
    // Otherwise dive down one more ply.
    //
    if (color_multiplier == 1) {  // Maximizing player
        double dMax_move_value = -DBL_MAX;
        for (Move& m : moves) {

            engine.pushMove(m);

            double score_value = -1 * get_value(depth - 1, color_multiplier * -1, alpha, beta);

            engine.popMove();

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

            engine.pushMove(m);

            double score_value = -1 * get_value(depth - 1, color_multiplier * -1, alpha, beta);

            engine.popMove();

            // Set beta here (only setting)
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
        engine.pushMove(m);
        double score_value = get_value(depth - 1, color_multiplier * -1, -DBL_MAX, DBL_MAX);
        if (score_value * -1 > dMax_move_value) {
            dMax_move_value = score_value * -1;
            move_chosen = m;
        }
        engine.popMove();
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




void MinimaxAI::sort_moves_by_score(
    MoveScoreList& moves_and_scores_list,
    bool sort_descending
)
{
    if (sort_descending) {
        std::sort(moves_and_scores_list.begin(), moves_and_scores_list.end(),
                  [](const MoveScore& a, const MoveScore& b) {
                      return a.second > b.second;
                  });
    } else {
        std::sort(moves_and_scores_list.begin(), moves_and_scores_list.end(),
                  [](const MoveScore& a, const MoveScore& b) {
                      return a.second < b.second;
                  });
    }
}



// void MinimaxAI::sort_moves_by_score(
//     MoveScoreList& moves_and_scores_list,  // note: pass by reference so we sort in place
//     bool sort_descending                   // true = highest score first
// )
// {
//     const size_t n = moves_and_scores_list.size();
//     for (size_t i = 1; i < n; ++i) {
//         MoveScore key = moves_and_scores_list[i];
//         double key_score = key.second;
//         size_t j = i;

//         if (sort_descending) {
//             while (j > 0 && moves_and_scores_list[j - 1].second < key_score) {
//                 moves_and_scores_list[j] = moves_and_scores_list[j - 1];
//                 --j;
//             }
//         } else {
//             while (j > 0 && moves_and_scores_list[j - 1].second > key_score) {
//                 moves_and_scores_list[j] = moves_and_scores_list[j - 1];
//                 --j;
//             }
//         }
//         moves_and_scores_list[j] = key;
//     }
// }




// Loop over all passed moves, find the best move by static evaluation.


std::tuple<double, ShumiChess::Move>
MinimaxAI::best_move_static(ShumiChess::Color color,
                            const std::vector<ShumiChess::Move>& legal_moves,
                            bool in_Check)
{
    // If there are no moves:
    // - not in check: return a single static (stand-pat) eval
    // - in check: treat as losing (no legal escapes here)
    if (legal_moves.empty()) {
        if (!in_Check) {
            double stand_pat = evaluate_board(color, legal_moves);         // positive is good for 'color'
            return { stand_pat, ShumiChess::Move{} };
        }
        return { -HUGE_SCORE, ShumiChess::Move{} };
    }

    double d_best = -HUGE_SCORE;
    ShumiChess::Move bestMove = ShumiChess::Move{};

    for (const auto& m : legal_moves) {

        engine.pushMove(m);
        
        double d_score = evaluate_board(color, legal_moves);  // positive is good for 'color'
        
        engine.popMove();

        if (d_score > d_best) {
            d_best = d_score;
            bestMove = m;
        }
    }

    return { d_best, bestMove };
}






/////////////////////////////////////////////////////////////////////

#ifdef _DEBUGGING_TO_FILE

void MinimaxAI::clear_stats_file(FILE*& fpDebug, const char* path) {
    if (fpDebug) { fclose(fpDebug); fpDebug = nullptr; }
    fpDebug = std::fopen(path, "w");        // truncates the file
    assert(fpDebug != nullptr);
    std::fflush(fpDebug);
}

// Prints the move history from oldest → most recent using print_move_to_file(...)
// Uses: nPly = -2, isInCheck = false, bFormated = false
void MinimaxAI::print_move_history_to_file(FILE* fp) {
    bool bFlipColor = false;
    int ierr = fputs("\nhistory:\n", fpDebug);
    assert (ierr!=EOF);

    // copy stack so we don't mutate Engine's history
    std::stack<ShumiChess::Move> tmp = engine.move_history;

    // collect in a vector (top = newest), then reverse to oldest → newest
    std::vector<ShumiChess::Move> seq;
    seq.reserve(tmp.size());
    while (!tmp.empty()) {
        seq.push_back(tmp.top());
        tmp.pop();
    }
    std::reverse(seq.begin(), seq.end());

    // print each move
    for (const ShumiChess::Move& m : seq) {
        bFlipColor = !bFlipColor;
        print_move_to_file(m, -2, (GameState::INPROGRESS), false, false, bFlipColor, fp);
    }
}


// Tabs over based on ply. Pass in nPly=-2 for no tabs
void MinimaxAI::print_move_to_file(ShumiChess::Move m, int nPly, GameState gs
                                    , bool isInCheck, bool bFormated, bool bFlipColor
                                    , FILE* fp
                                ) {

    // Get algebriac (SAN) text form of the last move.
    Color aColor = engine.game_board.turn;
    if (bFlipColor) aColor = utility::representation::opposite_color(aColor);

    engine.bitboards_to_algebraic(aColor, m
                                , gs
                                , isInCheck
                                , false
                                , engine.move_string);

    if (bFormated) { 
        print_move_to_file_from_string(engine.move_string.c_str(), aColor, nPly
                                        , '\n', ',', false
                                        , fp);
    } else {
        print_move_to_file_from_string(engine.move_string.c_str(), aColor, nPly
                                        , ' ', ' ', false
                                        , fp); 
    }

}

// Tabs over based on ply. Pass in nPly=-2 for no tabs
void MinimaxAI::print_move_to_file_from_string(const char* p_move_text, Color turn, int nPly
                                            , char preCharacter
                                            , char postCharacter
                                            , bool b_right_Pad
                                            , FILE* fp)
{

    int ierr = fputc(preCharacter, fp);
    assert (ierr!=EOF);

    // Indent the whole thing over based on depth level
    int nTabs = nPly+2;
    
    if (nTabs<0) nTabs=0;

    int nSpaces = nTabs*4;
    int nChars = fprintf(fp, "%*s", nSpaces, "");

    // compose "..."+move (for Black) or just move (for White)
    if (turn == opposite_color(ShumiChess::BLACK)) snprintf(szValue, sizeof(szValue), "...%s", p_move_text);
    else                                           snprintf(szValue, sizeof(szValue), "%s",    p_move_text);

    // print as a single left-justified 8-char field: "...e4   " or "e4     "
    //                                                 12345678
    
    if (b_right_Pad) fprintf(fp, "%-10.8s", szValue);  // option 1
    else             fprintf(fp, "%.8s", szValue);     // option 2

    ierr = fputc(postCharacter, fp);
    assert (ierr!=EOF);

    int ferr = fflush(fp);
    assert(ferr == 0);
}


#endif


#ifdef _DEBUGGING_MOVE_TREE


void MinimaxAI::print_moves_to_print_tree(std::vector<Move> mvs, int depth, char* szHeader, char* szTrailer)
{

    if (szHeader != NULL) {int ierr = fprintf(fpDebug, szHeader);}
    
     for (Move &m : mvs) {
        print_move_to_file(m, depth, state, false, false, fpDebug);
     }

    if (szTrailer != NULL) {int ierr = fprintf(fpDebug, szTrailer);}


}



void MinimaxAI::print_move_scores_to_file(
    FILE* fpDebug,
    const std::unordered_map<std::string,std::unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>>& move_scores_table
)
{

    // sprintf(szValue, "\n---------------------------------------------------------------------------");
    // fprintf(fpDebug, "%s", szValue);
    // size_t iFENS = move_scores_table.size();    // This returns the number of FEN rows.
    // sprintf(szValue, "\n\n  nFENS = %i", (int)iFENS);
    // fprintf(fpDebug, "%s", szValue);

    fputs("\n\n---------------------------------------------------------------------------", fpDebug);
    fprintf(fpDebug, "\n  nFENS = %zu\n", move_scores_table.size());  // %zu for size_t

    // cout << endl;
    // print_gameboard(engine.game_board);

    for (const auto& fen_row : move_scores_table) {
        const auto& moves_map = fen_row.second;

        // Optional: print the FEN once per block
        fprintf(fpDebug, "\nFEN: %s\n", fen_row.first.c_str());

        for (const auto& kv : moves_map) {
            const Move& m = kv.first;
            double score  = kv.second;

            std::string san_string;
            engine.bitboards_to_algebraic(
                                        ShumiChess::BLACK
                                        //utility::representation::opposite_color(engine.game_board.turn)
                                        ,m
                                        ,ShumiChess::GameState::INPROGRESS
                                        , false
                                        ,san_string);     // output

            fprintf(fpDebug, "%s  %.6f\n", san_string.c_str(), score);
        }
    }
    fputs("\n\n---------------------------------------------------------------------------", fpDebug);


}


#endif





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

//s#define _DEBUGGING_TO_FILE         // I must be defined to use either of the below
//#define _DEBUGGING_MOVE_TREE
//#define _DEBUGGING_MOVE_CHAIN
//#define _DEBUGGING_PV_ORDERING

// extern bool bMoreDebug;
// extern string debugMove;

//#ifdef _DEBUGGING_TO_FILE
    FILE *fpDebug = NULL;
    char szValue1[256];
//#endif

#define IS_CALLBACK_THREAD              // Uncomment to enable the callback to show "nPly", real time.

///////////////////////////////////////////////////////////////////////////

// Speedups?
#define FAST_EVALUATIONS
#define DELTA_PRUNING
#define DOING_TRANSPOSITION_TABLE
#define DOING_TRANSPOSITION_TABLE2
#define UNQUIET_SORT

// Only randomizes a small amount a list formed on the root node, when at maxiumu deepeing, AND 
// on first move. This simple diversity opens up hundreds of new lines for study and play.
#define RANDOMIZING_EQUAL_MOVES         // THIS IS REQUIRED TO BE DEFINED 



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


    engine.repetition_table.clear();
    //repetition_table.reserve(128);


    transposition_table.clear();
    transposition_table.reserve(10000000);
    
    // add the current position
    uint64_t key_now = engine.game_board.zobrist_key;
    engine.repetition_table[key_now] = 1;

    // Open a file for debug writing
    #ifdef _DEBUGGING_TO_FILE
        #ifdef __linux__
            fpDebug = fopen("/tmp/shumi-chess-debug.dat", "w");
            if (fpDebug) {
                int ierr = fputs("Opened /tmp/shumi-chess-debug.dat for debug output\n", fpDebug);
                assert (ierr!=EOF);
            }
        #else
            fpDebug = fopen("C:\\programming\\shumi-chess\\debug.dat", "w");
        #endif
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


void MinimaxAI::wakeup() {
    stop_calculation = true;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns " relative (negamax) score". Relative score is positive for great positions for the specified player. 
// Absolute score is always positive for great positions for white.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Follows the negamax convention, so a positive value at a leaf is “good for the side to move.” 
// returns a positive score, if "for_color" is ahead.
//
int MinimaxAI::evaluate_board(Color for_color, int nPlys, bool is_fast_style) //, const vector<ShumiChess::Move>& legal_moves)
{
    // move_history
    #ifdef _DEBUGGING_MOVE_CHAIN1
        //if (engine.move_history.size() > 7) {
        //if (look_for_king_moves()) {
        //if (has_repeated_move()) {
        if (alternating_repeat_prefix_exact(1)) {
            engine.print_move_history_to_file(fpDebug);    // debug only
        }
    #endif


    
    evals_visited++;

    int cp_score_adjusted = 0;  // Total score, centipawns.

    //
    // Material considerations only
    //

    int mat_cp_white = 0;
    int mat_cp_black = 0;


    int tempsum = 0.0;
    int cp_score_pieces_only = 0;        // Pieces only
    for (const auto& color1 : array<Color, 2>{Color::WHITE, Color::BLACK}) {

        // Get the centipawn value for this color
        int cp_score_pieces_only_temp = engine.game_board.get_material_for_color(color1);
        assert (cp_score_pieces_only_temp>=0);    // no negative value pieces

        if (for_color == ShumiChess::WHITE) mat_cp_white = cp_score_pieces_only_temp;
        else                                mat_cp_black = cp_score_pieces_only_temp;

        tempsum += cp_score_pieces_only_temp;

        // Take color into acccount
        if (color1 != for_color) cp_score_pieces_only_temp *= -1;
        cp_score_pieces_only += cp_score_pieces_only_temp;

    }

    assert (tempsum>=0);
    cp_score_pieces_only_avg = tempsum / 2.0;

    //
    // Positional considerations only
    //
    int cp_score_position = 0;
    bool isOK;

    
    if ( (!is_fast_style) ) {

        int mat_avg = cp_score_pieces_only_avg;
        for (const auto& color : array<Color, 2>{Color::WHITE, Color::BLACK}) {
            int cp_score_position_temp = 0;        // positional considerations only

            /////////////// start positional evals /////////////////

            // Note this return is in centpawns, and can be negative
            int test = cp_score_positional_get_opening(color);
            cp_score_position_temp += test;

            // Note this return is in centpawns, and can be negative
            test = cp_score_positional_get_middle(color, nPlys);
            cp_score_position_temp += test;         
            
            // Note this return is in centpawns, and can be negative
            test = cp_score_positional_get_end(color, mat_avg);
            cp_score_position_temp += test;      

    
            /////////////// end positional evals /////////////////

            // Add code to promote/discourage trading, depending on who is ahead.
            // (note. At beginning of game, there are 4000 centipawns of material.
            // SO a pawn up, is 1000 centpawns, the below adds 100 centipawns to trade if Im a pawn ahead.)
            int materialIAmPositive;
            if (color == ShumiChess::WHITE) materialIAmPositive = mat_cp_white;
            else                            materialIAmPositive = mat_cp_black;
            assert (materialIAmPositive>=0);

            cp_score_position_temp += (materialIAmPositive / 10);


            // Add positional eval to score
            if (color != for_color) cp_score_position_temp *= -1;
            cp_score_position += cp_score_position_temp;


        }

    }

    // Both position and pieces are now signed properly to be positive for the "for_color" side.
    // "cp_score_pieces_only" has been adjusted to be both colors added in. So 
    // if the for_color is up a pawn, this will be 1."
    cp_score_adjusted = cp_score_pieces_only + cp_score_position;


    #ifdef _DEBUGGING_MOVE_TREE
        fprintf(fpDebug, " ev=%i", cp_score_adjusted);
    #endif

    #ifdef _DEBUGGING_MOVE_CHAIN1
        engine.print_move_to_file(move_last, nPly, (GameState::INPROGRESS), false, true, true, fpDebug); 
    #endif

    // Convert sum (for both colors) from centipawns to double.
    //return ((double)cp_score_adjusted * 0.01);
    return ((double)cp_score_adjusted);
}




//////////////////////////////////////////////////////////////////////////////////////
//
// personalities
// 
#define MAX_personalities 50

struct person {
    int constants[MAX_personalities];   
};

person TheShumiFamily[20];

person MrShumi = {100, -10, -30, +14, 20, 20, 20};  // All of these are integer and are applied in centipawns
int pers_index = 0;

int MinimaxAI::cp_score_positional_get_opening(ShumiChess::Color color) {

    int cp_score_position_temp = 0;

    int iZeroToThree, iZeroToThirty;
    int iZeroToFour, iZeroToEight;

    //if (engine.g_iMove <= 2) {
        // Add code to discourage stupid early queen moves
        // iZeroToThirty = engine.game_board.queen_still_home(color);
        // assert (iZeroToThirty>=0);
        // cp_score_position_temp += iZeroToThirty*100;   // centipawns
     //}

    // personalities

    // Add code to make king: 1. want to retain castling rights, and 2. get castled. (this one more important)

    pers_index = 0;
    engine.game_board.king_castle_happiness(color, iZeroToThree);
    assert (iZeroToThree>=0);
    assert (iZeroToThree<=3);
    cp_score_position_temp += iZeroToThree*92;   // centipawns

    // Add code to discourage isolated pawns. (returns 1 for isolani, 2 for doubled isolani, 3 for tripled isolani)
    pers_index = 1;
    int isolanis =  engine.game_board.count_isolated_pawns(color);
    assert (isolanis>=0);
    cp_score_position_temp -= (isolanis*isolanis)*9;   // centipawns

    // Add code to discourage stupid occupation of d3/d6 with bishop, when pawn on d2/d7. 
    pers_index = 2;
    iZeroToThirty = engine.game_board.bishop_pawn_pattern(color);
    assert (iZeroToThirty>=0);
    cp_score_position_temp -= iZeroToThirty*30;   // centipawns

    // Add code to encourage pawns attacking the 4-square center 
    iZeroToFour = engine.game_board.pawns_attacking_center_squares(color);
    assert (iZeroToFour>=0);
    cp_score_position_temp += iZeroToFour*14;  // centipawns    

    // Add code to encourage knights attacking the 4-square center 
    iZeroToFour = engine.game_board.knights_attacking_center_squares(color);
    assert (iZeroToFour>=0);
    cp_score_position_temp += iZeroToFour*20;  // centipawns

    // Add code to encourage bishops attacking the 4-square center
    // (cannot see through pieces, but will include "captures" of friendly pieces) 
    iZeroToFour = engine.bishops_attacking_center_squares(color);
    assert (iZeroToFour>=0);
    cp_score_position_temp += iZeroToFour*20;  // centipawns

    
    // Add code to encourage occupation of open and semi open files
    iZeroToFour = engine.game_board.rook_file_status(color);
    assert (iZeroToFour>=0);
    cp_score_position_temp += iZeroToFour*20;  // centipawns       



    return cp_score_position_temp;

}
//
// should be called in middlegame, but tries to prepare for endgame.
int MinimaxAI::cp_score_positional_get_middle(ShumiChess::Color color, int nPly) {
    int cp_score_position_temp = 0;


    // bishop pair bonus (very small)
    int bishops = engine.game_board.bits_in(engine.game_board.get_pieces_template<Piece::BISHOP>(color));
    if (bishops >= 2) cp_score_position_temp += 10;   // in centipawns

    // Add code to encourage passed pawns. (1 for each passed pawn)
    //    TODO: does not see wether passed pawns are protected
    //    TODO: does not see wether passed pawns are isolated
    //    TODO: does not see wether passed pawns are on open enemy files
    int iZeroToThirty = engine.game_board.count_passed_pawns(color);
    assert (iZeroToThirty>=0);
    cp_score_position_temp += iZeroToThirty*03;   // centipawns
   

    int i_castle_status = engine.game_board.get_castle_status_for_color(color);
    // If castled...
 
        // Add code to encourage rook connections
        int connectiveness;     // One if rooks connected. 0 if not.
        bool isOK = engine.game_board.rook_connectiveness(color, connectiveness);
        assert (connectiveness>=0);
        //assert (isOK);    // isOK just means that there werent two rooks to connect
        cp_score_position_temp += connectiveness*150;
        
        // Add code to encourage occupation of 7th rank by queens and rook
        int iZeroToFour = engine.game_board.rook_7th_rankness(color);
        assert (iZeroToFour>=0);
        cp_score_position_temp += iZeroToFour*20;  // centipawns  
        
        if ( (i_castle_status >= 2) && ((engine.g_iMove+nPly)>17) ) { 
            // Add code to attack squares near the king
            int itemp = engine.game_board.attackers_on_enemy_king_near(color);
            assert (itemp>=0);
            cp_score_position_temp += itemp*20;  // centipawns  
        }



        

    return cp_score_position_temp;
}





int MinimaxAI::cp_score_positional_get_end(ShumiChess::Color color, int mat_avg) {

    int cp_score_position_temp = 0;

    // Add code to encourage ???
    //int iZeroToOne = engine.game_board.kings_in_opposition(color);
    //cp_score_position_temp += iZeroToOne*200;  // centipawns  


    if (mat_avg < 1000) {    // 10 pawns of material or less BRING KING TOWARDS CENTER
        //   cout << "fub";
        int iZeroToOne = engine.game_board.king_center_weight(color);
        cp_score_position_temp += iZeroToOne*20;  // centipawns  
        
    }



    return cp_score_position_temp;
}




///////////////////////////////////////////////////////////////////////////////////



void MinimaxAI::do_a_deepening() {


}





int g_this_depth = 6;

//////////////////////////////////////////////////////////////////////////////////
//
// NOTE: This the entry point into the C to get a minimax AI move.
//   It does "Iterative deepening".
//
//////////////////////////////////////////////////////////////////////////////////
//
// This is a "root position". The next human move triggers a new root position
// timeRequested now in *milliseconds*
Move MinimaxAI::get_move_iterative_deepening(double timeRequested, int max_deepening_requested) {  
    

    // cout << "\x1b[94mtime requested (msec) =" << timeRequested << "\x1b[0m" << endl;
    // scout << "\x1b[94mdept requested (ply)  =" << max_deepening_requested << "\x1b[0m" << endl;

    stop_calculation = false;
    
    
    auto start_time = chrono::high_resolution_clock::now();
    // CHANGED: interpret timeRequestedMsec as milliseconds
    auto required_end_time = start_time + chrono::duration<double, std::milli>(timeRequested);

	#ifdef IS_CALLBACK_THREAD
    	start_callback_thread();
    #endif

    engine.g_iMove++;                      // Increment real moves in whole game
    cout << "\x1b[94m\nMove: " << engine.g_iMove << "\x1b[0m" << endl;

    if (engine.g_iMove) timeRequested = 1000.0;    // HAck kto allow user to hit "autplay" button before Shumi moves

    nodes_visited = 0;
    nodes_visited_depth_zero = 0;

    seen_zobrist.clear();
    uint64_t zobrist_key_start = engine.game_board.zobrist_key;
    //cout << "zobrist_key at the root: " << zobrist_key_start << endl;

    transposition_table.clear();
   

    int this_deepening;

    Move best_move = {};
    double d_best_move_value = 0.0;

 


    //Move null_move = Move{};
    Move null_move = engine.users_last_move;



    this_deepening = engine.user_requested_next_move_deepening;
    //this_deepening = 5;        // Note: because i said so.
    this_deepening = max_deepening_requested;

    maximum_deepening = this_deepening;
    

    // CHANGED: milliseconds instead of seconds; use wider integer
    long long now_s;
    long long end_s;
    long long diff_s;     // Holds (actual time - requested time). Positive if past due. Negative if sooner than expected
    long long elapsed_time = 0; // in msec


    // defaults
    int depth = 1;
    
    int nPlys = 0;
    bool bThinkingOver = false;
    bool bThinkingOverByTime = false;
    bool bThinkingOverByDepth = false;

    for (int ii=0;ii<MAX_PLY;ii++) {
        killer1[ii] = {}; 
        killer2[ii] = {};
    }


    if (nPlys == 1 && top_deepening == maximum_deepening) {
        MovesFromRoot.clear();   // final pass: start fresh
    }

    //engine.print_move_history_to_file(fpDebug);    // debug only

    do {

        do_a_deepening();

        #ifdef _DEBUGGING_MOVE_CHAIN
            fputs("\n\n---------------------------------------------------------------------------", fpDebug);
            engine.print_move_to_file(null_move, nPly, (GameState::INPROGRESS), false, true, false, fpDebug);
        #endif

        cout << endl << "Deeping " << depth << " ply of " << maximum_deepening
                << " msec=" << std::setw(6) << elapsed_time << ' ';

        nPlys = 0;
        top_deepening = depth;      // deepening starts at this depth

        double alpha = -HUGE_SCORE;
        double beta = HUGE_SCORE;

        auto ret_val = recursive_negamax(depth, alpha, beta
                                                //, move_scores_table
                                                //, move_and_scores_list
                                                , null_move
                                                , (nPlys+1)
                                               );

        // ret_val is a tuple of the score and the move.
        double d_Return_score = get<0>(ret_val);

        if (d_Return_score == ABORT_SCORE) {
            cout << "\x1b[31m Aborting depth of " << depth << "\x1b[0m" << endl;
            break;   // Stop deepening, no more depths.
        } else {
            d_best_move_value = d_Return_score;
            best_move = get<1>(ret_val);
        }



        assert ((alpha <= d_Return_score) && (d_Return_score <= beta));

        // publish PV for the *next* iteration (the next root):   PV PUSH
        assert (depth >=0);
        prev_root_best_[depth] = best_move;


        engine.move_and_score_to_string(best_move, d_best_move_value , true);

        cout << " Best: " << engine.move_string;

        depth++;

        // CHANGED: time based ending of thinking to use milliseconds
        now_s = (long long)chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        end_s = (long long)chrono::duration_cast<chrono::milliseconds>(required_end_time.time_since_epoch()).count();
        diff_s = now_s - end_s;
        elapsed_time = now_s - (long long)chrono::duration_cast<chrono::milliseconds>(start_time.time_since_epoch()).count();

        bThinkingOverByTime = (diff_s > 0);

        // depth based ending of thinking
        bThinkingOverByDepth = (depth >= (maximum_deepening+1));

        // we are done thinking if both time and depth is ended.
        bThinkingOver = (bThinkingOverByDepth && bThinkingOverByTime);

        //if (depth > 7) bThinkingOver = true;       // Hard depth limit on 7

    } while (!bThinkingOver);

    cout << "\x1b[33m\nWent to depth " << (depth - 1)  
        << " elapsed msec= " << elapsed_time << " requested msec= " << timeRequested 
        << " TimOver= " << bThinkingOverByTime << " DepOver= " << bThinkingOverByDepth 
        << "\x1b[0m" << endl;


    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";


    if (engine.g_iMove == 1)
    {

        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()
                ).count();
        std::srand(static_cast<unsigned>(ms));


        #ifdef RANDOMIZING_EQUAL_MOVES
            d_random_delta = 0.2;
        #else
            d_random_delta = 0.0;
        #endif

        best_move = pick_random_within_delta_rand(MovesFromRoot, d_random_delta);
        //cout << engine.move_into_string(bestMove) << endl;
        engine.move_into_string(best_move);
        std::cout << engine.move_string << std::endl;
   
        for ( auto& ms : MovesFromRoot) {
            Move& m = ms.first;
            double sc = ms.second;
            engine.move_into_string(m);
           // std::cout << engine.move_string << " ; " << std::fixed << std::setprecision(2) << sc << "\n";
        }
    }
    MovesFromRoot.clear();



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


    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.3f", d_best_move_value_abs);
    abs_score_string = buf;

    cout << colorize(AColor::BRIGHT_CYAN, abs_score_string + " =score,  ");

    double percent_depth_zero = nodes_visited ? ( (double)nodes_visited_depth_zero / (double)nodes_visited ) : 0.0;



    char pct[32];
    snprintf(pct, sizeof(pct), "%.0f", percent_depth_zero * 100.0);

    cout << colorize(
        AColor::BRIGHT_YELLOW,
        "Visited: " + format_with_commas(nodes_visited) +
        " / " + std::string(pct) + "% nodes total" +
        " ---- " + format_with_commas(evals_visited) + " Evals"
    ) << endl;

    //cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " / " + percent_depth_zero + " nodes total" + " ---- " + format_with_commas(evals_visited) + " Evals") << endl;
    
    
    chrono::duration<double> total_time = chrono::high_resolution_clock::now() - start_time; // still prints seconds
    cout << colorize(AColor::BRIGHT_GREEN, (static_cast<std::ostringstream&&>(std::ostringstream() << "Total time: " << std::fixed << std::setprecision(2) << total_time.count() << " sec")).str());

    assert (total_time.count() > 0);
    double nodes_per_sec = nodes_visited / total_time.count();
    double evals_per_sec = evals_visited / total_time.count();
    cout << colorize(AColor::BRIGHT_GREEN, "   nodes/sec= " + format_with_commas(std::llround(nodes_per_sec)) + "   evals/sec= " + format_with_commas(std::llround(evals_per_sec))) << endl;

  
    // Debug only  playground   sandbox for testing evaluation functions
    // int isolanis;
    bool isOK;
    double centerness;

    // isolanis =  engine.game_board.count_isolated_pawns(Color::WHITE);
    //isOK = engine.game_board.knights_centerness(Color::WHITE, centerness);  // Gets smaller closer to center.
    
    //centerness = engine.game_board.get_material_for_color(Color::WHITE) * 0.01;
    //centerness = (double)engine.game_board.get_castle_status_for_color(Color::WHITE);

    //int itemp = engine.game_board.knights_attacking_square(Color::WHITE, square_d4);
    //int itemp = engine.game_board.knights_attacking_center_squares(Color::WHITE);
    //int itemp = engine.bishops_attacking_center_squares(Color::WHITE);
    //int itemp = engine.game_board.pawns_attacking_square(Color::WHITE, square_e4);
    //int itemp = engine.game_board. pawns_attacking_center_squares(Color::WHITE);
    //int itemp = engine.game_board.count_isolated_pawns(Color::WHITE);
    //int itemp = cp_score_positional_get_opening(Color::WHITE);
    int itemp, iNearSquares;
    int king_near_squares_out[9];
    ull utemp;

    //iNearSquares = engine.game_board.get_king_near_squares(Color::WHITE, king_near_squares_out);
    //utemp = engine.game_board.sliders_and_knights_attacking_square(Color::WHITE, engine.game_board.square_d5);
    //utemp = engine.game_board.attackers_on_enemy_king_near(Color::WHITE);

    //int connectiveness;     // One if rooks connected. 0 if not.
    //isOK = engine.game_board.rook_connectiveness(Color::WHITE, connectiveness);
    //utemp = engine.game_board.get_material_for_color(Color::WHITE);
    //utemp = (ull)engine.game_board.is_king_in_check_new(Color::WHITE);
    utemp = cp_score_pieces_only_avg;
    cout << "wht " << utemp << endl;
    
    //itemp = engine.game_board.knights_attacking_square(Color::BLACK, square_d5);
    //itemp = engine.bishops_attacking_center_squares(Color::BLACK);
    //itemp = engine.game_board.knights_attacking_center_squares(Color::BLACK)
    //itemp = engine.game_board.pawns_attacking_square(Color::BLACK, square_d5);
    //itemp = engine.game_board. pawns_attacking_center_squares(Color::BLACK);
    //itemp = engine.game_board.count_isolated_pawns(Color::BLACK);
    //itemp = cp_score_positional_get_opening(Color::BLACK);

    //iNearSquares = engine.game_board.get_king_near_squares(Color::BLACK, king_near_squares_out);
    //utemp = engine.game_board.sliders_and_knights_attacking_square(Color::BLACK, engine.game_board.square_d5);
    //utemp = engine.game_board.attackers_on_enemy_king_near(Color::BLACK);
    //utemp = transposition_table.size();
    //utemp = (ull)engine.game_board.is_king_in_check_new(Color::BLACK);
    utemp = (ull)(engine.game_board.insufficient_material_simple());
    cout << "blk " << utemp << endl;

    cout << endl;
 
    //engine.debug_print_repetition_table();

    // isolanis =  engine.game_board.count_isolated_pawns(Color::BLACK);
    // // engine.game_board.king_castle_happiness(Color::WHITE, centerness);  // Gets smaller closer to center.
    // cout << "blk " << isolanis << endl;

    // double connectiveness;
    // bool bStatus;
    // bStatus  = engine.game_board.rook_connectiveness(Color::WHITE, connectiveness);
 




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
//    the move
//
////////////////////////////////////////////////////////////////////////////////////////////////////

tuple<double, Move> MinimaxAI::recursive_negamax(
                    int depth, double alpha, double beta
                    //,unordered_map<std::string, unordered_map<Move, double, MoveHash>> &move_scores_table
                    //,MoveAndScoreList& move_and_scores_list
                    ,const ShumiChess::Move& move_last      // seems to be used for debug only...
                    ,int nPlys)
{

    // Initialize return 
    double d_best_score = 0.0;
    Move the_best_move = {};
    //std::tuple<double, ShumiChess::Move> final_result;

    int cp_score;

    vector<Move> *p_moves_to_loop_over = 0;
    vector<Move> unquiet_moves;

    nodes_visited++;
    if (depth==0) nodes_visited_depth_zero++;

    std::vector<Move> legal_moves;

    //bMoreDebug = true;
    //debugMove = engine.move_string;


    if (stop_calculation) {
        cout << "\n! STOP CALCULATION requested \n";
        stop_calculation = false;
        return { ABORT_SCORE, the_best_move };
    }



    //bMoreDebug = false;

    // =====================================================================
    // asserts
    // =====================================================================

    // Over analysis sentinal Sorry, I should not be this large
    assert(nPlys >= 0);
    assert(nPlys <= MAX_PLY);

    //if (alpha > beta) assert(0);

    
    // =====================================================================
    // Hard node-limit sentinel fuse colorize(AColor::BRIGHT_CYAN,
    // =====================================================================
    if (nodes_visited > 5.0e8) {    // 10,000,000 a good number here
        std::cout << "\x1b[31m! NODES VISITED trap#2 " << nodes_visited << "dep=" << depth << "  " << d_best_score << "\x1b[0m\n";

        return { ABORT_SCORE, the_best_move };

    }


    #ifdef _DEBUGGING_MOVE_CHAIN1
        engine.print_move_to_file(move_last, nPly, (GameState::INPROGRESS), false, true, true, fpDebug); 
    #endif


    // If mover is in check, this routine returns all check escapes and only the check escapes.
    legal_moves = engine.get_legal_moves();




    

    p_moves_to_loop_over = &legal_moves;


    GameState state = engine.game_over(legal_moves);


    // =====================================================================
    // Terminal positions (game over)
    // =====================================================================
    if (state != GameState::INPROGRESS) {

        int level = (top_deepening - depth);
        assert(level >= 0);
        assert(nPlys >= level);     // NOTE: we should use nPly here, simpler.

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
    // Quiescence entry when depth == 0
    // =====================================================================
    assert (depth >= 0);
    bool in_check = false;
    if (depth == 0) {

        // Static board evaluation
        // do "fast eval if nPly too high"
        bool bFast = false;
        #ifdef FAST_EVALUATIONS          
            if (nPlys > (maximum_deepening*2)) // because I said so    (14 or so)
            {
                //cout << " beeep ";
                bFast = true;
            }
        #endif


        #ifdef DOING_TRANSPOSITION_TABLE2
            auto it = transposition_table.find(engine.game_board.zobrist_key);
            if (it != transposition_table.end()) {
                const TTEntry &entry = it->second;

                // Only trust if this cached result was searched at least as deep
                // as what we're currently doing (top_deepening).
                if (entry.depth >= top_deepening) {
                    d_best_score = engine.convert_from_CP(entry.score_cp);

                    // Transposition table hit: reuse both score and move from table.
                    // This skips evaluation.
                    //cout << "\x1b[33m*\x1b[0m";
                    return { d_best_score, entry.movee };
                }
            }
        #endif

        cp_score = evaluate_board(engine.game_board.turn, nPlys, bFast);  //, legal_moves);
        d_best_score =  ((double)cp_score * 0.01);

        #ifdef DOING_TRANSPOSITION_TABLE2
            TTEntry &slot = transposition_table[engine.game_board.zobrist_key];

            // Only overwrite if this result is from at least as deep
            // as whatever is already stored.
            if (top_deepening >= slot.depth)
            {
                slot.score_cp = engine.convert_to_CP(d_best_score);
                slot.movee    = the_best_move;
                slot.depth    = top_deepening;
            }
        #endif



        #ifdef _DEBUGGING_MOVE_CHAIN
            engine.print_move_to_file(move_last, nPly, (GameState::INPROGRESS), false, true, true, fpDebug); 
        #endif


        unquiet_moves = engine.reduce_to_unquiet_moves_MVV_LVA(legal_moves);

        in_check = engine.is_king_in_check(engine.game_board.turn);

        // If quiet (not in check & no tactics), just return stand-pat
        if (!in_check && unquiet_moves.empty()) {
            return { d_best_score, Move{} };
        }

        // So "d_best_score" is my "stand pat value."
        if (!in_check) {

            // Stand-pat cutoff
            if (d_best_score >= beta) {
                return { d_best_score, Move{} };
            }
            alpha = std::max(alpha, d_best_score);

            // Extend on captures/promotions only
            p_moves_to_loop_over = &unquiet_moves; 

            #ifdef _DEBUGGING_MOVE_TREE
                int ierr = sprintf( szValue1, "\nonquiet=%zu ", unquiet_moves.size());
                assert (ierr!=EOF);

                print_moves_to_print_tree(unquiet_moves, depth, szValue1, "\n");
            #endif

        } else {
            // In check: use all legal moves, since by definition (see get_legal_moves() the set of all legal moves is equivnelent 
            // to the set of all check escapes. By definition.
            //moves_to_loop_over = legal_moves;  // not needed as its done ealier above. Sorry.
        }

		// Pick best static evaluation among all legal moves if hit the over ply
        constexpr int MAX_QPLY = 15;        // because i said so
        if (nPlys >= MAX_QPLY) {
            //std::cout << "\x1b[31m! MAX_QPLY trap " << nPly << "\x1b[0m\n";
            //std::cout << "\x1b[31m!" << "\x1b[0m";
            auto tup = best_move_static(engine.game_board.turn, (*p_moves_to_loop_over), nPlys, in_check, depth);
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

        
        // Resort moves based on varoius things
        //if (depth > 0) {
        if (p_moves_to_loop_over == &legal_moves) {     // Only sort if Depth>0, and not in check.
            sort_moves_for_search(p_moves_to_loop_over, depth, nPlys);
            assert(p_moves_to_loop_over == &legal_moves);
        } else {
            //assert(p_moves_to_loop_over == &unquiet_moves);   // NO not if in check
        }

        d_best_score = -HUGE_SCORE;
        the_best_move = sorted_moves[0];   // Note: Is this the correct intialization? Why not HUGE_SCORE

        int cp_pawn = engine.game_board.centipawn_score_of(ShumiChess::Piece::PAWN);
        for (const Move& m : sorted_moves) {


            #ifdef _DEBUGGING_PUSH_POP
                std::string temp_fen_before = engine.game_board.to_fen();
                ull zobrist_save = engine.game_board.zobrist_key;
                auto ep_history_saveb = engine.game_board.black_castle_rights;
                auto ep_history_savew = engine.game_board.white_castle_rights;
                auto enpassant_save = engine.game_board.en_passant_rights;
            #endif

            // Delta pruning (in qsearch at depth==0) estimates the most this capture/promotion could possibly 
            // improve the current stand-pat score (including material swing and a safety margin), and if 
            // even that optimistic bound still can't raise alpha, it just skips searching that move as futile. 
            #ifdef DELTA_PRUNING
                // --- Delta pruning (qsearch only)
                if (depth == 0 && !in_check) {
                    int ub = 0;
                    if (m.capture != ShumiChess::Piece::NONE) {
                        ub += engine.game_board.centipawn_score_of(m.capture); // victim value
                    }
                    if (m.promotion != ShumiChess::Piece::NONE) {
                        ub += engine.game_board.centipawn_score_of(m.promotion)
                            - engine.game_board.centipawn_score_of(ShumiChess::Piece::PAWN); // promo gain
                    }
                    const bool recapture = (!engine.move_history.empty() && (m.to == engine.move_history.top().to));
                    constexpr int DELTA_MARGIN_CP = 80; // tune 60..100

                    int cp_best_score = engine.convert_to_CP(d_best_score);
                    int cp_alpha = engine.convert_to_CP(alpha);

                    if (!recapture && (cp_best_score + ub + DELTA_MARGIN_CP < cp_alpha)) {
                        continue;   // skip this move its futile
                    }

                    // Pawn-victim futility (beyond delta): skip far-below-alpha pawn grabs (non-recapture)
                    if (!recapture &&
                        (m.capture == ShumiChess::Piece::PAWN) && (cp_best_score + cp_pawn + 60) < cp_alpha) {
                        continue;   // skip this move its futile
                    }
                }
                // --- end Delta pruning
            #endif
    

            engine.pushMove(m);


            #ifdef _DEBUGGING_MOVE_TREE
                engine.print_move_to_file(m, nPly, state, false, false, false, fpDebug);
            #endif
               
            #ifdef IS_CALLBACK_THREAD
            	g_live_ply = nPlys;
            #endif

            ++engine.repetition_table[engine.game_board.zobrist_key];

            //
            // Two parts in negamax: 1. "relative scores", the alpha betas are reversed in sign,
            //                       2. The beta and alpha arguments are staggered, or reversed. 
            auto ret_val = recursive_negamax(
                (depth > 0 ? depth - 1 : 0),                // Refuse to allow negative depth
                -beta, -alpha,      // reverse in sign and order at the same time
                //move_and_scores_list,
                //move_scores_table,
                m, (nPlys+1)
            );

            // The third part of negamax: negate the score to keep it relative.
            double d_return_score = get<0>(ret_val);
            Move d_return_move = get<1>(ret_val);

            --engine.repetition_table[engine.game_board.zobrist_key];
            if (engine.repetition_table[engine.game_board.zobrist_key] == 0) {
                engine.repetition_table.erase(engine.game_board.zobrist_key);
            }

            engine.popMove();





            #ifdef _DEBUGGING_PUSH_POP
                std::string temp_fen_after = engine.game_board.to_fen();
                if (temp_fen_before != temp_fen_after) {
                    std::cout << "\x1b[31m";
                    std::cout << "PROBLEM WITH PUSH POP!!!!!" << std::endl;
                    cout_move_info(m);
                    std::cout << "FEN before  push/pop: " << temp_fen_before  << std::endl;
                    std::cout << "FEN after   push/pop: " << temp_fen_after   << std::endl;
                    std::cout << "\x1b[0m";
                    assert(0);
                }
                if (zobrist_save != engine.game_board.zobrist_key) {
                    std::cout << "\x1b[31m";
                    std::cout << "PROBLEM WITH PUSH POP zobrist!!!!!" << std::endl;
                    std::cout << "\x1b[0m";
                    assert(0);        
                }
                if (ep_history_saveb != engine.game_board.black_castle_rights) {
                    std::cout << "\x1b[31m";
                    std::cout << "PROBLEM WITH PUSH POP B !!!!!" << std::endl;
                    std::cout << "\x1b[0m";
                    assert(0);   
                }
                if (ep_history_savew != engine.game_board.white_castle_rights) {
                    std::cout << "\x1b[31m";
                    std::cout << "PROBLEM WITH PUSH POP A !!!!!" << std::endl;
                    std::cout << "\x1b[0m";
                    assert(0);   
                }
                if (enpassant_save != engine.game_board.en_passant_rights) {
                    std::cout << "\x1b[31m";
                    std::cout << "PROBLEM WITH PUSH POP P !!!!!" << std::endl;
                    std::cout << "\x1b[0m";
                    assert(0);   
                }
            #endif

            // negamax, reverse returned score.  
            double d_score_value = -d_return_score;

            if (d_return_score == ABORT_SCORE) {
                //cout << "\n! STOP CALCULATION now \n" << endl;
                return {ABORT_SCORE, the_best_move};
            }

            if (nPlys == 1 && top_deepening == maximum_deepening) {
                MovesFromRoot.emplace_back(m, d_score_value);  // record ROOT move & its (negamax) score
            }


            #ifdef _DEBUGGING_MOVE_CHAIN
                char szTempo1[256];
                sprintf(szTempo1, " %f ", d_score_value);
                int nChars = fputs(szTempo1, fpDebug);
                if (nChars == EOF) assert(0);
            #endif

            // Store score
            //std::string temp_fen_now = engine.game_board.to_fen();
            //move_scores_table[temp_fen_now][m] = d_score_value;

            // record (move, score)
            //move_and_scores_list.emplace_back(m, d_score_value);

            // CAUTION! THIS IS BROKEN CODE. DO NOT TOUCH
             // Note: this should be done as a real random choice. (random over the moves possible). 
            // This dumb approach favors moves near the end of the list
   
            // Only randomize near-equal scores at the top level
            //bool is_at_bottom_root = ((top_deepening - depth) == 0);    // (depth == maximum_deepening) && 
            bool is_at_end_of_the_line = ( (engine.g_iMove==1) );
            //assert (!is_at_bottom_root);

            //if (is_at_end_of_the_line) cout << "spud";

            double delta_score = (is_at_end_of_the_line ? 0.1 : 0.0);
            delta_score = 0.0;     // personalities

            double d_difference_in_score = std::fabs(d_score_value - d_best_score);
            if (d_difference_in_score <= (delta_score + VERY_SMALL_SCORE)) {
                // tie (within delta): flip a coin at root; never inside the tree
                b_use_this_move = engine.flip_a_coin();
            } else {
                b_use_this_move = (d_score_value > d_best_score);
            }

            if (b_use_this_move) {
                d_best_score = d_score_value;
                the_best_move = m;
            }

            // Think of alpha as “best score found so far at this node.”
            alpha = std::max(alpha, d_best_score);

            // Alpha/beta "cutoff", (fail high), break the analysis
            // You just found a move so good that the opponent would never 
            // allow this position, because it already exceeds what they could tolerate.
            if (alpha >= beta + VERY_SMALL_SCORE) {

                if ((depth > 0) && (!engine.is_unquiet_move(m))) {
                    // Quiet moves that "cut off" are notable.
                    if (killer1[nPlys] == ShumiChess::Move{})        killer1[nPlys] = m;
                    else if (!(m == killer1[nPlys]))                 killer2[nPlys] = m;
                }

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
                        engine.print_move_to_file(m, nPly, state, false, false,false, fpDebug);
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


//
//  Note: I can be called at any depth.
void MinimaxAI::sort_moves_for_search(std::vector<ShumiChess::Move>* p_moves_to_loop_over   // input/output
                            , int depth, int nPlys)
{
    assert(top_deepening > 0);
    assert(p_moves_to_loop_over);
    auto& moves = *p_moves_to_loop_over;
    if (moves.empty()) return;

    const bool have_last = !engine.move_history.empty();
    const ull  last_to   = have_last ? engine.move_history.top().to : 0ULL;

    // --- 1. "PV ordering (root only)" --- 
    // Even though its called "root" in literature, but that is a misnomer. 
    // It really is the last deepening, the opposite of the real root, which is the first deepening.
    if (nPlys == 1)
    {
        const ShumiChess::Move pv_move = prev_root_best_[top_deepening - 1];
        for (size_t i = 0; i < moves.size(); ++i)
        {
            if (moves[i] == pv_move)
            {
                if (i != 0)
                    std::swap(moves[0], moves[i]);
                break;
            }
        }
    }

    //
    //   Resort moves in this order (they will later be searched in this order):
    //      1. unquiet moves (captures/promotions, sorted by MVV-LVA)
    //      2. killer moves (quiet, bubbled to the front of the "cutoff" quiet slice)
    //      3. remaining quiet moves
    //

    #ifdef UNQUIET_SORT
        // --- 1. Partition unquiet moves (captures/promotions) to the front ---
        auto it_split = std::partition(
            moves.begin(), moves.end(),
            [&](const ShumiChess::Move& mv)
            {
                return engine.is_unquiet_move(mv);
            });

        // --- 2. Sort the unquiet prefix using MVV-LVA ---
        std::sort(moves.begin(), it_split,
            [&](const ShumiChess::Move& a, const ShumiChess::Move& b)
            {
                int keyA = (a.capture != ShumiChess::Piece::NONE) ? engine.mvv_lva_key(a) : 0;
                int keyB = (b.capture != ShumiChess::Piece::NONE) ? engine.mvv_lva_key(b) : 0;
                if (have_last && a.to == last_to) keyA += 800;
                if (have_last && b.to == last_to) keyB += 800;
                return keyA > keyB;
            });

        // --- 3. Apply killer moves to the quiet region (for speed, not re-sorting) ---
        auto quiet_begin = it_split;
        auto quiet_end   = moves.end();

        auto bring_front = [&](const ShumiChess::Move& km)
        {
            if (km == ShumiChess::Move{}) return;
            for (auto it = quiet_begin; it != quiet_end; ++it)
            {
                if (*it == km)
                {
                    std::rotate(quiet_begin, it, it + 1);
                    ++quiet_begin; // next killer goes just after previous
                    break;
                }
            }
        };
        bring_front(killer1[nPlys]);
        bring_front(killer2[nPlys]);
    #endif



  
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
        int cp_score =  evaluate_board(color_perspective, 0, false) * color_multiplier;
        double d_best_score =  ((double)cp_score * 0.01);
        return d_best_score;

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
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + "  " + format_with_commas(nodes_visited_depth_zero) + " nodes total") << endl;

    chrono::duration<double> total_time = chrono::high_resolution_clock::now() - start_time;
    cout << colorize(AColor::BRIGHT_GREEN, "Total time taken for get_move(): " + to_string(total_time.count()) + " s") << endl;

    return move_chosen;
}

Move MinimaxAI::get_move() {
    int default_depth = 7;
    cout << colorize(AColor::BRIGHT_GREEN, "Called MinimaxAI::get_move() with no arguments, using default depth: " + to_string(default_depth)) << endl;
    return get_move(default_depth);
}


void MinimaxAI::sort_moves_by_score(
    MoveAndScoreList& moves_and_scores_list,
    bool sort_descending
)
{
    if (sort_descending) {
        std::sort(moves_and_scores_list.begin(), moves_and_scores_list.end(),
                  [](const MoveAndScore& a, const MoveAndScore& b) {
                      return a.second > b.second;
                  });
    } else {
        std::sort(moves_and_scores_list.begin(), moves_and_scores_list.end(),
                  [](const MoveAndScore& a, const MoveAndScore& b) {
                      return a.second < b.second;
                  });
    }
}


// Loop over all passed moves, find the best move by static evaluation.


std::tuple<double, ShumiChess::Move>
MinimaxAI::best_move_static(ShumiChess::Color color,
                            const std::vector<ShumiChess::Move>& legal_moves,
                            int nPly,
                            bool in_Check,
                            int depth)
{
    double d_best;
    ShumiChess::Move bestMove = ShumiChess::Move{};

  
    #ifdef DOING_TRANSPOSITION_TABLE
        uint64_t zob = engine.game_board.zobrist_key;

        auto it = transposition_table.find(zob);
        if (it != transposition_table.end()) {
            const TTEntry &entry = it->second;

            // Optional: only trust entries from at least this depth
            if (entry.depth >= top_deepening) {
                d_best = engine.convert_from_CP(entry.score_cp);

                // Transposition table hit: reuse both score and move
                //cout << "\x1b[33m&\x1b[0m";
                return { d_best, entry.movee };
            }
        }
    #endif


    // If there are no moves:
    // - not in check: return a single static (stand-pat) eval
    // - in check: treat as losing (no legal escapes here)
    if (legal_moves.empty()) {
        if (!in_Check) {

            int cp_score = evaluate_board(color, nPly, false);  //, legal_moves);         // positive is good for 'color'
            double stand_pat =  ((double)cp_score * 0.01);
            return { stand_pat, ShumiChess::Move{} };
        }
        return { -HUGE_SCORE, ShumiChess::Move{} };
    }

    d_best = -HUGE_SCORE;

    for (const auto& m : legal_moves) {

        engine.pushMove(m);
        
        int cp_score = evaluate_board(color, nPly, false); //, legal_moves);  // positive is good for 'color'
        double d_score = ((double)cp_score * 0.01);
        
        engine.popMove();

        if (d_score > d_best) {
            d_best = d_score;
            bestMove = m;
        }
    }

    #ifdef DOING_TRANSPOSITION_TABLE

        TTEntry &slot = transposition_table[engine.game_board.zobrist_key];

        // Only overwrite if this result is from at least as deep a search
        // as whatever was stored before.
        if (top_deepening >= slot.depth)
        {
            slot.score_cp = engine.convert_to_CP(d_best);
            slot.movee    = bestMove;
            slot.depth    = top_deepening;
        }
    #endif

    
    return { d_best, bestMove };
}



//
// Picks a random root move within delta (in pawns) of the best score (negamax-relative).
// If list is empty: returns default Move{}.
// If best is "mate-like": returns the best (no randomization).
Move MinimaxAI::pick_random_within_delta_rand(std::vector<std::pair<Move,double>>& MovsFromRoot,
                                              double delta_pawns)
{
    if (MovsFromRoot.empty()) return Move{};  // safety

    // 0) Sort DESC by score (leaves the caller’s list sorted)
    std::sort(MovsFromRoot.begin(), MovsFromRoot.end(),
              [](const auto& a, const auto& b){ return a.second > b.second; });

    // 1) Best is now front()
    const double best = MovsFromRoot.front().second;

    // 2) If mate-like, don’t randomize
    if (std::fabs(best) > 9000.0) return MovsFromRoot.front().first;

    // 3) Build the contiguous “within delta” prefix
    const double cutoff = best - delta_pawns;
    size_t n_top = 0;
    while (n_top < MovsFromRoot.size() && MovsFromRoot[n_top].second >= cutoff) ++n_top;

    // (fallback if something odd)
    if (n_top == 0) return MovsFromRoot.front().first;

    // 4) Uniform pick from [0, n_top)
    const int pick = engine.rand_int(0, static_cast<int>(n_top) - 1);  // assumes you’ve seeded srand(...)
    return MovsFromRoot[static_cast<size_t>(pick)].first;
}



/////////////////////////////////////////////////////////////////////




#ifdef _DEBUGGING_MOVE_TREE


void MinimaxAI::print_moves_to_print_tree(std::vector<Move> mvs, int depth, char* szHeader, char* szTrailer)
{

    if (szHeader != NULL) {int ierr = fprintf(fpDebug, szHeader);}
    
     for (Move &m : mvs) {
        engine.print_move_to_file(m, depth, (GameState::INPROGRESS), false, false, false, fpDebug);
     }

    if (szTrailer != NULL) {int ierr = fprintf(fpDebug, szTrailer);}


}



void MinimaxAI::print_move_scores_to_file(
    FILE* fpDebug,
    const std::unordered_map<std::string,std::unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>>& move_scores_table
)
{

    // sprintf(szValue1, "\n---------------------------------------------------------------------------");
    // fprintf(fpDebug, "%s", szValue1);
    // size_t iFENS = move_scores_table.size();    // This returns the number of FEN rows.
    // sprintf(szValue1, "\n\n  nFENS = %i", (int)iFENS);
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



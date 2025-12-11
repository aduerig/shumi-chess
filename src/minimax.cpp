
#include <float.h>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <locale>
#include <string>
#include <tuple>
#include <algorithm>
#include <set>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <iostream>
#include <atomic>

#include <globals.hpp>
#include "utility.hpp"
#include "minimax.hpp"
#include "salt.h"
#include "Features.hpp"

using namespace std;
using namespace ShumiChess;
using namespace utility;
using namespace utility::representation;
using namespace utility::bit;


#undef NDEBUG
//#define NDEBUG         // Define (uncomment) this to disable asserts
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////////////////////

// Debug   THESE SHOULD BE OFF only for debugging

//#define _DEBUGGING_PUSH_POP

//#define _DEBUGGING_TO_FILE         // I must be defined to use either of the below
//#define _DEBUGGING_MOVE_CHAIN
//#define _DEBUGGING_MOVE_SORT

// extern bool bMoreDebug;
// extern string debugMove;

//#define DEBUGGING_RANDOM_DELTA

// #define DOING_TT_EVAL2
// #define DOING_TT_EVAL_DEBUG

//#define DOING_TT2_NORM_DEBUG
//#define BURP2_THRESHOLD_CP 25

//#define DEBUGGING_KILLER_MOVES 

#ifdef _DEBUGGING_TO_FILE
    FILE *fpDebug = NULL;
    char szDebug[256];
    bool bSuppressOutput = false;
#endif

// It is known that Killer moves force "TT2 unrepeatibility". The theory is I guess that the
// later analysis is profited by these killer moves.
#ifndef DOING_TT2_NORM_DEBUG
    //#define KILLER_MOVES
#endif

// Obviously TT2 debug breaks repeatibility if we flip coins in the anaylisis. So what. 
#ifndef DOING_TT2_NORM_DEBUG
    //#define RANDOM_FLIP_COIN
#endif

//////////////////////////////////////////////////////////////////////////////

static std::atomic<int> g_live_ply{0};   // value the callback prints

// Used only in DOING_TT2_NORM_DEBUG
static void print_mismatch(std::ostream& os,
                        const char* label,
                        int found,
                        int actual) {
    if (found != actual) {
        os << " " << label << " " << found << " = " << actual << "\n";
    }
}

/////////////////////////////////////////////////////////////////////////

// Speedups?
//#define FAST_EVALUATIONS
//#define DELTA_PRUNING


// Only randomizes a small amount a list formed on the root node, when at maxiumum deepening, AND 
// on first move.
#define RANDOMIZING_EQUAL_MOVES
#define RANDOMIZING_EQUAL_MOVES_DELTA 0.001



#ifdef DEBUGGING_KILLER_MOVES
    static long long killer_tried = 0;
    static long long killer_cutoff = 0;
#endif


#define IS_CALLBACK_THREAD    // Uncomment to enable the callback to show "nPly", real time.
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

    // Initialize storage buffers (they live here to avoid extra allocation during the game)
    engine.repetition_table.clear();
    TTable.clear();
    TTable.reserve(10000000);    // NOTE: What size here?
    
    // add the current position
    uint64_t key_now = engine.game_board.zobrist_key;
    engine.repetition_table[key_now] = 1;

    // Set default features
    Features_mask = _DEFAULT_FEATURES_MASK;

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

    cout << "wake up " << endl;
    stop_calculation = true;
    //engine.debug_SEE_for_all_captures(fpDebug);
}


void MinimaxAI::resign() {

    cout << "resign " << endl;
    //stop_calculation = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is a "leaf". Returns " relative (negamax) score". in centipawns. Relative score is positive for great
// positions for the specified player. Absolute score is always positive for great positions for white.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Follows the negamax convention, so a positive value at a leaf is “good for the side to move.” (for_color)
// returns a positive score, if "for_color" is ahead.
//
// nPhase 0 opening
//        1 middle game
//        2 endgame
//        3 extreme endgame

int MinimaxAI::evaluate_board(Color for_color, int nPhase, bool is_fast_style, bool isQuietPosition
                                //const std::vector<ShumiChess::Move>* pLegal_moves  // may be nullptr
                            )
                                
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
    int pawns_cp_white = 0;
    int pawns_cp_black = 0;

    int tempsum = 0.0;
    int tempsumNP = 0.0;
    int cp_score_material_all = 0;
    int cp_score_pawns_only = 0;
    

    //
    // Do material. Note we compute non-pawn (NP) quantities in parallel with the full quantities.
    //
    for (const auto& color1 : array<Color, 2>{Color::WHITE, Color::BLACK}) {

        // Get the centipawn value for this color
        int cp_pawns_only_temp;
        int cp_score_mat_temp = engine.game_board.get_material_for_color(color1, cp_pawns_only_temp);
        assert (cp_score_mat_temp>=0);    // there is no negative value material
        assert (cp_pawns_only_temp>=0);   // there is no negative value material

        if (color1 == ShumiChess::WHITE) {
            mat_cp_white = cp_score_mat_temp;
            pawns_cp_white = cp_pawns_only_temp;
        } else {
            mat_cp_black = cp_score_mat_temp;
            pawns_cp_black = cp_pawns_only_temp;
        }

        tempsum += cp_score_mat_temp;
        tempsumNP += cp_score_mat_temp - cp_pawns_only_temp;
   
        // Take color into acccount
        if (color1 != for_color) 
        {
            cp_score_mat_temp *= -1;
            cp_pawns_only_temp *= -1;
        }
        cp_score_material_all += cp_score_mat_temp;
        cp_score_pawns_only += cp_pawns_only_temp;

    }

    assert(tempsum>=0);
    assert(tempsumNP>=0);
    cp_score_material_avg = tempsum / 2;
    //cp_score_material_NP_avg = tempsumNP / 2;

    int mat_np_white = mat_cp_white - pawns_cp_white;
    int mat_np_black = mat_cp_black - pawns_cp_black;
    assert(mat_np_white>=0);
    assert(mat_np_black>=0);
    //cp_score_material_NP_avg = (mat_np_white + mat_np_black) / 2;   // NEW


    // ??? display only
    engine.material_centPawns = cp_score_material_all;

    //
    // Positional considerations only
    //
    int cp_score_position = 0;
    bool isOK;
    int test;

    
    if ( (!is_fast_style) ) {

        // if either side has "only a king", opening/middle terms are skipped for both WHITE and BLACK.

        for (const auto& color : array<Color, 2>{Color::WHITE, Color::BLACK}) {
            int cp_score_position_temp = 0;        // positional considerations only

            Color enemy_of_color = (color == ShumiChess::WHITE) ? ShumiChess::BLACK : ShumiChess::WHITE;
            bool onlyKingEnemy   = engine.game_board.bIsOnlyKing(enemy_of_color);
            bool onlyKingFriend  = engine.game_board.bIsOnlyKing(color);

            /////////////// start positional evals /////////////////

            // Note this return is in centpawns, and can be negative
            if (!onlyKingEnemy) {

                test = cp_score_positional_get_opening(color);
                cp_score_position_temp += test;
       
                // Note this return is in centpawns, and can be negative
                test = cp_score_positional_get_middle(color);
                cp_score_position_temp += test;    
            }     
            
            // Note this return is in centpawns, and can be negative
            test = cp_score_positional_get_end(color, nPhase, cp_score_material_avg
                                            ,onlyKingFriend, onlyKingEnemy);
            cp_score_position_temp += test;      


            //  = (pLegal_moves != nullptr) && !

            //if (b_is_Quiet && color == engine.game_board.turn) {
            // trading
            if ( (isQuietPosition) && (color == for_color) ) {
                test = cp_score_get_trade_adjustment(color, mat_np_white, mat_np_black);
                cp_score_position_temp += test;

                #ifdef _DEBUGGING_MOVE_CHAIN1
                    sprintf(szDebug, "trd %ld", test);
                    fputs(szDebug, fpDebug);
                #endif

            }
    
            /////////////// end positional evals /////////////////


            // Add positional eval to score
            if (color != for_color) cp_score_position_temp *= -1;
            cp_score_position += cp_score_position_temp;

        }

    }

    //
    // Both position and material are now signed properly to be positive for the "for_color" side.
    // "cp_score_material_all" has been adjusted to be both colors added in. So 
    // if the for_color is up a pawn, this will be 1."
    cp_score_adjusted = cp_score_material_all + cp_score_position;


    #ifdef _DEBUGGING_MOVE_CHAIN1
        engine.print_move_to_file(move_last, nPlys, (GameState::INPROGRESS), false, true, true, fpDebug); 
    #endif

    // Convert sum (for both colors) from centipawns to double.
    return cp_score_adjusted;
}




//////////////////////////////////////////////////////////////////////////////////////
//
// personalities
// 
#define MAX_personalities 50

struct person {
    int constants[MAX_personalities];   
};

// NOte: what sort of nonsense is this? A family of evaluator's? Why not?
person TheShumiFamily[20];

person MrShumi = {100, -10, -30, +14, 20, 20, 20};  // All of these are integer and are applied in centipawns
int pers_index = 0;

int MinimaxAI::cp_score_positional_get_opening(ShumiChess::Color color) {

    int cp_score_position_temp = 0;

    int icp_temp, iZeroToThree, iZeroToThirty;
    int iZeroToFour, iZeroToEight;

    // personalities

    // Add code to make king: 1. want to retain castling rights, and 2. get castled. (this one more important)

    pers_index = 0;
    //icp_temp = engine.game_board.king_castle_happiness(color);
    icp_temp = engine.game_board.get_castle_status_for_color(color);

    //cp_score_position_temp += iZeroToThree*90;   // centipawns
    cp_score_position_temp += icp_temp;     // centipawns

    // Add code to discourage isolated pawns. (returns 1 for isolani, 2 for doubled isolani, 3 for tripled isolani)
    pers_index = 1;
    int isolanis =  engine.game_board.count_isolated_pawns(color);
    assert (isolanis>=0);
    cp_score_position_temp -= isolanis*8;   // centipawns

    // Note each pair of doubled pawns is 2.
    pers_index = 1;
    int doublees =  engine.game_board.count_doubled_pawns(color);
    assert (doublees>=0);
    cp_score_position_temp -= doublees*3;   // centipawns

    // Add code to discourage stupid occupation of d3/d6 with bishop, when pawn on d2/d7. 
    // Note: this is gross
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
    // (cannot see through material, but will include "captures" of friendly material) 
    iZeroToFour = engine.bishops_attacking_center_squares(color);
    assert (iZeroToFour>=0);
    cp_score_position_temp += iZeroToFour*20;  // centipawns

    
    // Add code to encourage occupation of open and semi open files
    icp_temp = engine.game_board.rook_file_status(color);
    assert (icp_temp>=0);
    cp_score_position_temp += icp_temp;  // centipawns       



    return cp_score_position_temp;

}
//
// should be called in middlegame, but tries to prepare for endgame.
int MinimaxAI::cp_score_positional_get_middle(ShumiChess::Color color) {
    int cp_score_position_temp = 0;


    // bishop pair bonus
    int bishops = engine.game_board.bits_in(engine.game_board.get_pieces_template<Piece::BISHOP>(color));
    if (bishops >= 2) cp_score_position_temp += 10;   // in centipawns

    // Add code to encourage passed pawns. (1 for each passed pawn)
    //    TODO: does not see wether passed pawns are protected
    //    TODO: does not see wether passed pawns are isolated
    //    TODO: does not see wether passed pawns are on open enemy files
    int iZeroToThirty = engine.game_board.count_passed_pawns(color);
    assert (iZeroToThirty>=0);
    cp_score_position_temp += iZeroToThirty*03;   // centipawns
   

    // Add code to encourage rook connections (files or ranks)
    int connectiveness;     // One if rooks connected. 0 if not.
    bool isOK = engine.game_board.rook_connectiveness(color, connectiveness);
    assert (connectiveness>=0);
    //assert (isOK);    // isOK just means that there werent two rooks to connect
    cp_score_position_temp += connectiveness*150;
    
    // Add code to encourage occupation of 7th rank by queens and rook
    int iZeroToFour = engine.game_board.rook_7th_rankness(color);
    assert (iZeroToFour>=0);
    cp_score_position_temp += iZeroToFour*20;  // centipawns  
    

    return cp_score_position_temp;
}


// trading "np" means "no pawns".
int MinimaxAI::cp_score_get_trade_adjustment(ShumiChess::Color color,
                                             int mat_np_white,      // centipawns
                                             int mat_np_black)      // centipawns
{
    int cp_clamp = 500;

    // NP advantage (centipawns) from "color"'s point of view (positive, if good for color)
    int np_adv_for_color =
        (color == ShumiChess::WHITE) ? (mat_np_white - mat_np_black)
                                     : (mat_np_black - mat_np_white);

    if (np_adv_for_color == 0) return 0;  // no NP edge, so no bonus


    int denominator =  (mat_np_black + mat_np_white);
    assert (denominator > 0);                      // no peices?

    double dRatio = (double)np_adv_for_color / (double)denominator;

    int iReturn = (int)(dRatio*700.0);
    if (iReturn >  cp_clamp) iReturn =  cp_clamp;
    if (iReturn < -cp_clamp) iReturn = -cp_clamp;

    return iReturn;

}


int MinimaxAI::cp_score_positional_get_end(ShumiChess::Color color, int nPhase, int mat_avg,
                                            bool onlyKngFriend, bool onlyKngEnemy
                                            ) {

    int cp_score_position_temp = 0;

    // Add code to encourage ???
    //int iZeroToOne = engine.game_board.kings_in_opposition(color);
    //cp_score_position_temp += iZeroToOne*200;  // centipawns  

    Color enemy_color = (color==ShumiChess::WHITE ? ShumiChess::BLACK : ShumiChess::WHITE);
    // if (mat_avg < 1000) {    // 10 pawns of material or less BRING KING TOWARDS CENTER
    //     //   cout << "fub";
    // }

    if ( nPhase > 0 ) { 
        // Add code to attack squares near the king
        int itemp = engine.game_board.attackers_on_enemy_king_near(color);
        assert (itemp>=0);
        cp_score_position_temp += itemp*20;  // centipawns  
    }

    if (onlyKngEnemy) {

        //     if (color == ShumiChess::WHITE) assert(0);   // exploratory

        // Rewards king near other king
        // Returns 2 to 10,. 2 if in opposition, 10 if in opposite corners.
        // Returns zero if other pieces on board
        double dkk = engine.game_board.king_near_other_king(color);  // ≈ 2..
        if ( (dkk!=0) && (dkk<2.0) ) {   // equal zero if other pieces 
            printf(" %f ", dkk);
            assert(0);
        }
        if (dkk > 10) {
            printf(" %f ", dkk);
            assert(0);
        }

        double dFarness = 10.0 - dkk;   // 8 if in opposition, 0, if in opposite corners
        cp_score_position_temp += (int)(dFarness * 50.0);


        // Rewards if enemy king ner corner. 0 for the inner ring (center) 3 for outer ring (edge squares)
        enemy_color = (color==ShumiChess::WHITE ? ShumiChess::BLACK : ShumiChess::WHITE);
        int edge_wght = engine.game_board.king_edge_weight(enemy_color);

        cp_score_position_temp += (int)(edge_wght * 80.0);


    //     #ifdef _DEBUGGING_MOVE_CHAIN
    //         sprintf(szDebug, "kclos %ld", cp_score_position_temp);
    //         fputs(szDebug, fpDebug);
    //     #endif


    }

    return cp_score_position_temp;
}




///////////////////////////////////////////////////////////////////////////////////


//
// Only returns false is if user aborts.
//
tuple<double, Move> MinimaxAI::do_a_deepening(int depth, long long elapsed_time, const Move& null_move) {

    tuple<double, Move> ret_val;

    int nPlys = 0;
    int qPlys = 0;

    top_deepening = depth;      // deepening starts at this depth
    int aspiration_tries = 0;   // safety fuse
    double widen = 0.5;         // example margin (in pawns)


    double alpha = -HUGE_SCORE;
    double beta = HUGE_SCORE;

    // assume: prevScore holds last iteration's exact root score (in pawns)
     
    double w = 0.20;                 // ≈20 centipawns
    if (0) {  // (depth > 1) {
        double prevScore;
        prevScore = prev_root_best_[depth - 1].second;   // score in pawns
        alpha = prevScore - widen;
        beta  = prevScore + widen;
    } else {
        alpha = -HUGE_SCORE;         // d == 1 → full window
        beta  =  HUGE_SCORE;
    }

    // the beast (root node of root nodes)
    bool bStillAspiring  = false;
    do {

        cout << endl << aspiration_tries << " Deeping " << depth << " ply of " << maximum_deepening
                    << " msec=" << std::setw(6) << elapsed_time << ' ';
                    

        ret_val = recursive_negamax(depth, alpha, beta
                                                , null_move
                                                , (nPlys+1)
                                                , qPlys
                                                );

        // ret_val is a tuple of the score and the move.
        double d_Return_score = get<0>(ret_val);
        if (d_Return_score == ABORT_SCORE) return ret_val;
        
        if (d_Return_score == ONLY_MOVE_SCORE) {
            return ret_val;
            //continue;
        }
        
        // Aspiration (just a guard right now)
        //assert ((alpha <= d_Return_score) && (d_Return_score <= beta));
        //printf("alpha, this, beta %f  %f  %f", alpha, d_Return_score, beta);
        assert((alpha <= d_Return_score) && (d_Return_score <= beta));
   

        // // --- What WOULD happen if the score were outside [alpha, beta] ---
        // // With an infinite window these branches are unreachable right now,
        // // but this is the template you’ll use once you narrow the window.

        // if (alpha > d_Return_score) {
        //     // Fail-low: score came in at or below alpha
        //     std::cout << "\x1b[38;2;255;165;0mfail low\x1b[0m" << std::endl;
        //     double widened = alpha - widen;
        //     alpha = (widened < -HUGE_SCORE) ? -HUGE_SCORE : widened;
        //     //double newBeta  = beta; // keep upper bound
        //     widen *= 2.0;  // widen window on fail-low

        //     // log_fail_low(depth, alpha, beta, d_Return_score, newAlpha, newBeta);
        //     // d_Return_score = search(position, depth, newAlpha, newBeta);
        //     bStillAspiring  = true;
        // }
        // else if (d_Return_score > beta) {
        //     // Fail-high: score came in at or above beta
        //     std::cout << "\x1b[38;2;255;165;0mfail high\x1b[0m" << std::endl;
        //     double widened = beta + widen;
        //     //double newAlpha = alpha; // keep lower bound
        //     beta  = (widened >  HUGE_SCORE) ?  HUGE_SCORE : widened;
        //     widen *= 2.0;  // widen window on fail-high

        //     // log_fail_high(depth, alpha, beta, d_Return_score, newAlpha, newBeta);
        //     // d_Return_score = search(position, depth, newAlpha, newBeta);
        //     bStillAspiring  = true;

        // }
        // else {

        //     // Continue with d_Return_score (exact if it fit; bound if you decide to flag it)
        //     bStillAspiring  = false;
        // }

        // aspiration_tries++;
        // if (bStillAspiring && aspiration_tries >= 5) {
        //      std::cout << "\x1b[38;2;255;165;0m\n[aspiration] giving up after 5 tries\x1b[0m\n";
        //      bStillAspiring  = false;
        //      break;
        // }


        // std::cout << "\x1b[38;2;255;165;0m[aspiration] fail "
        //         << (alpha >= d_Return_score ? "low " : "high ")
        //         << "α=" << alpha << " β=" << beta
        //         << " score=" << d_Return_score
        //         << " widen→" << widen * 2.0
        //         << "\x1b[0m\n";




    } while (bStillAspiring );




    return ret_val;

}





int g_this_depth = 6;

//////////////////////////////////////////////////////////////////////////////////
//
//   This the entry point into the C to get a minimax AI move.
//   It does "Iterative deepening". This is a "root position".
//
//////////////////////////////////////////////////////////////////////////////////
//
// This is a "root position". The next human move triggers a new root position
// timeRequested now in *milliseconds*
Move MinimaxAI::get_move_iterative_deepening(double timeRequested, int max_deepening_requested, int feat) {  
    

    using namespace std::chrono;
    auto now = high_resolution_clock::now().time_since_epoch();
    auto us  = duration_cast<microseconds>(now).count();
    std::srand(static_cast<unsigned>(us));   // reseed once per search

    cout << "\x1b[94mdept requested (ply)  =" << max_deepening_requested << "\x1b[0m" << endl;  
    cout << "\x1b[94mtime requested (msec) =" << timeRequested << "\x1b[0m" << endl;
    cout << "\x1b[94margu requested (msec) =" << feat << "\x1b[0m" << endl;

    auto start_time = chrono::high_resolution_clock::now();
    // CHANGED: interpret timeRequestedMsec as milliseconds
    auto required_end_time = start_time + chrono::duration<double, std::milli>(timeRequested);

    Features_mask = feat;

    //Move null_move = Move{};
    Move null_move = engine.users_last_move;    // Just to make the move history work?


    stop_calculation = false;

	#ifdef IS_CALLBACK_THREAD
    	start_callback_thread();
    #endif



    // if (engine.g_iMove==0) {
    //     engine.i_randomize_next_move = 1;
    // }

    if (engine.g_iMove == 0) {
        //
        // Here we do stuff to be done at the beginning of a game. Terrible way to have to detect 
        // this, but there it is.
        //
        TTable2.clear();

        NhitsTT = 0;
        NhitsTT2 = 0;
        nRandos = 0;

    }   

    engine.g_iMove++;                      // Increment real moves in whole game
    cout << "\x1b[94m\n\nMove: " << engine.g_iMove << "\x1b[0m";

    //if (engine.g_iMove) timeRequested = 1000.0;    // HAck kto allow user to hit "autplay" button before Shumi moves

    nodes_visited = 0;
    nodes_visited_depth_zero = 0;
    evals_visited = 0;

    seen_zobrist.clear();
    uint64_t zobrist_key_start = engine.game_board.zobrist_key;
    //cout << "zobrist_key at the root: " << zobrist_key_start << endl;

    // Cleared on every move
    TTable.clear();
   




    int this_deepening;

    Move best_move = {};
    double d_best_move_value = 0.0;

 



    this_deepening = engine.user_request_next_move;
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
    //bool bThinkingOverByEnding = false;

    for (int ii=0;ii<MAX_PLY;ii++) {
        killer1[ii] = {}; 
        killer2[ii] = {};
    }

    // Start each search with an empty root-move list for randomization.
    MovesFromRoot.clear();

    //engine.print_move_history_to_file(fpDebug);    // debug only
    double d_Return_score = 0.0;

    do {

        tuple<double, Move> ret_val;
        
        #ifdef _DEBUGGING_MOVE_CHAIN    // Start of a deepening
            bool bSide = (engine.game_board.turn == ShumiChess::BLACK);
            fputs("\n\n---------------------------------------------------------------------------", fpDebug);
            engine.print_move_to_file(null_move, nPlys, (GameState::INPROGRESS), false, true, bSide, fpDebug);
        #endif

        //
        // This case can happen in 50-move rule, all nodes return DRAW, so in so quickly
        // zips through these, that it runs out of depth before the time limit.
        if (depth>=MAXIMUM_DEEPENING) {
            //cout << "\x1b[31m \nOver Deepening " << depth << "\x1b[0m" << endl;
            //cout << gameboard_to_string(engine.game_board) << endl;
            break;   // Stop deepening, no more depths.
        }

        // the beast
        ret_val = do_a_deepening(depth, elapsed_time, null_move);

        d_Return_score = get<0>(ret_val);
        if (d_Return_score == ABORT_SCORE) {
            cout << "\x1b[31m Aborting depth of " << depth << "\x1b[0m" << endl;
            break;   // Stop deepening, no more depths.
        } else if (d_Return_score == ONLY_MOVE_SCORE) {
            d_best_move_value = 0.0;
            best_move = get<1>(ret_val);
            break;   // Stop deepening, no more depths.
        } else {
            d_best_move_value = d_Return_score;
            best_move = get<1>(ret_val);

            // Root sees a forced mate: no point deepening further.
            if (std::fabs(d_best_move_value) >= HUGE_SCORE/2.0)
            {
                cout << "\x1b[31m !!!!!!!! mate at exterior node (depth " << depth << ")\x1b[0m" << endl;
                break;   // Stop iterative deepening immediately.
            }

        }

        // Store PV for the *next* iteration. Called incorrectly in literture as: "PV at the root".
        assert (depth >=0);
        prev_root_best_[depth] = std::make_pair(best_move, d_best_move_value);   // move + score (pawns)

        engine.move_and_score_to_string(best_move, d_best_move_value , true);

        cout << " Best: " << engine.move_string;

        depth++;

        // CHANGED: time based ending of thinking to use milliseconds
        now_s = (long long)chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        end_s = (long long)chrono::duration_cast<chrono::milliseconds>(required_end_time.time_since_epoch()).count();
        diff_s = now_s - end_s;
        elapsed_time = now_s - (long long)chrono::duration_cast<chrono::milliseconds>(start_time.time_since_epoch()).count();

        // Endgame based ending of thinking (done to avoid overthinking engames)
        // #define MAX_DEEPENING_SIMPLE_ENDGAME 3
        // bThinkingOverByEnding = (engine.game_board.IsSimpleEndGame(engine.game_board.turn) 
        //                             && depth >= MAX_DEEPENING_SIMPLE_ENDGAME) ;

        // time based ending of thinking
        bThinkingOverByTime = (diff_s > 0);

        // depth based ending of thinking
        bThinkingOverByDepth = (depth >= (maximum_deepening+1));


        // we are done thinking if both time and depth is ended OR simple endgame is on.
        bThinkingOver = (bThinkingOverByDepth && bThinkingOverByTime);

        // This odd line
        // if (bThinkingOver && (depth >= MAXIMUM_DEEPENING-1))
        // {
        //     cout << " cheese ";
        //     bThinkingOver = false;
        // }

    } while (!bThinkingOver);

    cout << "\x1b[33m\nWent to depth " << (depth - 1)  
        << " elapsed msec= " << elapsed_time << " requested msec= " << timeRequested 
        << " TimOver= " << bThinkingOverByTime << " DepOver= " << bThinkingOverByDepth 
        << "\x1b[0m" << endl;


    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";

    // If the first move, MAYBE randomize the response some.
    if ( (d_Return_score != ABORT_SCORE) && (d_Return_score != ONLY_MOVE_SCORE) ) {

        // Reassign best move, if randomizing        
        d_random_delta = 0.0;
        #ifdef RANDOMIZING_EQUAL_MOVES
            if (engine.i_randomize_next_move>0) {
                d_random_delta = RANDOMIZING_EQUAL_MOVES_DELTA;
                engine.i_randomize_next_move--;
            }
        #endif
        if (d_random_delta != 0.0) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now().time_since_epoch()
                    ).count();

            nRandos++;
            cout << "\033[1;31m rando move \033[0m" << MovesFromRoot.size() << endl;

            best_move = pick_random_within_delta_rand(MovesFromRoot, d_random_delta);
        }

        // engine.move_into_string(best_move);
        // std::cout << engine.move_string << std::endl;
   
        // print moves and scores (debug only)
        // for ( auto& ms : MovesFromRoot) {
        //     Move& m = ms.first;
        //     double sc = ms.second;
        //     engine.move_into_string(m);
        //     std::cout << engine.move_string << " ; " << std::fixed << std::setprecision(2) << sc << "\n";
        // }
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
    cout << colorize(AColor::BRIGHT_GREEN, 
        "   nodes/sec= " + format_with_commas(std::llround(nodes_per_sec)) + 
        "   evals/sec= " + format_with_commas(std::llround(evals_per_sec))) << endl;

  


    engine.d_bestScore_at_root = d_best_move_value_abs;

    cout << "\n";

    #ifdef DEBUGGING_KILLER_MOVES
        cout << "Killers: tried=" << killer_tried
            << " cutoffs=" << killer_cutoff
            << " (" << (killer_tried ? (100.0 * killer_cutoff / killer_tried) : 0.0)
            << "%)\n\n";
    #endif


    /////////////////////////////////////////////////////////////////////////////////
    //
    // Debug only  playground   sandbox for testing evaluation functions
    // int isolanis;
    bool isOK;
    double centerness;

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
    double dTemp;
    int itemp1, itemp2, itemp3, itemp4;
    

    //iNearSquares = engine.game_board.get_king_near_squares(Color::WHITE, king_near_squares_out);
    //utemp = engine.game_board.sliders_and_knights_attacking_square(Color::WHITE, engine.game_board.square_d5);
    //utemp = engine.game_board.attackers_on_enemy_king_near(Color::WHITE);

    //int connectiveness;     // One if rooks connected. 0 if not.
    //isOK = engine.game_board.rook_connectiveness(Color::WHITE, connectiveness);
    //utemp = engine.game_board.get_material_for_color(Color::WHITE);
    //utemp = (ull)engine.game_board.is_king_in_check_new(Color::WHITE);
       // engine.repetition_table.size();    //cp_score_material_avg;
    //dTemp = engine.convert_from_CP(utemp);
    //dTemp = engine.game_board.king_near_other_king(Color::WHITE);
    //itemp = sizeof(Move);
    //dTemp = engine.game_board.distance_between_squares(engine.game_board.square_d3, engine.game_board.square_d3);
    //utemp = engine.repetition_table.size();
    //dTemp = engine.game_board.bIsOnlyKing(Color::WHITE);

    //itemp = engine.game_board.get_castle_status_for_color(Color::WHITE);
    //itemp = engine.game_board.count_doubled_pawns(Color::WHITE);
    //utemp = sizeof(Move);
    
    //itemp = engine.game_board.king_edge_weight(Color::WHITE);
    //sisOK = engine.game_board.IsSimpleEndGame(Color::WHITE);
    //utemp = Piece::NONE;
    //utemp = phaseOfGame(); 



     itemp1 = sizeof(Color);
     itemp2 = sizeof(Piece);
     itemp3 = sizeof(uint8_t);
    // itemp4 = engine.game_board.SEE(ShumiChess::WHITE, engine.game_board.square_d4);

    cout << "wht " << nFarts << endl;
    //cout << "wht " << itemp1 <<  "  " << itemp2 <<  "  " << itemp3 << endl;
    
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
    //utemp = TTable.size();
    //utemp = (ull)engine.game_board.is_king_in_check_new(Color::BLACK);
    //itemp = engine.game_board.SEE(ShumiChess::BLACK, engine.game_board.square_d5);   // TTable.size();    // (ull)(engine.game_board.insufficient_material_simple());
    // dTemp = engine.convert_from_CP(utemp);
    //dTemp = engine.game_board.distance_between_squares(engine.game_board.square_h1, engine.game_board.square_f3);
    //dTemp = engine.game_board.bIsOnlyKing(Color::BLACK);
    //dTemp = engine.game_board.king_near_other_king(Color::BLACK);
    //itemp = engine.game_board.king_edge_weight(Color::BLACK);
    //isOK = engine.game_board.IsSimpleEndGame(Color::BLACK);

    //itemp = engine.game_board.get_castle_status_for_color(Color::BLACK);
    //itemp = engine.game_board.count_doubled_pawns(Color::BLACK);

    itemp = nFarts;
    itemp = engine.i_randomize_next_move;
    cout << "blk " << itemp << endl;

    // itemp1 = engine.game_board.SEE(ShumiChess::BLACK, engine.game_board.square_d5);
    // itemp2 = engine.game_board.SEE(ShumiChess::BLACK, engine.game_board.square_e5);
    // itemp3 = engine.game_board.SEE(ShumiChess::BLACK, engine.game_board.square_e4);
    // itemp4 = engine.game_board.SEE(ShumiChess::BLACK, engine.game_board.square_d4);

    // cout << "blk " << itemp1 <<  "  " << itemp2 <<  "  " << itemp3 <<  "  " << itemp4 << endl;



    utemp = TTable.size();
    cout << "TT: " << utemp << " hits= " << NhitsTT << endl;
    utemp = TTable2.size();
    cout << "TT2: " << utemp << " hits= " << NhitsTT2 << endl;
 
    //engine.debug_print_repetition_table();

 
// printf("P = %d, N = %d, P-N = %d\n",
//        engine.game_board.centipawn_score_of(Piece::PAWN),
//        engine.game_board.centipawn_score_of(Piece::KNIGHT),
//        engine.game_board.centipawn_score_of(Piece::PAWN) - engine.game_board.centipawn_score_of(Piece::KNIGHT));




	#ifdef IS_CALLBACK_THREAD
    	stop_callback_thread();
    #endif

    return best_move;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Choose the "minimax" AI move.
// Returns a tuple of:
//    the best score (however, if this is ABORT_SCORE, its an abort)
//    the best move
//
////////////////////////////////////////////////////////////////////////////////////////////////////


tuple<double, Move> MinimaxAI::recursive_negamax(
                    int depth, double alpha, double beta
                    ,const ShumiChess::Move& move_last      // seems to be used for debug only...
                    ,int nPlys
                    ,int qPlys
                    )
{

    // Initialize return 
    double d_best_score = 0.0;
    Move the_best_move = {};
    //std::tuple<double, ShumiChess::Move> final_result;

    int cp_score_best;

    bool did_cutoff = false;    // TRUE if fail-high
    bool did_fail_low = false;  // TRUE if fail-low

    double alpha_in = alpha;   //  save original alpha window lower bound


    #ifdef  DOING_TT2_NORM_DEBUG       // debug for comparing recalled to actual
        bool   foundPos = false;
        int    foundScore = 0;
        Move   foundMove = {};
        //int    foundnPlys = 0;
        bool   foundDraw = 0;
        double foundAlpha = 0.0;
        double foundBeta  = 0.0;
        int    foundDepth = 0;
        bool   foundIsCheck = false;
        int    foundLegalMoveSize = 0;
        int    foundRepCount = 0;
        double foundRawScore = 0;

        // NEW: board snapshot from TT2 entry
        ull    found_wp = 0ULL;
        ull    found_wn = 0ULL;
        ull    found_wb = 0ULL;
        ull    found_wr = 0ULL;
        ull    found_wq = 0ULL;
        ull    found_wk = 0ULL;

        ull    found_bp = 0ULL;
        ull    found_bn = 0ULL;
        ull    found_bb = 0ULL;
        ull    found_br = 0ULL;
        ull    found_bq = 0ULL;
        ull    found_bk = 0ULL;
    #endif

        vector<Move> *p_moves_to_loop_over = 0;
        std::vector<Move> legal_moves = engine.get_legal_moves();
        p_moves_to_loop_over = &legal_moves;
        int legalMovesSize = legal_moves.size();
   



    if (Features_mask & _FEATURE_TT2) {  // probe in TT2
    //#ifdef DOING_TT2_NORM    // probe in TT2

        if (depth > 1) {
            // --- Normal TT2 probe (exact-only version, no flags/age yet)

            uint64_t key = engine.game_board.zobrist_key;

            // Salt the entry
            // unsigned mode  = salt_the_TT2(bOverFlow, nReps);
            // key ^= g_eval_salt[mode];

            auto it = TTable2.find(key);

            if (it != TTable2.end()) {
                const TTEntry2 &entry = it->second;

                // We can reuse an entry if it was searched at least as deep
                if (entry.depth >= depth) {

                    // Only accept hit if window matches stored one
                    bool windowMatches =
                        (std::fabs(entry.dAlphaDebug - alpha) <= VERY_SMALL_SCORE) &&
                        (std::fabs(entry.dBetaDebug  - beta ) <= VERY_SMALL_SCORE);

                    //bool plyMatches = (entry.nPlysDebug == nPlys);

                    if (!windowMatches) {


                    } else {
                        #ifdef DOING_TT2_NORM_DEBUG


                            foundPos   = true;
                            foundScore = entry.score_cp;
                            foundMove  = entry.best_move;
                            //foundnPlys = entry.nPlysDebug;
                            foundDraw  = entry.drawDebug;
                            foundAlpha = entry.dAlphaDebug;
                            foundBeta  = entry.dBetaDebug;
                            foundDepth = entry.depth;
                            foundIsCheck = entry.bIsInCheckDebug;
                            foundLegalMoveSize = entry.legalMovesSize;
                            foundRepCount = entry.repCountDebug;
                            foundRawScore = entry.dScoreDebug;

                            found_wp = entry.bb_wp;
                            found_wn = entry.bb_wn;
                            found_wb = entry.bb_wb;
                            found_wr = entry.bb_wr;
                            found_wq = entry.bb_wq;
                            found_wk = entry.bb_wk;

                            found_bp = entry.bb_bp;
                            found_bn = entry.bb_bn;
                            found_bb = entry.bb_bb;
                            found_br = entry.bb_br;
                            found_bq = entry.bb_bq;
                            found_bk = entry.bb_bk;


                        #else
                            double dScore = (double)entry.score_cp / 100.0;
                            return { dScore, entry.best_move };
                        #endif
                    }
                }


            }
        }
    }
    //#endif

    vector<ShumiChess::Move> unquiet_moves;   // This MUST be declared as local in this functio or horrible crashes

    nodes_visited++;
    if (depth==0) nodes_visited_depth_zero++;


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
    if (nPlys > MAX_PLY) {
        // If a draw by 50/3/insuffieceint time, then we can get in loop here, where each deepeining is only 
        // 1 msec so it runs off to many levels. 
        // 
        //cout << gameboard_to_string(engine.game_board) << endl;
        assert(0);
    }


    //if (alpha > beta) assert(0);

    
    // =====================================================================
    // Hard node-limit sentinel fuse colorize(AColor::BRIGHT_CYAN,
    // =====================================================================
    if (nodes_visited > 5.0e8) {    // 10,000,000 a good number here
        std::cout << "\x1b[31m! NODES VISITED trap#2 " << nodes_visited << "dep=" << depth << "  " << d_best_score << "\x1b[0m\n";

        return { ABORT_SCORE, the_best_move };

    }


    #ifdef _DEBUGGING_MOVE_CHAIN1
        engine.print_move_to_file(move_last, nPlys, (GameState::INPROGRESS), false, true, true, fpDebug); 
    #endif


    // Get all legal moves. If mover is in check, this routine returns all check escapes and only the check escapes.
    // std::vector<Move> legal_moves = engine.get_legal_moves();
    // p_moves_to_loop_over = &legal_moves;

    // Only one of me, per deepening.
    bool first_node_in_deepening = (top_deepening == depth);


    if (first_node_in_deepening) {
        if (legal_moves.size() == 1) {
            cout << "\x1b[94m!!!!! force !!!!!!!!!!!!!\x1b[0m" << endl;
            return {ONLY_MOVE_SCORE, legal_moves[0]};
        }
    }


    GameState state = engine.is_game_over(legal_moves);


    // =====================================================================
    // Terminal positions (game over)
    // =====================================================================
    if (state != GameState::INPROGRESS) {

        int level = (top_deepening - depth);
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
            //engine.reason_for_draw = "stalemate";
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
            //if (nPlys > 15) // because I said so    (14 or so)
            {
                //cout << " beeep ";
                bFast = true;
            }
        #endif

        bool b_is_Quiet = !engine.has_unquiet_move(legal_moves);
        int nPhase = phaseOfGame(); 

        int  cp_from_tt   = 0;
        bool have_tt_eval = false;

        // memoization of leafs
        if (Features_mask & _FEATURE_TT) {
            // Salt the entry
            unsigned mode  = salt_the_TT(b_is_Quiet, nPhase);

            uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

            // Look for the entry in the TT
            auto it = TTable.find(evalKey);
            if (it != TTable.end()) {
                const TTEntry &entry = it->second;
                cp_from_tt   = entry.score_cp;
                have_tt_eval = true;
            }
        }
        //#endif

        //
        // evaluate (main call)
        //
        if (have_tt_eval) {
            TT_ntrys++;
            #ifdef DOING_TT_EVAL_DEBUG
                cp_score_best = evaluate_board(engine.game_board.turn, nPhase, bFast, b_is_Quiet);
                if (cp_from_tt != cp_score_best) {
                    printf ("burp (MAIN) %ld %ld      %ld\n", cp_from_tt, cp_score_best, TT_ntrys);
                    assert(0);
                } else {
                    NhitsTT++;
            }
            #endif
            cp_score_best = cp_from_tt;

        }
        else {
            cp_score_best = evaluate_board(engine.game_board.turn, nPhase, bFast, b_is_Quiet);
        }


        d_best_score = engine.convert_from_CP(cp_score_best);
        
        // memoization
        if (Features_mask & _FEATURE_TT) {
            if (!bFast) {

                // Salt the entry
                unsigned mode  = salt_the_TT(b_is_Quiet, nPhase);

                uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

                 // Store this position away into the TT
                TTEntry &slot = TTable[evalKey];
                slot.score_cp = cp_score_best;   // or cp_score, whatever you just got
                slot.movee    = the_best_move;   // or bestMove, etc.
                slot.depth    = top_deepening;
            }
        }
        //endif

        in_check = engine.is_king_in_check(engine.game_board.turn);

        if (in_check) {
            // In check: use all legal moves, since by definition (see get_legal_moves() the set of all legal moves is equivnelent 
            // to the set of all check escapes. By definition.
            //moves_to_loop_over = legal_moves;  // not needed as its done ealier above. Sorry.
            //assert(!unquiet_moves.empty());  // oTherwise we are in check mate, and that would be caught earlier. 
    
        } else {

            // Obtain moves to use in the limited search. (Quiescence)
            unquiet_moves.reserve(MAX_MOVES);
            engine.reduce_to_unquiet_moves_MVV_LVA(legal_moves, qPlys, unquiet_moves);

            // If quiet (not in check & no tactics), just return stand-pat
            if (unquiet_moves.empty()) {
                return { d_best_score, Move{} };
            
            } else {
                // So "d_best_score" is my "stand pat value."
            
                // Stand-pat cutoff
                if (d_best_score >= beta) {
                    return { d_best_score, Move{} };
                }
                alpha = std::max(alpha, d_best_score);

                // Extend on captures/promotions only
                p_moves_to_loop_over = &unquiet_moves; 

            }

        }


		// Pick best static evaluation among all legal moves if hit the over ply
        if (qPlys >= MAX_QPLY) {
            //std::cout << "\x1b[31m! MAX_QPLY trap " << nPlys << "\x1b[0m\n";
            //std::cout << "\x1b[31m!" << "\x1b[0m";
            // auto tup = best_move_static(engine.game_board.turn, (*p_moves_to_loop_over), in_check, depth, bFast);
            // double scoreMe = std::get<0>(tup);
            // ShumiChess::Move moveMe = std::get<1>(tup);
            
            nFarts++;
           
            // debug
            //engine.print_move_history_to_file(fpDebug);

            //return { scoreMe, moveMe };
            return { d_best_score, Move{} };
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
        // Fascinating tradeoff. We could also call this if depth==0 and in check.
        // On one hand why not, becasuse there could be a lot of responses, But on 
        // the otherhand there wont be that many. ANf we must be super fast here.
        //if ( (depth > 0) || (depth==0 && in_check) ) {
        if (depth > 0) {
            assert(p_moves_to_loop_over == &legal_moves);
  
            #ifdef _DEBUGGING_MOVE_SORT
                vector<Move> tempMovs = *p_moves_to_loop_over;
            #endif

            bool is_top_of_deepening = (depth == top_deepening);

            sort_moves_for_search(p_moves_to_loop_over, depth, nPlys, is_top_of_deepening);

            #ifdef _DEBUGGING_MOVE_SORT
                if (tempMovs != *p_moves_to_loop_over) {
                    print_moves_to_file(tempMovs, depth, "pre", "\n");
                    print_moves_to_file(*p_moves_to_loop_over, depth, "pst", "\n");
                }
            #endif


        } else {
            // In depth==0 (Quiescence) and not in check)
        }

        d_best_score = -HUGE_SCORE;
        the_best_move = sorted_moves[0];   // Note: Is this the correct intialization? Why not HUGE_SCORE

        for (const Move& m : sorted_moves) {

            #ifdef _DEBUGGING_PUSH_POP
                std::string temp_fen_before = engine.game_board.to_fen();
                ull zobrist_save = engine.game_board.zobrist_key;
                auto ep_history_saveb = engine.game_board.black_castle_rights;
                auto ep_history_savew = engine.game_board.white_castle_rights;
                auto enpassant_save = engine.game_board.en_passant_rights;
            #endif

            #ifdef _DEBUGGING_MOVE_CHAIN    // Print move we are going to analyze
                bSuppressOutput = false;
                bool bSide = (engine.game_board.turn == ShumiChess::WHITE);
                int ichars = engine.print_move_to_file(m, nPlys, (GameState::INPROGRESS), false, true, bSide, fpDebug); 
            #endif


            // Delta pruning (in qsearch (Quiescence) at depth==0) estimates the most this capture/promotion could possibly 
            // improve the current stand-pat score (including material swing and a safety margin), and if 
            // even that optimistic bound still can't raise alpha, it just skips searching that move as futile. 
            #ifdef DELTA_PRUNING
                // --- Delta pruning (qsearch (Quiescence) only)
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
                    int cp_pawn = engine.game_board.centipawn_score_of(ShumiChess::Piece::PAWN);
                    if (!recapture &&
                        (m.capture == ShumiChess::Piece::PAWN) && (cp_best_score + cp_pawn + 60) < cp_alpha) {
                        continue;   // skip this move its futile
                    }
                }
                // --- end Delta pruning
            #endif
    
            bool is_killer_here = false;
            #ifdef DEBUGGING_KILLER_MOVES
                if (Features_mask & _FEATURE_KILLER) {
                    is_killer_here = (m == killer1[nPlys]) || (m == killer2[nPlys]);
                    if (is_killer_here) killer_tried++;
                }
            #endif


            assert(m.piece_type != Piece::NONE);
            engine.pushMove(m);
               
            #ifdef IS_CALLBACK_THREAD
            	g_live_ply = nPlys;
            #endif

            ++engine.repetition_table[engine.game_board.zobrist_key];

            //
            // Two parts in negamax: 1. "relative scores", the alpha betas are reversed in sign,
            //                       2. The beta and alpha arguments are staggered, or reversed. 
            auto ret_val = recursive_negamax(
                (depth > 0 ? depth - 1 : 0),                // Refuse to pass on negative depth
                -beta, -alpha,      // reverse in sign and order at the same time
                m, 
                (nPlys+1),
                (depth == 0 ? qPlys+1 : qPlys)
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

            bool isRoot = ( (top_deepening == maximum_deepening) && (nPlys == 1) );
            if (isRoot) {

                #ifdef _DEBUGGING_TO_FILE1
                    fprintf(fpDebug,
                            "PUSH_ROOT depth=%d top=%d max=%d nPlys=%d : ",
                            depth, top_deepening, maximum_deepening, nPlys);
                    engine.print_move_and_score_to_file(
                        {m, d_score_value}, false, fpDebug);
                #endif

                MovesFromRoot.emplace_back(m, d_score_value);  // record ROOT move & its (negamax) score
            }


            #ifdef _DEBUGGING_MOVE_CHAIN    // Print negamax (relative) score of move

                if (!bSuppressOutput) {
                    //if (d_score_value<0.0) ichars++;
                    fprintf(fpDebug, "%*s", (8-ichars), "");  // to line up for varing move size

                    fprintf(fpDebug, "%8.3f", d_score_value);
                    bSuppressOutput = true;
                }

                // int nChars = fputs(szDebug, fpDebug);
                // if (nChars == EOF) assert(0);
            #endif

            // Store score
            //std::string temp_fen_now = engine.game_board.to_fen();
            //move_scores_table[temp_fen_now][m] = d_score_value;

            // record (move, score)
            //move_and_scores_list.emplace_back(m, d_score_value);

            // CAUTION! THIS IS BROKEN CODE. DO NOT TOUCH
             // Note: this should be done as a real random choice. (random over the moves possible). 
            // This dumb approach favors moves near the end of the list
   
            // #ifdef RANDOMIZING_EQUAL_MOVES
            // Only randomize near-equal scores at the top level
            //bool is_at_bottom_root = ((top_deepening - depth) == 0);    // (depth == maximum_deepening) && 
            //bool is_at_end_of_the_line = false;    //( (engine.g_iMove==1) );
            //assert (!is_at_bottom_root);

            //if (is_at_end_of_the_line) cout << "spud";

            //double delta_score = (is_at_end_of_the_line ? 0.1 : 0.0);
            double delta_score = 0.0;     // personalities

            double d_difference_in_score = std::fabs(d_score_value - d_best_score);


            #ifdef RANDOM_FLIP_COIN
                //bool bUseMe = d_difference_in_score <= (delta_score + VERY_SMALL_SCORE);
                bool bUseMe = d_difference_in_score <= (delta_score);
            #else
                bool bUseMe = false;
            #endif

            if (bUseMe) {
                
                // tie (within delta): flip a coin.
                b_use_this_move = engine.flip_a_coin();

                //cout << d_score_value << " = " << d_best_score << " , " << b_use_this_move << endl;     

                #ifdef _DEBUGGING_MOVE_CHAIN1
                    //sprintf (szDebug, "flip %ld", b_use_this_move);
                    fputs(szDebug, fpDebug);
                #endif

  
            } else {
                b_use_this_move = (d_score_value > d_best_score);
            }

            if (b_use_this_move) {
                d_best_score = d_score_value;
                the_best_move = m;
            }

            // Think of alpha as “best score found so far at this node.”
            alpha = std::max(alpha, d_best_score);

            // Alpha/beta "cutoff", (fail-high), break the analysis
            // You just found a move so good that the opponent would never 
            // allow this position, because it already exceeds what they could tolerate.
            //if (alpha >= beta + TINY_SCORE) {
            if (alpha >= beta) {

                // You found a move so good that the opponent would never allow this position.
                // Therefore, further searching at this node is pointless.
                // You stop searching siblings and immediately return beta upward.

                did_cutoff = true;

                #ifdef DEBUGGING_KILLER_MOVES
                    if (Features_mask & _FEATURE_KILLER) {
                        if (is_killer_here) killer_cutoff++;
                    }
                #endif


                if ((depth > 0) && (!engine.is_unquiet_move(m))) {
                    // Quiet moves that "cut off" are "notable", or "killer moves"
                    if (killer1[nPlys] == ShumiChess::Move{}) {
                        killer1[nPlys] = m;
                        #ifdef DEBUGGING_KILLER_MOVES1
                            engine.move_into_string(killer1[nPlys]);
                            fprintf(fpDebug, "\nkiller1-> %s\n", engine.move_string.c_str());
                            
                            engine.print_move_history_to_file(fpDebug);
                        #endif
                    }
                    else if (!(m == killer1[nPlys])) {
                        killer2[nPlys] = m;
                    }
                }

                #ifdef _DEBUGGING_MOVE_CHAIN3
                    char szTemp[64];
                    sprintf(szTemp, " Beta cutoff %f %f",  alpha, beta);
                    fputs(szTemp, fpDebug);

                    if (!engine.is_unquiet_move(m)){

                        char szTemp[64];
                        sprintf(szTemp, " Beta quiet cutoff %f %f",  alpha, beta);
                        fputs(szTemp, fpDebug);
                        engine.print_move_to_file(m, nPlys, state, false, false, false, fpDebug);
                    }
                #endif

                break;
            }

        }   // End loop over all moves to look at
    }


    // Fail-low
    if (!did_cutoff && alpha <= alpha_in) {
    //if (!did_cutoff  && (alpha <= alpha_in + TINY_SCORE)) {
        did_fail_low = true;
    }

    if (Features_mask & _FEATURE_TT2) {  // store in TT2
    //#ifdef DOING_TT2_NORM    // store in TT2
        if (depth > 1) {
            if (!did_cutoff && !did_fail_low) {  // only store EXACT results for now

                uint64_t key = engine.game_board.zobrist_key;
                //unsigned mode  = salt_the_TT2(bOverFlow, nReps);

                //key = key ^ g_eval_salt[mode];

                int cp_score_temp = engine.convert_to_CP(d_best_score);

                // Rolling size cap for TT2  (2 million?)
                static const std::size_t MAX_TT2_SIZE = 2000000;
                if (TTable2.size() >= MAX_TT2_SIZE) {
                    auto it = TTable2.begin();
                    if (it != TTable2.end()) {
                        TTable2.erase(it);
                    }
                }

                // --- DEBUG check
                #ifdef DOING_TT2_NORM_DEBUG
                {
                    //if (foundPos) {
                    if (foundPos && (foundDepth == depth) ) {            

                        bool bBothScoresMates = (IS_MATE_SCORE(foundRawScore) && IS_MATE_SCORE(d_best_score));

                        if (!bBothScoresMates) {

                            int idelta;
                            int avgScore_cp = abs((foundScore + cp_score_temp)) / 2;
                        
                            //delta = BURP2_THRESHOLD_CP;
                            idelta = BURP2_THRESHOLD_CP + (avgScore_cp / 9); 
                            if ( abs(foundScore - cp_score_temp) > idelta) {

                                bool isFailure = true;
                                int repNow = 0;
                                auto itRepNow = engine.repetition_table.find(key);
                                if (itRepNow != engine.repetition_table.end()) repNow = itRepNow->second;

                                cout << endl << NhitsTT2 << " " << " burp2 " 
                                    << foundScore  << " = "  << cp_score_temp << "    " 
                                    << foundRawScore << " = " << d_best_score << "    "
                                    << endl;

                                print_mismatch(cout, "al", foundAlpha,         alpha_in);
                                print_mismatch(cout, "bt", foundBeta,          beta);
                                print_mismatch(cout, "dr", foundDraw,          (state == GameState::DRAW));
                                print_mismatch(cout, "ck", foundIsCheck,       in_check);
                                print_mismatch(cout, "lm", foundLegalMoveSize, legalMovesSize);
                                print_mismatch(cout, "rp", foundRepCount,      repNow);

                                print_mismatch(cout, "wp", found_wp, engine.game_board.white_pawns);
                                print_mismatch(cout, "wn", found_wn, engine.game_board.white_knights);
                                print_mismatch(cout, "wb", found_wb, engine.game_board.white_bishops);
                                print_mismatch(cout, "wr", found_wr, engine.game_board.white_rooks);
                                print_mismatch(cout, "wq", found_wq, engine.game_board.white_queens);
                                print_mismatch(cout, "wk", found_wk, engine.game_board.white_king);

                                print_mismatch(cout, "bp", found_bp, engine.game_board.black_pawns);
                                print_mismatch(cout, "bn", found_bn, engine.game_board.black_knights);
                                print_mismatch(cout, "bb", found_bb, engine.game_board.black_bishops);
                                print_mismatch(cout, "br", found_br, engine.game_board.black_rooks);
                                print_mismatch(cout, "bq", found_bq, engine.game_board.black_queens);
                                print_mismatch(cout, "bk", found_bk, engine.game_board.black_king);


                                cout << "  depth-> " << depth << " mv->" << engine.g_iMove 
                                << " dbg->" << bBothScoresMates << " idelta-> " << idelta 
                                << " , " << abs(foundScore - cp_score_temp)
                                << endl;
                                
                                string out = gameboard_to_string2(engine.game_board);
                                cout << out << endl;

                                //isFailure = true;
                                char* pszTemp = ""; 
                                (engine.game_board.turn == ShumiChess::WHITE) ? pszTemp = "WHITE to move" : pszTemp = "BLACK to move";
                                cout << pszTemp << endl;

                                string mv_string1;
                                string mv_string2;
                                engine.bitboards_to_algebraic(engine.game_board.turn, foundMove, (GameState::INPROGRESS)
                                                , false, false, mv_string1);
                                engine.bitboards_to_algebraic(engine.game_board.turn, the_best_move, (GameState::INPROGRESS)
                                                , false, false, mv_string2);
                                cout << mv_string1 << " = " << mv_string2 << endl;

                                if (!(foundMove == the_best_move)) {
                                    Move deadmove =  {};
                                    bool isEmpty = (the_best_move == deadmove);
                                    cout << " burp2m " << (int)foundMove.piece_type  << " = "  << (int)the_best_move.piece_type << "  " << isEmpty << " "  
                                        << NhitsTT2 << " " << endl;

                                    isFailure = true;
                                }

                                if (isFailure) assert(0);
                            }
                            else {
                                NhitsTT2++;
                            }
                        }
                    }
                }
                #endif
                if (!did_cutoff && !did_fail_low) {
                    // --- Always store (even in DEBUG)
                    TTEntry2 &slot = TTable2[key];
                    slot.score_cp  = cp_score_temp;
                    slot.best_move = the_best_move;
                    slot.depth     = depth;

                    slot.dAlphaDebug = alpha_in;
                    slot.dBetaDebug = beta;

                    #ifdef DOING_TT2_NORM_DEBUG
                        //slot.nPlysDebug = nPlys;
                        slot.drawDebug = (state == GameState::DRAW);
                        slot.bIsInCheckDebug = in_check;
                        slot.legalMovesSize = legalMovesSize;

                        int repNow = 0;
                        auto itRepNow = engine.repetition_table.find(key);
                        if (itRepNow != engine.repetition_table.end()) repNow = itRepNow->second;
                        slot.repCountDebug = repNow;

                        slot.dScoreDebug = d_best_score;


                        slot.bb_wp = engine.game_board.white_pawns;
                        slot.bb_wn = engine.game_board.white_knights;
                        slot.bb_wb = engine.game_board.white_bishops;
                        slot.bb_wr = engine.game_board.white_rooks;
                        slot.bb_wq = engine.game_board.white_queens;
                        slot.bb_wk = engine.game_board.white_king;

                        slot.bb_bp = engine.game_board.black_pawns;
                        slot.bb_bn = engine.game_board.black_knights;
                        slot.bb_bb = engine.game_board.black_bishops;
                        slot.bb_br = engine.game_board.black_rooks;
                        slot.bb_bq = engine.game_board.black_queens;
                        slot.bb_bk = engine.game_board.black_king;
                    #endif


                }

            }
        }
    }
    //#endif


    #ifdef _DEBUGGING_MOVE_CHAIN   // Print move summary. best move and best score
        int nChars;
        bool bSide = (engine.game_board.turn == ShumiChess::BLACK);

        engine.print_move_to_file(move_last, nPlys-1, (GameState::INPROGRESS), false, true, bSide, fpDebug);

        nChars = fputs(" BST=", fpDebug);
        if (nChars == EOF) assert(0);

        bSide = !bSide;
        engine.print_move_to_file(the_best_move, -2, (GameState::INPROGRESS), false, false, bSide, fpDebug);
   
        sprintf(szDebug, "%8.3f", -d_best_score);
        nChars = fputs(szDebug, fpDebug);
        if (nChars == EOF) assert(0);

        nChars = fputc('\n', fpDebug);
        if (nChars == EOF) assert(0);

        bSuppressOutput = true;

    #endif

    return {d_best_score, the_best_move};
}

//
//  Resort moves in this order (they will later be searched in this order):
//      1. PV (prevous deepenings best).
//      2. unquiet moves (captures/promotions, sorted by MVV-LVA, and "last square")
//      3. killer moves (quiet, bubbled to the front of the "cutoff" quiet slice).
//      4. remaining quiet moves.
//  Sorts moves in place.
//  I can be called at any depth, for depth==0 (Quiescence) or depth>0.
//
void MinimaxAI::sort_moves_for_search(std::vector<ShumiChess::Move>* p_moves_to_loop_over   // input/output
                            , int depth, int nPlys, bool is_top_of_deepening)
{
    assert(p_moves_to_loop_over);
    assert(depth>0);

    auto& moves = *p_moves_to_loop_over;
    if (moves.empty()) return;

    const bool have_last = !engine.move_history.empty();
    const ull  last_to   = have_last ? engine.move_history.top().to : 0ULL;

    //       1. PV from the previous iteration (previous deepening’s best). 
    if (is_top_of_deepening) 
    {
        assert(top_deepening > 0);

        const ShumiChess::Move pv_move = prev_root_best_[top_deepening - 1].first;

        for (size_t i = 0; i < moves.size(); ++i) {
            if (moves[i] == pv_move) {  // if this move is the previous best move (from previous deepenings root)
                if (i != 0)
                    std::swap(moves[0], moves[i]);
                break;
            }
        }
    }
    //      2. unquiet moves (captures/promotions, sorted by MVV-LVA).  Captures more importent than promotions.
    //         If capture to the "from" square of last move, give it higher priority.
    //      3. killer moves (quiet, bubbled to the front of the "cutoff" quiet slice)
    //      4. remaining quiet moves.
    //
    if (Features_mask &_FEATURE_UNQUIET_SORT) {
    //#ifdef UNQUIET_SORT
        // --- 1. Partition unquiet moves (captures/promotions) to the front ---
        auto it_split = std::partition(
            moves.begin(), moves.end(),
            [&](const ShumiChess::Move& mv)
            {
                return engine.is_unquiet_move(mv);
            });

        // OLD code (no SEE_for_capture())
        // --- 2. Sort the unquiet prefix using MVV-LVA ---
        // std::sort(moves.begin(), it_split,
        //     [&](const ShumiChess::Move& a, const ShumiChess::Move& b)
        //     {
        //         int keyA = (a.capture != ShumiChess::Piece::NONE) ? engine.mvv_lva_key(a) : 0;
        //         int keyB = (b.capture != ShumiChess::Piece::NONE) ? engine.mvv_lva_key(b) : 0;
                
        //         // If capture to the "from" square of last move, give it higher priority
        //         if (have_last && a.to == last_to) keyA += 800;
        //         if (have_last && b.to == last_to) keyB += 800;
        //         return keyA > keyB;
        //     });

        // --- 2. Sort the unquiet prefix using MVV-LVA ---
        std::sort(moves.begin(), it_split,
            [&](const ShumiChess::Move& a, const ShumiChess::Move& b)
            {
                // MVV-LVA  Most Valuable Victim, Least Valuable Attacker: prefer taking the 
                // biggest victim with the smallest attackers.
                // This establishs two top "tiers", Victim and -attacker.
                int keyA = (a.capture != ShumiChess::Piece::NONE) ? engine.mvv_lva_key(a) << 10 : 0;
                int keyB = (b.capture != ShumiChess::Piece::NONE) ? engine.mvv_lva_key(b) << 10 : 0;

                // SEE: strongly penalize obviously losing captures
                // This is the third tier", as in SEE is the third teir (centipawns)
                if (a.capture != ShumiChess::Piece::NONE)
                {
                    int seeA = engine.game_board.SEE_for_capture(engine.game_board.turn, a, nullptr);
                    if (seeA < 0) keyA += seeA * 100;   // negative pulls it way downc in the sort
                }
                if (b.capture != ShumiChess::Piece::NONE)
                {
                    int seeB = engine.game_board.SEE_for_capture(engine.game_board.turn, b, nullptr);
                    if (seeB < 0) keyB += seeB * 100;
                }

                // If capture to the "from" square of last move, give it higher priority
                // Fourth tier, so by default these are in centipawns ( as in 800 centipawns).
                if (have_last && a.to == last_to) keyA += 800;
                if (have_last && b.to == last_to) keyB += 800;

                return keyA > keyB;
            });

        if (Features_mask & _FEATURE_KILLER) {
        //#ifdef KILLER_MOVES
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

            #ifdef DEBUGGING_KILLER_MOVES1
                engine.move_into_string(killer1[nPlys]);
                fprintf(fpDebug, " killer1-> %s\n", engine.move_string.c_str());
                engine.print_move_history_to_file(fpDebug);
            #endif

            bring_front(killer2[nPlys]);

        }
        //#endif

    }
    //#endif
  
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
    GameState state = engine.is_game_over(moves);
    
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
        int cp_score =  evaluate_board(color_perspective, 0, false, false) * color_multiplier;

        double d_best_score = engine.convert_from_CP(cp_score);

        return d_best_score;

    }
    //
    // Otherwise dive down one more ply.
    //
    if (color_multiplier == 1) {  // Maximizing player
        double dMax_move_value = -DBL_MAX;
        for (Move& m : moves) {

            assert(m.piece_type != Piece::NONE);
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

            assert(m.piece_type != Piece::NONE);
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
        assert(m.piece_type != Piece::NONE);
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




// Loop over all passed moves, find the best move by static evaluation.
// Returns a tuple of: best_score and best move
std::tuple<double, ShumiChess::Move>
MinimaxAI::best_move_static(ShumiChess::Color for_color,
                            const std::vector<ShumiChess::Move>& legal_moves,
                            //int nPly,
                            bool in_Check,
                            int depth,
                            bool bFast)
{
    double d_best_pawns;
    ShumiChess::Move bestMove = ShumiChess::Move{};


    // If there are no moves:
    // - not in check: return a single static (stand-pat) eval
    // - in check: treat as losing (no legal escapes here)
    if (legal_moves.empty()) {
        if (!in_Check) {

            bool b_is_Quiet = !engine.has_unquiet_move(legal_moves);
            int nPhase = phaseOfGame(); 

            int cp_score = evaluate_board(for_color, nPhase, bFast, b_is_Quiet);         // positive is good for 'color'
           
            double best_score = engine.convert_from_CP(cp_score);

            return { best_score, ShumiChess::Move{} };
        }
        return { -HUGE_SCORE, ShumiChess::Move{} };
    }

    d_best_pawns = -HUGE_SCORE;
    int cp_score_best = 0;

    for (const auto& m : legal_moves) {

        assert(m.piece_type != Piece::NONE);
        engine.pushMove(m);

        // memoization

        bool b_is_Quiet = !engine.has_unquiet_move(legal_moves);
        int nPhase = phaseOfGame(); 

        int  cp_from_tt   = 0;
        bool have_tt_eval = false;

        #ifdef DOING_TT_EVAL2
            // Salt the entry
            unsigned mode  = salt_the_TT(b_is_Quiet, nPhase);

            uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

            // Look for the entry in the TT
            auto it = TTable.find(evalKey);
            if (it != TTable.end()) {
                const TTEntry &entry = it->second;
                cp_from_tt   = entry.score_cp;
                have_tt_eval = true;
            }
        #endif
        //
        // evaluate (side call)
        //
        if (have_tt_eval) {
            TT_ntrys1++;
            #ifdef DOING_TT_EVAL_DEBUG
                cp_score_best = evaluate_board(engine.game_board.turn, nPhase, bFast, b_is_Quiet);
                if (cp_from_tt != cp_score_best) {
                    printf ("burp SIDE %ld %ld      %ld\n", cp_from_tt, cp_score_best, TT_ntrys);
                    assert(0);
                }
            #endif
            cp_score_best = cp_from_tt;

        }
        else {
            cp_score_best = evaluate_board(engine.game_board.turn, nPhase, bFast, b_is_Quiet);
        }



        double d_score = engine.convert_from_CP(cp_score_best);
    
        #ifdef DOING_TT_EVAL2
            if (!bFast) {
                // Salt the entry
                unsigned mode  = salt_the_TT(b_is_Quiet, nPhase);

                uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

                 // Store this position away into the TT
                TTEntry &slot = TTable[evalKey];
                slot.score_cp = cp_score_best;   // or cp_score, whatever you just got
                slot.movee    = Move{};
                slot.depth    = top_deepening;
            }
        #endif

        engine.popMove();

        if (d_score > d_best_pawns) {
            d_best_pawns = d_score;
            bestMove = m;
        }
    }

    return { d_best_pawns, bestMove };
}



//
// Picks a random root move within delta (in pawns) of the best score (negamax-relative).
// If list is empty: returns default Move{}.
// If best is "mate-like": returns the best (no randomization).
Move MinimaxAI::pick_random_within_delta_rand(std::vector<std::pair<Move,double>>& MovsFromRoot,
                                              double delta_pawns)
{

    #ifdef _DEBUGGING_TO_FILE
        fprintf(fpDebug, "\n-===========================================\n");

        #ifdef DEBUGGING_RANDOM_DELTA
            engine.print_moves_and_scores_to_file(MovsFromRoot, false, fpDebug);
        #endif

        fprintf(fpDebug, "==================================================================\n");
    #endif


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
    const int pick = engine.rand_int(0, static_cast<int>(n_top) - 1);
    return MovsFromRoot[static_cast<size_t>(pick)].first;
}



/////////////////////////////////////////////////////////////////////



#ifdef _DEBUGGING_MOVE_SORT


void MinimaxAI::print_moves_to_file(const vector<Move> &mvs, int depth, char* szHeader, char* szTrailer) {

    if (szHeader != NULL) {int ierr = fprintf(fpDebug, szHeader);}
    
     for (const Move &m : mvs) {
        engine.print_move_to_file(m, depth, (GameState::INPROGRESS), false, true, false, fpDebug);
     }

    if (szTrailer != NULL) {int ierr = fprintf(fpDebug, szTrailer);}

}



#endif



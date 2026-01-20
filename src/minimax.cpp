
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

#include <stdio.h>

#ifdef _WIN32
  #include <io.h>     // _fileno, _chsize (MSVC/MinGW)
#else
  #include <unistd.h> // fileno, ftruncate (POSIX)
#endif


#include <globals.hpp>
#include "utility.hpp"
#include "minimax.hpp"
#include "salt.h"
#include "features.hpp"





using namespace std;
using namespace ShumiChess;
using namespace utility;
using namespace utility::representation;
using namespace utility::bit;


#undef NDEBUG
//#define NDEBUG         // Define (uncomment) this to disable asserts
#include <assert.h>
//
// NOTE: fix these
// These are still #defines when they should be features, or discarded:
//#define RANDOM_FLIP_COIN
//#define FAST_EVALUATIONS

/////////// Debug ////////////////////////////////////////////////////////////////////////////////////

// Debug   THESE SHOULD BE OFF. They are only for debugging

//#define _DEBUGGING_PUSH_POP

//#define _DEBUGGING_TO_FILE         // I must be defined to use either of the below
//#define _DEBUGGING_MOVE_CHAIN
//#define _DEBUGGING_MOVE_SORT

// extern bool bMoreDebug;
// extern string debugMove;

//#define DEBUGGING_RANDOM_DELTA

//#define DOING_TT_EVAL2       // used only in best_move_static (should be extinct)
//#define DEBUG_LEAF_TT

// #define DEBUG_NODE_TT2          // I must also be defined in the .hpp file to work
// #define BURP2_THRESHOLD_CP 2     // "burps" or fails if the stored (TT) does not match the evaluaton made.

//#define DEBUGGING_KILLER_MOVES 

bool global_debug_flag = 0;

#ifdef _DEBUGGING_TO_FILE   // Data used for debug
    FILE *fpDebug = NULL;
    char szDebug[512];
    bool bSuppressOutput = false;
    double dSupressValue = 0.0;

    static int clear_file_keep_fp(FILE *fp)
    {
        if (!fp) return -1;
        fflush(fp);
        #ifdef _WIN32
            int fd = _fileno(fp);
            if (fd < 0) return -1;
            if (_chsize(fd, 0) != 0) return -1;
        #else
            int fd = fileno(fp);
            if (fd < 0) return -1;
            if (ftruncate(fd, 0) != 0) return -1;
        #endif

        rewind(fp);   // same as fseek(fp, 0, SEEK_SET)
        clearerr(fp); // optional: clears EOF/error flags
        return 0;
    }

#endif

// Obviously TT2 debug breaks repeatibility if we flip coins in the anaylisis. So what. 
// Also these other features (below) breaks repeatability.
#ifndef DEBUG_NODE_TT2
    #undef RANDOM_FLIP_COIN
#endif

//#ifndef DEBUG_NODE_TT2
//    #undef FAST_EVALUATIONS
//#endif

#ifdef DEBUG_NODE_TT2
    static void print_mismatch(std::ostream& os,
                            const char* label,
                            int found,
                            int actual) {
        if (found != actual) {
            os << " " << label << " " << found << " = " << actual << "\n";
        }
    }
#endif

// Only randomizes a small amount a list formed on the root node, when at maxiumum deepening, AND 
// on first move.
#define RANDOMIZING_EQUAL_MOVES_DELTA 0.15       // In units of pawns

#ifdef DEBUGGING_KILLER_MOVES
    static long long killer_tried = 0;
    static long long killer_cutoff = 0;
#endif

//////////// Displays ////////////////////////////////////////////////////////////

#define DISPLAY_DEEPING

#define DISPLAY_PULSE_CALLBACK_THREAD    // Uncomment to enable the callback to show "nPly", real time.
#ifdef DISPLAY_PULSE_CALLBACK_THREAD
    #include <thread>
    #include <cstdio>

    static std::atomic<int> g_live_ply{0};   // callback prints

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

    cout << "\033[1;34mNew Match\033[0m" << endl;

    // Initialize storage buffers (they live here to avoid extra allocation during the game)

    TTable.clear();
    TTable.reserve(10000000);    // NOTE: What size here?
    
    TTable2.clear();
    TTable2.reserve(1000000);    // NOTE: What size here?


    // add the current position
    //engine.repetition_table.clear();
    //engine.repetition_table[key_now] = 1;

    uint64_t key_now = engine.game_board.zobrist_key;
    engine.key_stack.push_back(engine.game_board.zobrist_key);
    // if (move_is_irreversible) {
    //     boundary_stack.push_back((int)key_stack.size() - 1);
    // }

    // Set default features
    Features_mask = _DEFAULT_FEATURES_MASK;

    // Open a file for debug writing
    #ifdef _DEBUGGING_TO_FILE   // open debug file
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

    #ifndef DEBUG_NODE_TT2
        Features_mask &= ~_FEATURE_DELTA_PRUNE;
    #endif


}



MinimaxAI::~MinimaxAI() { 
    #ifdef _DEBUGGING_TO_FILE   // close debug file
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

int MinimaxAI::evaluate_board(Color for_color, EvalPersons evp, bool isQuietPosition
                                //, bool is_debug
                                //const std::vector<ShumiChess::Move>* pLegal_moves  // may be nullptr
                            )
                                
{

    #ifdef _DEBUGGING_MOVE_CHAIN    // Start of an evaluation
        //if (engine.move_history.size() > 7) {
        //if (look_for_king_moves()) {
        //if (has_repeated_move()) {
        //if (alternating_repeat_prefix_exact(1)) {
            //engine.print_move_history_to_file(fpDebug);    // debug only
        //}
        fputs("  e=", fpDebug);
    #endif

    evals_visited++;

    int cp_score_adjusted = 0;  // Total score, centipawns.

    //
    // Material considerations only
    //
    int mat_cp_white = 0;
    int mat_cp_black = 0;
    //int pawns_cp_white = 0;
    //int pawns_cp_black = 0;

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
            //pawns_cp_white = cp_pawns_only_temp;
        } else {
            mat_cp_black = cp_score_mat_temp;
            //pawns_cp_black = cp_pawns_only_temp;
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

    //int mat_np_white = mat_cp_white - pawns_cp_white;
    //int mat_np_black = mat_cp_black - pawns_cp_black;
    //assert(mat_np_white>=0);
    //assert(mat_np_black>=0);
    //cp_score_material_NP_avg = (mat_np_white + mat_np_black) / 2;   // NEW
    //if (is_debug) printf("\nmat: %ld", cp_score_material_all);

    // ??? Note: display only
    engine.material_centPawns = cp_score_material_all;


    int nPhase = phaseOfGame(cp_score_material_avg); 

    //
    // Positional considerations only
    //
    int cp_score_position = 0;
    bool isOK;
    int test;

 

    // if either side has "only a king", opening/middle terms are skipped for both WHITE and BLACK.

    for (const auto& color : array<Color, 2>{Color::WHITE, Color::BLACK}) {

        int cp_score_position_temp = 0;        // positional considerations only

        Color enemy_of_color = (color == ShumiChess::WHITE) ? ShumiChess::BLACK : ShumiChess::WHITE;

        // Has king only, or king with a single minor piece.
        bool onlyKingEnemy   = engine.game_board.bIsOnlyKing(enemy_of_color);
        bool onlyKingFriend  = engine.game_board.bIsOnlyKing(color);
   
        int bonus_cp;

        //printf("%ld", evp);
             
        switch (evp)
        {
            case MATERIAL_ONLY:
                // Do nothing positional (cp_score_position_temp is already zero)
                break;

            case CRAZY_IVAN:
                // Assign to cp_score_position_temp
                bonus_cp = engine.game_board.center_closeness_bonus(color);
                assert(bonus_cp >= 0);
                cp_score_position_temp = bonus_cp;
                break;  

            default:
            case UNCLE_SHUMI:
            {
                /////////////// start positional evals /////////////////

       
                if (!onlyKingEnemy) {

                    test = cp_score_positional_get_opening(color, nPhase);
                    //if (is_debug) printf("\nopening: %ld", test);
                    cp_score_position_temp += test;
            
                    // Note this return is in centpawns, and can be negative
                    test = cp_score_positional_get_middle(color);
                    //if (is_debug) printf("\nmiddle: %ld", test);
                    cp_score_position_temp += test;    
                }     
                
                // Note this return is in centpawns, and can be negative
                test = cp_score_positional_get_end(color, nPhase, onlyKingFriend, onlyKingEnemy);
                //if (is_debug) printf("\nend: %ld", test);
                cp_score_position_temp += test;      

                // trading
                // if ( (isQuietPosition) && (color == for_color) ) {
                //     test = cp_score_get_trade_adjustment(color, mat_np_white, mat_np_black);
                //     cp_score_position_temp += test;
                //     if (is_debug) printf("\ntrd: %ld", test);
                
                //     #ifdef _DEBUGGING_MOVE_CHAIN1
                //         sprintf(szDebug, "trd %ld", test);
                //         fputs(szDebug, fpDebug);
                //     #endif
                // }


                break;  
            }          

        }   // END switch over eval persons


        /////////////// end positional evals /////////////////


        // Add positional eval to score
        if (color != for_color) cp_score_position_temp *= -1;
        cp_score_position += cp_score_position_temp;


        //if (is_debug) printf("\npst: %ld   %ld", test, cp_score_position);

    }   // END loop over the two colors

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
// #define MAX_personalities 50

// struct person {
//     int constants[MAX_personalities];   
// };

// NOte: what sort of nonsense is this? A family of evaluator's? Why not?
//person TheShumiFamily[20];

// person MrShumi = {100, -10, -30, +14, 20, 20, 20};  // All of these are integer and are applied in centipawns
// int pers_index = 0;

int MinimaxAI::cp_score_positional_get_opening(ShumiChess::Color color, int nPhase) {

    int cp_score_position_temp = 0;
    bool bOK;
    int icp_temp, iZeroToThree, iZeroToThirty;
    int iZeroToFour, iZeroToEight;


    // Add code to make king: 1. want to retain castling rights, and 2. get castled. (this one more important)
    //pers_index = 0;
    icp_temp = engine.game_board.get_castle_bonus_cp(color, nPhase);
    cp_score_position_temp += icp_temp;     // centipawns

    // Get the friendly pawns summary (used by all the "count_" functions below)
    PawnFileInfo pawnFileInfo;
    bOK = engine.game_board.build_pawn_file_summary(color, pawnFileInfo.p[friendlyP]);

    if (bOK) {      // There are friendly pawns

        ull holes_bb = 0ULL;
        ull passed_pawns = 0ULL;

        Color enemyColor = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;

        // Get the enemy pawns summary (used by all the "count_" functions below)
        bOK = engine.game_board.build_pawn_file_summary(enemyColor, pawnFileInfo.p[enemyP]);
        //if (!bOK) // false if enemy color has no pawns

        // Add code to discourage isolated pawns.
        int isolanis = engine.game_board.count_isolated_pawns_cp(color, pawnFileInfo);
        cp_score_position_temp -= (isolanis);   // centipawns

        // Add code to discourage backward pawns/pawn holes
        isolanis = engine.game_board.count_pawn_holes_cp(color, pawnFileInfo, holes_bb);
        cp_score_position_temp -= (isolanis);   // centipawns

        isolanis = engine.game_board.count_knights_on_holes_cp(color, holes_bb);
        cp_score_position_temp -= (isolanis);   // cent
    
        // Add code to discourage doubled/tripled/quadrupled pawns. Note each pair of doubled pawns is 2. Each 
        // trio of tripled pawns is 3.
        //pers_index = 1;
        int doublees = engine.game_board.count_doubled_pawns_cp(color, pawnFileInfo);
        cp_score_position_temp -= doublees;   // centipawns

        // Add code to encourage passed pawns. (1 for each passed pawn)
        int iZeroToThirty = engine.game_board.count_passed_pawns_cp(color, pawnFileInfo, passed_pawns);
        assert (iZeroToThirty>=0);
        cp_score_position_temp += iZeroToThirty;   // centipawns

        // Remember where passed pawns are.
        (color==ShumiChess::WHITE) ? passed_pawns_white = passed_pawns : passed_pawns_black = passed_pawns; 

        // Add code to encourage occupation of open and semi open files by rooks
        icp_temp = engine.game_board.rooks_file_status_cp(color, pawnFileInfo);
        cp_score_position_temp += icp_temp;  // centipawns       


    }

    // if (nPhase == OPENING) {
    //     // Add code to discourage stupid occupation of d3/d6 with bishop, when pawn on d2/d7. 
    //     // Note: this is gross
    //     pers_index = 2;
    //     iZeroToThirty = engine.game_board.bishop_pawn_pattern(color);
    //     //assert (iZeroToThirty>=0);
    //     cp_score_position_temp -= iZeroToThirty*50;   // centipawns
    // }

    // Add code to encourage pawns attacking the 4-square center 
    iZeroToFour = engine.game_board.pawns_attacking_center_squares_cp(color);
    cp_score_position_temp += iZeroToFour;  // centipawns  (from 14)  

    // Add code to encourage knights attacking the 4-square center 
    iZeroToFour = engine.game_board.knights_attacking_center_squares(color);
    cp_score_position_temp += iZeroToFour*20;  // centipawns

    // Add code to encourage bishops attacking the 4-square center
    iZeroToFour = engine.bishops_attacking_center_squares(color);
    cp_score_position_temp += iZeroToFour*20;  // centipawns


    iZeroToFour = engine.game_board.is_knight_on_edge_cp(color);
    cp_score_position_temp -= iZeroToFour;  // centipawns


endEval:
    return cp_score_position_temp;

}
//
// should be called in middlegame, but tries to prepare for endgame.
int MinimaxAI::cp_score_positional_get_middle(ShumiChess::Color color) {
    int cp_score_position_temp = 0;

    // bishop pair bonus (two bishops) (2 bishops)
    int bishops = engine.game_board.bits_in(engine.game_board.get_pieces_template<Piece::BISHOP>(color));
    if (bishops >= 2) cp_score_position_temp += 10;   // in centipawns

    // Add code to encourage rook connections (files or ranks)
    int connectiveness;     // One if rooks connected. 0 if not.
    bool isOK = engine.game_board.rook_connectiveness(color, connectiveness);
    if (isOK) {    // !isOK just means that there werent two rooks to connect
        cp_score_position_temp += connectiveness*150;
    }
    
    // Add code to encourage occupation of 7th rank by queens and rook
    int iZeroToFour = engine.game_board.rook_7th_rankness_cp(color);
    cp_score_position_temp += iZeroToFour;  // centipawns  
    

    return cp_score_position_temp;
}


// Trading. "np" means "no pawns".
// Note: Trading broken right now (or weakened)
int MinimaxAI::cp_score_get_trade_adjustment(ShumiChess::Color color,
                                             int mat_np_white,      // centipawns
                                             int mat_np_black)      // centipawns
{
    int cp_clamp = 5;   // no more than a pawn of motivation

    // NP advantage (centipawns) from "color"'s point of view (positive, if good for color)
    int np_adv_for_color =
        (color == ShumiChess::WHITE) ? (mat_np_white - mat_np_black)
                                     : (mat_np_black - mat_np_white);

    if (np_adv_for_color == 0) return 0;  // no NP edge, so no bonus

    // Trades more important, the less pieces there are.
    int denominator =  (mat_np_black + mat_np_white);
    assert (denominator > 0);                      // no pieces?

    double dRatio = (double)np_adv_for_color / (double)denominator;

    // Full complement of pieces is 4000 centpawns. Suppose one side a knight up. Then 
    // ratio = 320 / 4000  or about 1 tenth, so we see about 60 cp motivation to trade.
    int iReturn = (int)(dRatio*1200.0);
    if (iReturn >  cp_clamp) iReturn =  cp_clamp;
    if (iReturn < -cp_clamp) iReturn = -cp_clamp;

    return iReturn;

}


int MinimaxAI::cp_score_positional_get_end(ShumiChess::Color color, int nPhase,
                                            bool onlyKngFriend, bool onlyKngEnemy
                                            ) {

    int cp_score_position_temp = 0;

    

    //if (is_debug) printf("\ngt1: %ld", cp_score_position_temp);

    // Add code to encourage ???
    //int iZeroToOne = engine.game_board.kings_in_opposition(color);
    //cp_score_position_temp += iZeroToOne*200;  // centipawns  

    //Color enemy_color = (color==ShumiChess::WHITE ? ShumiChess::BLACK : ShumiChess::WHITE);


    if (nPhase > GamePhase::OPENING) { 
        // Add code to attack squares near the king
        int itemp = engine.game_board.attackers_on_enemy_king_near(color);
        assert (itemp>=0);
        cp_score_position_temp += itemp*20;  // centipawns  
    }
    //if (is_debug) printf("\ngt2: %ld", cp_score_position_temp);

    if (onlyKngEnemy) {

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
        cp_score_position_temp += (int)(dFarness * 25.0);

        //if (is_debug) printf("\ngt3: %ld", cp_score_position_temp);


        // Rewards if enemy king near corner. 0 for the inner ring (center) 3 for outer ring (edge squares)
        Color enemy_color = (color==ShumiChess::WHITE ? ShumiChess::BLACK : ShumiChess::WHITE);
        int edge_wght = engine.game_board.king_edge_weight(enemy_color);

        cp_score_position_temp += (int)(edge_wght * 40.0);

        //if (is_debug) printf("\ngt4: %ld", cp_score_position_temp);

    }



    return cp_score_position_temp;
}

// material_cp must be average material (positive)
int MinimaxAI::phaseOfGame(int material_cp) {
    int nPhase = GamePhase::OPENING;
    assert(material_cp>=0);


    bool bWhiteCstled = engine.game_board.bHasCastled_fake(ShumiChess::WHITE);
    bool bBlackCstled = engine.game_board.bHasCastled_fake(ShumiChess::BLACK);

    //
    // Each side has 4000 centipawns
    if      (material_cp > 3500) nPhase = GamePhase::OPENING;
    else if (material_cp > 3000) nPhase = GamePhase::MIDDLE_EARLY;
    else if (material_cp > 2600) nPhase = GamePhase::MIDDLE;
    else if (material_cp > 1100) nPhase = GamePhase::ENDGAME;
    else if (material_cp >= 0)   nPhase = GamePhase::ENDGAME_LATE;
    else assert(0);

    bool bBothCastled = (bWhiteCstled && bBlackCstled);

    if (nPhase == GamePhase::OPENING) {
        if (bBothCastled) nPhase = GamePhase::MIDDLE;
    }
    else if (nPhase == GamePhase::MIDDLE_EARLY) {
        if (bBothCastled) nPhase = GamePhase::MIDDLE;
    }

    // int i_castle_status = engine.game_board.get_castle_bonus_cp(engine.game_board.turn);


    // bool bHasCastled = engine.game_board.bHasCastled(engine.game_board.turn);

    // int nPhase = (bHasCastled && ((engine.computer_ply_so_far)>17) );   // NOTE: this is crap

    return nPhase;
}


// phaseOfGame(), but gets the material(s) by itself. For display only.
int MinimaxAI::phase_of_game_full() {
    int cp_score_material_all = 0;
    for (const auto& color1 : array<Color, 2>{Color::WHITE, Color::BLACK}) {

        // Get the centipawn value for this color
        int cp_pawns_only_temp;
        int cp_score_mat_temp = engine.game_board.get_material_for_color(color1, cp_pawns_only_temp);
        assert (cp_score_mat_temp>=0);    // there is no negative value material 
        cp_score_material_all += cp_score_mat_temp;   
    }

    cp_score_material_all = cp_score_material_all / 2;

    //cout << "bogee " << cp_score_material_all;
    int nPhase = phaseOfGame(cp_score_material_all); 
    return nPhase;
}

///////////////////////////////////////////////////////////////////////////////////


//
// Only returns false is if user aborts.
//
tuple<double, Move> MinimaxAI::do_a_deepening(int depth, ull elapsed_time_display_only, const Move& null_move) {

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


        #ifdef DEBUGGING_RANDOM_DELTA1
            fprintf(fpDebug, "\n");
        #endif

        #ifdef DISPLAY_DEEPING
            cout << endl << aspiration_tries << " Deeping " << depth << " ply of " << maximum_deepening
                        << " msec=" << std::setw(6) << elapsed_time_display_only << ' ';
        #endif

        ret_val = recursive_negamax(depth
                                    , alpha, beta
                                    , true              // I am called from the root
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
// time_requested now in *milliseconds*
Move MinimaxAI::get_move_iterative_deepening(double time_requested, int max_deepening_requested, int feat) {  
    

    long long now_s;    // milliseconds 
    long long end_s;    // milliseconds
    long long diff_s;   // Holds (actual time - requested time). Positive if past due. Negative if sooner than expected
    ull elapsed_time = 0; // in msec

    // Obtain time now (milliseconds)
    auto start_time = chrono::high_resolution_clock::now();

    // Not actually a "endtime". The last deepening will start before or at this "requested" time. 
    auto requested_end_time = start_time + chrono::duration<double, std::milli>(time_requested);

    // debug
    // cout << "\x1b[94mdept requested (ply)  =" << max_deepening_requested << "\x1b[0m" << endl;  
    // cout << "\x1b[94mtime requested (msec) =" << time_requested << "\x1b[0m" << endl;
    // cout << "\x1b[94margu requested (msec) =" << feat << "\x1b[0m" << endl;

    bool b_Forced = false;

    Features_mask = feat;

    //Move null_move = Move{};
    Move null_move = engine.users_last_move;    // Just to make the move history work?


    stop_calculation = false;

	#ifdef DISPLAY_PULSE_CALLBACK_THREAD   // Used for debug display only
    	start_callback_thread();
    #endif

    if (engine.computer_ply_so_far == 0) {
        //
        // Here we do MinimaxAI stuff to be done at the beginning of a game. Clumsy way to have to detect 
        // this, but there it is: engine.reset_engine() starts a new game, and sets computer_ply_so_far = 0.
        //
        TTable2.clear();    // Clear even if we don't use it.

        NhitsTT = 0;
        NhitsTT2 = 0;
        nRandos = 0;

        nGames++;

    }   

    nFarts = 0;             // Queiseence low level (forced eval) this move
    // Clear debug every move
    #ifdef _DEBUGGING_TO_FILE 
        clear_file_keep_fp(fpDebug);
    #endif

    // NOTE: In 2 computer mode this is in plys. If one human, its in moves.
    engine.computer_ply_so_far++;                      // Increment real moves in whole game
    cout << "\x1b[94m\n\nMove: " << engine.computer_ply_so_far << "\x1b[0m";

    //engine.gamePGN.add(engine.users_last_move, engine);

    nodes_visited = 0;
    nodes_visited_depth_zero = 0;
    evals_visited = 0;

    seen_zobrist.clear();
    //uint64_t zobrist_key_start = engine.game_board.zobrist_key;
    //cout << "zobrist_key at the root: " << zobrist_key_start << endl;

    TTable.clear();       // Leaf TT cleared on every move (even if never used)

    Move best_move = {};
    double d_best_move_value = 0.0;

    // This nonsense is to allow the "max deepening" to be changed "on the fly" (inbetween moves).
    int this_deepening;
    this_deepening = engine.user_request_next_move;
    //this_deepening = 5;        // Note: because i said so.
    this_deepening = max_deepening_requested;

    maximum_deepening = this_deepening;

    // defaults
    int depth = 1;
    
    int nPlys = 0;
    bool bThinkingOver = false;
    bool bThinkingOverByTime = false;
    bool bThinkingOverByDepth = false;

    // INitialize these, whether we use them or not.
    for (int ii=0;ii<MAX_PLY;ii++) {
        killer1[ii] = {}; 
        killer2[ii] = {};
    }

    // Start each search with an empty root-move list for move randomization.
    MovesFromRoot.clear();

    double d_Return_score = 0.0;

    do {

        MovesFromRoot.clear();

        tuple<double, Move> ret_val;
        
        #ifdef _DEBUGGING_MOVE_CHAIN    // Start of a deepening
            bool bSide = (engine.game_board.turn == ShumiChess::BLACK);
            sprintf(szDebug,"\n\n--------------------------- %ld ------------------------------------------------", depth);
            fputs(szDebug, fpDebug);
            engine.print_move_to_file(null_move, nPlys, (GameState::INPROGRESS), false, true, bSide, fpDebug);
        #endif

        //
        // This case can happen in 50-move rule, all nodes return DRAW, so in so quickly
        // zips through these, that it runs out of depth before the time limit.
        if (depth>=MAXIMUM_DEEPENING) {
            //cout << "\x1b[31m \nOver Deepening " << depth << "\x1b[0m" << endl;
            //cout << gameboard_to_string(engine.game_board) << endl;
            //assert(0);      // NOTE: this happens close to draws. Noone knows why.
            break;   // Stop deepening, no more depths.
        }

        // the beast
        ret_val = do_a_deepening(depth, elapsed_time, null_move);

        d_Return_score = get<0>(ret_val);
        if (d_Return_score == ABORT_SCORE) {
            // User aborted the computation. Here we do nothing, ends up using the last deepeining. 
            cout << "\x1b[31m Aborting depth of " << depth << "\x1b[0m" << endl;
            break;   // Stop deepening, no more depths.
        } else if (d_Return_score == ONLY_MOVE_SCORE) {
            // We stopped analysis because there was only one legal move. We just play that.
            // Intersting, what should be the score here? Note: Could instead use the last deepeining?
            // instead this code makes us use the score from the last move.
            b_Forced = true;
            d_best_move_value = 0.0;
            best_move = get<1>(ret_val);        // this was the only legal move.
            break;   // Stop deepening, no more depths.
        } else {
            d_best_move_value = d_Return_score;
            best_move = get<1>(ret_val);

            // Root sees a forced mate: no point deepening further. 
            // Update: YES there is a point in continuing. We might find a shorter mate.
            //if (std::fabs(d_best_move_value) >= HUGE_SCORE/2.0)
            // if (IS_MATE_SCORE(d_best_move_value))
            // {
            //     cout << "\x1b[31m !!!!!!!! mate at exterior node (depth " << depth << ")\x1b[0m" << endl;

            //     #ifdef _DEBUGGING_TO_FILE1
            //         //engine.print_move_history_to_file(fpDebug);    // debug only
            //         cout << gameboard_to_string2(engine.game_board) << endl;
            //         assert(0);
            //     #endif

            //     break;   // Stop iterative deepening immediately.
            // }

        }

        // Store PV for the *next* iteration. Called incorrectly in literture as: "PV at the root".
        assert (depth >=0);
        prev_root_best_[depth] = std::make_pair(best_move, d_best_move_value);   // move + score (pawns)
        #ifdef _DEBUGGING_PV
            engine.move_into_string(best_move);
            sprintf(szDebug, "PV   %s  %ld", engine.move_string.c_str(), depth);
            fprintf(fpDebug, szDebug);
        #endif

        engine.move_and_score_to_string(best_move, d_best_move_value , true);

        #ifdef DISPLAY_DEEPING
            cout << " Best: " << engine.move_string;
        #endif

        //
        //  If diff_s < 0  → now is before requested_end_time (time remaining).
        //  If diff_s == 0 → exactly at the requested end time.
        //  If diff_s > 0  → now is after requested_end_time (you’ve gone over).
        now_s = (long long)chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        end_s = (long long)chrono::duration_cast<chrono::milliseconds>(requested_end_time.time_since_epoch()).count();
        diff_s = now_s - end_s;

        // Elapsed time for all deepenings up to now (milliseconds)
        elapsed_time = now_s - (ull)chrono::duration_cast<chrono::milliseconds>(start_time.time_since_epoch()).count();

        depth++;

        // time based ending of thinking
        bThinkingOverByTime = (diff_s > 0);

        // depth based ending of thinking
        bThinkingOverByDepth = (depth >= (maximum_deepening+1));

        // we are done thinking if both time and depth is ended OR simple endgame is on.
        bThinkingOver = (bThinkingOverByDepth && bThinkingOverByTime);


    } while (!bThinkingOver);

    //cout << endl << " Deep end " << depth << "          msec=" << std::setw(6) << elapsed_time << ' ';



    cout << "\x1b[33m\nWent to depth " << (depth - 1)  
        << " elapsed msec= " << elapsed_time << " requested msec= " << time_requested 
        << std::fixed << std::setprecision(1)
        << "    time overshoot= " << (elapsed_time / time_requested)
        //<< " TimOver= " << bThinkingOverByTime << " DepOver= " << bThinkingOverByDepth 
        << "\x1b[0m" << endl;


    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";

    // If the first move, MAYBE randomize the response some.
    if ( (d_Return_score != ABORT_SCORE) && (d_Return_score != ONLY_MOVE_SCORE) ) {


        #ifdef DEBUGGING_RANDOM_DELTA
            fprintf(fpDebug, "\n===============================================================\n");

            engine.print_moves_and_scores_to_file(MovesFromRoot, false, true, fpDebug);
    
            fprintf(fpDebug, "\n===============================================================\n");
        #endif


        // Reassign best move, if randomizing. The i_randomize_next_move decrements, after every random 
        // move chosen. When it hits zero, no more random moves will be chosen.
        d_random_delta = 0.0;
        if (engine.i_randomize_next_move>0) {
            d_random_delta = RANDOMIZING_EQUAL_MOVES_DELTA;
            engine.i_randomize_next_move--;             // Decrement number of randome moves to make
        }

        if (d_random_delta != 0.0) {
            nRandos++;
            int n_moves_within_delta=0;
            // The root is when the player starts thinking about his move.
            best_move = pick_random_within_delta_rand(MovesFromRoot, d_random_delta, n_moves_within_delta);
            // Show random move, and the "pool" (and reduced pool") it was chosen from.

            cout << "\033[1;34mrando move (out of: \033[0m" << MovesFromRoot.size() << " >> " << n_moves_within_delta << endl;
        
            string move_stringee = "";
            std::ostringstream oss;
            oss << "\033[1;34mrando move (out of: \033[0m"
                   
            << MovesFromRoot.size()
                    << " >> "
                    << n_moves_within_delta
                    << endl;

            move_stringee = oss.str();
      
        }

        // (debug only) engine.move_into_string(best_move);
        // std::cout << engine.move_string << std::endl;
   
        //  (debug only) print moves and scores
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
                , false
                , false
                , NULL
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

   
    chrono::duration<double> total_time = chrono::high_resolution_clock::now() - start_time; // still prints seconds
    //cout << total_time << endl;
    double dElapsedTime = total_time.count();
    cout << colorize(AColor::BRIGHT_GREEN, (static_cast<std::ostringstream&&>(std::ostringstream()
         << "Total time: " << std::fixed << std::setprecision(2) << dElapsedTime << " sec")).str());

    ull running_time_msec = (ull)(dElapsedTime * 1000.0);
    if (engine.game_board.turn == ShumiChess::WHITE) 
        engine.game_white_time_msec += running_time_msec;
    else 
        engine.game_black_time_msec += running_time_msec;


    assert (total_time.count() > 0);
    double nodes_per_sec = nodes_visited / total_time.count();
    double evals_per_sec = evals_visited / total_time.count();
    cout << colorize(AColor::BRIGHT_GREEN, 
        "   nodes/sec= " + format_with_commas(std::llround(nodes_per_sec)) + 
        "   evals/sec= " + format_with_commas(std::llround(evals_per_sec))) << endl;

    if (!b_Forced) {   // if we are forcing the move due to it being the only move, then don't update "score".
        engine.d_bestScore_at_root = d_best_move_value_abs;  
    }


    cout << "\n";

    #ifdef DEBUGGING_KILLER_MOVES
        cout << "Killers: tried=" << killer_tried
            << " cutoffs=" << killer_cutoff
            << " (" << (killer_tried ? (100.0 * killer_cutoff / killer_tried) : 0.0)
            << "%)\n\n";
    #endif

    //engine.gamePGN.add(best_move, engine);

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Now done with making, and measuring and displaying the computer move. Ready for exit BUT
    // Debug only  playground   sandbox for testing evaluation functions
    // int isolanis;
    bool isOK;
    double centerness;

    int itemp, iNearSquares, iPhase;
    int king_near_squares_out[9];
    ull utemp, utemp1, utemp2;
    double dTemp, dtemp1, dtemp2;
    int itemp3, itemp4;
    
    
    isOK = false; // engine.game_board.bHasCastled_fake(ShumiChess::WHITE);

    iPhase = phase_of_game_full();
    char szTemp[128];
    char* pszTemp = &szTemp[0];
    pszTemp = str_from_GamePhase(iPhase);

    //isOK = engine.game_board.isReversableMove(best_move);

    global_debug_flag = true;
    //itemp = engine.game_board.get_castle_bonus_cp(Color::WHITE, iPhase);
    //itemp = engine.bishops_attacking_center_squares(ShumiChess::WHITE);

    // test harness for testing isolated/doubled/passed pawns
    ull passed_pawns;
    ull holes;

    int itemp1=0;
    int itemp2=0;
    PawnFileInfo pawnFileInfo;

    isOK = engine.game_board.build_pawn_file_summary(Color::WHITE, pawnFileInfo.p[friendlyP]);
    isOK = isOK && engine.game_board.build_pawn_file_summary(Color::BLACK, pawnFileInfo.p[enemyP]);
    //if (isOK) itemp1 = engine.game_board.count_passed_pawns_cp(Color::WHITE, pawnFileInfo,passed_pawns);
    //if (isOK) itemp1 = engine.game_board.count_pawn_holes_cp(Color::WHITE, pawnFileInfo, holes);
    if (isOK) itemp1 = engine.game_board.rooks_file_status_cp(Color::WHITE, pawnFileInfo);
    

    isOK = engine.game_board.build_pawn_file_summary(Color::BLACK, pawnFileInfo.p[friendlyP]);
    isOK = isOK && engine.game_board.build_pawn_file_summary(Color::WHITE, pawnFileInfo.p[enemyP]);
    //if (isOK) itemp2 = engine.game_board.count_passed_pawns_cp(Color::BLACK, pawnFileInfo,passed_pawns);
    //if (isOK) itemp2 = engine.game_board.count_pawn_holes_cp(Color::BLACK, pawnFileInfo, holes);
    if (isOK) itemp2 = engine.game_board.rooks_file_status_cp(Color::BLACK, pawnFileInfo);
    
    cout << "wht " << itemp1 << "           blk " << itemp2 << endl;

    global_debug_flag = false;

    // cout << "blk " << itemp1 <<  "  " << itemp2 <<  "  " << itemp3 <<  "  " << itemp4 << endl;

    utemp1 = TTable.size();
    utemp2 = TTable2.size();
    cout << "TT: " << utemp1 << " mtches= " << NhitsTT << "       TT2: " << utemp2 << " mtches= " << NhitsTT2 << endl;
 
    //engine.debug_print_repetition_table();

	#ifdef DISPLAY_PULSE_CALLBACK_THREAD
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
                    int depth
                    ,double alpha, double beta
                    , bool is_from_root
                    ,const ShumiChess::Move& move_last      // seems to be used for debug only...
                    ,int nPlys
                    ,int qPlys
                    )
{

    // Initialize return 
    double d_best_score = 0.0;
    Move the_best_move = {};


    int cp_score_best;

    bool did_cutoff = false;    // TRUE if fail-high
    bool did_fail_low = false;  // TRUE if fail-low

    double alpha_in = alpha;   //  save original alpha window lower bound
    double beta_in = beta;   //  save original alpha window lower bound

    // I eat a lot of time. Expensive.
    std::vector<Move> legal_moves = engine.get_legal_moves();
    vector<Move>* p_moves_to_loop_over = &legal_moves;


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
    // Note: why does this so high? Must be a better way to handle this. Always happens near draws.
    // OR when 3-time rep is off.   // _SUPRESSING_MOVE_HISTORY_RESULTS is defined.
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
    // Hard node-limit sentinel fuse
    // =====================================================================
    if (nodes_visited > 9.0e7) {    // 10,000,000 1.0e7  a good number here
        std::cout << "\x1b[31m\n! NODES VISITED trap#2 " << nodes_visited << "dep=" << depth << "  "
                        << engine.get_best_score_at_root() << "\x1b[0m\n";
        //assert(0);

        // Note: fascinating. This happens when in mate looking. 

        return { ABORT_SCORE, the_best_move };

    }

    //EvalPersons evp = CRAZY_IVAN;   // UNCLE_SHUMI;
    EvalPersons evp = UNCLE_SHUMI;

    #ifdef  DEBUG_NODE_TT2       // Declare variables for holding "record" found in the TT2
        bool   foundPos = false;
        int    foundScore = 0;
        Move   foundMove = {};
        int    foundnPlys = 0;
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

        std::stack<ShumiChess::Move> found_move_history; 

        bool   found_white_castled = false;
        bool   found_black_castled = false;

        int legalMovesSize = legal_moves.size();

    #endif


    TT2_match_move = {};

    if (Features_mask & _FEATURE_TT2) {  // probe in TT2

        int iLimit = 1;   // (Features_mask & _FEATURE_ENHANCED_DEPTH_TT2) ? 0 : 1;       // 0 or 1 only
        if (depth > iLimit) {

            // --- Normal TT2 probe (Note: exact-only version, no flags/age yet)

            bool is_perfect_match = false;

            // Probe the table
            uint64_t key = engine.game_board.zobrist_key;
            auto it = TTable2.find(key);

            if (it != TTable2.end()) {

                // probe found for this zobrist key
                const TTEntry2 &entry = it->second;

                // Qualification #1 on probe: We can reuse an entry if it was searched at least as deep
                // We already searched this node to at least this depth so we can trust the stored result.
                if (entry.depth >= depth) {

                    // Qualification #2 on probe: Only accept hit if window matches stored one
                    // Since stored results is EXACT, the "debug" versions should be the full window now.
                    // So here we restrict ourselves to situations where the current window is full.
                    bool windowMatches =
                        (std::fabs(entry.dAlphaDebug - alpha) <= VERY_SMALL_SCORE) &&
                        (std::fabs(entry.dBetaDebug  - beta ) <= VERY_SMALL_SCORE);

                    // Qualification #3 on probe: This is a hack to cover up a unknown bug
                    // The burp2 nPlys bug ( found nPly always = current + 1)
                    bool horridHackMatch = true;  //(foundnPlys == nPlys);
                    
                    if (windowMatches && horridHackMatch) {
                        is_perfect_match = true;
                    }
                }


                if (is_perfect_match) {      
                    // If debug, just compare to the computation. If not debug actually use the result. 

                    #ifdef DEBUG_NODE_TT2        // Store information recalled from the TT2 "record"
                        foundPos   = true;
                        foundScore = entry.score_cp;
                        foundMove  = entry.best_move;
                        foundnPlys = entry.nPlysDebug;
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

                        found_white_castled = entry.white_castled_debug;
                        found_black_castled = entry.black_castled_debug;

                        found_move_history = entry.move_history_debug; 

                    #else
                        double dScore = (double)entry.score_cp / 100.0;
                        return { dScore, entry.best_move };
                    #endif

                } else {

                    // Its a match but not a perfect match. Use the move for ordering anyway.
                    bool is_in = is_move_in_list(entry.best_move, legal_moves);
                    assert(is_in);

                    //TT2_match_move = entry.best_move;

                }
                
                
            }   // END probe hit

        }   // END stupid 0/1 filter sub-feature


    }   // END TT2 feature


    vector<ShumiChess::Move> unquiet_moves;   // This MUST be declared as local in this function (not in the class) or horrible crashes


    // Only one of me, per deepening.
    bool first_node_in_deepening = (top_deepening == depth);

    if (first_node_in_deepening) {
        if (legal_moves.size() == 1) {
            cout << "\x1b[94m!!!!! force !!!!!!!!!!!!!\x1b[0m" << endl;

            #ifdef _DEBUGGING_TO_FILE1
                //engine.print_move_history_to_file(fpDebug, "forc");    // debug only
                cout << gameboard_to_string2(engine.game_board) << endl;
                assert(0);
            #endif

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

            if (is_from_root) engine.reason_for_draw = DRAW_STALEMATE;

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
    double d_stand_pat = HUGE_SCORE;   // If we evaluate, it will be the evaluate score.

    if (depth == 0) {

        // Static board evaluation

        bool b_is_Quiet = !engine.has_unquiet_move(legal_moves);

        int  cp_from_tt   = 0;
        bool have_tt_eval = false;


        // memoization of leafs
        if (Features_mask & _FEATURE_TT) {
            // Salt the entry
            unsigned mode  = salt_the_TT(b_is_Quiet);

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
            #ifdef DEBUG_LEAF_TT
                cp_score_best = evaluate_board(engine.game_board.turn, exp, b_is_Quiet);
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
            cp_score_best = evaluate_board(engine.game_board.turn, evp, b_is_Quiet);
        }


        d_best_score = engine.convert_from_CP(cp_score_best);
        d_stand_pat = d_best_score;  // "stand pat" means the evaluate_board() computed score

        // memoization at leaf
        if (Features_mask & _FEATURE_TT) {
            //if (!bFast) {

                // Salt the entry
                unsigned mode  = salt_the_TT(b_is_Quiet);

                uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

                 // Store this position away into the TT
                TTEntry &slot = TTable[evalKey];
                slot.score_cp = cp_score_best;   // or cp_score, whatever you just got
                slot.movee    = the_best_move;   // or bestMove, etc.
                slot.depth    = top_deepening;
            //}
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
            //engine.print_move_history_to_file(fpDebug, "MAX_QPLY");

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

        //
        /////////////////////////////////////////////////////////////////////////////////////////
        //
        // Look (recurse) over all moves chosen
        //
        int imovedebug = 0;
        int isizedebug = sorted_moves.size();
        for (const Move& m : sorted_moves) {
            int nChars;

            imovedebug++;

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

                // This output always starts a new line
                nChars = fputc('\n', fpDebug);
                if (nChars == EOF) assert(0);

                // Print move debug
                sprintf(szDebug, "[%ld/%ld]", imovedebug, isizedebug);

                // Print move (Move always starts a new line)
                int nCharsInMove = engine.print_move_to_file2(m, nPlys, (GameState::INPROGRESS)
                                    , false, bSide, szDebug
                                    , fpDebug); 
                if (nCharsInMove == EOF) assert(0);

                sprintf(szDebug, " A=%.3f, B=%.3f", alpha, beta);
                fprintf(fpDebug, szDebug);
            
            #endif

            //
            // Delta pruning (in qsearch (Quiescence) at depth==0) estimates the most this capture/promotion could possibly 
            // improve the current stand-pat score (including material swing and a safety margin), and if 
            // even that optimistic bound still can't raise alpha, it just skips searching that move as futile. 
            if (Features_mask & _FEATURE_DELTA_PRUNE) {
                // --- Delta pruning (qsearch (Quiescence) only)
                assert(0);
                if ( (depth == 0) && !in_check) {
                    int ub = 0;
                    if (m.capture != ShumiChess::Piece::NONE) {
                        ub += engine.game_board.centipawn_score_of(m.capture); // victim value
                    }
                    if (m.promotion != ShumiChess::Piece::NONE) {
                        ub += engine.game_board.centipawn_score_of(m.promotion)
                            - engine.game_board.centipawn_score_of(ShumiChess::Piece::PAWN); // promo gain
                    }
                    assert(ub != 0);    // we should not be looking at moves in Quiescence, unless they are captures or promotions
                       
                    const bool recapture = (!engine.move_history.empty() && (m.to == engine.move_history.top().to));

                    // we should not be looking at moves in Quiescence, unless we evaluated first
                    assert (d_stand_pat != HUGE_SCORE);  

                    int cp_stand_pat = engine.convert_to_CP(d_stand_pat);
                    int cp_alpha = engine.convert_to_CP(alpha);

                    // treat anything bigger than a minor as "heavy"
                    constexpr int HEAVY_DELTA_THRESHOLD_CP = 330; // just above a knight/bishop

                    if ( (qPlys > 2) && (ub <= HEAVY_DELTA_THRESHOLD_CP) ) {
                        constexpr int DELTA_MARGIN_CP = 80; // tune 60..100
                        if (!recapture && (cp_stand_pat + ub + DELTA_MARGIN_CP < cp_alpha)) {
                            //cout << "\033[31m#\033[0m";
                            continue;   // skip this move its futile
                        }

                        // Pawn-victim futility (beyond delta): skip far-below-alpha pawn grabs (non-recapture)
                        int cp_pawn = engine.game_board.centipawn_score_of(ShumiChess::Piece::PAWN);
                        if (!recapture &&
                            (m.capture == ShumiChess::Piece::PAWN) && (cp_stand_pat + cp_pawn + 60) < cp_alpha) {
                            //cout << "\033[31m#\033[0m";
                            continue;   // skip this move its futile
                        }
                    }
                }
            }
    
            bool is_killer_here = false;
            #ifdef DEBUGGING_KILLER_MOVES
                if (Features_mask & _FEATURE_KILLER) {
                    is_killer_here = (m == killer1[nPlys]) || (m == killer2[nPlys]);
                    if (is_killer_here) killer_tried++;
                }
            #endif


            assert(m.piece_type != Piece::NONE);
            engine.pushMove(m);
               
            #ifdef DISPLAY_PULSE_CALLBACK_THREAD
            	g_live_ply = nPlys;
            #endif

            //++engine.repetition_table[engine.game_board.zobrist_key];
            engine.key_stack.push_back(engine.game_board.zobrist_key);


            bool bRootWideWindowForRandom = is_from_root && (engine.i_randomize_next_move > 0);

            //
            // Two parts in negamax: 1. "relative scores", the alpha betas are reversed in sign,
            //                       2. The beta and alpha arguments are staggered, or reversed.
            double childAlpha = -beta;
            double childBeta  = -alpha;

            if (bRootWideWindowForRandom) {
                //assert(0);
                childAlpha = -HUGE_SCORE;
                childBeta  =  HUGE_SCORE;
            }

            auto ret_val = recursive_negamax(
                (depth > 0 ? depth - 1 : 0),
                childAlpha, childBeta,
                false,                    // I am NOT called from the root
                m,
                (nPlys+1),
                (depth == 0 ? qPlys+1 : qPlys)
            );

            // The third part of negamax: negate the score to keep it relative.
            double d_return_score = get<0>(ret_val);     // units are pawns
            Move d_return_move = get<1>(ret_val);


            //--engine.repetition_table[engine.game_board.zobrist_key];
            //if (engine.repetition_table[engine.game_board.zobrist_key] == 0) {
            //    engine.repetition_table.erase(engine.game_board.zobrist_key);
            //}
            engine.key_stack.pop_back();
            while (!engine.boundary_stack.empty() &&
                engine.boundary_stack.back() >= (int)engine.key_stack.size()) {
                engine.boundary_stack.pop_back();
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

            // Here we are coming back from a recursive call. 
            #ifdef _DEBUGGING_MOVE_CHAIN    // Print negamax (relative) score of move

                if (!bSuppressOutput) {
                    //if (d_score_value<0.0) ichars++;
                    fprintf(fpDebug, "%*s", (8-nCharsInMove), "");  // to line up for varying algebriac move size

                    fprintf(fpDebug, "%8.3f", d_score_value);
                    dSupressValue = d_score_value;
                    //bSuppressOutput = true;
                }
                else {
                    //assert(d_score_value==dSupressValue);
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
            //bool is_at_end_of_the_line = false;
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
                assert(0);
                // tie (within delta): flip a coin.
                b_use_this_move = engine.flip_a_coin();

                //cout << d_score_value << " = " << d_best_score << " , " << b_use_this_move << endl;     
  
            } else {
                b_use_this_move = (d_score_value > d_best_score);
            }

            if (b_use_this_move) {
                d_best_score = d_score_value;
                the_best_move = m;
            }


            // Record moves from the root.
            if (is_from_root) {

                #ifdef DEBUGGING_RANDOM_DELTA1
                    fprintf(fpDebug,
                            "  depth=%d top_deepening=%d maximum_deepening=%d nPlys=%d : ",
                            depth, top_deepening, maximum_deepening, nPlys);
                    engine.print_move_and_score_to_file(
                        {m, d_score_value}, false, fpDebug);
                #endif

                MovesFromRoot.emplace_back(m, d_score_value);  // record ROOT move & its (negamax) score (units are pawns)
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

                #ifdef _DEBUGGING_MOVE_CHAIN    // Beta cutoff (break move loop)
    
                    nChars = fputc('\n', fpDebug);
                    assert(nChars != EOF);

                    assert(nPlys>0);
                    engine.print_tabOver(nPlys, fpDebug);

                    sprintf(szDebug, "[%ld/%ld]", imovedebug+1, isizedebug);
                    fputs(szDebug, fpDebug);

                    // move_last
                    engine.move_into_string(move_last);
                    sprintf(szDebug, " Beta cutoff %f > %f   %s",  alpha, beta, engine.move_string.c_str());
                    fputs(szDebug, fpDebug);

                    // if (!engine.is_unquiet_move(m)){

                    //     char szTemp[64];
                    //     sprintf(szTemp, " Beta quiet cutoff %f %f",  alpha, beta);
                    //     fputs(szTemp, fpDebug);
                    //     engine.print_move_to_file(m, nPlys, state, false, false, false, fpDebug);
                    // }
                #endif

                // Record killer moves
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
                            
                            engine.print_move_history_to_file(fpDebug, "AA");
                        #endif
                    }
                    else if (!(m == killer1[nPlys])) {
                        killer2[nPlys] = m;
                    }
                }

              
                // Cutoff! (stop anaylizing moves to look at)
                break;
            }

        }   // End loop over all moves to look at

    }   // END non zero oves to look at


    // Fail-low
    if (!did_cutoff && (alpha <= alpha_in)) {
        //if (!did_cutoff  && (alpha <= alpha_in + TINY_SCORE)) {
        did_fail_low = true;
    }

    if (Features_mask & _FEATURE_TT2) {  // store in TT2

        int iLimit =1;  // (Features_mask & _FEATURE_ENHANCED_DEPTH_TT2) ? 0 : 1;       // 0 or 1 only



        int start = engine.boundary_stack.empty() ? 0 : engine.boundary_stack.back();

        int cnt = 0;
        uint64_t key = engine.game_board.zobrist_key;
        for (int i = start; i < (int)engine.key_stack.size(); ++i) {
            if (engine.key_stack[i] == key) ++cnt;
        }

        bool bStoreThis = ( (depth > iLimit) && 
                            (engine.game_board.halfmove < (FIFTY_MOVE_RULE_PLY/2)) &&
                            (cnt<=1)
                          );

        if (bStoreThis) {

            assert(qPlys==0);

            //if (!did_cutoff && !did_fail_low) {
            if ((alpha_in < d_best_score) && (d_best_score < beta)) {
                //
                //  This is an EXACT score (not an alpha/beta boundary).
                //
                uint64_t key = engine.game_board.zobrist_key;


                // Rolling size cap for TT2  (NOTE: 2 million?), This is a non-determinism that can break "burp2" TT2?
                static const std::size_t MAX_TT2_SIZE = 20000000;
                if (TTable2.size() >= MAX_TT2_SIZE) {
                    auto it = TTable2.begin();
                    if (it != TTable2.end()) {
                        TTable2.erase(it);
                    }
                }

                int cp_score_temp = engine.convert_to_CP(d_best_score);

                // --- DEBUG check
                #ifdef DEBUG_NODE_TT2        // Compare "found" record to actual situation now.
                {
                    if (foundPos && (foundDepth == depth) ) {            

                        bool bBothScoresMates = (IS_MATE_SCORE(foundRawScore) && IS_MATE_SCORE(d_best_score));

                        if (!bBothScoresMates) {

                            int iThreshold;
                        
                            iThreshold = BURP2_THRESHOLD_CP;

                            //assert(0);  // to maske sure we get here.

                            if ( abs(foundScore - cp_score_temp) > iThreshold) {
                                //
                                //  We found a burp.
                                bool isFailure = true;
                                int iRepCountNow = 0;
                                // auto itRepNow = engine.repetition_table.find(key);
                                // if (itRepNow != engine.repetition_table.end()) iRepCountNow = itRepNow->second;

                                char buf[256];
                                std::snprintf(
                                    buf, sizeof(buf),
                                    "\n%llu burp2 %ld = %ld    %.2f = %.2f     q=%ld    rep  %ld %ld \n",
                                    NhitsTT2,
                                    foundScore, cp_score_temp,
                                    foundRawScore, d_best_score,
                                    qPlys, foundRepCount, iRepCountNow
                                );
                                std::cout << buf;
                                if (fpDebug) {
                                    std::fputs(buf, fpDebug);

                                    // What we found in the table
                                    engine.print_move_history_to_file0(fpDebug, found_move_history);

                                    // What we have now from the analysis
                                    engine.print_move_history_to_file(fpDebug, "burp2 (actual)");

                                    std::fflush(fpDebug);   // force write to disk
                                }

                                print_mismatch(cout, "al", foundAlpha,         alpha_in);
                                print_mismatch(cout, "bt", foundBeta,          beta);
                                print_mismatch(cout, "dr", foundDraw,          (state == GameState::DRAW));
                                print_mismatch(cout, "ck", foundIsCheck,       in_check);
                                print_mismatch(cout, "lm", foundLegalMoveSize, legalMovesSize);
                                print_mismatch(cout, "rp", foundRepCount,      iRepCountNow);
                                print_mismatch(cout, "nPlys", foundnPlys, nPlys);

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

                                // print_mismatch(cout, "whit", found_white_castled, engine.game_board.bCastledWhite);
                                // print_mismatch(cout, "blac", found_black_castled, engine.game_board.bCastledBlack);

                                //  found_move_history

                                // cout << "  depth-> " << depth << " mv->" << engine.computer_ply_so_far 
                                // << " dbg->" << bBothScoresMates << " idelta-> " << idelta 
                                // << " , " << abs(foundScore - cp_score_temp)
                                // << endl;
                                
                                string out = gameboard_to_string2(engine.game_board);
                                cout << out << endl;

                                //isFailure = true;
                                char* pszTemp = ""; 
                                (engine.game_board.turn == ShumiChess::WHITE) ? pszTemp = "WHITE to move" : pszTemp = "BLACK to move";
                                cout << pszTemp << endl;

                                string mv_string1;
                                string mv_string2;
                                engine.bitboards_to_algebraic(engine.game_board.turn, foundMove, (GameState::INPROGRESS)
                                                , false, false, NULL, mv_string1);
                                engine.bitboards_to_algebraic(engine.game_board.turn, the_best_move, (GameState::INPROGRESS)
                                                , false, false, NULL, mv_string2);
                                cout << mv_string1 << " = " << mv_string2 << endl;

                
                                if (!(foundMove == the_best_move)) {
                                    Move deadmove =  {};
                                    bool isEmpty = (the_best_move == deadmove);
                                    cout << " burp2m " << (int)foundMove.piece_type  << " = "  << (int)the_best_move.piece_type << "  " << isEmpty << " "  
                                        << NhitsTT2 << " " << endl;

                                    isFailure = true;
                                }

                                if (isFailure) {
                                    #ifdef _DEBUGGING_TO_FILE
                                        if (fpDebug) {
                                            fflush(fpDebug);
                                            fclose(fpDebug);
                                            fpDebug = NULL;
                                        }
                                    #endif
                                    std::string temp_fen_ = engine.game_board.to_fen();
                                    sprintf (szDebug, "burp22 %llu      %s\n", nGames, temp_fen_.c_str());
                                    cout << szDebug;
                                    assert(0);
                                }
                            }
                            else {
                                NhitsTT2++;
                            }
                        }
                    }

                }   // END debug
                #endif

                //assert (!did_cutoff && !did_fail_low);
                    
                // this is an EXACT score (not an alpha/beta boundary).
                bool bdebug = false;

                // --- New entry: actually insert and fill TT2 slot ---
                TTEntry2 &slot = TTable2[key];   // this inserts, since we know !existed_before

                slot.score_cp  = cp_score_temp;
                slot.best_move = the_best_move;
                slot.depth     = depth;

                #ifdef DEBUG_NODE_TT2

                    slot.dAlphaDebug = alpha_in;
                    slot.dBetaDebug  = beta;

                    slot.nPlysDebug      = nPlys;
                    slot.drawDebug       = (state == GameState::DRAW);
                    slot.bIsInCheckDebug = in_check;
                    slot.legalMovesSize  = legalMovesSize;

                    int repCountNow = 0;
                    // auto itRepNow = engine.repetition_table.find(key);
                    // if (itRepNow != engine.repetition_table.end())
                    //     repCountNow = itRepNow->second;
                    slot.repCountDebug = repCountNow;

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

                    slot.move_history_debug = engine.move_history;
    
                    // slot.white_castled_debug = engine.game_board.bCastledWhite;
                    // slot.black_castled_debug = engine.game_board.bCastledBlack;

                    // Position specific debug 1
                    // --- Special debug for cxd4 / 338 ---
                    if (fpDebug)
                    {
                        char buf[128];
                        std::string mv_string11;
                        engine.bitboards_to_algebraic(
                            engine.game_board.turn,
                            the_best_move,
                            GameState::INPROGRESS,
                            false,
                            false,
                            nullptr,
                            mv_string11
                        );

                        if ((mv_string11 == "fxg4") && (cp_score_temp == 215))
                        {
                            // Report attempt to store, with size and whether key existed
                            std::sprintf(
                                buf,
                                "\n %s  store_attempt %d\n",
                                mv_string11.c_str(),
                                cp_score_temp
                            );
                            std::fputs(buf, fpDebug);
                            std::fflush(fpDebug);
                            bdebug = true;
                        }
                    }

                    // Position specific debug 2
                    if (bdebug && fpDebug)
                    {
                        char buf[128];
                        ull size_after = TTable2.size();
                        std::sprintf(
                            buf,
                            "  fxg4/338: INSERT new entry, size_after=%llu\n",
                            static_cast<unsigned long long>(size_after)
                        );
                        
                        std::fputs(buf, fpDebug);
                        engine.print_move_history_to_file(fpDebug, "insertNew");
                        std::fflush(fpDebug);
                    }   // END Position specific debug 2

                #endif  // END debug

            }      // END adding an entry to the TT2 (exact result)

        }        // END adding an entry to the TT2 (depth correct, just a stupid 0/1 feature)

    }     // END adding an entry to the TT2 feature
 
    assert(beta_in == beta);
    //assert(alpha_in < d_best_score && d_best_score < beta)
    //assert(alpha_in <= (d_best_score+FLT_EPSILON));

    #ifdef _DEBUGGING_MOVE_CHAIN    // Print summary: best move and best score
        int nChars;
        bool bSide = (engine.game_board.turn == ShumiChess::BLACK);

        // Summary always starts a new line
        nChars = fputc ('\n', fpDebug);
        assert(nChars != EOF);

        assert(nPlys>0);
        engine.print_tabOver(nPlys-1, fpDebug);

        nChars = fputs("BST=", fpDebug);
        assert(nChars != EOF);

        engine.print_move_to_file(move_last, -2, (GameState::INPROGRESS)
                                    , false, false, bSide
                                    , fpDebug);

        bSide = !bSide;
        engine.print_move_to_file(the_best_move, -2, (GameState::INPROGRESS)
                                    , false, false, bSide
                                    , fpDebug);
   
        //sprintf(szDebug, "%8.3fa", -d_best_score);
    
        nChars = fputs(szDebug, fpDebug);
        assert(nChars != EOF);

    #endif

    return {d_best_score, the_best_move};
}

//
//  Resort moves in this order (they will later be searched in this order):
//      0. move from the hash table hit (if any) 
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
    if (moves.empty()) 
    {
        return;
    }

    const bool have_last = !engine.move_history.empty();
    const ull  last_to   = have_last ? engine.move_history.top().to : 0ULL;

    
    //      2. unquiet moves (captures/promotions, sorted by MVV-LVA).  Captures more importent than promotions.
    //         If capture to the "from" square of last move, give it higher priority.
    //      3. killer moves (quiet, bubbled to the front of the "cutoff" quiet slice)
    //      4. remaining quiet moves.
    //
    if (Features_mask &_FEATURE_UNQUIET_SORT) {
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
                // MVV-LVA  Most Valuable Victim, Least Valuable Attacker: prefer taking the 
                // biggest victim with the smallest attackers.
                // This establishs two top "tiers", Victim and -attacker.
                int keyA = (a.capture != ShumiChess::Piece::NONE) ? engine.mvv_lva_key(a) << 10 : 0;
                int keyB = (b.capture != ShumiChess::Piece::NONE) ? engine.mvv_lva_key(b) << 10 : 0;

                // SEE: strongly penalize obviously losing captures
                // This is the third tier", as in SEE is the third tier (centipawns)
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

        // It is known that Killer moves force "TT2 unrepeatibility". The theory is I guess that the
        // later analysis is profited by these killer moves.
        #ifndef DEBUG_NODE_TT2
        if (Features_mask & _FEATURE_KILLER) {
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
                engine.print_move_history_to_file(fpDebug, "BB");
            #endif

            bring_front(killer2[nPlys]);

        }
        #endif

    }

    //       1. PV from the previous iteration (previous deepening’s best). 
    if (is_top_of_deepening) {
        assert(top_deepening > 0);
        assert(top_deepening == depth);

        const ShumiChess::Move pv_move = prev_root_best_[top_deepening-1].first;

        #ifdef _DEBUGGING_MOVE_CHAIN1
            engine.move_into_string(pv_move);
            fprintf(fpDebug, "PV try? %s ", engine.move_string.c_str());
        #endif


        if (!(pv_move == Move{})) {       
            auto it = std::find(moves.begin(), moves.end(), pv_move);
            if (it == moves.end()) {
                // item not in the list
                assert(0);
            }
            else if (it != moves.begin()) {
                // item not first in list
                // moves existing pv_move to front, preserving their relative order.
                std::rotate(moves.begin(), it, it + 1);        
                #ifdef _DEBUGGING_MOVE_CHAIN
                    fprintf(fpDebug, "PV bubble");
                #endif
            }
        }

    }

    //       0. move from the hash table hit (if any) 
    if (!(TT2_match_move == Move{})) {       
        auto it = std::find(moves.begin(), moves.end(), TT2_match_move);
        if (it == moves.end()) {
            // item not in the list
            assert(0);
        }
        else if (it != moves.begin()) {
            // item in list, but not first in list already
            // moves existing pv_move to front, preserving their relative order.
            std::rotate(moves.begin(), it, it + 1);        
            #ifdef _DEBUGGING_MOVE_CHAIN
                fprintf(fpDebug, "TT2_match_move bubble");
            #endif
        }
    }

  
}


// bool MinimaxAI::look_for_king_moves() const
// {
//     int king_moves = 0;
//     std::stack<ShumiChess::Move> tmp = engine.move_history; // copy; don't mutate engine

//     while (!tmp.empty()) {
//         ShumiChess::Move m = tmp.top(); tmp.pop();
//         if (m.piece_type == ShumiChess::Piece::KING) {
//             if (++king_moves >= 2) return true;
//         }
//     }
//     return false;
// }


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
        EvalPersons evp = UNCLE_SHUMI;
        int cp_score =  evaluate_board(color_perspective, evp, false) * color_multiplier;

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




// // Loop over all passed moves, find the best move by static evaluation.
// // Returns a tuple of: best_score and best move
// std::tuple<double, ShumiChess::Move>
// MinimaxAI::best_move_static(ShumiChess::Color for_color,
//                             const std::vector<ShumiChess::Move>& legal_moves,
//                             //int nPly,
//                             bool in_Check,
//                             int depth,
//                             bool bFast)
// {
//     double d_best_pawns;
//     ShumiChess::Move bestMove = ShumiChess::Move{};


//     // If there are no moves:
//     // - not in check: return a single static (stand-pat) eval
//     // - in check: treat as losing (no legal escapes here)
//     if (legal_moves.empty()) {
//         if (!in_Check) {

//             bool b_is_Quiet = !engine.has_unquiet_move(legal_moves);

//             int cp_score = evaluate_board(for_color, evp, b_is_Quiet);         // positive is good for 'color'
           
//             double best_score = engine.convert_from_CP(cp_score);

//             return { best_score, ShumiChess::Move{} };
//         }
//         return { -HUGE_SCORE, ShumiChess::Move{} };
//     }

//     d_best_pawns = -HUGE_SCORE;
//     int cp_score_best = 0;

//     for (const auto& m : legal_moves) {

//         assert(m.piece_type != Piece::NONE);
//         engine.pushMove(m);

//         // memoization
//         bool b_is_Quiet = !engine.has_unquiet_move(legal_moves);

//         int  cp_from_tt   = 0;
//         bool have_tt_eval = false;

//         #ifdef DOING_TT_EVAL2
//             // Salt the entry
//             unsigned mode  = salt_the_TT(b_is_Quiet);

//             uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

//             // Look for the entry in the TT
//             auto it = TTable.find(evalKey);
//             if (it != TTable.end()) {
//                 const TTEntry &entry = it->second;
//                 cp_from_tt   = entry.score_cp;
//                 have_tt_eval = true;
//             }
//         #endif
//         //
//         // evaluate (side call)
//         //
//         if (have_tt_eval) {
//             TT_ntrys1++;
//             if (Features_mask & _FEATURE_TT) {
//                 cp_score_best = evaluate_board(engine.game_board.turn, evp, b_is_Quiet);
//                 if (cp_from_tt != cp_score_best) {
//                     printf ("burp SIDE %ld %ld      %ld\n", cp_from_tt, cp_score_best, TT_ntrys);
//                     assert(0);
//                 }
//             }
//             cp_score_best = cp_from_tt;

//         }
//         else {
//             cp_score_best = evaluate_board(engine.game_board.turn, evp, b_is_Quiet);
//         }



//         double d_score = engine.convert_from_CP(cp_score_best);
    
//         if (Features_mask & _FEATURE_TT) {
//             if (!bFast) {
//                 // Salt the entry
//                 unsigned mode  = salt_the_TT(b_is_Quiet);

//                 uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

//                  // Store this position away into the TT
//                 TTEntry &slot = TTable[evalKey];
//                 slot.score_cp = cp_score_best;   // or cp_score, whatever you just got
//                 slot.movee    = Move{};
//                 slot.depth    = top_deepening;
//             }
//         }

//         engine.popMove();

//         if (d_score > d_best_pawns) {
//             d_best_pawns = d_score;
//             bestMove = m;
//         }
//     }

//     return { d_best_pawns, bestMove };
// }



//
// Picks a random root move within delta (in pawns) of the best score (negamax-relative).
// If list is empty: returns default Move{}.
// If best is "mate-like": returns the best (no randomization).
Move MinimaxAI::pick_random_within_delta_rand(std::vector<std::pair<Move,double>>& MovsFromRoot,
                                              double delta_pawns,
                                              int& n_moves_within_delta     // output
                                            )
{

    n_moves_within_delta = 0;
    if (MovsFromRoot.empty()) {
        assert(0);           // Better not happen  NOTE: but sometimes it does. 
                             // seems to be from odd button push combinations.
        return Move{};  // safety
    }

    // 0) Sort DESC by score (leaves the caller’s list sorted)
    std::sort(MovsFromRoot.begin(), MovsFromRoot.end(),
              [](const auto& a, const auto& b){ return a.second > b.second; });

    // 1) Best is now front()
    const double bestScorePawns = MovsFromRoot.front().second;

    // 2) If best score is mate-like, don’t randomize. Just pick the first one. 
    //    Here n_moves_within_delta is returned as zero.
    if (IS_MATE_SCORE(bestScorePawns)) return MovsFromRoot.front().first;

    // 3) Build the contiguous “within delta” prefix
    const double cutoff = bestScorePawns - delta_pawns;
    size_t n_top = 0;
    while (n_top < MovsFromRoot.size() && MovsFromRoot[n_top].second >= cutoff) ++n_top;

    // Return number of moves that qualified.
    n_moves_within_delta = (int)n_top;

    // (fallback if something odd, like nothing within the delta?)
    if (n_top == 0) {
        assert(0);      // better not happen
        return MovsFromRoot.front().first;
    }

    // #ifdef DEBUGGING_RANDOM_DELTA
    //     fprintf(fpDebug, "\n===============================================================\n");

    //     engine.print_moves_and_scores_to_file(MovsFromRoot, false, false, fpDebug);
 
    //     fprintf(fpDebug, "\n===============================================================\n");
    // #endif


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



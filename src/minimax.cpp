#define _CRT_SECURE_NO_WARNINGS     // To prevent dunb warnings about deprecated "strcpy/sprintf" like functions.

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
#include "score.hpp"
#include "salt.h"
#include "features.hpp"





using namespace std;
using namespace ShumiChess;
using namespace utility;
using namespace utility::representation;
using namespace utility::bit;


#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
    #undef NDEBUG
#endif   
#include <assert.h>
//
// NOTE: fix these
// These are still #defines when they should be features, or discarded:
//#define FAST_EVALUATIONS

/////////// Debug ////////////////////////////////////////////////////////////////////////////////////

// Debug   THESE SHOULD BE OFF. They are only for debugging

//#define _DEBUGGING_PUSH_POP

//#define _DEBUGGING_TO_FILE         // I must be defined to use either of the below
//#define _DEBUGGING_MOVE_CHAIN
//#define _DEBUGGING_MOVE_SORT
//#define _DEBUGGING_GAME
//#define DEBUGGING_TEMP

// extern bool bMoreDebug;
// extern string debugMove;

//#define DEBUGGING_RANDOM_DELTA

//#define DOING_TT_EVAL2       // used only in best_move_static (should be extinct)
//#define DEBUG_LEAF_TT

// #define DEBUG_NODE_TT2          // I must also be defined in the .hpp file to work
// #define BURP2_THRESHOLD_CP 2     // "burps" or fails if the stored (TT) does not match the evaluaton made.

//#define DEBUGGING_KILLER_MOVES 

//#define DEBUGGING_PAWN_HASH     // burp3

bool global_debug_flag = false;

#ifdef _DEBUGGING_TO_FILE   // Data used for debug
    FILE *fpDebug = NULL;
    char szDebug[512];
    bool bSuppressOutput = false;
    Score dSupressValue = 0.0;

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
        clearerr(fp); // optional: clears EOF/error
        return 0;
    }

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


#ifdef DEBUGGING_KILLER_MOVES
    static long long killer_tried = 0;
    static long long killer_cutoff = 0;
#endif

//////////// Displays ////////////////////////////////////////////////////////////

//#define DISPLAY_DEEPING     // Displays a lot of other stuff too

//#define DISPLAY_PULSE_CALLBACK_THREAD    // Uncomment to enable the callback to show "nPly", real time.
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

    //cout << "\033[1;34mNew Match\033[0m" << endl;

    // Initialize storage buffers (they live here to avoid extra allocation during the game)

    TTable.clear();
    TTable.reserve(1000000);    // NOTE: What size here? (we dont even normally use this table)
    
    TTable2.clear();
    TTable2.reserve(1000000);    // NOTE: What size here?

    pawn_file_info.clear();
    pawn_file_info.reserve(1000000);    // NOTE: What size here?

    // add the current position
    //engine.repetition_table.clear();
    //engine.repetition_table[key_now] = 1;

    uint64_t key_now = engine.game_board.zobrist_key;
    engine.three_time_rep_stack.push_back(engine.game_board.zobrist_key);

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

    excluded_root_moves.clear();

}



MinimaxAI::~MinimaxAI() { 
    #ifdef _DEBUGGING_TO_FILE   // close debug file
        if (fpDebug != NULL) fclose(fpDebug);
    #endif
}



template<class T> string MinimaxAI::format_with_commas(T value) {
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



//////////////////////////////////////////////////////////////////////////////////////
//
// ---------- trade_imbalance_cp_t ----------
// Position-only trade incentive. Safe for TT use.
//
// Requirement:
// If side c is ahead, and is not down in pieces, give a small bonus immediately.
// Then increase that bonus as side c's total material disappears.
//
// material_balance is polarized: positive is good for c.
// me_pawn_material is only c's pawn material, not a signed balance.
//
//////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////
//
// ---------- trade_imbalance_cp_t ----------
// Position-only trade incentive. Safe for TT use.
//
// Requirement:
// If side c is ahead, and is not down in non-pawn material, give a small
// bonus immediately. Then increase that bonus as side c's total material
// disappears.
//
// material_balance is polarized: positive is good for c.
// me_pawn_material is only c's pawn material, not a signed balance.
//
//////////////////////////////////////////////////////////////////////////////////////

template<ShumiChess::Color c> int MinimaxAI::trade_imbalance_cp_t(int material_balance, int me_pawn_material) const {
    
    constexpr Color enemy = utility::representation::opposite_color_t<c>;

    // I must be ahead.
    if (material_balance <= 0) return 0;

    // Get weights. All score values in this routine are centipawns.
    const int max_bonus = engine.game_board.wghts.GetWeight(TRADE_MAX_BONUS);
    const int advantage_denominator = engine.game_board.wghts.GetWeight(TRADE_ADVANTAGE_CAP);
    assert(max_bonus >= 0);
    assert(advantage_denominator > 0);

    // Get material values once.
    const int pawn_value   = engine.game_board.centipawn_score_of(Piece::PAWN);
    const int knight_value = engine.game_board.centipawn_score_of(Piece::KNIGHT);
    const int bishop_value = engine.game_board.centipawn_score_of(Piece::BISHOP);
    const int rook_value   = engine.game_board.centipawn_score_of(Piece::ROOK);
    const int queen_value  = engine.game_board.centipawn_score_of(Piece::QUEEN);

    // Count my pieces.
    const int me_knights = engine.game_board.Bits_In[c][Piece::KNIGHT];
    const int me_bishops = engine.game_board.Bits_In[c][Piece::BISHOP];
    const int me_rooks   = engine.game_board.Bits_In[c][Piece::ROOK];
    const int me_queens  = engine.game_board.Bits_In[c][Piece::QUEEN];

    // Count enemy material.
    const int enemy_pawns   = engine.game_board.Bits_In[enemy][Piece::PAWN];
    const int enemy_knights = engine.game_board.Bits_In[enemy][Piece::KNIGHT];
    const int enemy_bishops = engine.game_board.Bits_In[enemy][Piece::BISHOP];
    const int enemy_rooks   = engine.game_board.Bits_In[enemy][Piece::ROOK];
    const int enemy_queens  = engine.game_board.Bits_In[enemy][Piece::QUEEN];

    // Compare non-pawn material.
    // This allows exchange-up cases, even if I have fewer minor pieces.
    const int me_piece_material =
        me_knights * knight_value +
        me_bishops * bishop_value +
        me_rooks   * rook_value +
        me_queens  * queen_value;

    const int enemy_piece_material =
        enemy_knights * knight_value +
        enemy_bishops * bishop_value +
        enemy_rooks   * rook_value +
        enemy_queens  * queen_value;

    // I must not be down in non-pawn material.
    if (me_piece_material < enemy_piece_material) return 0;

    // My total material includes pawns and pieces.
    const int me_material =
        me_pawn_material +
        me_piece_material;

    // Enemy total material includes pawns and pieces.
    const int enemy_material =
        enemy_pawns * pawn_value +
        enemy_piece_material;

    // No reason to encourage trades if enemy has no material left.
    if (enemy_material <= 0) return 0;

    // Advantage factor numerator. This is capped.
    int advantage_numerator = material_balance;
    if (advantage_numerator > advantage_denominator) {
        advantage_numerator = advantage_denominator;
    }

    // Simplification factor numerator.
    // Give a small floor immediately, then increase the bonus as my total
    // material disappears. Total material includes pawns and pieces.
    const int simplification_denominator = MAX_CP_PER_SIDE;
    const int base_simplification_numerator = 3 * pawn_value;

    int simplification_numerator = simplification_denominator - me_material;
    if (simplification_numerator < 0) {
        simplification_numerator = 0;
    }

    simplification_numerator += base_simplification_numerator;

    if (simplification_numerator > simplification_denominator) {
        simplification_numerator = simplification_denominator;
    }

    // max_bonus * (advantage_numerator / advantage_denominator)
    //           * (simplification_numerator / simplification_denominator)
    // Use long long because the intermediate product can overflow int.
    const long long numerator =
        (long long)max_bonus *
        (long long)advantage_numerator *
        (long long)simplification_numerator;
    assert(numerator >= 0);

    const long long denominator =
        (long long)advantage_denominator *
        (long long)simplification_denominator;
    assert(denominator > 0);

    return (int)((numerator + (denominator / 2)) / denominator);
}




bool MinimaxAI::no_queens_on_board() {
    if (engine.game_board.white_queens != 0 ) return false;
    if (engine.game_board.black_queens != 0 ) return false;
    return true;
}

// material_cp must be average material (positive)
int MinimaxAI::phase_of_game(int material_cp_avg) {

    int king_sq;
    //int king_sq2;
    int k_file;
    int k_rank;
   //ull king_bb;

    int nPhase = GamePhase::OPENING;
    assert(material_cp_avg>=0);


    // white castling
    // king_bb = engine.game_board.get_pieces(ShumiChess::Color::WHITE, Piece::KING);
    // if (!king_bb) {
    //     assert(0);      // has to be a king!
    //     return 0;
    // }
    //king_sq2 = utility::bit::bitboard_to_lowest_square_fast(king_bb);
    king_sq = engine.white_king_square;
    //assert(king_sq == king_sq2);


    k_file  = king_sq & 7;
    k_rank  = king_sq >> 3;

    engine.game_board.bWhiteCstled = engine.game_board.bHasCastled_fake_t<ShumiChess::Color::WHITE>(k_rank, k_file);

    // black castling
    // king_bb = engine.game_board.get_pieces(ShumiChess::Color::BLACK, Piece::KING);
    // if (!king_bb) {
    //     assert(0);      // has to be a king!
    //     return 0;
    // }
    //king_sq2 = utility::bit::bitboard_to_lowest_square_fast(king_bb);
    king_sq = engine.black_king_square;
    //assert(king_sq == king_sq2);

    k_file  = king_sq & 7;
    k_rank  = king_sq >> 3;
    engine.game_board.bBlackCstled = engine.game_board.bHasCastled_fake_t<ShumiChess::Color::BLACK>(k_rank, k_file);


    //
    // Each side has MAX_CP_PER_SIDE centipawns at start. 
    //
    int lost_so_far_cp = (MAX_CP_PER_SIDE - material_cp_avg);

    if (lost_so_far_cp < 0) {   // Can happen if pawns queen, early in game. So what Its still the opening.
        // cout << "lost_so_far=" << lost_so_far;
        lost_so_far_cp = 0;      // Can happen if pawns queen early. So what Its still the opening.
        //assert(0);
    }

    // So 6 down is 2 pieces (per side). Or one piece and 3 pawns.  40 is all pieces.
    // 
    if      (lost_so_far_cp < 400)  nPhase = GamePhase::OPENING;
    else if (lost_so_far_cp < 800)  nPhase = GamePhase::MIDDLE_EARLY;
    else if (lost_so_far_cp < 1400) nPhase = GamePhase::MIDDLE;
    else if (lost_so_far_cp < 3000) nPhase = GamePhase::ENDGAME;
    else                            nPhase = GamePhase::ENDGAME_LATE; // only 1000 cp left

    bool b_both_castled = (engine.game_board.bWhiteCstled && engine.game_board.bBlackCstled);
    bool b_either_castled = (engine.game_board.bWhiteCstled || engine.game_board.bBlackCstled);

    if (nPhase == GamePhase::OPENING) {
        if (b_both_castled) nPhase = GamePhase::MIDDLE;         // Turns off all castling incentives
        if (no_queens_on_board()) nPhase = GamePhase::MIDDLE;
    }
    else if (nPhase == GamePhase::MIDDLE_EARLY) {
        if (b_both_castled) nPhase = GamePhase::MIDDLE;         // Turns off all castling incentives
        if (no_queens_on_board()) nPhase = GamePhase::MIDDLE;
    }

    return nPhase;
}


// Same as phase_of_game(), but gets the material(s) by itself. For display only. (too slow)
int MinimaxAI::phase_of_game_full() {
    int cp_score_material_avg_local = 0;
    {
        int cp_pawns_only_temp = 0;
        int cp_score_mat_temp;

        cp_score_mat_temp = engine.game_board.get_material_for_color_t<Color::WHITE>(cp_pawns_only_temp);
        assert(cp_score_mat_temp >= 0);
        cp_score_material_avg_local += cp_score_mat_temp;

        cp_score_mat_temp = engine.game_board.get_material_for_color_t<Color::BLACK>(cp_pawns_only_temp);
        assert(cp_score_mat_temp >= 0);
        cp_score_material_avg_local += cp_score_mat_temp;
    }

    cp_score_material_avg_local = cp_score_material_avg_local / 2;

    int nPhase = phase_of_game(cp_score_material_avg_local); 
    return nPhase;
}

///////////////////////////////////////////////////////////////////////////////////


//
// Only returns false is if user aborts.
//
tuple<Score, Move> MinimaxAI::do_a_deepening(int depth, ull elapsed_time_display_only, const Move& null_move) {

    tuple<Score, Move> ret_val;

    int nPlys = 0;
    int qPlys = 0;

    top_deepening = depth;      // deepening starts at this depth
    int aspiration_tries = 0;   // safety fuse
    Score widen = 0.5;         // example margin (in pawns)


    Score alpha = -HUGE_SCORE;
    Score beta = HUGE_SCORE;

    // assume: prevScore holds last iteration's exact root score (in pawns)
     
    if (0) {  // (depth > 1) {
        Score prevScore;
        prevScore = prev_root_best_[depth - 1].second;   // score in pawns
        alpha = prevScore - widen;
        beta  = prevScore + widen;
    } else {
        alpha = -HUGE_SCORE;         // d == 1 → full window
        beta  =  HUGE_SCORE;
    }

    // the beast (root node of root nodes)
    bool bStillAspiring  = false;
    ull last_elapsed = 1;
    do {

        #ifdef DEBUGGING_RANDOM_DELTA1
            fprintf(fpDebug, "\n");
        #endif
        // elapsed_time_display_only is from beginning of calculation
        // Ha.
        #ifdef DISPLAY_DEEPING      // i_duration_requested
            if (!last_elapsed) ratioLast = 0;
            else               ratioLast = elapsed_time_display_only / last_elapsed;
            last_elapsed = elapsed_time_display_only;
            if (depth==1) cout << "\n";
            cout << endl << aspiration_tries << " Deeping " << depth << " ply of " << maximum_deepening
                        << " msec=" << std::setw(6) << elapsed_time_display_only << '/' << ratioLast << ' ';
        #endif

        ret_val = recursive_negamax(depth
                                    , alpha, beta
                                    , true              // I am called from the root
                                    , null_move
                                    , (nPlys+1)
                                    , qPlys
                                 );

        // ret_val is a tuple of the score and the move.
        Score d_Return_score = get<0>(ret_val);
        if (d_Return_score == ABORT_SCORE) return ret_val;
        
        if (d_Return_score == ONLY_MOVE_SCORE) {
            return ret_val;
            //continue;
        }
        
        // Aspiration (just a guard right now)
        //printf("alpha, this, beta %f  %f  %f", alpha, d_Return_score, beta);
        #define ALPHA_BETA_FUZZ 1.0e10
        //assert((alpha <= d_Return_score) && (d_Return_score <= beta));
        assert( d_Return_score >= alpha - ALPHA_BETA_FUZZ &&
                d_Return_score <= beta  + ALPHA_BETA_FUZZ);     

        // // --- What WOULD happen if the score were outside [alpha, beta] ---
        // // With an infinite window these branches are unreachable right now,
        // // but this is the template you’ll use once you narrow the window.

        // if (alpha > d_Return_score) {
        //     // Fail-low: score came in at or below alpha
        //     std::cout << "\x1b[38;2;255;165;0mfail low\x1b[0m" << std::endl;
        //     Score widened = alpha - widen;
        //     alpha = (widened < -HUGE_SCORE) ? -HUGE_SCORE : widened;
        //     //Score newBeta  = beta; // keep upper bound
        //     widen *= 2.0;  // widen window on fail-low

        //     // log_fail_low(depth, alpha, beta, d_Return_score, newAlpha, newBeta);
        //     // d_Return_score = search(position, depth, newAlpha, newBeta);
        //     bStillAspiring  = true;
        // }
        // else if (d_Return_score > beta) {
        //     // Fail-high: score came in at or above beta
        //     std::cout << "\x1b[38;2;255;165;0mfail high\x1b[0m" << std::endl;
        //     Score widened = beta + widen;
        //     //Score newAlpha = alpha; // keep lower bound
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
//     i_time_requested         -  requested time to spend (milliseconds)
//     max_deepening_requested  -  requested depth
//
//////////////////////////////////////////////////////////////////////////////////
//
// This is a "root position". The next human move triggers a new root position
Move MinimaxAI::get_move_iterative_deepening(int i_duration_requested, int max_deepening_requested, int player_id, int iRandomMoves
                , int feat) {  


    if (iRandomMoves > 0) {
        //engine.i_randomize_next_move = iRandomMoves;
        engine.set_random_on_next_move(iRandomMoves);
    }

    assert (i_duration_requested > 0);

    //cout << " max_deepening_requested " << max_deepening_requested << "\n";

    // Obtain time now (milliseconds)
    TIME_TYPE start_of_calculation = chrono::high_resolution_clock::now();
    TIME_TYPE last_depth_time = start_of_calculation;
 
 

    eval_person = (ShumiChess::EvalPersons)player_id;   

    //cout << "\n FEAT = 0x" << hex << feat << dec << "\n";
    Features_mask = feat;


    //Move null_move = Move{};
    Move null_move = engine.users_last_move;    // Note: Just to make the move history work?


    stop_calculation = false;

	#ifdef DISPLAY_PULSE_CALLBACK_THREAD   // Used for debug display only
    	start_callback_thread();
    #endif

    #ifdef _DEBUGGING_GAME
        fprintf(fpDebug, "\n %ld   game %lld  move %ld  player=%ld\n"
            , engine.game_board.turn, nGames, engine.computer_ply_so_far, player_id);
    #endif

    if (engine.computer_ply_so_far == 0) {
        //
        // Here we do MinimaxAI stuff to be done at the beginning of a game. Clumsy way to have to detect 
        // this, but there it is: engine.reset_engine() starts a new game, and sets computer_ply_so_far = 0.
        //
        string sss;
        TTable2.clear();    // Clear even if we don't use it.

        //sss = format_with_commas(pawn_file_info.size());
        //printf("pawn_file_info size before clear = %s\n", sss.c_str());
        
        pawn_file_info.clear();

        //sss = format_with_commas(pawn_file_info.size());
        //printf("pawn_file_info size after clear = %s\n", sss.c_str());
        //system("pause");

        // Initialize hash table hit counts
        NhitsTT = 0;
        NhitsTT2 = 0;
        NhitsP = 0;
        NTriesP = 0;
        nRandos = 0;

        nGames++;

    }   

    nFarts = 0;             // Queiseence low level (forced eval) this move
    // Clear debug every move
    // #ifdef _DEBUGGING_TO_FILE 
    //     clear_file_keep_fp(fpDebug);
    // #endif

    // NOTE: In 2 computers playing this is in plys. If one human, its in moves.
    engine.computer_ply_so_far++;                      // Increment real moves in whole game

    #ifdef DISPLAY_DEEPING
        cout << "\x1b[94m\n\nMove: " << engine.computer_ply_so_far << "\x1b[0m";
    #endif

    //engine.gamePGN.add(engine.users_last_move, engine);

    nodes_visited = 0;
    nodes_visited_depth_zero = 0;
    evals_visited = 0;
    NhitsP = 0;
    NTriesP = 0;

    TTable.clear();       // Leaf TT cleared on every move (even if never used)

    Move best_move = {};
    Score d_best_move_score = 0.0;

  
    int this_deepening;

    this_deepening = max_deepening_requested;
    if ( (engine.i_randomize_next_move>0) && (this_deepening>2) ) {
        this_deepening--;
        i_duration_requested /= 6;
    }

    // Obey requested deepening and duration
    //  (not actually a "endtime". The last deepening will start before or at this "requested" time)
    TIME_TYPE requested_end_time = start_of_calculation + std::chrono::milliseconds(i_duration_requested); 
    maximum_deepening = this_deepening;
    maximum_duration = i_duration_requested;
    //cout << " maximum_deeeeeeeeeepening " << maximum_deepening << "\n";

    // defaults
    int depth = 1;
    int nPlys = 0;

    // INitialize these, whether we use them or not.
    for (int ii=0;ii<MAX_PLY;ii++) {
        killer1[ii] = {}; 
        killer2[ii] = {};
    }

    ull elapsed_time = 0ULL; // in msec
    tuple<Score, Move> ret_val;

    int n_Multis = 1;
    //
    // The -r option, runs MultiPV
    if (engine.i_randomize_next_move>0) {
        assert(RANDOM_MOVE_CANDIDATES>1);           // I must be greater than 1
        n_Multis = RANDOM_MOVE_CANDIDATES;          // We get this many options to choose from.
        engine.i_randomize_next_move--;             // Decrement number of randome moves to make
    }

    excluded_root_moves.clear();

    //
    // This loop always runs at least once. If only once, then there are no "random"/MultiPV moves
    for (int ii=0; ii<n_Multis; ii++) {
        
        // These are normally initialized at move start, but when doing MultiPV, qw init at start of every PV.
        nodes_visited = 0;
        start_of_calculation = chrono::high_resolution_clock::now();
        elapsed_time = 0ULL; // in msec

        ret_val = do_a_principal_variation(depth, null_move
                                        , start_of_calculation, i_duration_requested, requested_end_time
                                        , elapsed_time);        // Output
        d_best_move_score = get<0>(ret_val);
        if (d_best_move_score == ABORT_SCORE) break;
        //if (d_Return_score == ONLY_MOVE_SCORE)

        best_move = get<1>(ret_val);    

        engine.bitboards_to_algebraic(engine.game_board.turn, best_move
                , (GameState::INPROGRESS), false, false, NULL
                , engine.move_string);    // Output

        if (best_move.piece_type == Piece::NONE) break;     // NOTE: should this ever happen?

        excluded_root_moves.push_back(std::make_pair(best_move, d_best_move_score));
        if (d_best_move_score == ONLY_MOVE_SCORE) break;

        //cout << " pvar! " << d_best_move_score << " " << engine.move_string << " \n";

    }

    #ifdef DISPLAY_DEEPING
        if (n_Multis>1) cout << "\n" << " rand moves collected: " << n_Multis;
    #endif
    //engine.print_moves_and_scores_to_file(excluded_root_moves, false, false, stdout);

    if (n_Multis > 1) {

        tuple<Score, Move> ret_val0;
        int i_random_delta_cp = RANDOMIZING_EQUAL_MOVES_DELTA;
        int n_moves_within_delta;
        ret_val0 = pick_random_within_delta_rand(excluded_root_moves, i_random_delta_cp, engine.computer_ply_so_far
                                                    , n_moves_within_delta);      // output

        d_best_move_score = std::get<0>(ret_val0);
        best_move = std::get<1>(ret_val0);

        engine.bitboards_to_algebraic(engine.game_board.turn, best_move
                , (GameState::INPROGRESS), false, false, NULL
                , engine.move_string);    // Output

        #ifdef DISPLAY_DEEPING
            cout << "\n" << " end multisss " << d_best_move_score << "  "  
                    << engine.move_string << " out of=" <<  n_moves_within_delta << " \n";
        #endif
    }

    d_best_move_score_rel = d_best_move_score;

    // Convert the moves "relative score" to "absolute score". Relative means positve is good for mover. 
    // Absolute means positive is good for white. Most scores are relative. Absolute used only for display.
    Score d_best_move_score_abs = d_best_move_score;
    if (engine.game_board.turn == Color::BLACK) d_best_move_score_abs = -d_best_move_score_abs;
    if (std::abs(d_best_move_score_abs) < VERY_SMALL_SCORE) d_best_move_score_abs = 0.0;   // avoid negative zero
    

    ////////// done with move production. Now do debug and displays /////////////////////////////////////////////////////////////////////////
 
    //// for testing enpassant (pauses when it is move on board)
    // if (best_move.is_en_passent_capture) {
    //     printf("pause (press Enter)\n");
    //     fflush(stdout);
    //     getchar();
    // }

    int iPhase = phase_of_game_full();
    engine.game_phase = iPhase;


    TIME_TYPE now_time = chrono::high_resolution_clock::now();

    chrono::duration<double> total_time = now_time - start_of_calculation;
    chrono::duration<double> last_depth_delta_time = now_time - last_depth_time;

    last_depth_time = now_time;

    // Total time since this move search started.
    assert(total_time.count() > 0);
    double nodes_per_sec = nodes_visited / total_time.count();      // NPS   nodes per second
    iNodes_per_Second = (int)nodes_per_sec;

    // Time used by the deepening that just finished.
    double last_depth_seconds = last_depth_delta_time.count();
    int last_depth_ms = (int)(1000.0 * last_depth_seconds);

    #ifdef DISPLAY_DEEPING
        // Playground is for extended debug and status, and for whatever else. However there is a gotcha. Although we 
        // have found out move, we have not yet made the move. The found move is reported to the python, that then
        // calls engine_communicator_make_move_two_acn() to make the move.
        playground(iPhase);

        double elapsed_time_min = elapsed_time / 1000.0 / 60.0;
        cout << "\x1b[33m\nWent to depth " << (depth - 1)  
            << " elapsed min= " << elapsed_time_min
            << " elapsed msec= " << elapsed_time << " request msec= " << i_duration_requested 
            << std::fixed << std::setprecision(1)
            << "\x1b[0m" << endl;

        // Show board
        engine.bitboards_to_algebraic(engine.game_board.turn, best_move
                    , (GameState::INPROGRESS), false, false, NULL
                    , engine.move_string);    // Output
        cout << colorize(AColor::BRIGHT_CYAN,engine.move_string) << "   ";
 
        #ifdef _DEBUGGING_GAME
            fprintf(fpDebug, "%s game %lld  move %ld  player=%ld\n", engine.move_string.c_str(), nGames, engine.computer_ply_so_far, player_id);
        #endif
        
        string abs_score_string = to_string(d_best_move_score_abs);
        char buf[32];
        std::sprintf(buf, fmtMain, d_best_move_score_abs);
        abs_score_string = buf;

        cout << colorize(AColor::BRIGHT_CYAN, abs_score_string + " =score,  ");

        assert (nodes_visited!=0);
        double percent_depth_zero = nodes_visited ? ( (double)nodes_visited_depth_zero / (double)nodes_visited ) : 0.0;

        char pct[32];
        snprintf(pct, sizeof(pct), "%.0f", percent_depth_zero * 100.0);

        cout << colorize(
            AColor::BRIGHT_YELLOW,
            "Visited: " + format_with_commas(nodes_visited) +
            " / " + std::string(pct) + "% nodes total" +
            " ---- " + format_with_commas(evals_visited) + " Evals"
        ) << endl;

        chrono::duration<double> total_time2 = chrono::high_resolution_clock::now() - start_of_calculation; // still prints seconds
        //cout << total_time << endl;
        double dElapsedTime = total_time2.count();
        cout << colorize(AColor::BRIGHT_GREEN, (static_cast<std::ostringstream&&>(std::ostringstream()
            << "Total time: " << std::fixed << std::setprecision(2) << dElapsedTime << " sec")).str());

        ull running_time_msec = (ull)(dElapsedTime * 1000.0);
        if (engine.game_board.turn == ShumiChess::WHITE) 
            engine.game_white_time_msec += running_time_msec;
        else 
            engine.game_black_time_msec += running_time_msec;



        double evals_per_sec = evals_visited / total_time.count();
        cout << colorize(AColor::BRIGHT_GREEN, 
            "   nodes/sec= " + format_with_commas(std::llround(nodes_per_sec)) + 
            "   evals/sec= " + format_with_commas(std::llround(last_depth_seconds))) << endl;

  #endif


    // if we are forcing the move due to it being the only move, then don't update "score". Note: not sure of this.
    bool b_Forced = (d_best_move_score == ONLY_MOVE_SCORE);
    if (!b_Forced) {
        engine.d_bestScore_at_root = d_best_move_score_abs;  
    }

    // Note: this is required or tehe app hangs near the end of the game in autoplay?
    #ifdef DISPLAY_DEEPING
        cout << "\n";
    #endif

    #ifdef DEBUGGING_KILLER_MOVES
        cout << "Killers: tried=" << killer_tried
            << " cutoffs=" << killer_cutoff
            << " (" << (killer_tried ? (100.0 * killer_cutoff / killer_tried) : 0.0)
            << "%)\n\n";
    #endif


    
    ////////// done with "main" move displays /////////////////////////////////////////////////////////////////////////
  
    return best_move;
}


void MinimaxAI::playground(int iPhase) {
    //
    // Debug only  playground. Sandbox for testing evaluation functions
    // int isolanis;
    bool isOK;

    //int iNearSquares;
    //int king_near_squares_out[9];
    //ull utemp1;

    isOK = false;
    //ull holes;
    int itemp1=0;
    int itemp2=0;

    //PawnFileInfo pwnFileInfo;
    // ull holes_bb;
    // int holes_cp;
    // ull passed_pawns;
    // int passed_cp;      



    // isOK = engine.game_board.build_pawn_file_summary_t<Color::WHITE>( pwnFileInfo.p[0]);
    // isOK = engine.game_board.build_pawn_file_summary_t<Color::BLACK>( pwnFileInfo.p[1]);



    //int material_balance = ;  // evaluate_board();
    //itemp1 = engine.game_board.opposite_bishops_cp_t(material_balance);


    itemp1 = sizeof(PInfo);
    char szTemp[128];
    const char* pszPhase = &szTemp[0];
    pszPhase = str_from_GamePhase(iPhase);
    cout << "\n\n" << pszPhase << "  wht " << itemp1 << "           blk " << itemp2 << endl;

    // cout << "blk " << itemp1 <<  "  " << itemp2 <<  "  " << itemp3 <<  "  " << itemp4 << endl;

    //utemp1 = pawn_file_info.size();
    string sss1 = format_with_commas(NTriesP); 
    //string sss1 = format_with_commas(utemp1);
    string sss2 = format_with_commas(NhitsP);
    
    string sss3 = format_with_commas(evals_visited);
   
    cout << "PinfoTries: " << sss1 << " PinfoHits= " << sss2 << "  evals= " << sss3 << endl;
 

    cout << "nFarts: " << nFarts << "  "  << nSemiFarts << "  " << endl;

    //engine.debug_print_repetition_table();

	#ifdef DISPLAY_PULSE_CALLBACK_THREAD
    	stop_callback_thread();
    #endif

// engine.game_board.compute_bits_in();
// int white_pawns_only = 0;
// int black_pawns_only = 0;

// int white_material = engine.game_board.get_material_for_color_t<Color::WHITE>(white_pawns_only);
// int black_material = engine.game_board.get_material_for_color_t<Color::BLACK>(black_pawns_only);
// assert(white_material >= 0);
// assert(black_material >= 0);

// itemp1 = trade_imbalance_cp_t<Color::WHITE>((white_material - black_material), white_pawns_only);
// itemp2 = trade_imbalance_cp_t<Color::BLACK>((black_material - white_material), black_pawns_only);

// cout << "\n" << pszPhase << "  www " << (white_material - black_material) << "  " << white_pawns_only 
//      << "           bbb " << (black_material - white_material) << "  " << black_pawns_only << endl;

// cout << "\n\n" << pszPhase << "  tempW " << itemp1 << "           tempB " << itemp2 << endl;


// cout << "COUNTS"
//      << "  W minors=" << (int)(engine.game_board.Bits_In[Color::WHITE][Piece::KNIGHT] +
//                                engine.game_board.Bits_In[Color::WHITE][Piece::BISHOP])
//      << " rooks="     << (int)engine.game_board.Bits_In[Color::WHITE][Piece::ROOK]
//      << " queens="    << (int)engine.game_board.Bits_In[Color::WHITE][Piece::QUEEN]
//      << "     B minors=" << (int)(engine.game_board.Bits_In[Color::BLACK][Piece::KNIGHT] +
//                                   engine.game_board.Bits_In[Color::BLACK][Piece::BISHOP])
//      << " rooks="        << (int)engine.game_board.Bits_In[Color::BLACK][Piece::ROOK]
//      << " queens="       << (int)engine.game_board.Bits_In[Color::BLACK][Piece::QUEEN]
//      << endl;

// cout << "TRADE WEIGHTS"
//      << " max=" << engine.game_board.wghts.GetWeight(TRADE_MAX_BONUS)
//      << " cap=" << engine.game_board.wghts.GetWeight(TRADE_ADVANTAGE_CAP)
//      << " max_side=" << MAX_CP_PER_SIDE
//      << endl;


}

////////// loop over all "deepenings" /////////////////////////////////////////////////////////////////////////
//
//  elapsed_time is an output
//
std::tuple<Score, ShumiChess::Move> MinimaxAI::do_a_principal_variation(int depth, ShumiChess::Move null_move
                                        , TIME_TYPE start_time, int i_duration_requested, TIME_TYPE requested_end_time
                                        , ull& elapsed_time)      // output
{
// Ha.
    bool b_Forced = false;

    bool bThinkingOver = false;
    bool bThinkingOverByTime = false;
    bool bThinkingOverByDepth = false;

    long long now_s;    // milliseconds 
    long long end_s;    // milliseconds
    long long diff_s;   // Holds (actual time - requested time). Positive if past due. Negative if sooner than expected

    Move best_move = {};
    Score d_best_move_score = 0.0;
    
    elapsed_time = 0ULL; // in msec


    Score d_Return_score = 0.0;

    tuple<Score, Move> ret_val;

    do {

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
            //cout << gameboard_to_string_old(engine.game_board) << endl;
            //assert(0);      // NOTE: this happens close to draws. Noone knows why.
            break;   // Stop deepening, no more depths.
        }

        // the beast

        // Ha. Here we pass in the elapsed time, just for display, before the deeping.
        ret_val = do_a_deepening(depth, elapsed_time, null_move);

        d_Return_score = get<0>(ret_val);
        if (d_Return_score == ABORT_SCORE) {
            // User aborted the computation. Here we do nothing, ends up using the last deepeining. 
            cout << "\x1b[31m Aborting depth of " << depth << "\x1b[0m" << endl;
            break;   // Stop deepening, no more depths.
        } else if (d_Return_score == ONLY_MOVE_SCORE) {
            // We stopped analysis because there was only one legal move. We just play that.
            // Intersting, what should be the score here? 
            b_Forced = true;
            d_best_move_score = d_Return_score;
            best_move = get<1>(ret_val);        // this was the only legal move.
            break;   // Stop deepening, no more depths.
        } else {
            d_best_move_score = d_Return_score;
            best_move = get<1>(ret_val);

            // Root sees a forced mate: no point deepening further. 
            // Update: YES there is a point in continuing. We might find a shorter mate.
            //if (std::fabs(d_best_move_score) >= HUGE_SCORE/2.0)
            // if (IS_MATE_SCORE(d_best_move_score))
            // {
            //     cout << "\x1b[31m !!!!!!!! mate at exterior node (depth " << depth << ")\x1b[0m" << endl;

            //     #ifdef _DEBUGGING_TO_FILE1
            //         //engine.print_move_history_to_file(fpDebug);    // debug only
            //         cout << gameboard_to_string(engine.game_board) << endl;
            //         assert(0);
            //     #endif

            //     break;   // Stop iterative deepening immediately.
            // }

        }

        // Store PV for the *next* deepening iteration. Called incorrectly in literture as: "PV at the root".
        assert (depth >=0);
        prev_root_best_[depth] = std::make_pair(best_move, d_best_move_score);   // move + score (pawns)
        #ifdef _DEBUGGING_PV
            engine.move_into_string(best_move);
            sprintf(szDebug, "PV   %s  %ld", engine.move_string.c_str(), depth);
            fprintf(fpDebug, szDebug);
        #endif

        #ifdef DISPLAY_DEEPING
            engine.bitboards_to_algebraic(engine.game_board.turn, best_move
                    , (GameState::INPROGRESS), false, true, NULL
                    , engine.move_string);    // Output
            double temp = convert_to_pawns(d_best_move_score);
            if (engine.game_board.turn == ShumiChess::BLACK) temp = -temp;
            if (std::abs(temp) < convert_to_pawns(VERY_SMALL_SCORE)) temp = 0.0;
            char score_buf[32];
            if (temp < 0) {
                std::sprintf(score_buf, "%.2f", temp);
            }
            else {
                std::sprintf(score_buf, "%+.2f", temp);
            }
            engine.move_string += " ; ";
            engine.move_string += score_buf;
            cout << " Best: " << engine.move_string;
        #endif


        auto now_time = chrono::high_resolution_clock::now();

        elapsed_time = (ull)chrono::duration_cast<chrono::milliseconds>(now_time - start_time).count();
        diff_s = (long long)elapsed_time - (long long)i_duration_requested;

        // (Optional: keep these only for your printout)
        now_s = (long long)chrono::duration_cast<chrono::milliseconds>(now_time.time_since_epoch()).count();
        end_s = (long long)chrono::duration_cast<chrono::milliseconds>(requested_end_time.time_since_epoch()).count();

        depth++;

        // time based ending of thinking
        //bThinkingOverByTime = (diff_s > 0);
        // cout << "         weee " << elapsed_time << " >= " << i_duration_requested;
        bThinkingOverByTime = (elapsed_time >= (ull)i_duration_requested);


        // depth based ending of thinking
        bThinkingOverByDepth = (depth >= (maximum_deepening+1));

        // we are done thinking if both time and depth is ended OR simple endgame is on.
        bThinkingOver = (bThinkingOverByDepth && bThinkingOverByTime);

    } while (!bThinkingOver);


    max_attained_depth = depth-1;

    return { d_best_move_score, best_move };
}



////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Choose the "minimax" AI move.
// Returns a tuple of:
//    the best score (however, if this is ABORT_SCORE, its an abort)
//    the best move
//
////////////////////////////////////////////////////////////////////////////////////////////////////

tuple<Score, Move> MinimaxAI::recursive_negamax(
                    int depth
                    ,Score alpha, Score beta
                    ,bool is_from_root
                    ,const ShumiChess::Move& move_last      // seems to be used for debug only... (used only by _DEBUGGING_MOVE_CHAIN)
                    ,int nPlys
                    ,int qPlys
                    )
{

    // =====================================================================
    // Initialize
    // =====================================================================

    // Initialize return 
    Score d_best_score = 0.0;
    Move the_best_move = {};


    bool did_cutoff = false;    // TRUE if fail-high
    bool did_fail_low = false;  // TRUE if fail-low

    Score alpha_in = alpha;   //  save original alpha window lower bound
    Score beta_in = beta;   //  save original alpha window lower bound


    assert(nPlys < MAX_PLY0);
    vector<Move>& legal_moves = engine.all_legal_moves[nPlys];
    vector<Move>* p_moves_to_loop_over = &legal_moves;

    assert(depth>0);
    assert(qPlys==0);
 
    nodes_visited++;
  
    // =====================================================================
    // Get all legal moves
    // =====================================================================

    bool in_check = false;
   

    bool caps_only = false;
   

    int n_legal_moves_found;
    if (engine.game_board.turn == ShumiChess::Color::WHITE) {
        if (caps_only) n_legal_moves_found = engine.get_legal_moves_fast_t<ShumiChess::Color::WHITE, true>(false, legal_moves);
        else n_legal_moves_found = engine.get_legal_moves_fast_t<ShumiChess::Color::WHITE, false>(false, legal_moves);
    } else {
        if (caps_only) n_legal_moves_found = engine.get_legal_moves_fast_t<ShumiChess::Color::BLACK, true>(false, legal_moves);
        else n_legal_moves_found = engine.get_legal_moves_fast_t<ShumiChess::Color::BLACK, false>(false, legal_moves);
    }
    
    // Look, if caps_only is false, then n_legal_moves_found will be equal to legal_moves.size()
    // if caps_only is true, then n_legal_moves_found will be the count of moves, but 
    // only unquiet moves will be in legal_moves.
    #ifndef NDEBUG
        if (!caps_only) {
            // Change 0: legal_moves needs a size() that discounts "zero moves".
            assert (n_legal_moves_found == legal_moves.size());
        }
    #endif

    // =====================================================================
    // MultiPV      // remove excluded (already analyzed) moves from the list ONLY AT ROOT
    // =====================================================================
    if (is_from_root) {
        for (int i = (int)legal_moves.size() - 1; i >= 0; i--) {
            bool bExclude = false;

            for (int j = 0; j < (int)excluded_root_moves.size(); j++) {
                //if (legal_moves[i] == excluded_root_moves[j]) {
                if (legal_moves[i] == excluded_root_moves[j].first) {
                    bExclude = true;
                    break;
                }
            }

            if (bExclude) {
                //assert(0);
                legal_moves.erase(legal_moves.begin() + i);
            }

            if (legal_moves.empty()) {
                //return;    // this better not happen!
            }
        }
    }

    // =====================================================================
    // Asserts
    // =====================================================================

    // Over analysis sentinal Sorry, I should not be this large
    // Note: why does this so high? Must be a better way to handle this. Always happens near draws.
    // OR when 3-time rep is off.   // _SUPRESSING_MOVE_HISTORY_RESULTS is defined.
    assert(nPlys >= 0);
    if (nPlys > MAX_PLY) {
        // If a draw by 50/3/insuffieceint time, then we can get in loop here, where each deepeining is only 
        // 1 msec so it runs off to many levels. 
        // 
        //cout << gameboard_to_string_old(engine.game_board) << endl;
        assert(0);    
    }

    //if (alpha > beta) assert(0);

    
    // =====================================================================
    // Aborts
    // =====================================================================

    // User abort
    if (stop_calculation) {
        //cout << "\n! STOP CALCULATION requested \n";
        stop_calculation = false;
        return { ABORT_SCORE, the_best_move };
    }

    // Hard node-limit sentinel fuse
    if (nodes_visited > MAX_NODES) {
        string sss1 = format_with_commas(nodes_visited); 
        std::cout << "\x1b[31m\n! NODES VISITED trap#2 " << sss1 << " dep=" << depth << "  "
                        << engine.get_best_score_at_root() << "\x1b[0m\n";
        //assert(0);

        // Note: fascinating. This happens when in mate looking. 

        return { ABORT_SCORE, the_best_move };

    }

    // =====================================================================
    // Transposition table (TT2)
    // =====================================================================

    #ifdef  DEBUG_NODE_TT2       // Declare variables for holding "record" found in the TT2
        bool   foundPos = false;
        int    foundScore = 0;
        Move   foundMove = {};
        int    foundnPlys = 0;
        bool   foundDraw = 0;
        Score foundAlpha = 0.0;
        Score foundBeta  = 0.0;
        int    foundDepth = 0;
        bool   foundIsCheck = false;
        int    foundLegalMoveSize = 0;
        int    foundRepCount = 0;
        Score foundRawScore = 0;

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

        //int legalMovesSize = legal_moves.size();

    #endif


    TT2_match_move = {};

    if (Features_mask & _FEATURE_TT2) {  // probe in TT2

        int iLimit = 1;   // (Features_mask & _FEATURE_ENHANCED_DEPTH_TT2) ? 0 : 1;       // 0 or 1 only
        if (depth > iLimit) {

            // --- Normal TT2 probe (Note: exact-only version, no age yet)

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
                        (std::abs(entry.dAlphaDebug - alpha) <= VERY_SMALL_SCORE) &&
                        (std::abs(entry.dBetaDebug  - beta ) <= VERY_SMALL_SCORE);

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
                        //foundLegalMoveSize = entry.legalMovesSize;
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
                        Score dScore = convert_from_CP(entry.score_cp);
                        return { dScore, entry.best_move };
                    #endif

                } else {

                    // Its a match but not a perfect match. Use the move for ordering anyway.
                    // bool is_in = is_move_in_list(entry.best_move, legal_moves);
                    // assert(is_in);

                    //TT2_match_move = entry.best_move;

                }
                
                
            }   // END probe hit

        }   // END stupid 0/1 filter sub-feature

    }   // END TT2 feature

    // Purpose: avoid a false zero (no-move) result when the quick/capture-only generation missed moves
    // (or when you only needed to know whether any legal move exists). 
    if (n_legal_moves_found == 0) {
        //assert(depth==0);
        //assert (caps_only);

        // Call get_legal_moves_fast(), but only in "check mode". In this mode in is only trying to decide
        // wether its 0 moves or not. So it returns if it finds just one move.
        vector<Move> mvs;       // I am not used
        int n_legal_moves_found2 = engine.get_legal_moves_fast(engine.game_board.turn, false, true, mvs);
        
        // The plan part B.
        //assert(n_legal_moves_found == n_legal_moves_found2);
        if (n_legal_moves_found2 != 0) n_legal_moves_found = n_legal_moves_found2;
    }
    
    // Only one of me, per deepening.
    bool first_node_in_deepening = (top_deepening == depth);

    if (first_node_in_deepening) {
        // Change 3: legal_moves needs a size() that discounts "zero moves".
        if (legal_moves.size() == 1) {
            //cout << "\x1b[94m!!!!! force !!!!!!!!!!!!!\x1b[0m" << endl;

            #ifdef _DEBUGGING_TO_FILE1
                // Show board
                cout << gameboard_to_string(engine.game_board) << endl;
                assert(0);
            #endif

            return {ONLY_MOVE_SCORE, legal_moves[0]};
        }
    }

    const GameState state = engine.is_game_over(n_legal_moves_found);
    //GameState state = engine.is_game_over(legal_moves.size());

    // =====================================================================
    // Terminal positions (game over)
    // =====================================================================
    if (state != GameState::INPROGRESS) {

        int level = (top_deepening - depth);
        assert(level >= 0);

        Score d_level = static_cast<Score>(level);

        switch (state) {
            case GameState::WHITEWIN:
                d_best_score = (engine.game_board.turn == ShumiChess::WHITE)
                                ? (+HUGE_SCORE - d_level)
                                : (-HUGE_SCORE + d_level);
                break;

            case GameState::BLACKWIN:
                d_best_score = (engine.game_board.turn == ShumiChess::BLACK)
                                ? (+HUGE_SCORE - d_level)
                                : (-HUGE_SCORE + d_level);
                break;

            case GameState::DRAW:
                d_best_score = 0.0;          // Stalemate

                if (is_from_root) engine.reason_for_draw = DRAW_STALEMATE;
                break;

            default:
                assert(0);
                break;
        }

        return {d_best_score, the_best_move};

    }

    assert (depth > 0);
    Score d_stand_pat = HUGE_SCORE;   // If we evaluate, it will be the evaluate score.

    ////////////////////////////////////////////////////////////////////////////////////////////

    // =====================================================================
    // Recurse over selected move set "moves_to_loop_over"
    // =====================================================================
    
    assert(!p_moves_to_loop_over->empty());
    if (!p_moves_to_loop_over->empty()) {

        // Resort moves based on varoius things
        // Fascinating tradeoff. We could also call this if depth==0 and in check.
        // On one hand why not, because there could be a lot of responses, But on 
        // the otherhand there wont be that many. ANf we must be super fast here.
        //if ( (depth > 0) || (depth==0 && in_check) ) {

        assert(p_moves_to_loop_over == &legal_moves);

        #ifdef _DEBUGGING_MOVE_SORT
            vector<Move> tempMovs = *p_moves_to_loop_over;
        #endif

        bool is_top_of_deepening = (depth == top_deepening);

        // Change 8 : sort_moves_for_search function that discounts "zero moves"
        sort_moves_for_search(p_moves_to_loop_over, depth, nPlys, is_top_of_deepening);

        #ifdef _DEBUGGING_MOVE_SORT
            if (tempMovs != *p_moves_to_loop_over) {
                print_moves_to_file(tempMovs, depth, "pre", "\n");
                print_moves_to_file(*p_moves_to_loop_over, depth, "pst", "\n");
            }
        #endif


        d_best_score = -HUGE_SCORE;
        // Change 9: ???
        the_best_move = (*p_moves_to_loop_over)[0];   // Note: Is this the correct intialization? First move is the best move?

        //
        /////////////////////////////////////////////////////////////////////////////////////////
        //
        // Look (recurse) over all moves chosen
        //
        /////////////////////////////////////////////////////////////////////////////////////////
        
        // returns 0 if success, 1 if abort     n_legal_moves_found
        int ir = loop_over_all_moves(depth, alpha, beta, 
                        nPlys, qPlys, in_check, 
                        d_stand_pat, 
                        move_last,
                        p_moves_to_loop_over,               // input
                        the_best_move, d_best_score,        // outputs
                        did_cutoff);
        if (ir != 0) {
            return {ABORT_SCORE, the_best_move};
        }



    }   // END non zero oves to look at


    // Fail-low
    if (!did_cutoff && (alpha <= alpha_in)) {
        did_fail_low = true;
    }

    if (Features_mask & _FEATURE_TT2) {  // store in TT2

        int iLimit =1;  // (Features_mask & _FEATURE_ENHANCED_DEPTH_TT2) ? 0 : 1;       // 0 or 1 only


        //
        // Calculate "cnt", number of times this zobrist has been seen in the three_time_rep_stack.
        // This is so we don't add the same position to the TT2 twice.
        //
        int start = engine.boundary_stack.empty() ? 0 : engine.boundary_stack.back();
        int cnt = 0;
        uint64_t zkey = engine.game_board.zobrist_key;
        for (int i = start; i < (int)engine.three_time_rep_stack.size(); ++i) {
            if (engine.three_time_rep_stack[i] == zkey) ++cnt;
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

                int cp_score_temp = convert_to_CP(d_best_score);

                // --- DEBUG check
                #ifdef DEBUG_NODE_TT2        // Compare "found" record to actual situation now.
                {
                    if (foundPos && (foundDepth == depth) ) {            

                        bool bBothScoresMates = (IS_MATE_SCORE(foundRawScore) && IS_MATE_SCORE(d_best_score));

                        if (!bBothScoresMates) {

                            int iThreshold = BURP2_THRESHOLD_CP;

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
                                //print_mismatch(cout, "lm", foundLegalMoveSize, legalMovesSize);
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

                                //  found_move_history

                                // cout << "  depth-> " << depth << " mv->" << engine.computer_ply_so_far 
                                // << " dbg->" << bBothScoresMates << " idelta-> " << idelta 
                                // << " , " << abs(foundScore - cp_score_temp)
                                // << endl;
                                
                                // Show board
                                string out = gameboard_to_string(engine.game_board);
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
                                    getchar();
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
                    //slot.legalMovesSize  = legalMovesSize;

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


////////////////////////////////////////////////////////////////////////////////////////////////////


tuple<Score, Move> MinimaxAI::recursive_negamaxQ(
                    Score alpha, Score beta
                    ,const ShumiChess::Move& move_last      // seems to be used for debug only... (used only by _DEBUGGING_MOVE_CHAIN)
                    ,int nPlys
                    ,int qPlys
                    )
{

    // =====================================================================
    // Initialize
    // =====================================================================

    // Initialize return 
    Score d_best_score = 0.0;
    Move the_best_move = {};

    int cp_score_best;

    bool did_cutoff = false;    // TRUE if fail-high
    bool did_fail_low = false;  // TRUE if fail-low

    Score alpha_in = alpha;   //  save original alpha window lower bound
    Score beta_in = beta;   //  save original alpha window lower bound

    nodes_visited++;
    nodes_visited_depth_zero++;

    // =====================================================================
    // 1. Asserts
    // =====================================================================
    assert(qPlys>0);
    assert(nPlys>0);

    // Over analysis sentinal Sorry, I should not be this large
    // Note: why does this so high? Must be a better way to handle this. Always happens near draws.
    // OR when 3-time rep is off.   // _SUPRESSING_MOVE_HISTORY_RESULTS is defined.

    if (nPlys > MAX_PLY) {
        // If a draw by 50/3/insuffieceint time, then we can get in loop here, where each deepeining is only 
        // 1 msec so it runs off to many levels. 
        // 
        //cout << gameboard_to_string_old(engine.game_board) << endl;
        assert(0);    
    }

    //if (alpha > beta) assert(0);

    // Pick best static evaluation among all legal moves if hit the over ply
    if (qPlys >= MAX_QPLY) {
        //std::cout << "\x1b[31m! MAX_QPLY trap " << nPlys << "\x1b[0m\n";
        //std::cout << "\x1b[31m!" << "\x1b[0m";
        
        if (engine.game_board.turn == ShumiChess::Color::WHITE)
            cp_score_best = evaluate_board_t<ShumiChess::Color::WHITE>(eval_person);
        else
            cp_score_best = evaluate_board_t<ShumiChess::Color::BLACK>(eval_person);

        d_best_score = convert_from_CP(cp_score_best);
        
        nFarts++;
        
        // debug
        //engine.print_move_history_to_file(fpDebug, "MAX_QPLY");

        return { d_best_score, Move{} };
    }



    #ifdef DEBUGGING_TEMP
            fprintf(fpDebug, "nPlys=%ld, qPlys=%ld\n", nPlys, qPlys);
    #endif
    // =====================================================================
    // 2. Get all legal moves
    // =====================================================================

    // Get pointer to buffer where we will put the legal moves.
    assert(nPlys < MAX_PLY0);
    vector<Move>& legal_moves = engine.all_legal_moves[nPlys];
    vector<Move>* p_moves_to_loop_over = &legal_moves;

    //
    //  If not in check, generate only captures. If in check generate all moves.
    //
    bool in_check = false;
    in_check = (engine.game_board.turn == Color::WHITE)
        ? engine.is_king_in_check_t<Color::WHITE>()
        : engine.is_king_in_check_t<Color::BLACK>();

    bool caps_only = false;
    if (!in_check) caps_only = true;

    int n_legal_moves_found;
    if (engine.game_board.turn == ShumiChess::Color::WHITE) {
        if (caps_only) n_legal_moves_found = engine.get_legal_moves_fast_t<ShumiChess::Color::WHITE, true>(false, legal_moves);
        else           n_legal_moves_found = engine.get_legal_moves_fast_t<ShumiChess::Color::WHITE, false>(false, legal_moves);
    } else {
        if (caps_only) n_legal_moves_found = engine.get_legal_moves_fast_t<ShumiChess::Color::BLACK, true>(false, legal_moves);
        else           n_legal_moves_found = engine.get_legal_moves_fast_t<ShumiChess::Color::BLACK, false>(false, legal_moves);
    }
    
    // Look, if caps_only is false, then n_legal_moves_found will be equal to legal_moves.size()
    // if caps_only is true, then n_legal_moves_found will be the count of moves, but 
    // only unquiet moves will be in legal_moves.
    #ifndef NDEBUG
        if (!caps_only) {
            // Change 0: legal_moves needs a size() that discounts "zero moves".
            assert (n_legal_moves_found == legal_moves.size());
        }
    #endif

    // =====================================================================
    // Aborts
    // =====================================================================

    // User abort
    if (stop_calculation) {
        //cout << "\n! STOP CALCULATION requested \n";
        stop_calculation = false;
        return { ABORT_SCORE, the_best_move };
    }

    // Hard node-limit sentinel fuse
    if (nodes_visited > MAX_NODES) {
        string sss1 = format_with_commas(nodes_visited); 
        std::cout << "\x1b[31m\n! NODES VISITED trap#2 Q " << sss1
                        << engine.get_best_score_at_root() << "\x1b[0m\n";
        //assert(0);

        return { ABORT_SCORE, the_best_move };

    }

    // Purpose: avoid a false zero (no-move, thus end-of-game) result when the quick/capture-only generation missed moves
    // (or when you only needed to know whether any legal move exists). 
    // In qsearch we only generate captures and check evades, so this happens a lot. If so We have to check again
    // full "window" (all moves), but only when we find one move.
    if (n_legal_moves_found == 0) {
        //assert (caps_only);       // no, could be a check evade.

        // Call get_legal_moves_fast(), but only in "check mode". In this mode in is only trying to decide
        // wether its 0 moves or not. So it returns if it finds just one move.
        vector<Move> mvs;       // I am not used
        int n_legal_moves_found2 = engine.get_legal_moves_fast(engine.game_board.turn, false, true, mvs);
        
        //assert(n_legal_moves_found == n_legal_moves_found2);
        if (n_legal_moves_found2 != 0) n_legal_moves_found = n_legal_moves_found2;
    }
    


    const GameState state = engine.is_game_over(n_legal_moves_found);
    //GameState state = engine.is_game_over(legal_moves.size());

    // =====================================================================
    // Terminal positions (game over)
    // =====================================================================
    if (state != GameState::INPROGRESS) {

        int level = (top_deepening);
        assert(level >= 0);

        Score d_level = static_cast<Score>(level);

        switch (state) {
            case GameState::WHITEWIN:
                d_best_score = (engine.game_board.turn == ShumiChess::WHITE)
                                ? (+HUGE_SCORE - d_level)
                                : (-HUGE_SCORE + d_level);
                break;

            case GameState::BLACKWIN:
                d_best_score = (engine.game_board.turn == ShumiChess::BLACK)
                                ? (+HUGE_SCORE - d_level)
                                : (-HUGE_SCORE + d_level);
                break;

            case GameState::DRAW:
                d_best_score = 0.0;          // Stalemate
                break;

            default:
                assert(0);
                break;
        }

        return {d_best_score, the_best_move};

    }


    //assert (depth >= 0);
    Score d_stand_pat = HUGE_SCORE;   // If we evaluate, it will be the evaluate score.


    //////////////////////////////////////////////////////////////////////////////////////////////
    // Static board evaluation
    //////////////////////////////////////////////////////////////////////////////////////////////
    //bool b_is_Quiet = !engine.has_unquiet_move(legal_moves);   // only way this can happen is if we are in check

    // int  cp_from_tt   = 0;
    // bool have_tt_eval = false;


    // // memoization of leafs
    // if (Features_mask & _FEATURE_TT) {      // qsearch
    //     // Salt the entry
    //     unsigned mode  = salt_the_TT(b_is_Quiet);

    //     uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

    //     // Look for the entry in the TT
    //     auto it = TTable.find(evalKey);
    //     if (it != TTable.end()) {
    //         const TTEntry &entry = it->second;
    //         cp_from_tt   = entry.score_cp;
    //         have_tt_eval = true;
    //     }
    // }

    //
    // evaluate (main call)
    //
    // if (0) {
    //     TT_ntrys++;
    //     #ifdef DEBUG_LEAF_TT
    //         if (engine.game_board.turn == ShumiChess::Color::WHITE)
    //             cp_score_best = evaluate_board_t<ShumiChess::Color::WHITE>(exp, b_is_Quiet);
    //         else
    //             cp_score_best = evaluate_board_t<ShumiChess::Color::BLACK>(exp, b_is_Quiet);
    //         if (cp_from_tt != cp_score_best) {
    //             printf ("burp (MAIN) %ld %ld      %ld\n", cp_from_tt, cp_score_best, TT_ntrys);
    //             assert(0);
    //         } else {
    //             NhitsTT++;
    //     }
    //     #endif
    //     cp_score_best = cp_from_tt;

    // }
    // else {
        if (engine.game_board.turn == ShumiChess::Color::WHITE)
            cp_score_best = evaluate_board_t<ShumiChess::Color::WHITE>(eval_person);
        else
            cp_score_best = evaluate_board_t<ShumiChess::Color::BLACK>(eval_person);
//    }


    d_best_score = convert_from_CP(cp_score_best);
    d_stand_pat = d_best_score;  // "stand pat" means the evaluate_board() computed score


    ////////////////////////////////////////////////////////////////////////////////////////////

    //#define NO_QUISSENCE
    #ifdef NO_QUISSENCE
        return { d_best_score, Move{} };
    #endif

    // memoization at leaf
    // if (Features_mask & _FEATURE_TT) {

    //     // Salt the entry
    //     unsigned mode  = salt_the_TT(b_is_Quiet);

    //     uint64_t evalKey = engine.game_board.zobrist_key ^ g_eval_salt[mode];

    //         // Store this position away into the TT
    //     TTEntry &slot = TTable[evalKey];
    //     slot.score_cp = cp_score_best;   // or cp_score, whatever you just got
    //     slot.movee    = the_best_move;   // or bestMove, etc.
    //     slot.depth    = top_deepening;

    // }



    if (in_check) {
        // In check: use all legal moves, since by definition (see get_legal_moves() the set of all legal moves is equivnelent 
        // to the set of all check escapes. By definition. So there.
        ////moves_to_loop_over = legal_moves;  // not needed as its done ealier above. Sorry.
        engine.sort_check_evasions_qsearch(legal_moves, engine.all_unquiet_moves[nPlys]);
        vector<Move>& unquiet_moves = engine.all_unquiet_moves[nPlys];
        assert(!unquiet_moves.empty());  // oTherwise we are in check mate, and that would be caught earlier. 

        assert(legal_moves.size() == unquiet_moves.size());

        p_moves_to_loop_over = &unquiet_moves; 

    } else {

        assert (qPlys <= MAX_QPLY);  // already dealt with this
        if (qPlys > MAX_QPLY2) {
            engine.sort_unquiet_moves_qsearch_H(legal_moves, engine.all_unquiet_moves[nPlys]);
        } else {
            engine.sort_unquiet_moves_qsearch_L(legal_moves, engine.all_unquiet_moves[nPlys]);
        }

        vector<Move>& unquiet_moves = engine.all_unquiet_moves[nPlys];

        // Show moves
        // string mvss = engine.moves_into_string(unquiet_moves);
        // int nChars = fputs("\n srt:", fpDebug);
        // if (nChars == EOF) assert(0);
        // nChars = fputs(mvss.c_str(), fpDebug);
        // if (nChars == EOF) assert(0);
        // nChars = fputs(":end\n", fpDebug);
        // if (nChars == EOF) assert(0);
        // cout << "\n" << "mvss " << mvss << "\n";

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

    

    ////////////////////////////////////////////////////////////////////////////////////////////

    // =====================================================================
    // Recurse over selected move set "moves_to_loop_over"
    // =====================================================================
    
    assert(!p_moves_to_loop_over->empty());
    if (!p_moves_to_loop_over->empty()) {

        // Resort moves based on varoius things
        // Fascinating tradeoff. We could also call this if depth==0 and in check.
        // On one hand why not, becasuse there could be a lot of responses, But on 
        // the otherhand there wont be that many. ANf we must be super fast here.
        //if ( (depth > 0) || (depth==0 && in_check) ) {
 
        // In depth==0 (Quiescence) and not in check)

        d_best_score = -HUGE_SCORE;
        // Change 9: ???
        the_best_move = (*p_moves_to_loop_over)[0];   // Note: Is this the correct intialization? First move is the best move?

        //
        /////////////////////////////////////////////////////////////////////////////////////////
        //
        // Look (recurse) over all moves chosen
        //
        /////////////////////////////////////////////////////////////////////////////////////////
        
        // returns 0 if success, 1 if abort     n_legal_moves_found
        int ir = loop_over_all_moves(0, alpha, beta, 
                        nPlys, qPlys, in_check, 
                        d_stand_pat, 
                        move_last,
                        p_moves_to_loop_over,               // input
                        the_best_move, d_best_score,        // outputs
                        did_cutoff);
        if (ir != 0) {
            return {ABORT_SCORE, the_best_move};
        }



    }   // END non zero oves to look at


    // Fail-low
    if (!did_cutoff && (alpha <= alpha_in)) {
        did_fail_low = true;
    }

    
 
    assert(beta_in == beta);

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






////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Choose the "minimax" AI move.
// Returns a tuple of:
//    the best score (however, if this is ABORT_SCORE, its an abort)
//    the best move
//
////////////////////////////////////////////////////////////////////////////////////////////////////


// returns 0 if success, 1 if abort
int MinimaxAI::loop_over_all_moves(int depth, Score &alpha, const Score beta, int nPlys, int qPlys,
                       bool in_check,       // Used only by delta pruning
                       Score d_stand_pat, 
                       const ShumiChess::Move& move_last,       // seems to be used for debug only... (used only by _DEBUGGING_MOVE_CHAIN)
                       const vector<ShumiChess::Move>* pMoves, 
                       ShumiChess::Move &bestMoveOut, Score &bestScoreOut,
                       bool& did_cutoff)
{
    bool b_use_this_move;
    int imovedebug = 0;

    for (const Move& m : *pMoves) {
        //int nChars;

        // Change 10, "continue" on "zero moves"

        imovedebug++;

        #ifdef _DEBUGGING_PUSH_POP
            std::string temp_fen_before = engine.game_board.to_fen();
            ull zobrist_save = engine.game_board.zobrist_key;
            auto ep_history_save = engine.game_board.castle_rights;
            auto enpassant_save = engine.game_board.en_passant_landing_bb;
        #endif

        #ifdef _DEBUGGING_MOVE_CHAIN    // Print move we are going to analyze
            bSuppressOutput = false;
            bool bSide = (engine.game_board.turn == ShumiChess::WHITE);

            // This output always starts a new line
            nChars = fputc('\n', fpDebug);
            if (nChars == EOF) assert(0);

            // Print move number (out of) (this is printed as the move prefix)
            // Change 11, need a size() that ignores "zero moves"
            int isizedebug = pMoves->size();
            sprintf(szDebug, "[%2d/%2d]", imovedebug, isizedebug);

            // Print move with prefix (Move always starts a new line)
            engine.print_move_to_file_with_prefix(m, nPlys, (GameState::INPROGRESS)
                                , false, bSide, szDebug
                                , (depth==0)
                                , fpDebug); 
            //if (nCharsInMove == EOF) assert(0);

            //sprintf(szDebug, " A=%10.3f, B=%10.3f", alpha, beta);
            //fprintf(fpDebug, szDebug);
        
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
                   
                bool recapture = false;
                if (!engine.move_history.empty()) {
                    ull mto = m.toSQ;
                    ull eto =  engine.move_history.top().toSQ;
                    recapture = (mto == eto);
                }

                // we should not be looking at moves in Quiescence, unless we evaluated first
                assert (d_stand_pat != HUGE_SCORE);  

                int cp_stand_pat = convert_to_CP(d_stand_pat);
                int cp_alpha = convert_to_CP(alpha);

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
        if (m.color == Color::WHITE) engine.pushMove_t<Color::WHITE>(m);
        else                         engine.pushMove_t<Color::BLACK>(m);

        #ifdef DISPLAY_PULSE_CALLBACK_THREAD
        	g_live_ply = nPlys;
        #endif

        //++engine.repetition_table[engine.game_board.zobrist_key];
        engine.three_time_rep_stack.push_back(engine.game_board.zobrist_key);

        //
        // Three parts in negamax: 1. "relative scores", the alpha betas are reversed in sign,
        //                         2. The beta and alpha arguments are staggered, or reversed.
        Score childAlpha = -beta;
        Score childBeta  = -alpha;

        if (0) {                // Full wide window
            childAlpha = -HUGE_SCORE;
            childBeta  =  HUGE_SCORE;
        }

        int new_depth = (depth > 0 ? depth - 1 : 0);
        tuple<Score, Move> ret_val;

        #ifdef DEBUGGING_TEMP1
            fprintf(fpDebug, "dep=%ld, nPlys=%ld, qPlys=%ld\n", new_depth, nPlys, qPlys);
        #endif

        if (new_depth) {

            ret_val = recursive_negamax(
                new_depth,
                childAlpha, childBeta,
                false,                    // I am NOT called from the root
                m,
                (nPlys+1),
                qPlys
            );

        } else {

            ret_val = recursive_negamaxQ(
                childAlpha, childBeta,
                m,
                (nPlys+1),
                (qPlys+1)
            );
      
        }




        // The third part of negamax: negate the score to keep it relative.
        Score d_return_score = get<0>(ret_val);     // units are pawns
        Move d_return_move = get<1>(ret_val);


        // We are done with the recursion. Remove zkey from the 3-time rep collector.
        assert(!engine.three_time_rep_stack.empty());   // Better not be empty, we just pushed a move up above.
        //assert(engine.boundary_stack.empty() || engine.boundary_stack.back() <= (int)engine.three_time_rep_stack.size());
        engine.three_time_rep_stack.pop_back();
        while (!engine.boundary_stack.empty() &&
            engine.boundary_stack.back() >= (int)engine.three_time_rep_stack.size()) {
            engine.boundary_stack.pop_back();
        }
        //assert(engine.boundary_stack.empty() || engine.boundary_stack.back() < (int)engine.three_time_rep_stack.size());


        if (m.color == Color::WHITE) engine.popMove_t<Color::WHITE>();
        else                         engine.popMove_t<Color::BLACK>();



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
            if (ep_history_save != engine.game_board.castle_rights) {
                std::cout << "\x1b[31m";
                std::cout << "PROBLEM WITH PUSH POP B !!!!!" << std::endl;
                std::cout << "\x1b[0m";
                assert(0);   
            }
            if (ep_history_save != engine.game_board.castle_rights) {
                std::cout << "\x1b[31m";
                std::cout << "PROBLEM WITH PUSH POP A !!!!!" << std::endl;
                std::cout << "\x1b[0m";
                assert(0);   
            }
            if (enpassant_save != engine.game_board.en_passant_landing_bb) {
                std::cout << "\x1b[31m";
                std::cout << "PROBLEM WITH PUSH POP P !!!!!" << std::endl;
                std::cout << "\x1b[0m";
                assert(0);   
            }
        #endif

        // negamax, reverse returned score.  
        Score d_score_value = -d_return_score;

        if (d_return_score == ABORT_SCORE) {
            //cout << "\n! STOP CALCULATION now \n" << endl;
            return 1;
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

        Score d_difference_in_score = std::abs(d_score_value - bestScoreOut);

        b_use_this_move = (d_score_value > bestScoreOut);

        if (b_use_this_move) {
            bestScoreOut = d_score_value;
            bestMoveOut = m;
        }

        // Think of alpha as “best score found so far at this node.”
        alpha = std::max(alpha, bestScoreOut);

        // Alpha/beta "cutoff", (fail-high), break the analysis
        // You just found a move so good that the opponent would never 
        // allow this position, because it already exceeds what they could tolerate.
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

    return 0;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Resort moves in this order (they will later be searched in this order):
//      TT move              (move from the hash table hit (if any))
//      PV move              (prevous deepenings best).
//      captures/promotions  (unquiet moves (captures/promotions, sorted by MVV-LVA, and "last square")
//      castling
//      killer moves         (quiet, bubbled to the front of the "cutoff" quiet slice).
//      remaining quiet moves
//  Sorts moves in place.
//
void MinimaxAI::sort_moves_for_search(std::vector<ShumiChess::Move>* pMovesInOut   // input/output
                            , int depth, int nPlys, bool is_top_of_deepening)
{
    assert(pMovesInOut);
    assert(depth>0);            // This routine should never be called in qsearch

    if (pMovesInOut->empty()) {
        return;
    }

    const bool have_last = !engine.move_history.empty();
    Square last_toSQ = ShumiChess::NO_SQUARE;
    if (have_last) {
        last_toSQ = engine.move_history.top().toSQ;
    }

    //  The sort, from top to bottom. Items 0 and 1 done always, the rest done if the unquiet sort is on.
    //      0. Move from the hash table hit (if any).
    //      1. PV from the previous iteration (previous deepening’s best).
    //      2. Unquiet moves (captures and promotions). Captures receive MVV-LVA-based
    //         ordering; non-capture promotions stay in the unquiet region but do not
    //         get an MVV-LVA base score here.
    //      2.5 Bubble castling moves to the front of the quiet region.
    //      3. Killer moves (quiet, bubbled to the front of the remaining quiet region).
    //      4. Remaining quiet moves.

    if (Features_mask &_FEATURE_UNQUIET_SORT) {

        // --- 1. Partition unquiet moves (captures/promotions) to the front ---
        auto it_split = std::partition(
            pMovesInOut->begin(), pMovesInOut->end(),
            [&](const ShumiChess::Move& mv)
            {
                return engine.is_unquiet_move(mv);
            });

        // --- 2. Sort the unquiet prefix using MVV-LVA ---
        std::sort(pMovesInOut->begin(), it_split,
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
                    
                    // remove me!
                    //int seeA2 = engine.game_board.SEE_for_capture(engine.game_board.turn, a, nullptr);
                    int seeA = engine.game_board.SEE_for_capture_new(engine.game_board.turn, a, nullptr);
                    //assert(seeA == seeA2);

                    if (seeA < 0) keyA += seeA * 100;   // negative pulls it way down in the sort
                }
                if (b.capture != ShumiChess::Piece::NONE)
                {
                    
                    // remove me!
                    //int seeB2 = engine.game_board.SEE_for_capture(engine.game_board.turn, b, nullptr);
                    int seeB = engine.game_board.SEE_for_capture_new(engine.game_board.turn, b, nullptr);
                    //assert(seeB == seeB2);

                    if (seeB < 0) keyB += seeB * 100;
                }

                // If capture to the "from" square of last move, give it higher priority
                // Fourth tier, so by default these are in centipawns ( as in 800 centipawns).
                if (a.toSQ == last_toSQ) keyA += 800;
                if (b.toSQ == last_toSQ) keyB += 800;

                return keyA > keyB;
            });

        // It is known that Killer moves force "TT2 unrepeatibility". The theory is I guess that the
        // later analysis is profited by these killer moves.
        #ifndef DEBUG_NODE_TT2
        if (Features_mask & _FEATURE_KILLER) {
            // --- 3. Apply killer moves to the quiet region (for speed, not re-sorting) ---
            auto quiet_begin = it_split;
            auto quiet_end   = pMovesInOut->end();


            // 2.5  Bubble castling moves to the front of the quiet region ---
            for (auto it = quiet_begin; it != quiet_end; ++it)
            {
                const ShumiChess::Move& mv = *it;

                if (mv.flags & FLAGS_IS_CASTLE_MOVE)
                {
                    std::rotate(quiet_begin, it, it + 1);
                    ++quiet_begin;   // if a second castle move exists, it goes just after the first
                }
            }

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
            auto it = std::find(pMovesInOut->begin(), pMovesInOut->end(), pv_move);
            if (it == pMovesInOut->end()) {
                // item not in the list
                assert(0);
            }
            else if (it != pMovesInOut->begin()) {
                // item not first in list
                // moves existing pv_move to front, preserving their relative order.
                std::rotate(pMovesInOut->begin(), it, it + 1);        
                // #ifdef _DEBUGGING_MOVE_CHAIN
                //     fprintf(fpDebug, "PV bubble");
                // #endif
            }
        }

    }

    //       0. move from the hash table hit (if any) 
    if (!(TT2_match_move == Move{})) {       
        auto it = std::find(pMovesInOut->begin(), pMovesInOut->end(), TT2_match_move);
        if (it == pMovesInOut->end()) {
            // item not in the list
            assert(0);
        }
        else if (it != pMovesInOut->begin()) {
            // item in list, but not first in list already
            // moves existing pv_move to front, preserving their relative order.
            std::rotate(pMovesInOut->begin(), it, it + 1);        
            #ifdef _DEBUGGING_MOVE_CHAIN
                fprintf(fpDebug, "TT2_match_move bubble");
            #endif
        }
    }

  
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


std::tuple<Score, ShumiChess::Move> MinimaxAI::pick_random_within_delta_rand(std::vector<std::pair<Move,Score>>& MovsFromRoot,
                                             int i_delta_cp,
                                             int i_computer_ply_so_far,
                                             int& n_moves_within_delta     // output
                                            ) {
    int nTries = 0;

try_again:
    nTries++;
    n_moves_within_delta = 0;
    if (MovsFromRoot.empty()) {
        assert(0);                    // Better not happen 
        return {0, Move{}};         // safety
    }

    // 0) Sort MovsFromRoot by score (leaves the caller’s list sorted)
    std::sort(MovsFromRoot.begin(), MovsFromRoot.end(),
              [](const auto& a, const auto& b){ return a.second > b.second; });

    // 1) Best is now front()
    const Score bestScorePawns = MovsFromRoot.front().second;

    // 2) If best score is mate-like, don’t randomize. Just pick the first one.
    if (IS_MATE_SCORE(bestScorePawns)) return {MovsFromRoot.front().second, MovsFromRoot.front().first};

    // Convert best score to centipawns once (rounded).
    // Assumes score is in pawns (e.g., +0.23 == +23 cp).
    const int bestScoreCp = convert_to_CP(bestScorePawns);


    // 3) Build the contiguous “within delta” prefix in centipawns
    const int cutoffCp = bestScoreCp - i_delta_cp;

    size_t n_top = 0;
    while (n_top < MovsFromRoot.size()) {

        const Score scP = MovsFromRoot[n_top].second;
      
        // int cp_score_temp = convert_to_CP(d_best_score);
        const int scCp = convert_to_CP(scP);

        if (scCp < cutoffCp) break;
        ++n_top;
    }

    // to return number of moves that qualified.
    n_moves_within_delta = (int)n_top;

    assert(n_top != 0);      // better not happen. At least the best move must be within zero delta

    bool b_use_this_one;
    if (i_computer_ply_so_far <= 1) 
        b_use_this_one = (n_top > 2);
    else
        b_use_this_one = (n_top > 1);

    // (fallback if something odd, like nothing within the delta? At least the best move must be within zero delta)

    if (!b_use_this_one) {
        // increase delta a bit and try again
        
        i_delta_cp = ((i_delta_cp * 3) / 2) + 1;   // *1.5, integer
        assert (nTries<8);
        goto try_again;
    }

    // PRINT HERE (only when randomizing)

    assert (b_use_this_one);
    if (b_use_this_one) {
        #ifdef DISPLAY_DEEPING
            cout << "\n[pick_random_within_delta_rand] n_top=" << (int)n_top << " from candidates: \n" << i_computer_ply_so_far;

            for (size_t i = 0; i < n_top; ++i) {
                engine.move_into_string(MovsFromRoot[i].first);   // fills engine.move_string
                cout << "  [" << (int)i << "]  " << engine.move_string << "\n";
            }
            cout.flush();
        #endif
    }

    // 4) Uniform pick from [0, n_top)
    const int pick = engine.rand_int(0, (int)n_top - 1);
    return {MovsFromRoot[(size_t)pick].second, MovsFromRoot[(size_t)pick].first};
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


// ============================================================================
// Template implementations (compile-time Color)
// ============================================================================


const PawnFileInfo& MinimaxAI::get_pawn_file_info_for_position() {
    PawnFileInfo pawnFileInfoTemp;

    // Probe the pawn/file hash
    const uint64_t key = engine.game_board.pawn_zobrist_key;

    NTriesP++;

    auto it = pawn_file_info.find(key);
    if (it != pawn_file_info.end()) {
        // found an "pawn summary" entry in the hash (this is the more common case)
        NhitsP++;

        #ifdef DEBUGGING_PAWN_HASH
            engine.game_board.build_pawn_summaries(pawnFileInfoTemp);
            if (!(pawnFileInfoTemp.p[0] == it->second.p[0])) {
                assert(0);
            }
            if (!(pawnFileInfoTemp.p[1] == it->second.p[1])) {
                assert(0);
            }
        #endif

        return it->second;
    }

    // rebuild the pawn summary.
    engine.game_board.build_pawn_summaries(pawnFileInfoTemp);

    // Add to hash
    auto result = pawn_file_info.emplace(key, pawnFileInfoTemp);
    return result.first->second;
}

//
// I am called UNLESS there are no enemy major pieces left
//
template<ShumiChess::Color c>
int MinimaxAI::cp_score_positional_get_open_cp_t(int nPhase, const PawnFileInfo*& pawnFileInfoP) {
    using namespace ShumiChess;

    constexpr Color enemyColor = utility::representation::opposite_color_t<c>;

    int cp_score_position_temp = 0;
    int icp_temp;

    // Lazy pointer. Get pawn/file info only once for the pair of color eval calls.
    if (pawnFileInfoP == NULL) {
        pawnFileInfoP = &get_pawn_file_info_for_position();
    }

    const PawnFileInfo& pawnFileInfo = *pawnFileInfoP;

    // Create pawn info links the rest of the routine will use.
    const PInfo& pawnF = pawnFileInfo.p[c];
    const PInfo& pawnE = pawnFileInfo.p[enemyColor];

    icp_temp = engine.game_board.get_castled_bonus_cp_t<c>(nPhase, pawnF);
    cp_score_position_temp += icp_temp;


    ull holes_bb = 0ULL;
    ull passed_pawns_bb = 0ULL;

    icp_temp = engine.game_board.count_isolated_and_doubled_pawns_cp_t<c>(pawnF, pawnE);
    cp_score_position_temp += icp_temp;

    int holes_cp;
    int passed_cp;
    engine.game_board.count_pawn_holes_and_passed_pawns_cp_new_t<c>(pawnFileInfo,
                                                        holes_bb,
                                                        holes_cp,
                                                        passed_pawns_bb,
                                                        passed_cp);
    //  codex resume 019ecee5-85b1-7003-933c-04d9569ab113
    // #ifndef NDEBUG
    //     ull holes_bb_old = 0ULL;
    //     ull passed_pawns_bb_old = 0ULL;
    //     int holes_cp_old;
    //     int passed_cp_old;

    //     engine.game_board.count_pawn_holes_and_passed_pawns_cp_t<c>(pawnF, pawnE,
    //                                                         holes_bb_old,
    //                                                         holes_cp_old,
    //                                                         passed_pawns_bb_old,
    //                                                         passed_cp_old);

    //     assert(holes_bb_old == holes_bb);
    //     assert(holes_cp_old == holes_cp);
    //     assert(passed_pawns_bb_old == passed_pawns_bb);
    //     assert(passed_cp_old == passed_cp);
    // #endif

    cp_score_position_temp += holes_cp;

    cp_score_position_temp += passed_cp;


    icp_temp = engine.game_board.count_knights_on_holes_cp_t<c>(holes_bb);
    cp_score_position_temp += icp_temp;

    

    if constexpr (c == Color::WHITE)
        passed_pawns_white = passed_pawns_bb;
    else
        passed_pawns_black = passed_pawns_bb;

    icp_temp = engine.game_board.rooks_file_status_cp_t<c>(pawnF, pawnE);
    cp_score_position_temp += icp_temp;


    if ((nPhase == GamePhase::OPENING) || (nPhase == GamePhase::MIDDLE_EARLY)) {
        icp_temp = engine.game_board.development_minor_cp_t<c>();
        cp_score_position_temp += icp_temp;
    }

    if ((nPhase == GamePhase::OPENING) || (nPhase == GamePhase::MIDDLE_EARLY)) {
        icp_temp = engine.game_board.bishop_pawn_pattern_cp_t<c>();
        cp_score_position_temp += icp_temp;
    }

    if ((nPhase == GamePhase::OPENING)) {       // Really, Im just here to stop center counter and center openings
        icp_temp = engine.game_board.queenOnCenterSquare_cp_t<c>();
        cp_score_position_temp += icp_temp;
    }

    // if ((nPhase == GamePhase::OPENING) || (nPhase == GamePhase::MIDDLE_EARLY)) {
    //     icp_temp = engine.game_board.moved_f_pawn_early_cp_t<c>();
    //     cp_score_position_temp += icp_temp;
    // }

    // remove me
    //icp_temp2 = engine.game_board.pawns_attacking_center_squares_cp_t<c>();
    icp_temp = engine.game_board.pawns_attacking_center_squares_cp_fast_t<c>();
    //assert(icp_temp == icp_temp2);

    cp_score_position_temp += icp_temp;

    icp_temp = engine.game_board.knights_attacking_center_squares_cp_t<c>();
    cp_score_position_temp += icp_temp;

    int multiplier;
    if (nPhase == GamePhase::OPENING) multiplier = 2;
    if (nPhase == GamePhase::ENDGAME_LATE) multiplier = 0;
    else                              multiplier = 1;
    icp_temp = engine.game_board.bishops_attacking_center_squares_cp_t<c>();
    cp_score_position_temp += (icp_temp*multiplier);

    icp_temp = engine.game_board.is_knight_on_edge_cp_t<c>();
    cp_score_position_temp += icp_temp;

    return cp_score_position_temp;
}

//
// I am called UNLESS there are no enemy major pieces left
//
template<ShumiChess::Color c>
int MinimaxAI::cp_score_positional_get_middle_cp_t(int nPhase) {
    int cp_score_position_temp = 0;
    int icp_temp;

    if ((nPhase == GamePhase::MIDDLE) || (nPhase == GamePhase::ENDGAME)) {
        icp_temp = engine.game_board.advanced_unpushable_knights_cp_t<c>();
        cp_score_position_temp += icp_temp;
    }

    icp_temp = engine.game_board.two_bishops_cp_t<c>(nPhase);
    cp_score_position_temp += icp_temp;

    icp_temp = engine.game_board.rook_connectiveness_cp_t<c>();
    cp_score_position_temp += icp_temp;

    icp_temp = engine.game_board.rook_7th_rankness_cp_t<c>();
    cp_score_position_temp += icp_temp;

    return cp_score_position_temp;
}

//
// I am called always
//
template<ShumiChess::Color c>
int MinimaxAI::cp_score_positional_get_end_t(int nPhase, int cp_material_all, bool noMajorPiecesFriend, bool noMajorPiecesEnemy) {
    using namespace ShumiChess;

    int icp_temp;
    int cp_score_position_temp = 0;

    if (nPhase > GamePhase::OPENING) {
        icp_temp = engine.game_board.attackers_on_enemy_king_near_cp_t<c>();
        cp_score_position_temp += icp_temp;
    }

    if (nPhase == GamePhase::ENDGAME_LATE) {
        icp_temp = engine.game_board.rook_endgame_keep_rooks_when_down_cp_t<c>();
        cp_score_position_temp += icp_temp;
    }

    // King to center in end
    if (nPhase >= GamePhase::ENDGAME) {
        icp_temp = engine.game_board.king_centerness_cp_t<c>();
        cp_score_position_temp += icp_temp;       
    }

    // if (nPhase >= GamePhase::MIDDLE) {
    //     icp_temp = engine.game_board.opposite_bishops_cp_t<c>(cp_material_all);
    //     cp_score_position_temp += icp_temp;
    // }

    if (noMajorPiecesEnemy) {
        double dcp_temp;
        dcp_temp = engine.game_board.kings_close_toegather_cp_t<c>();
        icp_temp = (int)dcp_temp;
        cp_score_position_temp += (int)dcp_temp;

        constexpr Color enemy_color = utility::representation::opposite_color_t<c>;
        icp_temp = engine.game_board.king_edgeness_cp_t<enemy_color>();
        cp_score_position_temp += icp_temp;
        
    }

    return cp_score_position_temp;
}

//  Final eval is (material+positional).
template<ShumiChess::Color for_color>
int MinimaxAI::evaluate_board_t(ShumiChess::EvalPersons evp) {
    using namespace ShumiChess;

    evals_visited++;

    int cp_score_adjusted = 0;

    int mat_cp_white = 0;
    int mat_cp_black = 0;

    int tempsum = 0;

    // Computes "Bits_In" (many eval routines use these shortcuts for speed)
    engine.game_board.compute_bits_in();        // Computes shortcuts for "bits_in()", used in the eval.

    //int tempsumNP = 0;

    // 
    // First compute up the material.  (final eval is (material+positional)).
    // Outputs of this section:
    //    cp_score_material_all
    //    cp_score_pawns_only 
    // Note that these are signed for me/enemy, so positions good for me are positive, good for the enemy are negative.
    //
    int cp_score_material_all = 0;
    int cp_score_pawns_only = 0;

    for (const auto& color1 : std::array<Color, 2>{Color::WHITE, Color::BLACK}) {
        int cp_pawns_only_temp;

        int cp_score_mat_temp = (color1 == Color::WHITE)
            ? engine.game_board.get_material_for_color2_t<Color::WHITE>(cp_pawns_only_temp)
            : engine.game_board.get_material_for_color2_t<Color::BLACK>(cp_pawns_only_temp);
        //assert(cp_score_mat_temp == cp_score_mat_temp2);
        assert(cp_score_mat_temp >= 0);

        if (color1 == Color::WHITE) {
            mat_cp_white = cp_score_mat_temp;
        } else {
            mat_cp_black = cp_score_mat_temp;
        }

        tempsum += cp_score_mat_temp;
        //tempsumNP += cp_score_mat_temp - cp_pawns_only_temp;

        if (color1 != for_color) {  // Its the "enemy". So show negative values.
            cp_score_mat_temp *= -1;
            cp_pawns_only_temp *= -1;
        }
        cp_score_material_all += cp_score_mat_temp;
        cp_score_pawns_only += cp_pawns_only_temp;
    }

    assert(tempsum >= 0);
    //assert(tempsumNP >= 0);
    cp_score_material_avg = tempsum / 2;

    assert(cp_score_material_avg >= 0);

    //
    //  Now do the "positional" stuff. (final eval is (material+positional)).
    //

    //
    // Get phase of game (note phase is not used for material)
    // NOTE phase must be computed before any positional eval.
    int nPhase = phase_of_game(cp_score_material_avg);

    int cp_score_position = 0;
    //bool isOK;
    int cp_score_position_temp;

    const PawnFileInfo* pawnFileInfoP = NULL;   // Initialize the lazy pointer

    cp_score_position_temp =  get_positional_for_one_color<Color::WHITE>(nPhase, evp, cp_score_material_all, pawnFileInfoP);
    if (Color::WHITE != for_color) cp_score_position_temp *= -1;
    cp_score_position += cp_score_position_temp;

    cp_score_position_temp =  get_positional_for_one_color<Color::BLACK>(nPhase, evp, cp_score_material_all, pawnFileInfoP);
    if (Color::BLACK != for_color) cp_score_position_temp *= -1;
    cp_score_position += cp_score_position_temp;

    //
    // Add the material and positional togather to get a final return in centipawns.
    cp_score_adjusted = cp_score_material_all + cp_score_position;

    return cp_score_adjusted;
}

// Final eval is (material+positional).
template<ShumiChess::Color c> 
int MinimaxAI::get_positional_for_one_color(int nPhase, ShumiChess::EvalPersons evp, int cp_score_material_all
                                            , const PawnFileInfo*& pawnFileInfoP)
{
    int bonus_cp;
    int temp;
    int cp_score_position_temp = 0;

    constexpr Color enemy_of_color = utility::representation::opposite_color_t<c>;

    switch (evp) {

        case RANDOM:
        case SLUG:          // There is no "positional considerations", for the slug.
            break;

        case CRAZY_IVAN:
            bonus_cp = engine.game_board.center_closeness_bonus<c>();
            assert(bonus_cp >= 0);
            cp_score_position_temp = bonus_cp;
            break;

        case UNCLE_SHUMI:
        default:
        {
            // no major pieces, and no more than one minor piece
            bool NoMajorPiecesEnemy  = engine.game_board.hasNoMajorPieces_t<enemy_of_color>();
            bool NoMajorPiecesFriend = engine.game_board.hasNoMajorPieces_t<c>();

            if (!NoMajorPiecesEnemy) {
                temp = cp_score_positional_get_open_cp_t<c>(nPhase, pawnFileInfoP);
                cp_score_position_temp += temp;

                temp = cp_score_positional_get_middle_cp_t<c>(nPhase);
                cp_score_position_temp += temp;
            }
            temp = cp_score_positional_get_end_t<c>(nPhase, cp_score_material_all, NoMajorPiecesFriend, NoMajorPiecesEnemy);
            cp_score_position_temp += temp;

            break;
        }
    }

    return cp_score_position_temp;
    
}

// Explicit template instantiations
template int MinimaxAI::evaluate_board_t<ShumiChess::Color::WHITE>(ShumiChess::EvalPersons);
template int MinimaxAI::evaluate_board_t<ShumiChess::Color::BLACK>(ShumiChess::EvalPersons);

template int MinimaxAI::get_positional_for_one_color<ShumiChess::Color::WHITE>(int nPhase, ShumiChess::EvalPersons evp, int cp_score_material_all, const ShumiChess::PawnFileInfo*& pawnFileInfoP);
template int MinimaxAI::get_positional_for_one_color<ShumiChess::Color::BLACK>(int nPhase, ShumiChess::EvalPersons evp, int cp_score_material_all, const ShumiChess::PawnFileInfo*& pawnFileInfoP);



template int MinimaxAI::cp_score_positional_get_open_cp_t<ShumiChess::Color::WHITE>(int, const ShumiChess::PawnFileInfo*&);
template int MinimaxAI::cp_score_positional_get_open_cp_t<ShumiChess::Color::BLACK>(int, const ShumiChess::PawnFileInfo*&);
template int MinimaxAI::cp_score_positional_get_middle_cp_t<ShumiChess::Color::WHITE>(int);
template int MinimaxAI::cp_score_positional_get_middle_cp_t<ShumiChess::Color::BLACK>(int);
template int MinimaxAI::cp_score_positional_get_end_t<ShumiChess::Color::WHITE>(int, int cp_material_all, bool, bool);
template int MinimaxAI::cp_score_positional_get_end_t<ShumiChess::Color::BLACK>(int, int cp_material_all, bool, bool);





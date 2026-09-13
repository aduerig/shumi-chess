#pragma once

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>

#include <vector>
#include <string>
#include <limits>
#include <tuple>

#include "features.hpp"
#include "gameboard.hpp"


using MoveAndScore     = std::pair<ShumiChess::Move, Score>;
using MoveAndScoreList = std::vector<MoveAndScore>;


/////////// Debug ////////////////////////////////////////////////////////////////////////////////////

//#define DEBUG_NODE_TT2    // I must also be defined in the .cpp file to work



/////////////////////////////////////////////////////////////////////////////////////////////////////


class RandomAI {
public:

    ShumiChess::Engine engine;
    
    RandomAI(ShumiChess::Engine&);
    //ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////

class MinimaxAI {
public:

    MinimaxAI(ShumiChess::Engine&);
    ~MinimaxAI();

    // The chess engine
    ShumiChess::Engine& engine;

    struct SearchTimeControl {
        ull time_left_msec = 0;            // Milliseconds remaining until time control, before this search starts
        int moves_left = 0;                // Moves until time control. (includes the move currently being searched)
        ull nominal_time_per_move = 0;     // k: time-control period / moves in period.
        ull maximum_loan = 0;              // Maximum permitted debt against future moves.
        ull minimum_future_time = 0;       // Time (milliseconds) protected for each future move.
        ull clock_reserve = 0;             // Time (milliseconds) never deliberately allocated to search.

        bool enabled() const {
            return time_left_msec > clock_reserve
                && moves_left > 0
                && nominal_time_per_move > 0;
        }
        ull hard_abort_threshold_ms = 0;

    };

    ShumiChess::EvalPersons eval_person = ShumiChess::UNCLE_SHUMI;

    ull Features_mask = _DEFAULT_FEATURES_MASK;

    std::atomic<bool> stop_calculation{false};

    ull nodes_visited = 0;
    ull nodes_visited_depth_zero = 0;
    ull evals_visited = 0;
    int iNodes_per_Second = 0;

    // Aspiration statistics
    // ---case---msec------approx sucess rates
    // Baseline 44592       na
    // 0.5      32300       %75 
    // 1.0      36100       %100
    // 1.25     37700       %100
    // 1.5      38600       %100
    // 2.0      42000       %100
    ull aspiration_attempts = 0;
    ull aspiration_successes = 0;

    ull aspiration_fail_lows = 0;
    ull aspiration_fail_highs = 0;
    ull aspiration_full_retries = 0;
    ull aspiration_first_try_nodes = 0;
    ull aspiration_retry_nodes = 0;

    ull pvs_attempts = 0;    // completed narrow-window PVS searches.
    ull pvs_successes = 0;   // narrow searches that did not require a full-window re-search.
    ull pvs_researches = 0;  // narrow searches followed by a full-window re-search.
    ull pvs_cutoffs = 0;     // narrow searches that returned a score greater than or equal to beta and therefore caused a cutoff without a full-window re-search.

    int top_deepening = 0;         // thhis is depth at top of recursion (depth==0 at bottom of recursion)
    int maximum_deepening = 0;      // used for display only

    int cp_score_material_avg = 0;

    ull passed_pawns_white = 0ULL; 
    ull passed_pawns_black = 0ULL; 

    // For PV from previous iteration
    static constexpr int MAX_PLY_PV = 256;
    std::pair<ShumiChess::Move, Score> prev_root_best_[MAX_PLY_PV + 2];


    // Hash table hit counts
    ull NhitsTT2 = 0;           // node Transposition table (TT2)
    ull NhitsP = 0;             // pawn/file hash table
    ull NTriesP = 0;            // pawn/file hash table

    ull nRandos = 0;
    ull nGames = 0;

  
    ShumiChess::Move TT2_match_move = {};
    

    /////////////////////////////////////////////////////////////////////
    // Transposition table #2 (TT2)     Protects the node (recursive_negamax()). Cleared on every game start. 
    enum class TTFlag : unsigned char {
        EXACT,       // exact alpha–beta result
        LOWER_BOUND, // fail-high node
        UPPER_BOUND  // fail-low node
    };

    struct TTEntry2 {
        int              score_cp;   // search score in centipawns
        uint8_t          depthh;      // depth this node was searched to
        ShumiChess::Move best_move;  // move that produced score_cp
        TTFlag           flagg;      // EXACT / LOWER_BOUND / UPPER_BOUND
        unsigned char    age;        // optional: for aging/replacement

  
        #ifdef DEBUG_NODE_TT2

            Score dAlphaDebug;
            Score dBetaDebug;

            // All the below to end is debug
            int nPlysDebug;
            bool drawDebug;  // 0 = not draw, 1 = draw
            //bool bIsInCheckDebug;
            //int legalMovesSize;
            int repCountDebug;
            Score dScoreDebug;

            ull   bb_wp, bb_wn, bb_wb, bb_wr, bb_wq, bb_wk;
            ull   bb_bp, bb_bn, bb_bb, bb_br, bb_bq, bb_bk;

            std::stack<ShumiChess::Move> move_history_debug; 

            bool white_castled_debug;
            bool black_castled_debug;

        #endif

    };

    std::unordered_map<uint64_t, TTEntry2> TTable2;
    ull max_TTable2_size = 0;

    std::unordered_map<uint64_t, ShumiChess::PawnFileInfo> pawn_file_info;


    ull passed_white_pawns = 0ULL; // im a bitmap
    ull passed_black_pawns = 0ULL; // im a bitmap

    // Killer moves
    ShumiChess::Move killer1[MAX_PLY]; 
    ShumiChess::Move killer2[MAX_PLY];


    // Template variants (compile-time color)
    const ShumiChess::PawnFileInfo& get_pawn_file_info_for_position();
    template<ShumiChess::Color c> int cp_score_positional_get_open_cp_t(int nPhase, const ShumiChess::PawnFileInfo*& pawnFileInfoP);
    template<ShumiChess::Color c> int cp_score_positional_get_middle_cp_t(int nPhase);
    template<ShumiChess::Color c> int cp_score_positional_get_end_t(int nPly, int cp_score_material_all, bool noMajorPiecesFriend, bool noMajorPiecesEnemy);
    template<ShumiChess::Color for_color> int evaluate_board_t(ShumiChess::EvalPersons evp);
    template<ShumiChess::Color c> int get_positional_for_one_color(int nPhase, ShumiChess::EvalPersons evp, int cp_score_material_all, const ShumiChess::PawnFileInfo*& pawnFileInfoP);
    
    template<ShumiChess::Color c> int trade_imbalance_cp_t(int material_balance, int me_pawn_material) const;
    


    void wakeup();
    void resign();
    bool should_hard_abort();
    bool should_abort_search_by_soft_time();

    bool sort_moves_for_search(vector<ShumiChess::Move>* p_moves_to_loop_over, int depth, int nPlys, bool is_top_of_deepening);
   
    
    typedef std::chrono::high_resolution_clock::time_point TIME_TYPE;


    std::tuple<Score, ShumiChess::Move> do_a_principal_variation(int depth
                                        , TIME_TYPE start_time, ull i_time_requested   //, TIME_TYPE requested_end_time
                                        , const SearchTimeControl& time_control
                                        , ull& cumul_time_msec);      // Output

    static bool should_stop_by_time(ull accum_time, double growth_factor
                                        , ull fallback_move_budget
                                        , const SearchTimeControl& time_control
                                        , double& estimated_accum_time
                                        , ull& move_budget_ms);

    tuple<Score, ShumiChess::Move> do_a_deepening(int depth, ull elapsed_time_display_only
                                                , ull& last_elapsed_time_display_only
                                                , double estimated_elapsed_time
                                                , bool estimated_elapsed_time_available);


    std::tuple<Score, ShumiChess::Move> pick_random_within_delta_rand(std::vector<std::pair<ShumiChess::Move,Score>>& MovsFromRoot,
                                             int delta_cp,
                                             int i_computer_ply_so_far,
                                             int& n_moves_within_delta     // output
                                            );

    ShumiChess::Move get_move_iterative_deepening(ull i_time_requested, int max_deepening_requested, int player_id
                                                , int iRandomMoves, int feat
                                                , SearchTimeControl time_control);

    // Overload rather than a defaulted argument: a nested class's default member
    // initializers are not available inside the enclosing class definition, so
    // "= {}" on the declaration above is ill-formed. A function body is parsed
    // after the enclosing class is complete, so building the default here is fine.
    ShumiChess::Move get_move_iterative_deepening(ull i_time_requested, int max_deepening_requested, int player_id
                                                , int iRandomMoves, int feat)
    {
        return get_move_iterative_deepening(i_time_requested, max_deepening_requested, player_id
                                          , iRandomMoves, feat, SearchTimeControl{});
    }

    //
    // Return true when the current position is safe for exact TT2 score reuse.
    //      TT2 is disabled when the search result could depend on information that is
    //      not fully represented by the position's Zobrist key:
    //          - MultiPV searches after the first variation exclude root moves.
    //          - A large halfmove count can make the 50-move rule affect the result.
    //          - A repeated position can make threefold-repetition history affect the result.
    //
    inline bool is_TT2_Valid() {
        if (n_Multis > 1) return false;
        if (engine.game_board.halfmove > (FIFTY_MOVE_RULE_PLY / 2)) return false;
    
        int cnt = engine.times_in_three_time_rep_stack();
        if (cnt > 1) return false;

        return true;
    }



    std::tuple<Score, ShumiChess::Move> recursive_negamax(int depth
                                            , Score alpha, Score beta
                                            , bool is_from_root
                                            , int nPlys
                                            , int qPlys
                                        );
    std::tuple<Score, ShumiChess::Move> recursive_negamaxQ( 
                                            //int depth,
                                            Score alpha, Score beta
                                            //, bool is_from_root
                                            //, const ShumiChess::Move& move_last //  debug (used only by _DEBUGGING_MOVE_CHAIN)
                                            , int nPlys
                                            , int qPlys
                                        );

    bool loop_over_all_moves(int depth, Score &alpha, 
                       const Score beta, 
                       int nPlys, int qPlys,
                       bool in_check, Score d_stand_pat, 
                       const vector<ShumiChess::Move>* pMoves, 
                       ShumiChess::Move &bestMoveOut, Score &bestScoreOut,
                       bool& did_cutoff);     // outputs


    int phase_of_game(int material_cp);
    int phase_of_game_full();

    bool no_queens_on_board();

    // These are reported to other "GUI" tournement directors
    Score d_best_move_score_rel = ZERO_SCORE;
    int max_attained_depth = 0;
    int max_attained_qdepth = 0;

    std::vector<std::pair<ShumiChess::Move, Score>> excluded_root_moves;          // for "MultiPV"

    //bool is_debug = false;
    int nFarts = 0;
    int nSemiFarts = 0;
    int n_futility_tosses = 0;
    ull n_delta_tosses = 0;
    ull n_delta_tries = 0;

    double response_time_sum = 0.0;
    ull response_time_cnts = 0;


    template<class T> string format_with_commas(T value);
    void playgroundOld(int iPhase);
    void playground(int iPhase);

    void print_moves_to_file(const vector<ShumiChess::Move> &mvs, int depth, char* szHeader, char* szTrailer);


    // Salt the entry. Specific to evalute_board() TT leaf protection 
    unsigned salt_the_TT(int b_is_Quiet)
    {
        unsigned mode = 0u;
        if (engine.game_board.turn == ShumiChess::BLACK) mode |= (1u << 0);   // bit0 = color
        if (b_is_Quiet)                                  mode |= (1u << 1);   // bit1 = quiet

        return mode;
    }



private:

    int n_Multis = 1;           // MultiPV if greater than 1


    bool aborts_allowed = false;        // Aborts cant happen until at least one deepening has finished, and
                                        // given us a fallback move to use.

    bool hard_abort_enabled = false;
    ull hard_abort_budget_ms = 0;           // a relative time or duration
    ull hard_abort_start_time_ms = 0;       // holds start time
    
    void hard_abort_start(ull hard_duration);
    void hard_abort_end();

    bool soft_abort_enabled = false;
    ull soft_abort_budget_ms = 0;       // Relative duration in milliseconds
    ull soft_abort_start_time_ms = 0;   // Absolute steady-clock time in milliseconds

    void soft_abort_start(ull soft_duration);
    void soft_abort_end();

};

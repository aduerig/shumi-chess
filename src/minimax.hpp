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

//
// fuses. Causes varous actions when limits hit
constexpr int MAXIMUM_DEEPENING = 40;       // If this wall hit, deepening stops. This can happen  50-move rule, all nodes return DRAW, so in so quickly
                                            // zips through these, that it runs out of depth before the time limit
constexpr ull MAX_NODES = (ull)5.0e10;      // When this happens, it acts like a user abort (last deepeining discarded)
constexpr int MAX_PLY = 50;                 // Last fuse! Can never look ahead past this far.
//assert(MAXIMUM_DEEPENING < MAX_PLY);

// Only randomizes a small amount a list formed on the root node, when at maxiumum deepening-1.
constexpr int RANDOMIZING_EQUAL_MOVES_DELTA = 45;      // In units of centi-pawns
constexpr int RANDOM_MOVE_CANDIDATES = 10;             // I must be greater than 1

class MinimaxAI {
public:

    MinimaxAI(ShumiChess::Engine&);
    ~MinimaxAI();

    ShumiChess::EvalPersons eval_person = ShumiChess::UNCLE_SHUMI;   // UNCLE_SHUMI;
    //EvalPersons evp = CRAZY_IVAN;

    ull Features_mask = _DEFAULT_FEATURES_MASK;


    bool stop_calculation = false;

    ull nodes_visited = 0;
    ull nodes_visited_depth_zero = 0;
    ull evals_visited = 0;
    int iNodes_per_Second = 0;

    int top_deepening = 0;         // thhis is depth at top of recursion (depth==0 at bottom of recursion)
    int maximum_deepening = 0;
    int maximum_duration = 0;

    int cp_score_material_avg = 0;
    //int cp_score_material_NP_avg = 0;

    ull passed_pawns_white = 0ULL; 
    ull passed_pawns_black = 0ULL; 

    // For PV from previous iteration
    static constexpr int MAX_PLY_PV = 256;
    std::pair<ShumiChess::Move, Score> prev_root_best_[MAX_PLY_PV + 2];

    // The chess engine
    ShumiChess::Engine& engine;

    // Hash table hit counts
    ull NhitsTT = 0;            // eval Transposition table (TT) (not normally used)
    ull NhitsTT2 = 0;           // node Transposition table (TT2)
    ull NhitsP = 0;             // pawn/file hash table
    ull NTriesP = 0;             // pawn/file hash table

    ull nRandos = 0;
    ull nGames = 0;

    ShumiChess::Move TT2_match_move = {};
    
    /////////////////////////////////////////////////////////////////////
    // Transposition table (TT)    Protects the evaluator (evaluate_board(). Cleared on every move 
    struct TTEntry {
        int score_cp;
        ShumiChess::Move movee;
        int depth;
    };

    std::unordered_map<uint64_t, TTEntry> TTable;


    /////////////////////////////////////////////////////////////////////
    // Transposition table #2 (TT2)     Protects the node (recursive_negamax()). Cleared on every ??? 
    enum class TTFlag : unsigned char {
        EXACT,       // exact alpha–beta result
        LOWER_BOUND, // fail-high node
        UPPER_BOUND  // fail-low node
    };

    struct TTEntry2 {
        int              score_cp;   // search score in centipawns
        int              depth;      // depth this node was searched to
        ShumiChess::Move best_move;  // move that produced score_cp
        TTFlag           flag;       // optional: EXACT / LOWER_BOUND / UPPER_BOUND
        unsigned char    age;        // optional: for aging/replacement

        Score dAlphaDebug;
        Score dBetaDebug;

        #ifdef DEBUG_NODE_TT2
            // All the below to end is debug
            int nPlysDebug;
            bool drawDebug;  // 0 = not draw, 1 = draw
            bool bIsInCheckDebug;
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

    std::unordered_map<uint64_t, ShumiChess::PawnFileInfo> pawn_file_info;


    ull passed_white_pawns = 0ULL; // im a bitmap
    ull passed_black_pawns = 0ULL; // im a bitmap

    // Killer moves
    ShumiChess::Move killer1[MAX_PLY]; 
    ShumiChess::Move killer2[MAX_PLY];


    int TT_ntrys = 0;
    int TT_ntrys1 = 0;

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

    void sort_moves_for_search(vector<ShumiChess::Move>* p_moves_to_loop_over, int depth, int nPlys, bool is_top_of_deepening);
   
    
    typedef std::chrono::high_resolution_clock::time_point TIME_TYPE;


    std::tuple<Score, ShumiChess::Move> do_a_principal_variation(int depth, ShumiChess::Move null_move
                                        , TIME_TYPE start_time, int i_time_requested, TIME_TYPE requested_end_time
                                        , ull& elapsed_time);      // Output

    tuple<Score, ShumiChess::Move> do_a_deepening(int depth, ull elapsed_time, const ShumiChess::Move& null_move);


    std::tuple<Score, ShumiChess::Move> pick_random_within_delta_rand(std::vector<std::pair<ShumiChess::Move,Score>>& MovsFromRoot,
                                             int delta_cp,
                                             int i_computer_ply_so_far,
                                             int& n_moves_within_delta     // output
                                            );

     ShumiChess::Move get_move_iterative_deepening(int i_time_requested, int max_deepening_requested, int player_id
                                                , int iRandomMoves, int feat);

    std::tuple<Score, ShumiChess::Move> recursive_negamax(int depth
                                            , Score alpha, Score beta
                                            , bool is_from_root
                                            , const ShumiChess::Move& move_last //  debug (used only by _DEBUGGING_MOVE_CHAIN)
                                            , int nPlys
                                            , int qPlys
                                        );
    std::tuple<Score, ShumiChess::Move> recursive_negamaxQ( 
                                            //int depth,
                                            Score alpha, Score beta
                                            //, bool is_from_root
                                            , const ShumiChess::Move& move_last //  debug (used only by _DEBUGGING_MOVE_CHAIN)
                                            , int nPlys
                                            , int qPlys
                                        );

    int loop_over_all_moves(int depth, Score &alpha, const Score beta, int nPlys, int qPlys,
                       bool in_check, Score d_stand_pat, 
                       const ShumiChess::Move& move_last,       // NOTE: remove me
                       const vector<ShumiChess::Move>* pMoves, 
                       ShumiChess::Move &bestMoveOut, Score &bestScoreOut,
                       bool& did_cutoff);     // outputs

    // Total of 4000 centipawns for each side.  Suppose minor pieces are all 300. 
    // Say two minor pieces traded. Then 4*300=1200, and 8000-1200=6800
    // Suppose queens and rooks traded , its 2*900+4*500=3800 or 8000-3800=4200
    // 0 - opening, 1- middle, 2- ending, 3 - ? extreme ending?


    int phase_of_game(int material_cp);
    int phase_of_game_full();

    bool no_queens_on_board();

    // These are reported to other "GUI" tournement directors
    Score d_best_move_score_rel = 0.0;
    int max_attained_depth = 0;

    //std::vector<ShumiChess::Move> excluded_root_moves;      // for "MultiPV"
    std::vector<std::pair<ShumiChess::Move, Score>> excluded_root_moves;

    //bool is_debug = false;
    int nFarts = 0;
    int nSemiFarts = 0;

    template<class T> string format_with_commas(T value);
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

};

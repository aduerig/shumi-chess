#pragma once

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>

#include <vector>
#include <unordered_map>
#include <string>
#include <limits>

#include "features.hpp"

using MoveAndScore     = std::pair<ShumiChess::Move, double>;
using MoveAndScoreList = std::vector<MoveAndScore>;

// #define DEBUG_NODE_TT2    // I must also be defined in the .cpp file to work


class RandomAI {
public:

    ShumiChess::Engine engine;
    
    //unordered_map<ull, int> piece_values;

    RandomAI(ShumiChess::Engine&);
    ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};

constexpr int MAXIMUM_DEEPENING = 50;
constexpr int MAX_PLY = 49;     // Can never look ahead past this far.

class MinimaxAI {
public:

    MinimaxAI(ShumiChess::Engine&);
    ~MinimaxAI();


    ull Features_mask = _DEFAULT_FEATURES_MASK;


    bool stop_calculation = false;

    int nodes_visited = 0;
    int nodes_visited_depth_zero = 0;
    int evals_visited = 0;
    
    int top_deepening = 0;         // thhis is depth at top of recursion (depth==0 at bottom of recursion)
    int maximum_deepening = 0;

    int cp_score_material_avg = 0;
    //int cp_score_material_NP_avg = 0;

    // For PV from previous iteration
    static constexpr int MAX_PLY_PV = 256;
    std::pair<ShumiChess::Move, double> prev_root_best_[MAX_PLY_PV + 2];

    // The chess engine
    ShumiChess::Engine& engine;

    // Zobrist
    unordered_map<uint64_t, std::string> seen_zobrist;

    ull NhitsTT = 0;
    ull NhitsTT2 = 0;
    ull nRandos = 0;

    
    // Transposition table (TT)    Protects the evaluator (evaluate_board(). Cleared on every move 
    struct TTEntry {
        int score_cp;
        ShumiChess::Move movee;
        int depth;
    };
    std::unordered_map<uint64_t, TTEntry> TTable;


    
    // Transposition table #2 (normal node-based TT)      Protects the node (recursive_negamax()). Cleared on every ??? 
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

        double dAlphaDebug;
        double dBetaDebug;

        #ifdef DEBUG_NODE_TT2
            // All the below to end is debug
            //int nPlysDebug;
            bool drawDebug;  // 0 = not draw, 1 = draw
            bool bIsInCheckDebug;
            int legalMovesSize;
            int repCountDebug;
            double dScoreDebug;

            ull   bb_wp, bb_wn, bb_wb, bb_wr, bb_wq, bb_wk;
            ull   bb_bp, bb_bn, bb_bb, bb_br, bb_bq, bb_bk;
        #endif

    };

    // Transposition table 2 (protects nodes)
    std::unordered_map<uint64_t, TTEntry2> TTable2;


    ull passed_white_pawns = 0ULL; // im a bitmap
    ull passed_black_pawns = 0ULL; // im a bitmap

    // Killer moves
    ShumiChess::Move killer1[MAX_PLY]; 
    ShumiChess::Move killer2[MAX_PLY];

    double d_random_delta = 0.0;

    int TT_ntrys = 0;
    int TT_ntrys1 = 0;


    int cp_score_get_trade_adjustment(ShumiChess::Color color, int mat_np_white, int mat_np_black);

    int cp_score_positional_get_opening(ShumiChess::Color color); 
    int cp_score_positional_get_middle(ShumiChess::Color color); 
    int cp_score_positional_get_end(ShumiChess::Color color, int nPly, int mat_avg,
                                    bool onlyKngFriend, bool onlyKngEnemy
                                ); 

    int evaluate_board(ShumiChess::Color for_color, int nPhase, bool fast_style_eval, bool isQuietPosition
                   //const std::vector<ShumiChess::Move>* pLegal_moves  // may be nullptr
    );

    void wakeup();
    void resign();

    void sort_moves_for_search(vector<ShumiChess::Move>* p_moves_to_loop_over, int depth, int nPlys, bool is_top_of_deepening);
    tuple<double, ShumiChess::Move> do_a_deepening(int depth, ull elapsed_time, const ShumiChess::Move& null_move);

    // Note: All moves from the "root" position. The root is when the player starts thinking about his move.
    std::vector<std::pair<ShumiChess::Move, double>> MovesFromRoot;

    ShumiChess::Move pick_random_within_delta_rand(
                    std::vector<MoveAndScore>& MovsFromRoot,
                    double delta_pawns,
                    int& n_moves_within_delta
                );

    ShumiChess::Move get_move_iterative_deepening(double time_requested, int max_deepening_requested, int feat);

    std::tuple<double, ShumiChess::Move> recursive_negamax(int depth
                                            , double alpha, double beta
                                            , bool is_from_root
                                            , const ShumiChess::Move& move_last
                                            , int nPlys
                                            , int qPlys
                                        );

    bool look_for_king_moves() const;
    int enemyKingSquare; 


    // 0 - opening, 1- middle, 2- ending, 3 - ? extreme ending?
    inline int phaseOfGame() {

        int i_castle_status = engine.game_board.get_castle_status_for_color(engine.game_board.turn);


        bool bHasCastled = engine.game_board.bHasCastled(engine.game_board.turn);

        int nPhase = (bHasCastled && ((engine.g_iMove)>17) );   // NOTE: this is crap
        return nPhase;
    }


    // oLD CHESS engine
    double get_value(int depth, int color_multiplier, double alpha, double beta);
    ShumiChess::Move get_move(int);
    ShumiChess::Move get_move();
    // end oLD CHESS engine


    int nFarts = 0;

    std::tuple<double, ShumiChess::Move> best_move_static(ShumiChess::Color for_color,
                                const std::vector<ShumiChess::Move>& moves,
                                //int nPly,
                                bool in_Check,
                                int depth,
                                bool isFast
                            );
               
    void print_moves_to_file(const vector<ShumiChess::Move> &mvs, int depth, char* szHeader, char* szTrailer);


    // Salt the entry. Specific to evalute_board() TT leaf protection 
    unsigned salt_the_TT(int b_is_Quiet, int nPhase)
    {
        unsigned mode = 0u;
        if (engine.game_board.turn == ShumiChess::BLACK) mode |= 1u;   // bit0 = color
        if (b_is_Quiet)                                  mode |= 2u;   // bit1 = quiet
        int phaseBits = (nPhase & 3);          // 0..3
        mode |= (unsigned)(phaseBits << 2);    // bits 2–3 = phase
        if (engine.game_board.bCastledWhite) mode |= (1u << 4);        // bit 4
        if (engine.game_board.bCastledBlack) mode |= (1u << 5);        // bit 5
        return mode;
    }

};
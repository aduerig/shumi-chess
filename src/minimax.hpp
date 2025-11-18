#pragma once

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>

#include <vector>
#include <unordered_map>
#include <string>
#include <limits>



using MoveAndScore     = std::pair<ShumiChess::Move, double>;
using MoveAndScoreList = std::vector<MoveAndScore>;




class RandomAI {
public:

    ShumiChess::Engine engine;
    
    //unordered_map<ull, int> piece_values;

    RandomAI(ShumiChess::Engine&);
    ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};


constexpr int MAX_PLY = 32;     // Can never look ahead past this far.

class MinimaxAI {
public:

    MinimaxAI(ShumiChess::Engine&);
    ~MinimaxAI();


    bool stop_calculation = false;

    int nodes_visited = 0;
    int nodes_visited_depth_zero = 0;
    int evals_visited = 0;
    
    int top_deepening = 0;         // thhis is depth at top of recursion (depth==0 at bottom of recursion)
    int maximum_deepening = 0;

    int cp_score_material_avg = 0;
    //int cp_score_material_NP_avg = 0;

    // For PV at root
    static constexpr int MAX_PLY_PV = 256;
    std::pair<ShumiChess::Move, double> prev_root_best_[MAX_PLY_PV + 2];

    // The chess engine
    ShumiChess::Engine& engine;

    // Zobrist
    unordered_map<uint64_t, std::string> seen_zobrist;

    // Transition table (TT)
    struct TTEntry {
        int score_cp;
        ShumiChess::Move movee;
        int depth;
    };
    std::unordered_map<uint64_t, TTEntry> transposition_table;

    // Killer moves
    ShumiChess::Move killer1[MAX_PLY]; 
    ShumiChess::Move killer2[MAX_PLY];

    double d_random_delta = 0.0;    // note: what am I

    int TT_ntrys = 0;
    int TT_ntrys1 = 0;

    // Storage buffers (they live here to avoid extra allocation during the game)
    //vector<ShumiChess::Move> unquiet_moves;

    int cp_score_get_trade_adjustment(ShumiChess::Color color, int mat_np_white, int mat_np_black);

    int cp_score_positional_get_opening(ShumiChess::Color color); 
    int cp_score_positional_get_middle(ShumiChess::Color color); 
    int cp_score_positional_get_end(ShumiChess::Color color, int nPly, int mat_avg,
                                    bool onlyKingFriend, bool onlyKingEnemy
                                ); 

    int evaluate_board(ShumiChess::Color for_color, int nPhase, bool fast_style_eval, bool isQuietPosition
                   //const std::vector<ShumiChess::Move>* pLegal_moves  // may be nullptr
    );

    void wakeup();

    void sort_moves_for_search(vector<ShumiChess::Move>* p_moves_to_loop_over, int depth, int nPlys);
    tuple<double, ShumiChess::Move> do_a_deepening(int depth, long long elapsed_time, const ShumiChess::Move& null_move);

    // Note: what am i?
    std::vector<std::pair<ShumiChess::Move, double>> MovesFromRoot;

    ShumiChess::Move pick_random_within_delta_rand(
                    std::vector<MoveAndScore>& MovesFromRoot,
                    double delta_pawns);

    ShumiChess::Move get_move_iterative_deepening(double timeRequested, int max_deepening_requested);

    std::tuple<double, ShumiChess::Move> recursive_negamax(int depth, double alpha, double beta
                                            //, unordered_map<uint64_t, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &move_scores_table
                                            //, unordered_map<std::string, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &move_scores_table
                                            //, MoveAndScoreList& move_and_scores_list
                                            , const ShumiChess::Move& move_last
                                            , int nPlys);

    bool look_for_king_moves() const;

    // Used for "evaluate_board" salting of te TT.
    inline unsigned MinimaxAI::make_eval_salt(bool b_is_Quiet, int nPhase) const {
        unsigned mode = 0u;
        if (engine.game_board.turn == ShumiChess::BLACK) mode |= 1u;
        if (b_is_Quiet)                                  mode |= 2u;
        int phaseBits = (nPhase & 3);
        mode |= (unsigned)(phaseBits << 2);
        if (engine.game_board.bCastledWhite) mode |= (1u << 4);
        if (engine.game_board.bCastledBlack) mode |= (1u << 5);
        return mode;
    }



    // oLD CHESS engine
    double get_value(int depth, int color_multiplier, double alpha, double beta);
    ShumiChess::Move get_move(int);
    ShumiChess::Move get_move();

    std::tuple<double, ShumiChess::Move> best_move_static(ShumiChess::Color for_color,
                                const std::vector<ShumiChess::Move>& moves,
                                int nPly,
                                bool in_Check,
                                int depth,
                                bool isFast
                            );
               
    void print_moves_to_file(const vector<ShumiChess::Move> &mvs, int depth, char* szHeader, char* szTrailer);


    // Salt the entry. Specific to evalute_board() TT protection 
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
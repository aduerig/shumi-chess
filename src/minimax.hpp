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

    int cp_score_pieces_only_avg = 0;

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

    double d_random_delta = 0.0;



    int cp_score_positional_get_opening(ShumiChess::Color color); 
    int cp_score_positional_get_middle(ShumiChess::Color color, int nPly); 
    int cp_score_positional_get_end(ShumiChess::Color color, int mat_avg); 

    int evaluate_board(ShumiChess::Color for_color, int nPly, bool fast_style_eval); //, const vector<ShumiChess::Move>& legal_moves);
    void wakeup();

    void sort_moves_for_search(vector<ShumiChess::Move>* p_moves_to_loop_over, int depth, int nPlys);
    tuple<double, ShumiChess::Move> do_a_deepening(int depth, long long elapsed_time);


    std::vector<std::pair<ShumiChess::Move, double>> MovesFromRoot;

    ShumiChess::Move pick_random_within_delta_rand(
                    std::vector<MoveAndScore>& MovesFromRoot,
                    double delta_pawns);

    //ShumiChess::Move get_move_iterative_deepening(double timeRequested);
    ShumiChess::Move get_move_iterative_deepening(double timeRequested, int max_deepening_requested);

    std::tuple<double, ShumiChess::Move> recursive_negamax(int depth, double alpha, double beta
                                            //, unordered_map<uint64_t, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &move_scores_table
                                            //, unordered_map<std::string, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &move_scores_table
                                            //, MoveAndScoreList& move_and_scores_list
                                            , const ShumiChess::Move& move_last
                                            , int nPlys);

    bool look_for_king_moves() const;

    // oLD CHESS engine
    double get_value(int depth, int color_multiplier, double alpha, double beta);
    ShumiChess::Move get_move(int);
    ShumiChess::Move get_move();


    void sort_moves_by_score(
                        MoveAndScoreList& moves_and_scores_list,  // note: pass by reference so we sort in place
                        bool sort_descending  
    );

    std::tuple<double, ShumiChess::Move>
    best_move_static(ShumiChess::Color color,
                                const std::vector<ShumiChess::Move>& moves,
                                int nPly,
                                bool in_Check,
                                int depth
                            );
               
    void print_moves_to_print_tree(std::vector<ShumiChess::Move> mvs, int depth, char* szHeader, char* szTrailer);


    void print_move_scores_to_file(
        FILE* fpDebug,
        const std::unordered_map<std::string,std::unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>>& move_scores_table
    );



};
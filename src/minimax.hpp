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
    int rand_int(int, int);
    ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};



class MinimaxAI {
public:
    bool stop_calculation = false;

    int nodes_visited = 0;
    int evals_visited = 0;
    
    int top_deepening = 0;         // thhis is depth at top of recursion (depth==0 at bottom of recursion)
    int maximum_deepening = 0;

    static constexpr int MAX_PLY_PV = 256;
    array<ShumiChess::Move, MAX_PLY_PV> prev_root_best_{};   
    //ShumiChess::Move prev_root_best_{};  // best root move from the last *completed* iteration   PV PUSH


    // The chess engine
    ShumiChess::Engine& engine;


    unordered_map<uint64_t, std::string> seen_zobrist;

    // NOTE: move me to top of class declaration.
    MinimaxAI(ShumiChess::Engine&);
    ~MinimaxAI();

    int cp_score_positional_get_opening(ShumiChess::Color color); 
    int cp_score_positional_get_middle(ShumiChess::Color color, int nPly); 
    int cp_score_positional_get_end(ShumiChess::Color color); 

    double evaluate_board(ShumiChess::Color for_color, int nPly, bool fast_style_eval); //, const vector<ShumiChess::Move>& legal_moves);
    void wakeup();

    void sort_moves_for_search(vector<ShumiChess::Move>* p_moves_to_loop_over, int depth, int nPlys);
    void do_a_deepening();

    // struct TTEntry
    // {
    //     int score_cp;            // evaluation in centipawns
    //     ShumiChess::Move movee;  // best move found for this position
    // };

    // std::unordered_map<uint64_t, std::unordered_map<int, TTEntry>> transposition_table;

    struct TTEntry {
        int score_cp;
        ShumiChess::Move movee;
        int depth;
    };

    std::unordered_map<uint64_t, TTEntry> transposition_table;




    ShumiChess::Move get_move_iterative_deepening(double);

    std::tuple<double, ShumiChess::Move> store_board_values_negamax(int depth, double alpha, double beta
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
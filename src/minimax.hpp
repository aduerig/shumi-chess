#pragma once

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>

#include <vector>
#include <unordered_map>
#include <string>
#include <limits>



using MoveScore     = std::pair<ShumiChess::Move, double>;
using MoveScoreList = std::vector<MoveScore>;




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

    int nodes_visited = 0;
    int evals_visited = 0;
    
    int top_depth = 0;         // thhis is depth at top of recursion (depth==0 at bottom of recursion)

    ShumiChess::Move prev_root_best_{};  // best root move from the last *completed* iteration   PV PUSH


    // The chess engine
    ShumiChess::Engine& engine;


    unordered_map<uint64_t, std::string> seen_zobrist;

    // NOTE: move me to top of class declaration.
    MinimaxAI(ShumiChess::Engine&);
    ~MinimaxAI();

    double evaluate_board(ShumiChess::Color for_color, const vector<ShumiChess::Move>& legal_moves);


    ShumiChess::Move get_move_iterative_deepening(double);

    std::tuple<double, ShumiChess::Move> store_board_values_negamax(int depth, double alpha, double beta
                                            //, unordered_map<uint64_t, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &move_scores_table
                                            //, unordered_map<std::string, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &move_scores_table
                                            //, MoveScoreList& move_and_scores_list
                                            , const ShumiChess::Move& move_last
                                            , int nPly);

    ShumiChess::GameState draw_by_repetition() const;
    ShumiChess::GameState draw_by_twofold() const;
    bool look_for_king_moves() const;
    bool has_repeated_move() const;
    bool alternating_repeat_prefix_exact(int pairs) const;

    // oLD CHESS engine
    double get_value(int depth, int color_multiplier, double alpha, double beta);
    ShumiChess::Move get_move(int);
    ShumiChess::Move get_move();


    void sort_moves_by_score(
                        MoveScoreList& moves_and_scores_list,  // note: pass by reference so we sort in place
                        bool sort_descending  
    );

    std::tuple<double, ShumiChess::Move>
    best_move_static(ShumiChess::Color color,
                                const std::vector<ShumiChess::Move>& moves,
                                bool in_Check
                            );


    void clear_stats_file(const char* path, FILE*& fp);


    void print_moves_to_print_tree(std::vector<ShumiChess::Move> mvs, int depth, char* szHeader, char* szTrailer);


    void print_move_scores_to_file(
        FILE* fpDebug,
        const std::unordered_map<std::string,std::unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>>& move_scores_table
    );



};
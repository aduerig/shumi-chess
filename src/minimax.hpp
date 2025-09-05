#pragma once

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>

class RandomAI {
public:
    ShumiChess::Engine engine;
    unordered_map<ull, int> piece_values;

    RandomAI(ShumiChess::Engine&);
    int rand_int(int, int);
    ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};



class MinimaxAI {
public:
    int nodes_visited = 0;
    
    int top_depth = 0;         // thhis is depth at top of recursion (depth==0 at bottom of recursion)

    // The chess engine
    ShumiChess::Engine& engine;

    // NOTE: Can these be more accurate (i.e. decimals like 2.8 for knight)
    // NOTE: Can these be more accurate as to "2 bishops" etc.
    std::array<std::tuple<ShumiChess::Piece, double>, 6> piece_values = {
        make_tuple(ShumiChess::Piece::PAWN, 1),
        make_tuple(ShumiChess::Piece::ROOK, 5),
        make_tuple(ShumiChess::Piece::KNIGHT, 3),
        make_tuple(ShumiChess::Piece::BISHOP, 3),
        make_tuple(ShumiChess::Piece::QUEEN, 8),
        make_tuple(ShumiChess::Piece::KING, 0),
    };
    unordered_map<uint64_t, std::string> seen_zobrist;

    // NOTE: move me to top of class declaration.
    MinimaxAI(ShumiChess::Engine&);
    ~MinimaxAI();

    double evaluate_board(ShumiChess::Color, vector<ShumiChess::Move>&);

    std::tuple<double, ShumiChess::Move> store_board_values_negamax(int depth, double alpha, double beta
                                            , unordered_map<uint64_t, unordered_map<ShumiChess::Move
                                            , double, utility::representation::MoveHash>> &board_values
                                            , ShumiChess::Move& move_last, bool debug);
    ShumiChess::Move get_move_iterative_deepening(double);

    double get_value(int depth, int color_multiplier, double alpha, double beta);
    ShumiChess::Move get_move(int);
    ShumiChess::Move get_move();

    void Print_tree_to_file(FILE* fpStatistics
                                    ,ShumiChess::Move& move_last
                                    ,std::tuple<double, ShumiChess::Move> final_result //, double d_end_score
                                    , ShumiChess::GameState state
                                    ,int depth);

};
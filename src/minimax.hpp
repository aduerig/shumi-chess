#pragma once

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>

class RandomAI {
    public:

    ShumiChess::Engine engine;
    unordered_map<ull, int> piece_and_values;

    RandomAI(ShumiChess::Engine&);
    int rand_int(int, int);
    ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};


class MinimaxAI {
    public:

    int nodes_visited = 0;

    ShumiChess::Engine& engine;
    unordered_map<ull, int> piece_and_values;

    MinimaxAI(ShumiChess::Engine&);

    int bits_in(ull);
    int evaluate_board();
    int evaluate_board(ShumiChess::Color);

    double store_board_values(int, int, double, double, unordered_map<int, double>);
    ShumiChess::Move MinimaxAI::get_move_iterative_deepening(double);

    double get_value(int, int, double, double);
    ShumiChess::Move get_move(int);
    ShumiChess::Move get_move();
};
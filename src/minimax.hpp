#pragma once

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>

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
    std::array<std::tuple<ShumiChess::Piece, double>, 6> piece_and_values = {
        make_tuple(ShumiChess::Piece::PAWN, 1),
        make_tuple(ShumiChess::Piece::ROOK, 5),
        make_tuple(ShumiChess::Piece::KNIGHT, 3),
        make_tuple(ShumiChess::Piece::BISHOP, 3),
        make_tuple(ShumiChess::Piece::QUEEN, 8),
        make_tuple(ShumiChess::Piece::KING, 0),
    };
    unordered_map<uint64_t, std::string> seen_zobrist;

    MinimaxAI(ShumiChess::Engine&);

    int bits_in(ull);
    double evaluate_board(ShumiChess::Color, vector<ShumiChess::Move>&);

    std::tuple<double, ShumiChess::Move> store_board_values_negamax(int depth, double alpha, double beta, unordered_map<uint64_t, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &board_values, bool);
    ShumiChess::Move get_move_iterative_deepening(double);

    double get_value(int, int, double, double);
    ShumiChess::Move get_move(int);
    ShumiChess::Move get_move();
};
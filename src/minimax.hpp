#pragma once

#include <spp.h>
#include <bitset>

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>


class RandomAI {
public:
    ShumiChess::Engine engine;
    spp::sparse_hash_map<ull, int> piece_and_values;

    RandomAI(ShumiChess::Engine&);
    int rand_int(int, int);
    ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};


class MinimaxAI {
public:
    int nodes_visited = 0;

    ShumiChess::Engine& engine;
    std::array<ShumiChess::Piece, 6> all_pieces = {
        ShumiChess::Piece::PAWN,
        ShumiChess::Piece::ROOK,
        ShumiChess::Piece::KNIGHT,
        ShumiChess::Piece::QUEEN,
        ShumiChess::Piece::KING,
    };
    std::array<double, 6> all_piece_values = {1, 5, 3, 8, 0};
    spp::sparse_hash_map<uint64_t, std::string> seen_zobrist;

    std::array<std::array<double, 8>, 8> pawn_values = {{
        {5, 5, 7, 7, 0, 0, 0, 0},
        {4, 6, 6, 8, 8, 8, 8, 0},
        {5, 5, 7, 7, 7, 7, 8, 0},
        {4, 6, 6, 6, 6, 6, 8, 0},
        {5, 5, 5, 5, 5, 5, 8, 7},
        {4, 4, 4, 4, 4, 7, 6, 7},
        {3, 3, 3, 3, 6, 5, 6, 5},
        {2, 2, 2, 5, 4, 5, 4, 5},
    }};

    std::array<double, 64> black_pawn_value_lookup;
    std::array<double, 64> white_pawn_value_lookup;


    MinimaxAI(ShumiChess::Engine&);

    inline int bits_in(ull bitboard) {
        auto bs = bitset<64>(bitboard);
        return (int) bs.count();
        
        // unsigned int c; // c accumulates the total bits set in v
        // option 1, for at most 14-bit values in v:
        // c = (bitboard * 0x200040008001ULL & 0x111111111111111ULL) % 0xf;
        // return c;

        // return std::popcount(bitboard);
    }

    double evaluate_board(ShumiChess::Color, tuple<ShumiChess::Move*, int>);

    std::tuple<double, ShumiChess::Move> store_board_values_negamax(int, double, double, spp::sparse_hash_map<uint64_t, spp::sparse_hash_map<ShumiChess::Move, double, utility::representation::MoveHash>> &, bool);
    ShumiChess::Move get_move_iterative_deepening(double);
};
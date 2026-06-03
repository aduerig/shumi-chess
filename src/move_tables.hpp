#pragma once

#include <array>
#include <cstddef>
#include <iostream>

#include "globals.hpp"
#include "utility.hpp"

namespace tables::movegen
{
    void initialize_straight_magic_tables();
    void initialize_diagonal_magic_tables();
    ull get_straight_magic_attack(ull all_pieces_but_self, int square);
    ull get_diagonal_magic_attack(ull all_pieces_but_self, int square);
    
    constexpr  std::array<ull, 64> init_king_attack_table() {
        std::array<ull, 64> king_attacks = {};

        for(int i = 0; i < 64; ++i) {
            if (i / 8 < 7 && i % 8 < 7) { // top left
                king_attacks[i] |= 1ULL << (i+9);
            }
            if (i / 8 < 7) { // top
                king_attacks[i] |= 1ULL << (i+8);
            }
            if (i / 8 < 7 && i % 8 > 0) { // top right
                king_attacks[i] |= 1ULL << (i+7);
            }
            if (i % 8 > 0) { // right
                king_attacks[i] |= 1ULL << (i-1);
            }
            if (i / 8 > 0 && i % 8 > 0) { // bottom right
                king_attacks[i] |= 1ULL << (i-9);
            }
            if (i / 8 > 0) { // bottom
                king_attacks[i] |= 1ULL << (i-8);
            }
            if (i / 8 > 0 && i % 8 < 7) { // bottom left
                king_attacks[i] |= 1ULL << (i-7);
            }
            if (i % 8 < 7) { // left
                king_attacks[i] |= 1ULL << (i+1);
            }
        }
        return king_attacks;
    }

    constexpr  std::array<ull, 64> init_knight_attack_table() {
        std::array<ull, 64> knight_attacks = {};

        for(int i=0; i < 64; ++i) {
            if (i/8 < 7 && i%8 < 6) {knight_attacks[i] |= 1ULL << (i+10);} //2l 1u
            if (i/8 < 6 && i%8 < 7) {knight_attacks[i] |= 1ULL << (i+17);} //1l 2u
            if (i/8 < 6 && i%8 > 0) {knight_attacks[i] |= 1ULL << (i+15);} //1r 2u
            if (i/8 < 7 && i%8 > 1) {knight_attacks[i] |= 1ULL << (i+6);} //2r 1u
            if (i/8 > 0 && i%8 > 1) {knight_attacks[i] |= 1ULL << (i-10);} //2r 1d
            if (i/8 > 1 && i%8 > 0) {knight_attacks[i] |= 1ULL << (i-17);} //1r 2d
            if (i/8 > 1 && i%8 < 7) {knight_attacks[i] |= 1ULL << (i-15);} //1l 2d
            if (i/8 > 0 && i%8 < 6) {knight_attacks[i] |= 1ULL << (i-6);} //2l 1d
        }

        return knight_attacks;
    }

    constexpr  std::array<ull, 64> init_white_pawn_capture_table() {
        std::array<ull, 64> pawn_captures = {};

        for(int i=0; i < 64; ++i) {
            if (i/8 < 7 && i%8 < 7) {pawn_captures[i] |= 1ULL << (i+9);} //1l 1u
            if (i/8 < 7 && i%8 > 0) {pawn_captures[i] |= 1ULL << (i+7);} //1r 1u
        }

        return pawn_captures;
    }

    constexpr  std::array<ull, 64> init_black_pawn_capture_table() {
        std::array<ull, 64> pawn_captures = {};

        for(int i=0; i < 64; ++i) {
            if (i/8 > 0 && i%8 > 0) {pawn_captures[i] |= 1ULL << (i-9);} //1r 1d
            if (i/8 > 0 && i%8 < 7) {pawn_captures[i] |= 1ULL << (i-7);} //1l 1d
        }

        return pawn_captures;
    }   

    // I include advances that are promotions.
    constexpr  std::array<ull, 64> init_white_pawn_advance_table() {
        std::array<ull, 64> pawn_advances = {};

        for(int i=0; i < 64; ++i) {
            if (i/8 < 7) {pawn_advances[i] |= 1ULL << (i+8);} //1u
        }

        return pawn_advances;
    }

    // I include advances that are promotions.
    constexpr  std::array<ull, 64> init_black_pawn_advance_table() {
        std::array<ull, 64> pawn_advances = {};

        for(int i=0; i < 64; ++i) {
            if (i/8 > 0) {pawn_advances[i] |= 1ULL << (i-8);} //1d
        }

        return pawn_advances;
    }

    constexpr  std::array<ull, 64> init_white_pawn_double_advance_table() {
        std::array<ull, 64> pawn_advances = {};

        for(int i=0; i < 64; ++i) {
            if (i/8 == 1) {pawn_advances[i] |= 1ULL << (i+16);} //2u
        }

        return pawn_advances;
    }

    constexpr  std::array<ull, 64> init_black_pawn_double_advance_table() {
        std::array<ull, 64> pawn_advances = {};

        for(int i=0; i < 64; ++i) {
            if (i/8 == 6) {pawn_advances[i] |= 1ULL << (i-16);} //2d
        }

        return pawn_advances;
    }


    inline constexpr std::array<ull,64> king_attack_table   = init_king_attack_table();
    inline constexpr std::array<ull,64> knight_attack_table = init_knight_attack_table();
    inline constexpr std::array<ull,64> white_pawn_attack_table = init_white_pawn_capture_table();
    inline constexpr std::array<ull,64> black_pawn_attack_table = init_black_pawn_capture_table();
    inline constexpr std::array<ull,64> white_pawn_adv_table = init_white_pawn_advance_table();
    inline constexpr std::array<ull,64> black_pawn_adv_table = init_black_pawn_advance_table();
    inline constexpr std::array<ull,64> white_pawn_double_adv_table = init_white_pawn_double_advance_table();
    inline constexpr std::array<ull,64> black_pawn_double_adv_table = init_black_pawn_double_advance_table();








} // namespace tables::movegen

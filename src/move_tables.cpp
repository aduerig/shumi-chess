#include "move_tables.hpp"

namespace tables::movegen {

const std::array<ull, 64> init_king_attack_table() {
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

const std::array<ull, 64> init_knight_attack_table() {
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

const std::array<ull, 64> king_attack_table = init_king_attack_table();
const std::array<ull, 64> knight_attack_table = init_knight_attack_table();

} // tables::movegen
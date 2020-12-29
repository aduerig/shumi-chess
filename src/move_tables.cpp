#include "move_tables.hpp"

namespace tables::movegen {

const std::array<ull, 64> init_king_attack_table() {
    std::array<ull, 64> king_attacks = {};

    for(int i=0; i < 64; ++i) {
        if (i/8 < 7 && i%8 < 7) {king_attacks[i] |= 1<<(i+9);} //top left
        if (i/8 < 7) {king_attacks[i] |= 1<<(i+8);} //top
        if (i/8 < 7 && i%8 > 0) {king_attacks[i] |= 1<<(i+7);} //top right
        if (i%8 > 0) {king_attacks[i] |= 1<<(i-1);} //right
        if (i/8 > 0 && i%8 > 0) {king_attacks[i] |= 1<<(i-9);} //bottom right
        if (i/8 > 0) {king_attacks[i] |= 1<<(i-8);} //bottom
        if (i/8 > 0 && i%8 < 7) {king_attacks[i] |= 1<<(i-7);} //bottom left
        if (i%8 < 7) {king_attacks[i] |= 1<<(i+1);} //left
    }

    return king_attacks;
}

const std::array<ull, 64> init_knight_attack_table() {
    std::array<ull, 64> knight_attacks = {};

    for(int i=0; i < 64; ++i) {
        if (i/8 < 7 && i%8 < 6) {knight_attacks[i] |= 1<<(i+10);} //2l 1u
        if (i/8 < 6 && i%8 < 7) {knight_attacks[i] |= 1<<(i+17);} //1l 2u
        if (i/8 < 6 && i%8 > 0) {knight_attacks[i] |= 1<<(i+15);} //1r 2u
        if (i/8 < 7 && i%8 > 1) {knight_attacks[i] |= 1<<(i+6);} //2r 1u
        if (i/8 > 0 && i%8 > 1) {knight_attacks[i] |= 1<<(i-10);} //2r 1d
        if (i/8 > 1 && i%8 > 0) {knight_attacks[i] |= 1<<(i-17);} //1r 2d
        if (i/8 > 1 && i%8 < 7) {knight_attacks[i] |= 1<<(i-15);} //1l 2d
        if (i/8 > 0 && i%8 < 6) {knight_attacks[i] |= 1<<(i-6);} //2l 1d
    }

    return knight_attacks;
}

} // tables::movegen
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

} // tables::movegen
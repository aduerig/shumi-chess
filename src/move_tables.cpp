#include "move_tables.hpp"

namespace tables::movegen {

std::array<StraightMagicEntry, 64> straight_magic_entries = {};
std::array<ull, straight_magic_attack_table_size> straight_magic_attack_table = {};
std::array<DiagonalMagicEntry, 64> diagonal_magic_entries = {};
std::array<ull, diagonal_magic_attack_table_size> diagonal_magic_attack_table = {};

namespace {

std::array<bool, 64> straight_magic_initialized = {};
std::array<bool, 64> diagonal_magic_initialized = {};

ull straight_magic_random_state = 0x9E3779B97F4A7C15ULL;
ull diagonal_magic_random_state = 0xD1B54A32D192ED03ULL;

ull next_straight_magic_random() {
    straight_magic_random_state ^= straight_magic_random_state >> 12;
    straight_magic_random_state ^= straight_magic_random_state << 25;
    straight_magic_random_state ^= straight_magic_random_state >> 27;
    return straight_magic_random_state * 2685821657736338717ULL;
}

ull next_straight_magic_candidate() {
    return next_straight_magic_random()
        & next_straight_magic_random()
        & next_straight_magic_random();
}

ull next_diagonal_magic_random() {
    diagonal_magic_random_state ^= diagonal_magic_random_state >> 12;
    diagonal_magic_random_state ^= diagonal_magic_random_state << 25;
    diagonal_magic_random_state ^= diagonal_magic_random_state >> 27;
    return diagonal_magic_random_state * 2685821657736338717ULL;
}

ull next_diagonal_magic_candidate() {
    return next_diagonal_magic_random()
        & next_diagonal_magic_random()
        & next_diagonal_magic_random();
}

int popcount64(ull value) {
    int count = 0;
    while (value) {
        value &= value - 1;
        ++count;
    }
    return count;
}

ull straight_relevant_occupancy_mask(int square) {
    ull mask = 0ULL;
    const int square_x = square % 8;
    const int square_y = square / 8;

    for (int y = square_y + 1; y <= 6; ++y) {
        mask |= 1ULL << (y * 8 + square_x);
    }
    for (int y = square_y - 1; y >= 1; --y) {
        mask |= 1ULL << (y * 8 + square_x);
    }
    for (int x = square_x + 1; x <= 6; ++x) {
        mask |= 1ULL << (square_y * 8 + x);
    }
    for (int x = square_x - 1; x >= 1; --x) {
        mask |= 1ULL << (square_y * 8 + x);
    }

    return mask;
}

ull diagonal_relevant_occupancy_mask(int square) {
    ull mask = 0ULL;
    const int square_x = square % 8;
    const int square_y = square / 8;

    for (int x = square_x - 1, y = square_y + 1; x >= 1 && y <= 6; --x, ++y) {
        mask |= 1ULL << (y * 8 + x);
    }
    for (int x = square_x + 1, y = square_y + 1; x <= 6 && y <= 6; ++x, ++y) {
        mask |= 1ULL << (y * 8 + x);
    }
    for (int x = square_x - 1, y = square_y - 1; x >= 1 && y >= 1; --x, --y) {
        mask |= 1ULL << (y * 8 + x);
    }
    for (int x = square_x + 1, y = square_y - 1; x <= 6 && y >= 1; ++x, --y) {
        mask |= 1ULL << (y * 8 + x);
    }

    return mask;
}

ull straight_attacks_on_the_fly(int square, ull blockers) {
    ull attacks = 0ULL;
    const int square_x = square % 8;
    const int square_y = square / 8;

    for (int y = square_y + 1; y <= 7; ++y) {
        const ull target = 1ULL << (y * 8 + square_x);
        attacks |= target;
        if (blockers & target) {
            break;
        }
    }
    for (int y = square_y - 1; y >= 0; --y) {
        const ull target = 1ULL << (y * 8 + square_x);
        attacks |= target;
        if (blockers & target) {
            break;
        }
    }
    for (int x = square_x + 1; x <= 7; ++x) {
        const ull target = 1ULL << (square_y * 8 + x);
        attacks |= target;
        if (blockers & target) {
            break;
        }
    }
    for (int x = square_x - 1; x >= 0; --x) {
        const ull target = 1ULL << (square_y * 8 + x);
        attacks |= target;
        if (blockers & target) {
            break;
        }
    }

    return attacks;
}

ull diagonal_attacks_on_the_fly(int square, ull blockers) {
    ull attacks = 0ULL;
    const int square_x = square % 8;
    const int square_y = square / 8;

    for (int x = square_x - 1, y = square_y + 1; x >= 0 && y <= 7; --x, ++y) {
        const ull target = 1ULL << (y * 8 + x);
        attacks |= target;
        if (blockers & target) {
            break;
        }
    }
    for (int x = square_x + 1, y = square_y + 1; x <= 7 && y <= 7; ++x, ++y) {
        const ull target = 1ULL << (y * 8 + x);
        attacks |= target;
        if (blockers & target) {
            break;
        }
    }
    for (int x = square_x - 1, y = square_y - 1; x >= 0 && y >= 0; --x, --y) {
        const ull target = 1ULL << (y * 8 + x);
        attacks |= target;
        if (blockers & target) {
            break;
        }
    }
    for (int x = square_x + 1, y = square_y - 1; x <= 7 && y >= 0; ++x, --y) {
        const ull target = 1ULL << (y * 8 + x);
        attacks |= target;
        if (blockers & target) {
            break;
        }
    }

    return attacks;
}

ull occupancy_from_index(int index, ull mask) {
    ull occupancy = 0ULL;
    int bit_index = 0;

    while (mask) {
        const ull bit = mask & -mask;
        mask &= mask - 1;

        if (index & (1 << bit_index)) {
            occupancy |= bit;
        }
        ++bit_index;
    }

    return occupancy;
}

void initialize_straight_magic_square(int square, std::size_t offset) {
    StraightMagicEntry& entry = straight_magic_entries[square];
    entry.mask = straight_relevant_occupancy_mask(square);
    const int relevant_bits = popcount64(entry.mask);
    const int occupancy_count = 1 << relevant_bits;
    entry.shift = 64 - relevant_bits;
    entry.offset = offset;

    std::array<ull, 4096> occupancies = {};
    std::array<ull, 4096> attacks = {};
    for (int index = 0; index < occupancy_count; ++index) {
        occupancies[index] = occupancy_from_index(index, entry.mask);
        attacks[index] = straight_attacks_on_the_fly(square, occupancies[index]);
    }

    std::array<ull, 4096> used_attacks = {};
    bool found_magic = false;
    while (!found_magic) {
        const ull magic = next_straight_magic_candidate();
        if (popcount64((entry.mask * magic) & 0xFF00000000000000ULL) < 6) {
            continue;
        }

        used_attacks.fill(0ULL);
        found_magic = true;
        for (int index = 0; index < occupancy_count; ++index) {
            const std::size_t magic_index = (occupancies[index] * magic) >> entry.shift;
            if (used_attacks[magic_index] == 0ULL) {
                used_attacks[magic_index] = attacks[index];
            } else if (used_attacks[magic_index] != attacks[index]) {
                found_magic = false;
                break;
            }
        }

        if (found_magic) {
            entry.magic = magic;
        }
    }

    for (int index = 0; index < occupancy_count; ++index) {
        const std::size_t magic_index = (occupancies[index] * entry.magic) >> entry.shift;
        straight_magic_attack_table[entry.offset + magic_index] = attacks[index];
    }
}

void initialize_diagonal_magic_square(int square, std::size_t offset) {
    DiagonalMagicEntry& entry = diagonal_magic_entries[square];
    entry.mask = diagonal_relevant_occupancy_mask(square);
    const int relevant_bits = popcount64(entry.mask);
    const int occupancy_count = 1 << relevant_bits;
    entry.shift = 64 - relevant_bits;
    entry.offset = offset;

    std::array<ull, 512> occupancies = {};
    std::array<ull, 512> attacks = {};
    for (int index = 0; index < occupancy_count; ++index) {
        occupancies[index] = occupancy_from_index(index, entry.mask);
        attacks[index] = diagonal_attacks_on_the_fly(square, occupancies[index]);
    }

    std::array<ull, 512> used_attacks = {};
    bool found_magic = false;
    while (!found_magic) {
        const ull magic = next_diagonal_magic_candidate();
        if (popcount64((entry.mask * magic) & 0xFF00000000000000ULL) < 6) {
            continue;
        }

        used_attacks.fill(0ULL);
        found_magic = true;
        for (int index = 0; index < occupancy_count; ++index) {
            const std::size_t magic_index = (occupancies[index] * magic) >> entry.shift;
            if (used_attacks[magic_index] == 0ULL) {
                used_attacks[magic_index] = attacks[index];
            } else if (used_attacks[magic_index] != attacks[index]) {
                found_magic = false;
                break;
            }
        }

        if (found_magic) {
            entry.magic = magic;
        }
    }

    for (int index = 0; index < occupancy_count; ++index) {
        const std::size_t magic_index = (occupancies[index] * entry.magic) >> entry.shift;
        diagonal_magic_attack_table[entry.offset + magic_index] = attacks[index];
    }
}

struct StraightMagicInitializer {
    StraightMagicInitializer() {
        initialize_straight_magic_tables();
    }
};

StraightMagicInitializer straight_magic_initializer;

struct DiagonalMagicInitializer {
    DiagonalMagicInitializer() {
        initialize_diagonal_magic_tables();
    }
};

DiagonalMagicInitializer diagonal_magic_initializer;

} // namespace

void initialize_straight_magic_tables() {
    std::size_t offset = 0;
    for (int square = 0; square < 64; ++square) {
        if (!straight_magic_initialized[square]) {
            initialize_straight_magic_square(square, offset);
            straight_magic_initialized[square] = true;
        }
        offset += 1ULL << popcount64(straight_magic_entries[square].mask);
    }

    assert(offset == straight_magic_attack_table_size);
}

void initialize_diagonal_magic_tables() {
    std::size_t offset = 0;
    for (int square = 0; square < 64; ++square) {
        if (!diagonal_magic_initialized[square]) {
            initialize_diagonal_magic_square(square, offset);
            diagonal_magic_initialized[square] = true;
        }
        offset += 1ULL << popcount64(diagonal_magic_entries[square].mask);
    }

    assert(offset == diagonal_magic_attack_table_size);
}

ull get_straight_magic_attack(ull all_pieces_but_self, int square) {
    assert(square >= 0);
    assert(square < 64);

    if (!straight_magic_initialized[square]) {
        initialize_straight_magic_tables();
    }

    const StraightMagicEntry& entry = straight_magic_entries[square];
    const ull blockers = all_pieces_but_self & entry.mask;
    const std::size_t magic_index = (blockers * entry.magic) >> entry.shift;
    return straight_magic_attack_table[entry.offset + magic_index];
}

ull get_diagonal_magic_attack(ull all_pieces_but_self, int square) {
    assert(square >= 0);
    assert(square < 64);

    if (!diagonal_magic_initialized[square]) {
        initialize_diagonal_magic_tables();
    }

    const DiagonalMagicEntry& entry = diagonal_magic_entries[square];
    const ull blockers = all_pieces_but_self & entry.mask;
    const std::size_t magic_index = (blockers * entry.magic) >> entry.shift;
    return diagonal_magic_attack_table[entry.offset + magic_index];
}

} // tables::movegen

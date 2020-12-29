#pragma once

#include <array>

#include "globals.hpp"

namespace tables::movegen
{
    const std::array<ull, 64> init_king_attack_table();
    const std::array<ull, 64> init_knight_attack_table();

    extern const std::array<ull, 64> king_attack_table = init_king_attack_table();
    extern const std::array<ull, 64> knight_attack_table = init_knight_attack_table();
} // namespace tables::movegen

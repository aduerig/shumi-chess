#pragma once

#include <array>

#include "globals.hpp"

namespace tables::movegen
{
    const std::array<ull, 64> init_king_attack_table();

    extern const std::array<ull, 64> king_attacks = init_king_attack_table();

} // namespace tables::movegen

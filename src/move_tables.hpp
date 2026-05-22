#pragma once

#include <array>
#include <iostream>

#include "globals.hpp"
#include "utility.hpp"

namespace tables::movegen
{
    const std::array<ull, 64> init_king_attack_table();
    const std::array<ull, 64> init_knight_attack_table();
    const std::array<ull, 64> init_white_pawn_capture_table();
    const std::array<ull, 64> init_black_pawn_capture_table();

    inline const std::array<ull,64> king_attack_table   = init_king_attack_table();
    inline const std::array<ull,64> knight_attack_table = init_knight_attack_table();
    inline const std::array<ull,64> white_pawn_capture_table = init_white_pawn_capture_table();
    inline const std::array<ull,64> black_pawn_capture_table = init_black_pawn_capture_table();

} // namespace tables::movegen

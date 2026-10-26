#pragma once

#include <string>
#include <vector>

#include "score.hpp"

namespace ShumiChess {

struct FEN_Score {
    std::string fen;
    // Absolute centipawns: positive favors White, negative favors Black.
    // HUGE_SCORE denotes a missing score.
    Score score;
};

// FEN text retains its internal whitespace. Record/separator padding is ignored.
// On failure, report a diagnostic and leave positions unchanged.
bool read_fen_database(
    const std::string& filename,
    std::vector<FEN_Score>& positions);

bool write_fen_database(
    const std::string& filename,
    const std::vector<FEN_Score>& positions);

bool append_fen_database(
    const std::string& filename,
    const FEN_Score& position);

} // namespace ShumiChess

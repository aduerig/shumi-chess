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

// Returns false on file or record errors and leaves positions unchanged.
// If error is supplied, it is cleared on entry and receives the diagnostic on
// failure (filename, line number when available, and reason). Otherwise,
// the diagnostic is written to stderr.
// FEN text retains its internal whitespace. Record/separator padding is ignored.
[[nodiscard]] bool read_fen_database(
    const std::string& filename,
    std::vector<FEN_Score>& positions,
    std::string* error = nullptr);

bool write_fen_database(
    const std::string& filename,
    const std::vector<FEN_Score>& positions);

bool append_fen_database(
    const std::string& filename,
    const FEN_Score& position);

} // namespace ShumiChess

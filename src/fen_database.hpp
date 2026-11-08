#pragma once

#include <cstddef>
#include <filesystem>
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

struct FENCollectionStats {
    std::size_t games = 0;
    std::size_t positions = 0;
};

// Replay one PGN file, sampling every_n_plies from each game's starting position.
// Append a source comment block and unscored FENs using CRLF line endings.
// Relative paths are resolved from the repository root; absolute paths work too.
// A malformed PGN appends no records. Stats are assigned only on success.
// Errors are returned through error, or written to stderr when it is null.
// This function does not print progress, pause, or terminate the application.
[[nodiscard]] bool collect_fens_from_pgn_file(
    const std::filesystem::path& input_filename,
    const std::filesystem::path& output_filename,
    int every_n_plies,
    FENCollectionStats& stats,
    std::string* error = nullptr);

} // namespace ShumiChess

#include "fen_database.hpp"

#include <algorithm>
#include <charconv>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

#include "engine.hpp"

namespace ShumiChess {
namespace {

constexpr std::string_view whitespace = " \t\r\n\v\f";

std::string_view trim(std::string_view text)
{
    const size_t first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

// Check the record structure without constructing or normalizing a board.
bool has_six_fen_fields(std::string_view fen)
{
    if (fen.find_first_of("|#\r\n") != std::string_view::npos) {
        return false;
    }

    size_t fields = 0;
    fen = trim(fen);
    while (!fen.empty()) {
        ++fields;
        const size_t end = fen.find_first_of(whitespace);
        if (end == std::string_view::npos) {
            break;
        }
        fen = trim(fen.substr(end));
    }
    return fields == 6;
}

bool database_error(const std::string& filename, size_t line,
                    const char* message, std::string* error = nullptr)
{
    std::string diagnostic = filename;
    if (line != 0) {
        diagnostic += ':' + std::to_string(line);
    }
    diagnostic += ": ";
    diagnostic += message;
    if (error) {
        *error = diagnostic;
    } else {
        std::cerr << diagnostic << '\n';
    }
    return false;
}

void write_record(std::ostream& file, const FEN_Score& position)
{
    file << position.fen << " | ";
    if (position.score == HUGE_SCORE) {
        file << '*';
    } else {
        file << std::to_string(position.score);
    }
    file << '\n';
}

} // namespace

bool read_fen_database(const std::string& filename,
                       std::vector<FEN_Score>& positions,
                       std::string* error)
{
    if (error) {
        error->clear();
    }
    std::ifstream file(filename);
    if (!file.is_open()) {
        return database_error(filename, 0, "could not open FEN database", error);
    }

    std::vector<FEN_Score> loaded;
    std::string line;
    size_t line_number = 0;
    while (std::getline(file, line)) {
        ++line_number;
        const std::string_view record = trim(line);
        if (record.empty() || record.front() == '#') {
            continue;
        }

        const size_t separator = record.find('|');
        const std::string_view fen = trim(record.substr(0, separator));
        if (!has_six_fen_fields(fen)) {
            return database_error(filename, line_number,
                "expected exactly six FEN fields", error);
        }

        Score score = HUGE_SCORE;
        if (separator != std::string_view::npos) {
            std::string_view value = trim(record.substr(separator + 1));
            if (value != "*") {
                if (!value.empty() && value.front() == '+') {
                    value.remove_prefix(1);
                    if (value.empty() || value.front() < '0' ||
                        value.front() > '9') {
                        return database_error(filename, line_number,
                            "expected an integer score or '*'", error);
                    }
                }
                if (value.empty()) {
                    return database_error(filename, line_number,
                        "expected an integer score or '*'", error);
                }

                int parsed = 0;
                const auto result = std::from_chars(
                    value.data(), value.data() + value.size(), parsed);
                if (result.ec != std::errc{} ||
                    result.ptr != value.data() + value.size() ||
                    parsed < std::numeric_limits<Score>::min() ||
                    parsed > std::numeric_limits<Score>::max()) {
                    return database_error(filename, line_number,
                        "expected an integer score in the Score range", error);
                }
                if (parsed == HUGE_SCORE) {
                    return database_error(filename, line_number,
                        "HUGE_SCORE is reserved; use '*' for a missing score", error);
                }
                score = static_cast<Score>(parsed);
            }
        }

        loaded.push_back({std::string(fen), score});
    }

    if (file.bad() || !file.eof()) {
        return database_error(filename, line_number + 1,
            "could not read FEN database", error);
    }
    positions.swap(loaded);
    return true;
}

bool write_fen_database(const std::string& filename,
                        const std::vector<FEN_Score>& positions)
{
    for (size_t i = 0; i < positions.size(); ++i) {
        if (!has_six_fen_fields(positions[i].fen)) {
            return database_error(filename, i + 1,
                "expected exactly six FEN fields");
        }
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        return database_error(filename, 0,
            "could not open FEN database for writing");
    }
    for (const FEN_Score& position : positions) {
        write_record(file, position);
        if (!file) {
            break;
        }
    }
    file.close();
    if (!file) {
        return database_error(filename, 0, "could not write FEN database");
    }
    return true;
}

bool append_fen_database(const std::string& filename,
                         const FEN_Score& position)
{
    if (!has_six_fen_fields(position.fen)) {
        return database_error(filename, 0,
            "expected exactly six FEN fields");
    }

    std::ofstream file(filename, std::ios::app | std::ios::ate);
    if (!file.is_open()) {
        return database_error(filename, 0,
            "could not open FEN database for appending");
    }
    const auto end = file.tellp();
    if (end == std::streampos(-1)) {
        return database_error(filename, 0,
            "could not determine FEN database append position");
    }
    // Separate an existing last line even if it has no newline. Blank lines
    // are allowed, so no existing database contents need to be read.
    if (end != std::streampos(0)) {
        file << '\n';
    }
    write_record(file, position);
    file.close();
    if (!file) {
        return database_error(filename, 0, "could not append FEN database");
    }
    return true;
}

bool append_fens_from_pgn_file(
    const std::filesystem::path& input_filename,
    const std::filesystem::path& output_filename,
    int every_n_plies,
    bool shuffle_fens,
    FENCollectionStats& stats,
    std::string* error)
{
    if (error) error->clear();
    const auto fail = [error](const std::string& message) {
        if (error) *error = message;
        else std::cerr << message << '\n';
        return false;
    };

    try {
        if (every_n_plies <= 0) {
            return fail("EVERY_N_PLIES must be greater than zero.");
        }

        const std::filesystem::path repo_root =
            std::filesystem::path(__FILE__).parent_path().parent_path();
        const std::filesystem::path input_path = repo_root / input_filename;
        const std::filesystem::path output_path = repo_root / output_filename;
        std::ifstream input(input_path, std::ios::binary);
        if (!input.is_open()) {
            return fail("Could not open PGN input: " + input_path.string());
        }
        const bool output_exists = std::filesystem::exists(output_path);
        if (output_exists && std::filesystem::equivalent(input_path, output_path)) {
            return fail("PGN input and FEN output must be different files.");
        }

        // Read the text once. The replay cursor below only advances through it.
        std::string pgn_text, line;
        while (std::getline(input, line)) {
            pgn_text += line;
            pgn_text += '\n';
        }
        if (input.bad() || !input.eof()) {
            return fail("Could not read PGN input: " + input_path.string());
        }
        if (pgn_text.compare(0, 3, "\xEF\xBB\xBF") == 0) pgn_text.erase(0, 3);

        // Keep an existing final record separate even if its newline is missing.
        bool needs_newline = false;
        if (output_exists) {
            std::ifstream tail(output_path, std::ios::binary | std::ios::ate);
            if (!tail.is_open() || tail.tellg() == std::streampos(-1)) {
                return fail("Could not inspect FEN output: " + output_path.string());
            }
            if (tail.tellg() > std::streampos(0)) {
                tail.seekg(-1, std::ios::end);
                char last = 0;
                if (!tail.get(last)) {
                    return fail("Could not read end of FEN output: " + output_path.string());
                }
                needs_newline = last != '\n';
            }
        }

        std::ofstream output(output_path, std::ios::app | std::ios::binary);
        if (!output.is_open()) {
            return fail("Could not open FEN output for appending: " + output_path.string());
        }

        PGN pgn;
        Engine engine;
        std::size_t position = 0;
        FENCollectionStats collected;
        // Buffer one file so malformed input cannot append a partial set.
        std::vector<std::string> fen_records;
        std::string replay_error;
        while (position < pgn_text.size()) {
            const std::string context = "Game " + std::to_string(collected.games + 1)
                                        + " in " + input_path.string() + ": ";
            if (!pgn.start_pgn_game(pgn_text, position, engine, &replay_error)) {
                return fail(context + replay_error);
            }
            // Starting a game also skips trailing whitespace and comments.
            if (position == pgn_text.size()) break;
            ++collected.games;
            for (;;) {
                const auto result = pgn.play_next_pgn_move(pgn_text, position, engine, &replay_error);
                if (result == PGN::MoveReadResult::Error) {
                    return fail(context + replay_error);
                }
                if (result == PGN::MoveReadResult::GameEnded) break;
                if (engine.ply_so_far % every_n_plies != 0) continue;

                fen_records.push_back(engine.game_board.to_fen() + " | *\r\n");
                ++collected.positions;
            }
        }
        if (collected.positions != 0) {
            if (shuffle_fens && fen_records.size() > 1) {
                std::mt19937 rng(std::random_device{}());
                std::shuffle(fen_records.begin(), fen_records.end(), rng);
            }
            const std::time_t collected_at = std::time(nullptr);
            std::tm collection_date{};
#ifdef _WIN32
            const bool date_ready = localtime_s(&collection_date, &collected_at) == 0;
#else
            const bool date_ready = localtime_r(&collected_at, &collection_date) != nullptr;
#endif
            if (!date_ready) {
                return fail("Could not determine the FEN collection date.");
            }
            if (needs_newline) {
                output << "\r\n";
            }
            output << "#\r\n# FENs SOURCE: " << input_filename.generic_string()
                   << " (" << std::put_time(&collection_date, "%Y-%m-%d %H:%M") << ")"
                   << "\r\n#    POSITIONS=" << collected.positions << " SAMPLE RATE (ply)=" << every_n_plies
                   << " SHUFFLE_FENS=" << (shuffle_fens ? "true" : "false")
                   << "\r\n#    OUTPUT: " << output_filename.generic_string()
                   << "\r\n#\r\n";
            for (const auto& record : fen_records) {
                output << record;
            }
        }
        output.close();
        if (!output) {
            return fail("Could not write or close FEN output: " + output_path.string());
        }
        stats = collected;
        return true;
    } catch (const std::exception& exception) {
        return fail(exception.what());
    }
}

} // namespace ShumiChess

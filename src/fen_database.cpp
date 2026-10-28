#include "fen_database.hpp"

#include <charconv>
#include <fstream>
#include <iostream>
#include <limits>
#include <string_view>

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

} // namespace ShumiChess

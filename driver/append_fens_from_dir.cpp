#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "fen_database.hpp"
#include "status_output.hpp"

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <conio.h>
#else
    static inline int _getch() { return std::cin.get(); }
#endif

//////////////////////////////////////////////////////////////////////////////////////

// Relative paths are resolved from the repository root; absolute paths work too.
// Process every regular file directly inside this directory, regardless of extension.
// Append each file's source header and FENs.
constexpr char INPUT_DIRECTORY[] = "C:/chess/chessGames/pgn";
constexpr char OUTPUT_FILENAME[] = "NNUE/FENs/collected_from_pgn.txt";
constexpr int EVERY_N_PLIES = 1;
constexpr bool SHUFFLE_FENS = true; // Shuffle each file's new FENs before appending.
// true: gather all existing and new comment lines at the top after collection.
// false: keep each source header immediately before that file's FEN block.
constexpr bool COMMENTS_AT_TOP = true;
// The pipeline accepts this status with a warning and evaluates the valid files.
constexpr int PARTIAL_SUCCESS = 2;

/////////////////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace ShumiChess;

namespace {

void move_comments_to_top(const filesystem::path& output_path)
{
    // Stage beside the output so replacement stays on the same filesystem.
    // An exclusive directory avoids overwriting any existing temporary file.
    filesystem::path temp_directory = output_path;
    temp_directory += ".comments.tmp";
    if (!filesystem::create_directory(temp_directory)) {
        throw runtime_error("Comment staging directory already exists: " + temp_directory.string());
    }
    const filesystem::path temp_path = temp_directory / "reordered.txt";
    struct Cleanup {
        filesystem::path file, directory;
        ~Cleanup() {
            error_code ignored;
            filesystem::remove(file, ignored);
            filesystem::remove(directory, ignored);
        }
    } cleanup{temp_path, temp_directory};

    ifstream input(output_path, ios::binary);
    ofstream output(temp_path, ios::binary);
    if (!input || !output) {
        throw runtime_error("Could not open files to gather comments: " + output_path.string());
    }

    const auto is_comment = [](const string& line) {
        const auto first = line.find_first_not_of(" \t\r\n\v\f");
        return first != string::npos && line[first] == '#';
    };
    // Two streaming passes keep memory bounded even for large FEN databases.
    // Preserve comment order, FEN order, whitespace, and existing line endings.
    bool seen_record = false;
    bool needs_reordering = false;
    string line;
    while (getline(input, line)) {
        if (is_comment(line)) {
            needs_reordering |= seen_record;
            output << line;
            // A final comment without a newline must not swallow the first FEN.
            output << (input.eof() ? "\r\n" : "\n");
        } else if (line.find_first_not_of(" \t\r\n\v\f") != string::npos) {
            seen_record = true;
        }
    }
    if (input.bad() || !input.eof()) {
        throw runtime_error("Could not read FEN output: " + output_path.string());
    }
    if (!needs_reordering) return;

    input.clear();
    input.seekg(0);
    while (getline(input, line)) {
        if (!is_comment(line)) {
            output << line;
            if (!input.eof()) output << '\n';
        }
    }
    if (input.bad() || !input.eof()) {
        throw runtime_error("Could not reread FEN output: " + output_path.string());
    }
    input.close();
    output.close();
    if (!output) {
        throw runtime_error("Could not write reordered FEN output: " + temp_path.string());
    }

    // Replace only after the complete reordered file has been written and closed.
#ifdef _WIN32
    if (!MoveFileExW(temp_path.c_str(), output_path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        throw system_error(static_cast<int>(GetLastError()), system_category(),
                           "Could not replace FEN output: " + output_path.string());
    }
#else
    filesystem::rename(temp_path, output_path);
#endif
}

} // namespace

int stop_after_error(const string& error)
{
    sout << error << endl;
    sout << "Press any key to exit..." << endl;
    _getch();
    return EXIT_FAILURE;
}

int main()
{
    try {
        if (EVERY_N_PLIES <= 0) {
            return stop_after_error("EVERY_N_PLIES must be greater than zero.");
        }

        const filesystem::path repo_root = filesystem::path(__FILE__).parent_path().parent_path();
        const filesystem::path input_directory = repo_root / INPUT_DIRECTORY;
        const filesystem::path output_path = repo_root / OUTPUT_FILENAME;
        if (!filesystem::is_directory(input_directory)) {
            return stop_after_error("PGN input directory does not exist or is not a directory: "
                                    + input_directory.string());
        }

        // Take the complete input list before appending, excluding the output itself.
        const bool output_exists = filesystem::exists(output_path);
        vector<filesystem::path> input_files;
        for (const auto& entry : filesystem::directory_iterator(input_directory)) {
            if (!entry.is_regular_file()) continue;
            if (output_exists && filesystem::equivalent(entry.path(), output_path)) continue;
            input_files.push_back(filesystem::path(INPUT_DIRECTORY) / entry.path().filename());
        }
        sort(input_files.begin(), input_files.end(), [](const auto& left, const auto& right) {
            return left.filename() < right.filename();
        });
        if (input_files.empty()) {
            return stop_after_error("No input files found in PGN directory: " + input_directory.string());
        }

        sout << "Reading PGN directory " << INPUT_DIRECTORY << endl;
        sout << "Files to process: " << input_files.size() << endl;
        sout << "SAMPLE RATE (ply)=" << EVERY_N_PLIES << endl;
        sout << "COMMENTS_AT_TOP=" << (COMMENTS_AT_TOP ? "true" : "false") << endl;
        sout << "..." << endl;

        FENCollectionStats totals;
        vector<filesystem::path> failed_files;
        for (const auto& input_file : input_files) {
            sout << "Reading PGN file " << input_file.generic_string() << endl;
            FENCollectionStats stats;
            string error;
            if (!append_fens_from_pgn_file(
                    input_file, OUTPUT_FILENAME, EVERY_N_PLIES, SHUFFLE_FENS, stats, &error)) {
                sout << "Failed: " << input_file.generic_string() << ": " << error << endl;
                failed_files.push_back(input_file);
                continue;
            }
            totals.games += stats.games;
            totals.positions += stats.positions;
            sout << "Processed " << stats.games << " games; appended " << stats.positions << " FENs." << endl;
        }

        if (COMMENTS_AT_TOP && filesystem::exists(output_path)) {
            sout << "Gathering all source comments at the top of " << output_path.string() << endl;
            move_comments_to_top(output_path);
        }

        sout << "Processed " << input_files.size() - failed_files.size() << " files and "
             << totals.games << " games; failed files: " << failed_files.size() << endl;
        sout << "Appended " << totals.positions << " FENs to " << output_path.string() << endl;
        if (!failed_files.empty()) {
            sout << "WARNING: These PGN files failed; see errors above:" << endl;
            for (const auto& file : failed_files) sout << "  " << file.generic_string() << endl;
        }
        const int exit_code = totals.positions == 0 ? EXIT_FAILURE
            : failed_files.empty() ? EXIT_SUCCESS : PARTIAL_SUCCESS;
        if (exit_code == PARTIAL_SUCCESS) {
            sout << "PARTIAL SUCCESS - collected FENs from successful files can still be evaluated." << endl;
        } else if (exit_code == EXIT_SUCCESS) {
            sout << "SUCCESS" << endl;
        } else {
            sout << "FAILED - no FENs were appended." << endl;
        }
        sout << "Press any key to exit..." << endl;
        _getch();
        return exit_code;
    } catch (const exception& error) {
        return stop_after_error(error.what());
    }
}

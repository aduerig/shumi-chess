#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include "fen_database.hpp"
#include "status_output.hpp"

#ifdef _WIN32
    #include <conio.h>
#else
    static inline int _getch() { return std::cin.get(); }
#endif

// Relative paths are resolved from the repository root; absolute paths work too.
// Process every regular file directly inside this directory, regardless of extension.
constexpr char INPUT_DIRECTORY[] = "C:/chess/chessGames/pgn";
constexpr char OUTPUT_FILENAME[] = "NNUE/FENs/collected_from_pgn.txt";
constexpr int EVERY_N_PLIES = 5;

using namespace std;
using namespace ShumiChess;

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
        sout << "..." << endl;

        FENCollectionStats totals;
        vector<filesystem::path> failed_files;
        for (const auto& input_file : input_files) {
            sout << "Reading PGN file " << input_file.generic_string() << endl;
            FENCollectionStats stats;
            string error;
            if (!collect_fens_from_pgn_file(input_file, OUTPUT_FILENAME, EVERY_N_PLIES, stats, &error)) {
                sout << "Failed: " << input_file.generic_string() << ": " << error << endl;
                failed_files.push_back(input_file);
                continue;
            }
            totals.games += stats.games;
            totals.positions += stats.positions;
            sout << "Processed " << stats.games << " games; appended " << stats.positions << " FENs." << endl;
        }

        sout << "Processed " << input_files.size() - failed_files.size() << " files and "
             << totals.games << " games; failed files: " << failed_files.size() << endl;
        sout << "Appended " << totals.positions << " FENs to " << output_path.string() << endl;
        if (!failed_files.empty()) {
            sout << "Failed files:" << endl;
            for (const auto& file : failed_files) sout << "  " << file.generic_string() << endl;
        }
        sout << "Press any key to exit..." << endl;
        _getch();
        return failed_files.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
    } catch (const exception& error) {
        return stop_after_error(error.what());
    }
}

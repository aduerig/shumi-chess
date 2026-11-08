#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>

#include "fen_database.hpp"
#include "status_output.hpp"

#ifdef _WIN32
    #include <conio.h>
#else
    static inline int _getch() { return std::cin.get(); }
#endif

// Relative filenames are resolved from the repository root; absolute paths work too.
// The existing Morphy PGN is stored with a .txt extension. Set this to your .pgn file.
//constexpr char INPUT_FILENAME[] = "tests/FENs/Morphy_random_pgn.txt";
constexpr char INPUT_FILENAME[] = "C:/chess/chessGames/pgn/Fine.pgn";
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
        const filesystem::path output_path = repo_root / OUTPUT_FILENAME;

        sout << "Reading PGN file " << INPUT_FILENAME << endl;
        sout << "SAMPLE RATE (ply)=" << EVERY_N_PLIES << endl;
        sout << "..." << endl;

        FENCollectionStats stats;
        string error;
        if (!collect_fens_from_pgn_file(INPUT_FILENAME, OUTPUT_FILENAME, EVERY_N_PLIES, stats, &error)) {
            return stop_after_error(error);
        }
        sout << "Appended " << stats.positions << " FENs to " << output_path.string() << endl;
        sout << "Press any key to exit..." << endl;
        _getch();
        return EXIT_SUCCESS;
    } catch (const exception& error) {
        return stop_after_error(error.what());
    }
}

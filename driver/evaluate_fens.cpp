#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "NNUE.hpp"
#include "status_output.hpp"

#ifdef _WIN32
    #include <conio.h>
#else
    static inline int _getch() { return std::cin.get(); }
#endif

using namespace std;
using namespace ShumiChess;

//////////////////////////////////////////////////////////////////////////////////////////

const filesystem::path repo_root = filesystem::path(__FILE__).parent_path().parent_path();
const filesystem::path input_path = repo_root / "NNUE" / "FENs" / "collected_from_pgn.txt";
const filesystem::path output_path = repo_root / "NNUE" / "FENs" / "evaluated_FENs.txt";

constexpr int DEPTH_MINIMUM = 3;
constexpr int TIME_REQUESTED = 3000;
constexpr EvalPersons PLAYER = UNCLE_SHUMI;
const auto iMethod = NNUETrainingMethod::STATIC_EVAL;

/////////////////////////////////////////////////////////////////////////////////////

// make with: cmake --build C:\programming\shumi-chess\build --config RelWithDebInfo --target evaluate_fens

int main(int argc, char** argv) {
    // Read in datafile into the training examples.
    vector<NNUETrainingExample> examples;
    vector<string> retained_fens;


    const char* method_name = "UNKNOWN";
    switch (iMethod) {
        case NNUETrainingMethod::NONE:        method_name = "NONE"; break;
        case NNUETrainingMethod::FULL_SEARCH: method_name = "FULL_SEARCH"; break;
        case NNUETrainingMethod::STATIC_EVAL: method_name = "STATIC_EVAL"; break;
    }


    const char* player_name = "UNKNOWN";
    switch (PLAYER) {
        case NNUE:        player_name = "NNUE"; break;
        case SLUG:        player_name = "SLUG"; break;
        case CRAZY_IVAN:  player_name = "CRAZY_IVAN"; break;
        case UNCLE_SHUMI: player_name = "UNCLE_SHUMI"; break;
    }
    
    sout << "Evaluating FENs: -d" << DEPTH_MINIMUM << " -t" << TIME_REQUESTED
         << " player=" << player_name << " method=" << method_name << endl;
    sout << "Input file: " << input_path.string() << endl;

    string database_error;
    sout << "..." << endl;
    const int read_error = multi_FEN_readin(input_path, examples, iMethod
                                            , DEPTH_MINIMUM, TIME_REQUESTED, PLAYER
                                            , /*material_after_best_move=*/true, /*monitor=*/true
                                            , &database_error, &retained_fens);
    if (read_error != 0) {
        sout << database_error << endl;
        _getch();
        return read_error;
    }

    // Reopen the input for its comments. The reader supplies the retained FENs.
    ifstream input(input_path);
    if (!input.is_open()) {
        sout << "Could not reopen FEN input: " << input_path << endl;
        return EXIT_FAILURE;
    }
    const time_t generated_at = time(nullptr);
    tm generation_date{};
#ifdef _WIN32
    const bool date_ready = localtime_s(&generation_date, &generated_at) == 0;
#else
    const bool date_ready = localtime_r(&generated_at, &generation_date) != nullptr;
#endif
    if (!date_ready) {
        sout << "Could not determine the score generation date" << endl;
        return EXIT_FAILURE;
    }

    ofstream output(output_path);
    if (!output.is_open()) {
        sout << "Could not open FEN output: " << output_path << endl;
        return EXIT_FAILURE;
    }

    // Copy all input comments, including FENs SOURCE, adding '#' to each marker.
    string line;
    while (getline(input, line)) {
        const auto first = line.find_first_not_of(" \t\r\n\v\f");
        if (first != string::npos && line[first] == '#') {
            output << line.substr(0, first) << '#' << line.substr(first) << '\n';
        }
    }
    if (input.bad() || !input.eof()) {
        sout << "Could not read FEN input: " << input_path << endl;
        return EXIT_FAILURE;
    }

    // Record the evaluation settings after the input comments and before the FENs.
    output << "#\n# SCOREs SOURCE: " << input_path.lexically_relative(repo_root).generic_string()
           << " (" << put_time(&generation_date, "%Y-%m-%d %H:%M") << ")\n"
           << "#     -d" << DEPTH_MINIMUM << " -t" << TIME_REQUESTED
           << " player=" << player_name << " method=" << method_name << '\n'
           << "#     POSITIONS=" << examples.size() << '\n'
           << "#     OUTPUT: " << output_path.lexically_relative(repo_root).generic_string() << "\n#\n";

    // Use the retained original FENs so skipped mates cannot shift score alignment.
    // NNUE inputs omit castling, en passant, and move counters.
    // NONE expects decimal integers, so avoid fractional or scientific notation.
    output << fixed << setprecision(0);
    for (size_t position = 0; position < examples.size(); ++position) {
        output << retained_fens[position] << " | "
               << examples[position].target_positional_score_cp << '\n';
    }
    output.close();
    if (!output) {
        sout << "Could not write FEN output: " << output_path << endl;
        return EXIT_FAILURE;
    }

    sout << "Wrote " << examples.size() << " evaluated FENs to " << output_path << endl;
    sout << "SUCCESS - press any key" << endl << endl;
    _getch();
    return EXIT_SUCCESS;
}

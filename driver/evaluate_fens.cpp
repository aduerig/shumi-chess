#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
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

const filesystem::path input_path = repo_root / "NNUE" / "FENs" / "collected_from_pgn.txt";   //  NNUE\FENs\collected_from_pgn.txt
const filesystem::path output_path = repo_root / "NNUE" / "FENs" / "evaluated_FENs.txt";
// const filesystem::path input_path = repo_root / "doc" / "evals.txt";
// const filesystem::path output_path = repo_root / "doc" / "evalsPLUS.txt";

constexpr int DEPTH_MINIMUM = 3;
constexpr int TIME_REQUESTED = 3000;
constexpr EvalPersons PLAYER = UNCLE_SHUMI;
const auto iMethod = NNUETrainingMethod::STATIC_EVAL;

/////////////////////////////////////////////////////////////////////////////////////

// make with: cmake --build C:\programming\shumi-chess\build --config RelWithDebInfo --target evaluate_fens

int main(int argc, char** argv) {
    const auto started_at = chrono::steady_clock::now();

    // Read in datafile into the training examples.
    vector<NNUETrainingExample> examples;
    vector<string> retained_fens;


    const char* method_name = "UNKNOWN";
    switch (iMethod) {
        case NNUETrainingMethod::NONE:        method_name = "NONE"; break;
        case NNUETrainingMethod::FULL_SEARCH: method_name = "FULL_SEARCH"; break;
        case NNUETrainingMethod::STATIC_EVAL: method_name = "STATIC_EVAL"; break;
        case NNUETrainingMethod::RANDOM_EVAL: method_name = "RANDOM_EVAL"; break;
    }


    const char* player_name = "UNKNOWN";
    switch (PLAYER) {
        case NNUE1:       player_name = "NNUE1"; break;
        case NNUE2:       player_name = "NNUE2"; break;
        case SLUG:        player_name = "SLUG"; break;
        case CRAZY_IVAN:  player_name = "CRAZY_IVAN"; break;
        case UNCLE_SHUMI: player_name = "UNCLE_SHUMI"; break;
    }
    
    sout << "Evaluating FENs: -d" << DEPTH_MINIMUM << " -t" << TIME_REQUESTED
         << " player=" << player_name << " method=" << method_name << endl;
    sout << "Input file: " << input_path.string() << endl;

    string database_error;
    sout << "..." << endl;
    const auto evaluation_started_at = chrono::steady_clock::now();
    const int read_error = multi_FEN_readin(input_path, examples, iMethod
                                            , DEPTH_MINIMUM, TIME_REQUESTED, PLAYER
                                            , /*material_after_best_move=*/true, /*monitor=*/true
                                            , &database_error, &retained_fens);
    const double evaluation_seconds = chrono::duration<double>(
        chrono::steady_clock::now() - evaluation_started_at).count();
    if (read_error != 0) {
        sout << database_error << endl;
        _getch();
        return read_error;
    }

    // Preserve the original lines, including comments and line endings.
    ifstream input(input_path, ios::binary);
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

    ofstream output(output_path, ios::binary);
    if (!output.is_open()) {
        sout << "Could not open FEN output: " << output_path << endl;
        return EXIT_FAILURE;
    }

    const auto trim = [](string_view text) -> string_view {
        constexpr string_view whitespace = " \t\r\n\v\f";
        const auto first = text.find_first_not_of(whitespace);
        if (first == string_view::npos) return {};
        return text.substr(first, text.find_last_not_of(whitespace) - first + 1);
    };

    // Finish the opening comment block with the evaluation summary so model
    // saving copies it along with the source headers.
    bool score_header_written = false;

    // Use the retained FENs so skipped mates cannot shift score alignment.
    // Apart from the added summary, only score placeholders change.
    output << fixed << setprecision(0);
    size_t position = 0;
    string line;
    while (getline(input, line)) {
        const string_view record = string_view(line).substr(0, line.find('#'));
        if (trim(record).empty()) {
            output << line;
        } else {
            const auto separator = record.find('|');
            const auto fen = trim(record.substr(0, separator));
            if (position == examples.size() || fen != retained_fens[position]) {
                continue; // FULL_SEARCH omits positions with mate scores.
            }
            if (!score_header_written) {
                output << "#\r\n# SCOREs SOURCE: " << input_path.lexically_relative(repo_root).generic_string()
                       << " (" << put_time(&generation_date, "%Y-%m-%d %H:%M") << ")\r\n"
                       << "#     -d" << DEPTH_MINIMUM << " -t" << TIME_REQUESTED
                       << " player=" << player_name << " method=" << method_name << "\r\n"
                       << "#     POSITIONS=" << examples.size() << "\r\n"
                       << "#     OUTPUT: " << output_path.lexically_relative(repo_root).generic_string() << "\r\n#\r\n";
                score_header_written = true;
            }
            if (iMethod == NNUETrainingMethod::NONE) {
                output << line;
            } else {
                const auto placeholder = record.find('*', separator);
                if (placeholder == string_view::npos) {
                    sout << "Missing score placeholder in FEN input" << endl;
                    return EXIT_FAILURE;
                }
                output << line.substr(0, placeholder)
                       << examples[position].target_positional_score_cp
                       << line.substr(placeholder + 1);
            }
            ++position;
        }
        if (!input.eof()) output << '\n';
    }
    if (input.bad() || !input.eof() || position != examples.size()) {
        sout << "Could not read FEN input: " << input_path << endl;
        return EXIT_FAILURE;
    }
    output.close();
    if (!output) {
        sout << "Could not write FEN output: " << output_path << endl;
        return EXIT_FAILURE;
    }

    sout << "Wrote " << examples.size() << " evaluated FENs to " << output_path << endl;
    const auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - started_at).count();
    sout << "Elapsed time: " << elapsed_ms / 3'600'000 << "h "
         << (elapsed_ms / 60'000) % 60 << "m "
         << fixed << setprecision(3) << (elapsed_ms % 60'000) / 1000.0 << "s" << endl;
    sout << "Evaluations per second: " << fixed << setprecision(2)
         << (evaluation_seconds > 0.0 ? examples.size() / evaluation_seconds : 0.0) << endl;
    sout << "SUCCESS - press any key" << endl << endl;
    _getch();
    return EXIT_SUCCESS;
}

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "NNUE.hpp"
#include "status_output.hpp"
#ifdef _WIN32
    #include <conio.h>
#else
    // conio.h is Microsoft-only. On other platforms wait on a line instead of a keystroke.
    #include <iostream>
    static inline int _getch() { return std::cin.get(); }
#endif


////////////////////////////////////////////////////////////////////////////////////////////

// Training input and output, relative to the repository root.
constexpr const char* database_path = "NNUE/FENs/mixed_330K_shuffled.txt";   // "C:\programming\shumi-chess\NNUE\FENs\mixed_330K_shuffled.txt"
constexpr const char* NNUE_model_output_file = "NNUE/models/temp.mod";

constexpr int TRAINING_ITERATIONS = 400;
constexpr float learning_rate = 0.0001f;

// true: update the current iteration in place; false: print metrics every 10 iterations.
constexpr bool ITERIVE_DISPLAY = false;

constexpr int N_ACTIVE_NEURONS = 127;

///////////////////////////////////////////////////////////////////////////////////////////

using namespace std;
using namespace ShumiChess;

////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    const auto started_at = chrono::steady_clock::now();
 
    // Compose filename of where the training datafile is (FENScores)
    const filesystem::path repo_root = filesystem::path(__FILE__).parent_path().parent_path();
    const filesystem::path input_path = repo_root / database_path;

    sout << "Reading FEN scores file: " << input_path.string() << endl;

    // Read in datafile into the training examples
    vector<NNUETrainingExample> examples;
    const auto iMethod = NNUETrainingMethod::NONE;
    constexpr int DEPTH_MINIMUM = 0;
    constexpr int TIME_REQUESTED = 0;
    string database_error;
    const int read_error = multi_FEN_readin(input_path, examples, iMethod
                                            , DEPTH_MINIMUM, TIME_REQUESTED, UNCLE_SHUMI
                                            , /*material_after_best_move=*/false, /*monitor=*/false
                                            , &database_error);
    if (read_error != 0) {
        sout << database_error << endl;
        _getch();
        return read_error;
    }


    // Train a NNUE model based on the training examples
    NNUE_data nnue;
    const int nTrainingPositions = static_cast<int>(examples.size());
    sout << "Training NNUE model: positions=" << nTrainingPositions
         << " iterations=" << TRAINING_ITERATIONS
         << " learning_rate=" << learning_rate << endl;
    const auto training_started_at = chrono::steady_clock::now();
    const auto training_result = NNUE_train_mult_positions(nnue, examples.data(), nTrainingPositions
                                , N_ACTIVE_NEURONS, TRAINING_ITERATIONS, learning_rate
                                , true, nullptr, ITERIVE_DISPLAY
                                );
    const double training_seconds = chrono::duration<double>(
        chrono::steady_clock::now() - training_started_at).count();

    // Write out the NNUE model to a file
    const filesystem::path parameters_path = repo_root / NNUE_model_output_file;
    // save_parameters copies the input comments with one additional '#'.
    const bool saved_parameters = nnue.save_parameters(parameters_path, input_path, N_ACTIVE_NEURONS,
                                                       TRAINING_ITERATIONS, learning_rate, training_result);
    if (!saved_parameters) {
        sout << "Failed to save NNUE parameters to " << parameters_path << endl;
        sout << "Press any key to exit..." << endl;
    } else {
        sout << "Saved NNUE model to: " << parameters_path.string() << endl;
        const auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - started_at).count();
        sout << "Elapsed time: " << elapsed_ms / 3'600'000 << "h "
             << (elapsed_ms / 60'000) % 60 << "m "
             << fixed << setprecision(3) << (elapsed_ms % 60'000) / 1000.0 << "s" << endl;
        sout << "Training iterations per second: " << fixed << setprecision(2)
             << (training_seconds > 0.0 ? TRAINING_ITERATIONS / training_seconds : 0.0) << endl;
        sout << "SUCCESS - press any key" << endl << endl;
    }

    _getch();

    return saved_parameters ? EXIT_SUCCESS : EXIT_FAILURE;
}

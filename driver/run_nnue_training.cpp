#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "NNUE.hpp"
#include "status_output.hpp"
#include "weights.hpp"
#ifdef _WIN32
    #include <conio.h>
#else
    // conio.h is Microsoft-only. On other platforms wait on a line instead of a keystroke.
    #include <iostream>
    static inline int _getch() { return std::cin.get(); }
#endif


using namespace std;
using namespace ShumiChess;

////////////////////////////////////////////////////////////////////////////////////
// make with: cmake --build C:\programming\shumi-chess\build --config RelWithDebInfo --target run_nnue_training
// run from : C:\programming\shumi-chess\build\bin\RelWithDebInfo

// Alternate entry point: swap the names main and main2 to run this version.
int main2(int argc, char** argv) {
    const filesystem::path repo_root = filesystem::path(__FILE__).parent_path().parent_path();
    const filesystem::path database_path = repo_root / "NNUE" / "FENs" / "1000_FenScores_001.txt";

    // Read the same training examples as main(), once for all runs.
    vector<NNUETrainingExample> examples;
    const auto iMethod = NNUETrainingMethod::STATIC_EVAL;
    string database_error;
    const int read_error = multi_FEN_readin(database_path, examples, iMethod, &database_error);
    if (read_error != 0) {
        sout << database_error << endl;
        _getch();
        return read_error;
    }

    constexpr int TRAINING_ITERATIONS = 100;
    constexpr float learning_rate = 0.001f;

    const int nTrainingPositions = static_cast<int>(examples.size());
    NNUE_data nnue;

    for (int nActiveNeurons = 1; nActiveNeurons <= 31; ++nActiveNeurons) {
        NNUETrainingStats training_stats{};
        NNUE_train_mult_positions(nnue, examples.data(), nTrainingPositions,
                                  nActiveNeurons, TRAINING_ITERATIONS, learning_rate,
                                  false, &training_stats);

        sout << "Neurons=" << nActiveNeurons
             << " Init RMSe=" << training_stats.initial_rmse
             << " END RMSe=" << training_stats.final_rmse
             << " END lck%=" << training_stats.final_lock_percent 
             << " train iter=" << TRAINING_ITERATIONS
             << " learn rate=" << learning_rate
             << endl;
    }

    sout << "Press any key to exit..." << endl;
    _getch();

    return 0;
}

int main(int argc, char** argv) {
 
    // Compose filename of where the training datafile is (FENScores)
    const filesystem::path repo_root = filesystem::path(__FILE__).parent_path().parent_path();
    //const filesystem::path database_path = repo_root / "tests" / "FENs" / "fullGamr001.txt";
    //const filesystem::path database_path = repo_root / "tests" / "FENs" / "fullGamr001_scrambled.txt";
    const filesystem::path database_path = repo_root / "NNUE" / "FENs" / "1000_FenScores_001.txt";

    // Read in datafile into the training examples
    vector<NNUETrainingExample> examples;
    const auto iMethod = NNUETrainingMethod::STATIC_EVAL;
    string database_error;
    const int read_error = multi_FEN_readin(database_path, examples, iMethod, &database_error);
    if (read_error != 0) {
        sout << database_error << endl;
        _getch();
        return read_error;
    }

    constexpr int TRAINING_ITERATIONS = 100;
    constexpr int N_ACTIVE_NEURONS = 28;
    constexpr float learning_rate = 0.001f;

    // Train a NNUE model based on the training examples
    NNUE_data nnue;
    NNUETrainingStats training_stats{};
    const int nTrainingPositions = static_cast<int>(examples.size());
    const auto training_result = NNUE_train_mult_positions(nnue, examples.data(), nTrainingPositions
                                , N_ACTIVE_NEURONS, TRAINING_ITERATIONS, learning_rate
                                , false, &training_stats
                                );

    sout << endl;
    sout << "Activate: " << N_ACTIVE_NEURONS << endl;
    sout << "Initial RMSe: " << training_stats.initial_rmse << " cp" << endl;
    sout << "Final RMSe=" << training_stats.final_rmse
         << " nlocks%=" << training_stats.final_lock_percent << endl;

    // Write out the NNUE model to a file
    const filesystem::path parameters_path = repo_root / NNUE_model_file;
    if (nnue.save_parameters(parameters_path, database_path, N_ACTIVE_NEURONS,
                             TRAINING_ITERATIONS, learning_rate, training_result)) {
        //sout << "Saved NNUE parameters to " << parameters_path << endl;
    } else {
        sout << "Failed to save NNUE parameters to " << parameters_path << endl;
    }

    sout << "Press any key to exit..." << endl;
    _getch();

    return 0;
}

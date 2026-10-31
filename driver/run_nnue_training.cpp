#include <cstdlib>
#include <filesystem>
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


using namespace std;
using namespace ShumiChess;

////////////////////////////////////////////////////////////////////////////////////
// make with: cmake --build C:\programming\shumi-chess\build --config RelWithDebInfo --target run_nnue_training
// run from : C:\programming\shumi-chess\build\bin\RelWithDebInfo

int main(int argc, char** argv) {
 
    // Compose filename of where the datafile is
    const filesystem::path repo_root = filesystem::path(__FILE__).parent_path().parent_path();
    //const filesystem::path database_path = repo_root / "tests" / "FENs" / "fullGamr001.txt";
    const filesystem::path database_path = repo_root / "tests" / "FENs" / "fullGamr001_scrambled.txt";
    //const filesystem::path database_path = repo_root / "tests" / "FENs" / "six_trainings_positions.txt";

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

    // Train a nnue based on the training examples
    NNUE_data nnue;
    const int NPositions = static_cast<int>(examples.size());
    test_NNUE_training_mult_positions(nnue, examples.data(), NPositions);

    // Write it out to a file
    const filesystem::path parameters_path = repo_root / "tests" / "FENs" / "ShumiNNUE.txt";
    if (nnue.save_parameters(parameters_path)) {
        sout << "Saved NNUE parameters to " << parameters_path << endl;
    } else {
        sout << "Failed to save NNUE parameters to " << parameters_path << endl;
    }

    sout << "Press any key to exit..." << endl;
    _getch();

    return 0;
}

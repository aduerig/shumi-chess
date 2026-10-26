#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "engine.hpp"
#include "fen_database.hpp"
#include "minimax.hpp"
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
    const filesystem::path database_path = repo_root / "FEN" / "run_nnue_training.txt";

    // Read in postitions from FEN database
    vector<FEN_Score> positions;
    if (!read_fen_database(database_path.string(), positions)) {
        return EXIT_FAILURE;
    }
    if (positions.empty()) {
        sout << database_path.string() << ": no usable FEN positions" << endl;
        return EXIT_FAILURE;
    }
    // The existing training routine accepts an int position count.
    if (positions.size() > static_cast<size_t>(numeric_limits<int>::max())) {
        sout << database_path.string()
             << ": position count exceeds the training routine's int range"
             << endl;
        return EXIT_FAILURE;
    }

    // Insures all positions as input, have no attached score
    const int NPositions = static_cast<int>(positions.size());
    for (int i = 0; i < NPositions; ++i) {
        if (positions[i].score != HUGE_SCORE) {
            sout << database_path.string() << ": record " << (i + 1)
                 << " has a numerical score; expected '*' for NNUE training"
                 << endl;
            return EXIT_FAILURE;
        }
    }

    vector<NNUETrainingExample> examples(NPositions);
    NNUE_data nnue;

    for (int iPositions = (NPositions-1); iPositions >=0 ; --iPositions) {

        sout << "Position index: " << iPositions << endl;

        Engine engine(positions[iPositions].fen);
        MinimaxAI minimax_ai(engine);

        // Get the chessboard inputs for this position.
        nnue.fill_nnue_inputs_fast(engine.game_board, examples[iPositions].inputs);

        // Get Shumi's target positional score for this position.
        int material_score_cp;
        int positional_score_cp;

        minimax_ai.evaluate_board_t<Color::WHITE>(
            UNCLE_SHUMI, &material_score_cp, &positional_score_cp);

        examples[iPositions].target_positional_score_cp = (float)positional_score_cp;

    }   // End loop over all starting positions


    // Each call initializes and trains a fresh NNUE for this position.
   // test_NNUE_training_one_position(&minimax_ai);

    test_NNUE_training_mult_positions(nnue, examples.data(), NPositions);






    sout << "Press any key to exit..." << endl;
    _getch();

    return 0;
}

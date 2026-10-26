#include <iostream>
#include <string>

#include "engine.hpp"
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

    // Setup the starting positions.
    constexpr int MAX_FENS = 10;
    string FENs[MAX_FENS];

    #define OPENING_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"


    //FENs[0] = "rnbqk2r/ppp2ppp/3b4/3p4/3Pn3/2PB1N2/PP3PPP/RNBQK2R w KQkq - 1 8";        // Petrov
    FENs[0] = OPENING_FEN;

    FENs[1] = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2BPP3/2P2N2/PP3PPP/RNBQK2R b KQkq d3 0 5";  // Giuco Piano
    FENs[2] = "2k4r/1p3Rpp/p1p5/2p1p3/4P2P/3rP3/NPP5/2K2R2 w - - 0 20";                 // random middlegame
    FENs[3] = "2b2rrk/1p5p/pnp1Rp1p/8/3P4/PNP2B1P/1P3PP1/2K1R3 w - - 1 30";             // random middlegame
    //FENs[4] = "8/p4qpk/p1p4p/4n3/1P2P2P/1R2Q1Pb/3r1P2/4R1K1 b - - 6 42";                             // random endgame
    FENs[4] = "r1b2rk1/pp3ppp/1qp1pn2/8/2QP4/2N1PN2/PP3PPP/1R3RK1 b - - 2 14";     // middlegame from QP opening

    int NPositions = 5;



    NNUETrainingExample examples[MAX_FENS]{};
    NNUE_data nnue;

    for (int iPositions = (NPositions-1); iPositions >=0 ; --iPositions) {

        sout << "Position index: " << iPositions << endl;

        Engine engine(FENs[iPositions]);
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

    test_NNUE_training_mult_positions(nnue, examples, NPositions);






    sout << "Press any key to exit..." << endl;
    _getch();

    return 0;
}

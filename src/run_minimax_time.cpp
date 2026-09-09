
#include <math.h>

#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#include <conio.h>
#else
// conio.h is Microsoft-only. On other platforms wait on a line instead of a keystroke.
#include <iostream>
static inline int _getch() { return std::cin.get(); }
#endif
#include <cstdio>
#include <iostream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <thread>

#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
    #undef NDEBUG
#endif
#include <assert.h>


#include "engine.hpp"
#include "globals.hpp"
#include "utility.hpp"
#include "minimax.hpp"
#include "status_output.hpp"

using namespace std;
using namespace ShumiChess;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////////
// make with: cmake --build C:\programming\shumi-chess\build --config RelWithDebInfo --target run_minimax_time --clean-first -v
// run from : C:\programming\shumi-chess\build\bin\RelWithDebInfo

static const char* game_state_to_string(GameState state);
static string move_to_uci(const Move& move);
static long long elapsed_time_msec(steady_clock::time_point start_time, steady_clock::time_point end_time);
static void make_engine_move(Engine& engine, Move move);
static ofstream open_minimax_results_file(filesystem::path& opened_path);

class TeeStreamBuf : public streambuf
{
public:
    TeeStreamBuf(streambuf* first, streambuf* second) : first(first), second(second) {}

protected:
    int overflow(int ch) override
    {
        if (ch == traits_type::eof()) {
            return traits_type::not_eof(ch);
        }

        const int first_result = first->sputc(static_cast<char>(ch));
        const int second_result = second ? second->sputc(static_cast<char>(ch)) : ch;
        return (first_result == traits_type::eof() || second_result == traits_type::eof())
            ? traits_type::eof()
            : ch;
    }

    int sync() override
    {
        const int first_result = first->pubsync();
        const int second_result = second ? second->pubsync() : 0;
        return (first_result == 0 && second_result == 0) ? 0 : -1;
    }

private:
    streambuf* first;
    streambuf* second;
};


/////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

    assert(true);       // Keeps assert compilation visible without aborting this runner.

    /////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // This is just to measure speed, and number of nodes ... over a fixed set of chess positrions.
    // Look at a fixed number of starting positions, searching a given number of ply. 
    // The difficulty here is that this approach underrepresents endsgames.
    //
    /////////////////////////////////////////////////////////////////////////////////////////////////

    // Make board
    //string FENString = "r2qnrk1/1p2ppbp/p5p1/2p1N3/b1B5/1PN5/1B1P1PPP/R1R1Q1K1 w - - 0 14";


    // Setup the starting positions. We will sample a given number of ply from each starting position
    constexpr int MAX_FENS = 10;
    string FENs[MAX_FENS];
    string PGNs[MAX_FENS];

    FENs[0] = "rnbqk2r/ppp2ppp/3b4/3p4/3Pn3/2PB1N2/PP3PPP/RNBQK2R w KQkq - 1 8";        // Petrov
    FENs[1] = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2BPP3/2P2N2/PP3PPP/RNBQK2R b KQkq d3 0 5";  // Giuco Piano
    FENs[2] = "2k4r/1p3Rpp/p1p5/2p1p3/4P2P/3rP3/NPP5/2K2R2 w - - 0 20";                 // random middlegame
    FENs[3] = "2b2rrk/1p5p/pnp1Rp1p/8/3P4/PNP2B1P/1P3PP1/2K1R3 w - - 1 30";             // random middlegame

    FENs[4] = "r1b2rk1/pp3ppp/1qp1pn2/8/2QP4/2N1PN2/PP3PPP/1R3RK1 b - - 2 14";     // middlegame from QP opening
    FENs[5] = "8/p4qpk/p1p4p/4n3/1P2P2P/1R2Q1Pb/3r1P2/4R1K1 b - - 6 42";                             // random endgame

    int NPositions = 6;
    int max_ply_to_play = 14;    // measured from the start of each starting position


    // Deterime the "time arguments" to the search
    int depth_to_use = 7;       // We will look this many deepenings for each move.

    // Milliseconds. The purpose of this low value, is to make us go only (and exactly) a fixed number of deepenings 
    ull time_to_use = 2;      

    int player_id = UNCLE_SHUMI;       //  UNCLE_SHUMI;

    // if (argc < 2) {
    //     //sout << "You entered no argument for 'time_to_use', using default value of " << time_to_use << "msec" << endl;
    // } else {
    //     const long long parsed_time_msec = atoll(argv[1]);
    //     if (parsed_time_msec > 0) {
    //         time_to_use = (ull)parsed_time_msec;
    //     }
    //     //sout << "You entered time_to_use of: " << time_to_use << endl;
    // }
    // if (argc >= 3) {
    //     depth_to_use = atoi(argv[2]);
    // }1
    // if (argc >= 4) {
    //     max_ply_to_play = atoi(argv[3]);
    // }

    int flags = _DEFAULT_FEATURES_MASK;

    // Since there is no time control there is no hard abort.
    flags &= ~_FEATURE_SOFT_ABORT;      // remove soft abort

    sout << "uzing level= " << depth_to_use
         << "  msec = " << time_to_use
         << "  max ply = " << max_ply_to_play 
         << "  play id = " << player_id
         << "  FEAT = 0x" << hex << flags << dec
         << endl;

    ////////////////////////////////////////////////////////////////////////////////////

    steady_clock::time_point start_time = steady_clock::now();

    GameState state;

    long long totalNodesSum = 0;
    ull totalNodesPerMove=0;

    long long total_elapsed_time=0; 

    for (int iPositions=0; iPositions<NPositions; iPositions++) {

        // Make engine
        Engine engine(FENs[iPositions]);
        //Engine engine;

        //std::this_thread::sleep_for(std::chrono::seconds(3));   // debug only

        MinimaxAI minimax_ai(engine);

        // Show board
        string out = utility::representation::gameboard_to_string(engine.game_board);
        sout << out << endl;


        state = engine.is_game_over();
        //
        // Loop over ply
        //
        assert (max_ply_to_play > 0);
        for (int ply = 1; ( (state == INPROGRESS) && (ply <= max_ply_to_play)); ++ply) {
           
            int iRandomMoves = 0;
            Move move = minimax_ai.get_move_iterative_deepening(time_to_use, depth_to_use, player_id
                                                            , iRandomMoves, flags);

            if (move.piece_type == Piece::NONE) {
                sout << "No legal move returned at ply " << ply << endl;
                break;
            }

            make_engine_move(engine, move);

            // Show board
            // out = utility::representation::gameboard_to_string(engine.game_board);
            // sout << out << endl;

            // Sum total number of nodes used in move search
            totalNodesSum += minimax_ai.nodes_visited;
            totalNodesPerMove++;
            //sout << "nodes=" << (totalNodesSum/totalNodesPerMove) << endl;


            state = engine.is_game_over();
        }

        //sout << "Gammme state: " << game_state_to_string(state) << endl;
        PGNs[iPositions] = engine.gamePGN.spitout();
        sout << "PGN: " << PGNs[iPositions] << endl;
        steady_clock::time_point end_time = steady_clock::now();
        total_elapsed_time = elapsed_time_msec(start_time, end_time);
        sout << iPositions << "  Elapsed time: " << total_elapsed_time << " msec" << endl;

    }   // End loop over all starting positions

    //
    // Show results
    //
    filesystem::path minimax_results_path;
    ofstream minimax_results_file = open_minimax_results_file(minimax_results_path);
    if (!minimax_results_file.is_open()) {
        sout << "Could not open doc\\minimax_out.txt for writing; results will only be printed to sout." << endl;
    } else if (minimax_results_path.filename() != "minimax_out.txt") {
        sout << "doc\\minimax_out.txt was not writable; writing results to "
             << minimax_results_path.string() << endl;
    }

    TeeStreamBuf results_buf(sout.rdbuf(), minimax_results_file.is_open() ? minimax_results_file.rdbuf() : nullptr);
    ostream results(&results_buf);

    results << endl;
    results <<" ep=" << total_elapsed_time <<" nd=" << (totalNodesSum/totalNodesPerMove) << endl;

    results << "flags=0x" << std::hex << flags << std::dec << endl;
    for (int i=0;i<NPositions;i++) {
        results << PGNs[i] << endl;
    }
    assert (totalNodesPerMove > 0);


    sout << "Press any keeeeeeey to exit..." << endl;
    _getch();

    return 0;
}

// codex resume 01a07fb8-ab90-7fc2-98ba-9b09335a1803

/////////////////////////////////////////////////////////////////////////////

static ofstream open_minimax_results_file(filesystem::path& opened_path)
{
    const filesystem::path repo_root = filesystem::path(__FILE__).parent_path().parent_path();
    const filesystem::path doc_dir = repo_root / "doc";
    error_code ignored_error;
    filesystem::create_directories(doc_dir, ignored_error);

    opened_path = doc_dir / "minimax_out.txt";
    ofstream file;
    file.open(opened_path, ios::out | ios::trunc);
    if (file.is_open()) {
        return file;
    }
    file.clear();

    for (int i = 1; i <= 99; ++i) {
        opened_path = doc_dir / ("minimax_out (" + to_string(i) + ").txt");
        file.open(opened_path, ios::out | ios::trunc);
        if (file.is_open()) {
            return file;
        }
        file.clear();
    }

    opened_path.clear();
    return file;
}


static const char* game_state_to_string(GameState state)
{
    switch (state) {
        case INPROGRESS: return "in progress";
        case WHITEWIN:   return "white wins";
        case BLACKWIN:   return "black wins";
        case DRAW:       return "draw";
        default:         return "unknown";
    }
}

static string move_to_uci(const Move& move)
{
    string move_text = utility::representation::move_to_string(move);
    char promo = utility::representation::piece_to_charactor(move.promotion);
    if (promo != ' ') {
        move_text += promo;
    }
    return move_text;
}

static long long elapsed_time_msec(steady_clock::time_point start_time, steady_clock::time_point end_time)
{
    return duration_cast<milliseconds>(end_time - start_time).count();
}

static void make_engine_move(Engine& engine, Move move)
{
    engine.users_last_move = move;
    engine.ply_so_far++;

    engine.gamePGN.addMe(move, engine);

    engine.move_history = stack<Move>();

    if (move.piece_type == Piece::NONE) {
        sout << "\x1b[1;31mNo move to make\x1b[0m" << endl;
        return;
    }

    // Make the move
    if (move.color == Color::WHITE) {
        engine.pushMove_t<Color::WHITE>(move);
    } else {
        engine.pushMove_t<Color::BLACK>(move);
    }

    // Manage tree time repetition
    engine.push_to_three_time_rep_stack(move);
   
}

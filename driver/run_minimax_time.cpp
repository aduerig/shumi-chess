
#include <math.h>

#include <cstdlib>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
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
#include <tuple>
#include <vector>

#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
    #undef NDEBUG
#endif
#include <assert.h>


#include "engine.hpp"
#include "fen_database.hpp"
#include "globals.hpp"
#include "utility.hpp"
#include "minimax.hpp"
#include "status_output.hpp"

#ifndef NDEBUG
#error "run_minimax_time must be compiled with NDEBUG so assert() checks are disabled."
#endif

using namespace std;
using namespace ShumiChess;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////////
// make with: cmake --build C:\programming\shumi-chess\build --config RelWithDebInfo --target run_minimax_time --clean-first -v
// run from : C:\programming\shumi-chess\build\bin\RelWithDebInfo

static const char* game_state_to_string(GameState state);
static string move_to_uci(const Move& move);
static string pgn_with_fen_move_numbers(const string& pgn, const string& fen);
static string add_score_comments_to_pgn(const string& numbered_pgn, const vector<double>& move_scores);
static string wrap_pgn_line(const string& pgn);
static long long elapsed_time_msec(steady_clock::time_point start_time, steady_clock::time_point end_time);
static void make_engine_move(Engine& engine, Move move);
static ofstream open_minimax_results_file(const filesystem::path& results_dir, const string& filename);

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
//      // Sample a given number of ply from each database starting position.
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

    // Hardcoded input and results locations.

    // Input data file
    const filesystem::path repo_root = filesystem::path(__FILE__).parent_path().parent_path();
    const filesystem::path database_path = repo_root / "tests" / "FENs" / "run_minimax_time.txt";

    // Output data file
    const filesystem::path results_dir = repo_root / "tests" / "data";
 
    // Read in positions from FEN database
    vector<FEN_Score> positions;
    string database_error;
    if (!read_fen_database(database_path.string(), positions, &database_error)) {
        sout << database_error << endl;
        _getch();
        return EXIT_FAILURE;
    }
    if (positions.empty()) {
        sout << database_path.string() << ": no usable FEN positions" << endl;
        _getch();
        return EXIT_FAILURE;
    }

    const size_t NPositions = positions.size();
    vector<string> PGNs(NPositions);
    vector<ull> positionNodes(NPositions);
    vector<int> positionPlyPlayed(NPositions);
    vector<GameState> positionFinalStates(NPositions);
    vector<vector<double>> positionMoveScores(NPositions);

    int max_ply_to_play = 20;    // measured from the start of each starting position


    // Deterime the "time arguments" to the search
    int depth_to_use = 7;       // We will look this many deepenings for each move.

    // Milliseconds. The purpose of this low value, is to make us go only (and exactly) a fixed number of deepenings 
    ull time_to_use = 2;      

    int player_id = UNCLE_SHUMI;       //  UNCLE_SHUMI;
    bool display_results_to_terminal = false;

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
    // here we remove the soft abort also, since we are depth-fixed here.
    flags &= ~_FEATURE_SOFT_ABORT;      // remove soft abort

    sout << "uzzzzzzzzzzzzzzzzzzzzing level= " << depth_to_use
         << "  msec = " << time_to_use
         << "  max ply = " << max_ply_to_play 
         << "  play id = " << player_id
         << "  FEAT = 0x" << hex << flags << dec
         << endl;

    ////////////////////////////////////////////////////////////////////////////////////

    steady_clock::time_point start_time = steady_clock::now();

    GameState state;

    ull totalNodesSum = 0;
    ull totalNodesPerMove=0;

    long long total_elapsed_time=0; 

    for (size_t iPositions=0; iPositions<NPositions; iPositions++) {

        // Make engine
        Engine engine(positions[iPositions].fen);
        //Engine engine;

        //std::this_thread::sleep_for(std::chrono::seconds(3));   // debug only

        MinimaxAI minimax_ai(engine);

        // Show board
        string out = utility::representation::gameboard_to_string(engine.game_board);
        sout << out << endl;


        state = engine.is_game_over();
        ull nodes_before_position = totalNodesSum;
        //
        // Loop over ply
        //
        assert (max_ply_to_play > 0);
        for (int ply = 1; ( (state == INPROGRESS) && (ply <= max_ply_to_play)); ++ply) {
           
            int iRandomMoves = 0;
            tuple<Score, Move> ret_val = minimax_ai.get_move_iterative_deepening(time_to_use, depth_to_use, player_id
                                                            , iRandomMoves, flags);
            Score move_score = get<0>(ret_val);
            Move move = get<1>(ret_val);

            if (move.piece_type == Piece::NONE) {
                sout << "No legal move returned at ply " << ply << endl;
                break;
            }

            // Shumi scores are stored side-to-move relative.
            double score_pawns = (double)move_score / (double)ONE_PAWN;
            positionMoveScores[iPositions].push_back(score_pawns);

            make_engine_move(engine, move);

            // Show board
            // out = utility::representation::gameboard_to_string(engine.game_board);
            // sout << out << endl;

            // Sum total number of nodes used in move search
            totalNodesSum += minimax_ai.nodes_visited;
            totalNodesPerMove++;
            positionPlyPlayed[iPositions] = ply;
            //sout << "nodes=" << (totalNodesSum/totalNodesPerMove) << endl;


            state = engine.is_game_over();
        }

        //sout << "Gammme state: " << game_state_to_string(state) << endl;
        positionNodes[iPositions] = totalNodesSum - nodes_before_position;
        positionFinalStates[iPositions] = state;

        PGNs[iPositions] = engine.gamePGN.spitout();
        sout << "PGN: " << PGNs[iPositions] << endl;

        steady_clock::time_point end_time = steady_clock::now();
        total_elapsed_time = elapsed_time_msec(start_time, end_time);
        sout << iPositions << "  Elapsed time: " << total_elapsed_time << " msec" << endl;

    }   // End loop over all starting positions

    //
    // Show results
    //
    ofstream with_scores_file = open_minimax_results_file(results_dir, "with_scores.txt");
    if (!with_scores_file.is_open()) {
        sout << "Could not open " << (results_dir / "with_scores.txt").string()
             << " for writing; results will only be printed to sout." << endl;
    }

    ofstream without_scores_file = open_minimax_results_file(results_dir, "without_scores.txt");
    if (!without_scores_file.is_open()) {
        sout << "Could not open " << (results_dir / "without_scores.txt").string()
             << " for writing." << endl;
    }

    TeeStreamBuf results_buf(with_scores_file.is_open() ? with_scores_file.rdbuf() : sout.rdbuf(),
                             display_results_to_terminal && with_scores_file.is_open() ? sout.rdbuf() : nullptr);
    ostream results(&results_buf);

    results << endl;
    if (without_scores_file.is_open()) {
        without_scores_file << endl;
    }
    results << "settings"
            << " depth=" << depth_to_use
            << " max_ply=" << max_ply_to_play
            << " positions=" << NPositions
            << " flags=0x" << std::hex << flags << std::dec
            << " player=" << player_id
            << " time_ms=" << time_to_use
            << endl;
    if (without_scores_file.is_open()) {
        without_scores_file << "settings"
                            << " depth=" << depth_to_use
                            << " max_ply=" << max_ply_to_play
                            << " positions=" << NPositions
                            << " flags=0x" << std::hex << flags << std::dec
                            << " player=" << player_id
                            << " time_ms=" << time_to_use
                            << endl;
    }
    results << "summary"
            << " ep=" << total_elapsed_time
            << " nd=" << totalNodesSum
            << " nd_avg=" << (totalNodesSum/totalNodesPerMove)
            << " moves=" << totalNodesPerMove
            << endl;
    results << endl;
    if (without_scores_file.is_open()) {
        without_scores_file << "summary"
                            << " ep=" << total_elapsed_time
                            << " nd=" << totalNodesSum
                            << " nd_avg=" << (totalNodesSum/totalNodesPerMove)
                            << " moves=" << totalNodesPerMove
                            << endl;
        without_scores_file << endl;
    }

    for (size_t i=0;i<NPositions;i++) {
        results << "[Event \"Shumi minimax regression pos " << i << "\"]" << endl;
        results << "[SetUp \"1\"]" << endl;
        results << "[FEN \"" << positions[i].fen << "\"]" << endl;
        results << "[Result \"*\"]" << endl;
        results << endl;
        string numbered_pgn =
            pgn_with_fen_move_numbers(PGNs[i], positions[i].fen);
        string scored_pgn = add_score_comments_to_pgn(numbered_pgn, positionMoveScores[i]);
        results << wrap_pgn_line(scored_pgn) << endl;
        results << endl;

        if (without_scores_file.is_open()) {
            without_scores_file << "[Event \"Shumi minimax regression pos " << i << "\"]" << endl;
            without_scores_file << "[SetUp \"1\"]" << endl;
            without_scores_file << "[FEN \"" << positions[i].fen
                                << "\"]" << endl;
            without_scores_file << "[Result \"*\"]" << endl;
            without_scores_file << endl;
            without_scores_file << wrap_pgn_line(numbered_pgn) << endl;
            without_scores_file << endl;
        }
    }
    assert (totalNodesPerMove > 0);


    sout << "Press any keeeeeeey to exit..." << endl;
    _getch();

    return 0;
}


/////////////////////////////////////////////////////////////////////////////

static ofstream open_minimax_results_file(const filesystem::path& results_dir, const string& filename)
{
    error_code ignored_error;
    filesystem::create_directories(results_dir, ignored_error);

    const filesystem::path requested_path = results_dir / filename;
    
    ofstream file;
    file.open(requested_path, ios::out | ios::trunc);
    if (file.is_open()) {
        return file;
    }
    file.clear();

    for (int i = 1; i <= 99; ++i) {
        const filesystem::path fallback_filename = filesystem::path(filename).stem().string()
            + " (" + to_string(i) + ")"
            + filesystem::path(filename).extension().string();
        const filesystem::path fallback_path = results_dir / fallback_filename;
        file.open(fallback_path, ios::out | ios::trunc);
        if (file.is_open()) {
            sout << requested_path.string() << " was not writable; writing results to "
                 << fallback_path.string() << endl;
            return file;
        }
        file.clear();
    }

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

static string pgn_with_fen_move_numbers(const string& pgn, const string& fen)
{
    string board_part;
    string side_to_move;
    string castling;
    string en_passant;
    int halfmove = 0;
    int fullmove = 1;

    stringstream fen_stream(fen);
    fen_stream >> board_part >> side_to_move >> castling >> en_passant >> halfmove >> fullmove;

    vector<string> moves;
    string token;
    stringstream pgn_stream(pgn);
    while (pgn_stream >> token) {
        if (token == "*" || token == "1-0" || token == "0-1" || token == "1/2-1/2") {
            continue;
        }

        bool is_move_number = true;
        bool has_digit = false;
        for (char ch : token) {
            const unsigned char uch = static_cast<unsigned char>(ch);
            if (isdigit(uch)) {
                has_digit = true;
            } else if (ch != '.') {
                is_move_number = false;
                break;
            }
        }

        if (has_digit && is_move_number) {
            continue;
        }

        moves.push_back(token);
    }

    const bool starts_with_white = (side_to_move != "b");
    bool white_to_move = starts_with_white;
    int move_number = fullmove;

    string out;
    for (size_t i = 0; i < moves.size(); ++i) {
        if (!out.empty()) {
            out += " ";
        }

        if (white_to_move) {
            out += to_string(move_number);
            out += ". ";
        } else if (i == 0) {
            out += to_string(move_number);
            out += "... ";
        }

        out += moves[i];

        if (!white_to_move) {
            ++move_number;
        }
        white_to_move = !white_to_move;
    }

    if (!out.empty()) {
        out += " ";
    }
    out += "*";
    return out;
}

static string add_score_comments_to_pgn(const string& numbered_pgn, const vector<double>& move_scores)
{
    string out;
    string token;
    stringstream stream(numbered_pgn);
    size_t score_index = 0;

    while (stream >> token) {
        if (!out.empty()) {
            out += " ";
        }
        out += token;

        const bool is_result = (token == "*" || token == "1-0" || token == "0-1" || token == "1/2-1/2");
        bool is_move_number = true;
        bool has_digit = false;
        for (char ch : token) {
            const unsigned char uch = static_cast<unsigned char>(ch);
            if (isdigit(uch)) {
                has_digit = true;
            } else if (ch != '.') {
                is_move_number = false;
                break;
            }
        }

        if (!is_result && !(has_digit && is_move_number)) {
            assert(score_index < move_scores.size());
            ostringstream score_text;
            score_text << fixed << setprecision(2) << move_scores.at(score_index);
            out += " {";
            out += score_text.str();
            out += "}";
            ++score_index;
        }
    }

    assert(score_index == move_scores.size());
    return out;
}

static string wrap_pgn_line(const string& pgn)
{
    constexpr size_t MAX_LINE_LENGTH = 78;

    string wrapped;
    string line;
    string token;
    stringstream stream(pgn);

    while (stream >> token) {
        const size_t next_length = line.empty()
            ? token.size()
            : line.size() + 1 + token.size();

        if (!line.empty() && next_length > MAX_LINE_LENGTH) {
            if (!wrapped.empty()) {
                wrapped += '\n';
            }
            wrapped += line;
            line = token;
        } else {
            if (!line.empty()) {
                line += ' ';
            }
            line += token;
        }
    }

    if (!line.empty()) {
        if (!wrapped.empty()) {
            wrapped += '\n';
        }
        wrapped += line;
    }

    return wrapped;
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

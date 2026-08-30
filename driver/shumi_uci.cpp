
#include <math.h>
#include <cstdlib>

#include <atomic>
#include <chrono>
#ifdef _WIN32
    #include <conio.h>
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <errno.h>
    #include <sys/select.h>
    #include <unistd.h>
#endif

#include <cstdio>
//#include <deque>
#include <iostream>
#include <ostream>
#include <sstream>
#include <thread>
#include <vector>

#define SHUMI_FORCE_ASSERTS
#ifdef SHUMI_FORCE_ASSERTS
    #undef NDEBUG
#endif
#include <assert.h>

#include <engine.hpp>
#include <globals.hpp>
#include <utility.hpp>
#include <fstream>

#include "minimax.hpp"
#include "status_output.hpp"


using namespace std;
using namespace ShumiChess;
using namespace std::chrono;

//////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////


static void make_engine_move(Engine& engine, Move move);
static string move_to_uci(const Move& move);
static bool make_uci_move(Engine& engine, const string& move_uci);
static bool is_prefix(const vector<string>& old_moves, const vector<string>& new_moves);
static bool parse_position_command(const string& line, string& new_base, vector<string>& new_moves);
static bool create_position(const string& base,
                            const vector<string>& moves,
                            Engine*& engine,
                            MinimaxAI*& minimax_ai);
static bool try_read_uci_line(std::string& line, bool& input_closed);

struct UciSearchState {
    std::atomic<bool> running{false};
    std::atomic<bool> done{false};
    std::thread thread;
    Move move;
    ull go_id = 0;
};

static void start_searching_for_move(MinimaxAI& minimax_ai,
                                     UciSearchState& search_thread,
                                     ull go_id,
                                     ull search_time_to_use,
                                     int depth_to_use,
                                     int player_id,
                                     int iRandomMoves,
                                     int flags,
                                     MinimaxAI::SearchTimeControl time_control);
static void found_move(Engine& engine,
                       MinimaxAI& minimax_ai,
                       UciSearchState& search_thread,
                       vector<string>& moves_so_far,
                       int& iMovesInGame);


///////////////////////////////////////////////////////////////////////////////////////////////////


static std::ofstream sout_file;

int main()
{


    // open assert debug file
    FILE* assertion_log = nullptr;

    freopen_s(
        &assertion_log,
        "C:\\programming\\shumi-chess\\uci_assert.txt",
        "a",
        stderr
    );

    if (assertion_log != nullptr) {
        // Make assertion diagnostics appear immediately.
        setvbuf(stderr, nullptr, _IONBF, 0);
    }

    #ifdef _WIN32
        _set_error_mode(_OUT_TO_STDERR);
    #endif




    // open debug file
    sout_file.open(
        "C:\\programming\\shumi-chess\\uci_debug.txt",
        std::ios::out | std::ios::trunc
    );

    if (!sout_file.is_open()) {
        std::cerr << "Could not open uci_debug.txt\n";
        return 1;
    }

    // For this UCI executable, redirect all Shumi status output to the file.
    sout.rdbuf(sout_file.rdbuf());

    // Send cerr output to the same debug file.
    std::cerr.rdbuf(sout_file.rdbuf());


    sout << "STARTING main()" << endl;


    int iMovesInGame = 0;

    // 
    // Decide on Shumi engine chess arguments
    //     7, 17000 is about 40 moves in 5 minumtes
    //
    int depth_to_use = 5;           // A minimum. Not  too large or SHumi loses in tuime control
   
    ull nominal_time_per_move[2] = {0, 0};
    int previous_moves_to_go[2] = {0, 0};
    //int max_ply_to_play = 4;
    int player_id = UNCLE_SHUMI;       //  UNCLE_SHUMI;
    int flags = 0;
    flags = flags | _FEATURE_KILLER | _FEATURE_UNQUIET_SORT;
    flags = flags | _FEATURE_TT2;

    int iRandomMoves = 0;
    if (iMovesInGame < 3) iRandomMoves = 1;     // Just one random move.



    constexpr int MAX_FENS = 10;
    string FENs[MAX_FENS];


    FENs[0] = "rnbqk2r/ppp2ppp/3b4/3p4/3Pn3/2PB1N2/PP3PPP/RNBQK2R w KQkq - 1 8";        // Random Petrov
    FENs[1] = "r1bqk2r/pppp1ppp/2n2n2/2b1p3/2BPP3/2P2N2/PP3PPP/RNBQK2R b KQkq d3 0 5";  // Giaco
    int iPositions = 0;

    Engine* engine = nullptr;
    MinimaxAI* minimax_ai = nullptr;
    bool have_position = false;
    std::string current_base;
    std::vector<std::string> moves_so_far;

    std::string line;
    //std::deque<std::string> pending_lines;
    bool input_closed = false;
    UciSearchState search_thread;

    ull current_go_id = 0;

    while (true) {
        // Keep reading UCI commands until stdin is closed.
        //const bool uci_input_may_arrive = !input_closed;

        // Keep the main thread alive while the search thread may still publish a best move.
        const bool search_thread_is_running = search_thread.running.load(std::memory_order_acquire);

        // This loop should only run if either there is UCI input to process, or the search_thread is still busy. 
        if (input_closed && !search_thread_is_running) {
            break;  // exit the forever loop
        }

        const bool search_thread_is_done = search_thread.done.load(std::memory_order_acquire);
        if (search_thread_is_done) {
            found_move(*engine, *minimax_ai, search_thread, moves_so_far, iMovesInGame);
        }

        if (!try_read_uci_line(line, input_closed)) {
            // Nothing is ready: avoid a tight CPU spin while polling stdin
            // and waiting for search completion.
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // Echo the recieved UCI line to debug log
        sout << line << endl;

        //
        // other commands?
        // stop
        // quit
        // ponderhit

       if (search_thread.running.load(std::memory_order_acquire)
            && line != "isready"
            && line != "stop"
            && line != "quit") {

            sout << "UCI ERROR: unexpected command received while searching: "
                << line
                << endl;

            // Ignore the command. Do not allow the main thread to alter the
            // board while the search thread is using it.
            continue;
        }

        if (line == "uci") {
        //************************************************************************************** */
            std::cout << "id name ShumiChess\n";
            std::cout << "id author Paul Duerig\n";
            std::cout << "uciok\n";
            std::cout.flush();

        } else if (line == "isready") {
        //************************************************************************************** */
            std::cout << "readyok\n";
            std::cout.flush();

        } else if (line == "ucinewgame") {
        //************************************************************************************** */
            // clear TT, repetition table, history, etc.
            current_base.clear();
            moves_so_far.clear();
            have_position = false;
            nominal_time_per_move[0] = 0;
            nominal_time_per_move[1] = 0;
            previous_moves_to_go[0] = 0;
            previous_moves_to_go[1] = 0;

        } else if (line.rfind("position ", 0) == 0) {
        //************************************************************************************** */
            // set board from "startpos" or "fen"
            // then play the listed moves

            // position startpos
            // position startpos moves e2e4 e7e5 g1f3
            // position fen <FEN>
            // position fen <FEN> moves e2e4 e7e5

            string new_base;
            vector<string> new_moves;

            if (!parse_position_command(line, new_base, new_moves)) {
                sout << "Invalid position command: " << line << endl;
                continue;
            }

            bool position_updated = false;

            if (!have_position || engine == nullptr || minimax_ai == nullptr) {

                iMovesInGame = 0;
                position_updated = create_position(new_base, new_moves, engine, minimax_ai);

            } else if (new_base == current_base && is_prefix(moves_so_far, new_moves)) {

                position_updated = true;

                for (size_t i = moves_so_far.size(); i < new_moves.size(); i++) {
                    if (!make_uci_move(*engine, new_moves[i])) {
                        position_updated = false;
                        break;
                    }
                }

                if (!position_updated) {
                    iMovesInGame = 0;
                    position_updated = create_position(new_base, new_moves, engine, minimax_ai);
                }
            } else {
                iMovesInGame = 0;
                position_updated = create_position(new_base, new_moves, engine, minimax_ai);
            }

            if (!position_updated) {
                sout << "Could not apply position command: " << line << endl;
                continue;
            }

            current_base = new_base;
            moves_so_far = new_moves;
            have_position = true;


        } else if (line == "go" || line.rfind("go ", 0) == 0) {

            ull this_go_id = current_go_id;
            current_go_id++;

            if (!have_position || engine == nullptr || minimax_ai == nullptr) {
                // No valid position exists yet, so create the standard starting position.

                vector<string> no_moves;

                // Create a starting position
                if (!create_position("startpos", no_moves, engine, minimax_ai)) {
                    std::cout << "bestmove 0000\n";
                    std::cout.flush();
                    continue;
                }

                current_base = "startpos";
                moves_so_far.clear();
                have_position = true;
            }

            // prepare arguments to the call
            long long white_time = -1;
            long long black_time = -1;
            long long move_time = -1;
            int moves_to_go = 0;

            // parse the "go" line
            istringstream go_command(line);
            string go_token;
            go_command >> go_token; // "go"
            while (go_command >> go_token) {
                if (go_token == "wtime") go_command >> white_time;
                else if (go_token == "btime") go_command >> black_time;
                else if (go_token == "movetime") go_command >> move_time;
                else if (go_token == "movestogo") go_command >> moves_to_go;
            }


            // Assign these
            ull search_time_to_use;                         // Milliseconds allocated to this search.
            MinimaxAI::SearchTimeControl time_control;

           

            if (move_time > 0) {    
                // A "movetime" parameter was passed by Cutechess. So Cutechess wants a constant time per move.

                // An explicit UCI movetime is a per-move limit in milliseconds,
                // not a multi-move clock, so borrowing is deliberately disabled.
                search_time_to_use = (ull)move_time;

            } else if (moves_to_go > 0) {
                // A "moves_to_go"  parameter was passed by Cutechess.  So Cutechess wants a constant time per set of moves. 
                
                // (here we assumme "wtime" and "btime" parameters were also passed by Cutechess). 
                // Note: Can a valid UCI command can contain wtime and btime without movestogo — for example, a 
                // sudden-death time control?
                assert(white_time != -1);
                assert(black_time != -1);
             
                // time_remaining_msec is the time allotted to shumi, until time control (including this move)
                // Note that it can be zero or negative.
                const int side = engine->game_board.turn == Color::WHITE ? 0 : 1;
                const long long time_remaining_msec = (side == 0 ? white_time : black_time);

                if (time_remaining_msec > 0) {
               
                    const ull time_left_msec = (ull)time_remaining_msec;

                    // Reserve 1% of the remaining clock for timing overhead, but never reserve 
                    // more than 500 milliseconds.
                    const ull reserve = std::min<ull>(500, (time_left_msec / 100));

                    // Establish the nominal time per move only once per time-control period.
                    // Recomputing it from clock/movestogo after every move would erase any
                    // accumulated borrowing or saving.
                    bool b_no_time_for_move = (nominal_time_per_move[side] == 0);
                    bool b_new_time_control = (moves_to_go > previous_moves_to_go[side]);
                    if (b_no_time_for_move || b_new_time_control) {
                        const ull usable_clock = time_left_msec - reserve;
            
                        // nominal time must be at least 1 millisecond 
                        assert(moves_to_go>0);
                        nominal_time_per_move[side] = std::max<ull>(1, (usable_clock / (ull)moves_to_go));
                    }
                    previous_moves_to_go[side] = moves_to_go;
                    assert(nominal_time_per_move[side] > 0);        // better have been computed

                    const ull time_per_move_msec = nominal_time_per_move[side];

                    search_time_to_use = time_per_move_msec;


                    time_control.time_left_msec = time_left_msec;
                    time_control.moves_left = moves_to_go;
                    time_control.nominal_time_per_move = time_per_move_msec;

                    // Allow this move to borrow up to one full nominal move's time (or more).
                    // For example, if time_per_move_msec is 10s, Shumi may add up to 10s beyond the normal budget.
                    // This is the main direct knob for how aggressive borrowing can be.
                    time_control.maximum_loan = 3*time_per_move_msec/2;

                    // Protect future moves from being starved after borrowing on this move.
                    // Here each future move must be left at least k / 4 time, but never less than 1 ms.
                    // Lowering this makes borrowing more aggressive; raising it makes borrowing safer.
                    time_control.minimum_future_time = std::max<ull>(1, time_per_move_msec / 4);

                    // Keep this much clock completely outside Shumi's usable budget.
                    // The search budgets from (time_left_msec - reserve), not the full clock.
                    // This protects against overshoot, GUI delay, and stop-check granularity.
                    time_control.clock_reserve = reserve;
                }
                else {      // time_remaining_msec is zero or negative

                    // The clock has already expired. Search with the smallest possible
                    // budget so Shumi returns a legal move as quickly as possible.
                    search_time_to_use = 1;  // milliseconds

                    time_control.time_left_msec = 1;
                    time_control.moves_left = moves_to_go;
                    time_control.nominal_time_per_move = 1;
                    time_control.maximum_loan = 0;
                    time_control.minimum_future_time = 1;
                    time_control.clock_reserve = 0;

                }
            } else {
                // Should this ever happen? Not unless cutechess passed us nonsense parameters.
                search_time_to_use = 0;
                sout << "Unsupported go command: " << line << endl;
                assert(0);
                continue;       // skip this go command (in release build)
            }

            // set the hard abort time. This is the time before the end of the game,
            // that the hard_abort logic kicks in. Zero means no hard abort.
            time_control.hard_abort_threshold_ms = 10'000;
   
            //
            // Start thread to "Get "best move" from Shumi"
            start_searching_for_move(*minimax_ai, search_thread, this_go_id, search_time_to_use, depth_to_use,
                                      player_id, iRandomMoves, flags, time_control);


        } else if (line == "stop") {
            // Note: test me
            if (search_thread.running.load(std::memory_order_acquire) && minimax_ai != nullptr) {
                minimax_ai->stop_calculation = true;
            }
            // std::cout << "stop ok" << endl;
            // std::cout.flush();
           

        } else if (line == "quit") {
            sout << "quit received" << endl;
            sout.flush();
            if (search_thread.running.load(std::memory_order_acquire) && minimax_ai != nullptr) {
                minimax_ai->stop_calculation = true;
                if (search_thread.thread.joinable()) {
                    // Blocking call: wait so engine/minimax are not deleted under the worker thread.
                    search_thread.thread.join();
                }
                search_thread.running.store(false, std::memory_order_release);
                search_thread.done.store(false, std::memory_order_release);
            }
            break;
        }
    }

    sout << "UCI INPUT LOOP ENDED"
              << " eof=" << std::cin.eof()
              << " fail=" << std::cin.fail()
              << " bad=" << std::cin.bad()
              << endl;


    delete minimax_ai;
    delete engine;


    sout << "Shumi UCI exiting normally\n";

    // Disconnect these streams from sout_file before closing it.
    sout.rdbuf(std::clog.rdbuf());
    std::cerr.rdbuf(std::clog.rdbuf());

    sout.flush();
    sout_file.close();

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////

static void start_searching_for_move(
    MinimaxAI& minimax_ai,
    UciSearchState& search_thread,
    ull go_id,
    ull search_time_to_use,
    int depth_to_use,
    int player_id,
    int iRandomMoves,
    int flags,
    MinimaxAI::SearchTimeControl time_control)
{
    if (search_thread.running.load(std::memory_order_acquire)) {
        sout << "Ignoring go while search is already running go_id="
             << search_thread.go_id
             << endl;
        return;
    }

    if (search_thread.thread.joinable()) {
        // Blocking call: reap the previous finished worker before
        // starting another one.
        search_thread.thread.join();
    }

    search_thread.go_id = go_id;
    search_thread.done.store(false, std::memory_order_release);
    search_thread.running.store(true, std::memory_order_release);

    search_thread.thread = std::thread(
        [&minimax_ai,
         &search_thread,
         go_id,
         search_time_to_use,
         depth_to_use,
         player_id,
         iRandomMoves,
         flags,
         time_control]()
        {
            try {
                // sout << "\nSEARCH START go_id="
                //      << go_id
                //      << endl;

                // Blocking call: this worker thread is occupied here until Shumi returns a move.
                search_thread.move =
                    minimax_ai.get_move_iterative_deepening(
                        search_time_to_use,
                        depth_to_use,
                        player_id,
                        iRandomMoves,
                        flags,
                        time_control
                    );

                sout << "SEARCH RETURNED go_id="
                     << go_id
                     << endl;
            }
            catch (const std::exception& exception) {
                sout << "SEARCH EXCEPTION go_id="
                     << go_id
                     << " what=" << exception.what()
                     << endl;

                sout.flush();

                // Causes found_move() to send "bestmove 0000".
                search_thread.move = Move{};
            }
            catch (...) {
                sout << "SEARCH UNKNOWN EXCEPTION go_id="
                     << go_id
                     << endl;

                sout.flush();

                // Causes found_move() to send "bestmove 0000".
                search_thread.move = Move{};
            }

            // This must happen after either a normal return or a caught
            // exception so that the main UCI thread reaps this thread.
            search_thread.done.store(true, std::memory_order_release);
        }
    );
}

///////////////////////////////////////////////////////////////////////////////////////////////

static void found_move(Engine& engine,
                       MinimaxAI& minimax_ai,
                       UciSearchState& search_thread,
                       vector<string>& moves_so_far,
                       int& iMovesInGame)
{
    if (search_thread.thread.joinable()) {
        // Blocking call, but only after done is true, so this should return immediately.
        search_thread.thread.join();
    }

    search_thread.running.store(false, std::memory_order_release);
    search_thread.done.store(false, std::memory_order_release);

    Move move = search_thread.move;

    if (move.piece_type == Piece::NONE) {
        sout << "No legal move returned at ply " << endl;
        std::cout << "bestmove 0000\n";
        std::cout.flush();
        return;
    }

    // Translate this Move into UCI coordinate notation
    string move_str = move_to_uci(move);
    //sout << "STARTING move_into_string from shumi_uci" << endl;
    engine.move_into_string(move);
    //sout << "ENDING move_into_string" << endl;
    string move_str_alebriac = engine.move_string;
    

    // bitboards_to_algebraic

    //
    // Make Shumi move in the Shumi engine
    make_engine_move(engine, move);
    moves_so_far.push_back(move_str);


    int nodesSeen = minimax_ai.nodes_visited;

    //
    // Show move info
    // options to the "info" command sent to the "GUI"
    //
    // depth 8                  // search depth reached
    // seldepth 14              // deepest selective/qsearch depth reached
    // time 1234                // elapsed search time in milliseconds
    // nodes 456789             // total nodes searched
    // nps 1234567              // nodes per second
    // score cp 34              // score in centipawns
    // score mate 3             // mate in 3
    // score cp 34 lowerbound   // score is at least this good
    // score cp 34 upperbound   // score is at most this good
    // pv e2e4 e7e5 g1f3        // principal variation
    // multipv 2                // this is the second-best PV line
    // currmove e2e4            // move currently being searched
    // currmovenumber 5         // current move number in the move list
    // hashfull 123             // hash fullness, 0 to 1000
    // tbhits 0                 // tablebase hits
    // cpuload 850              // CPU load, 0 to 1000
    // string text here         // debug/status text
    // refutation e2e4 e7e5     // refutation line for a move
    // currline 1 e2e4 e7e5     // current line for CPU/thread 1

    //int nps = 1234567;
    int nps = minimax_ai.iNodes_per_Second;

    //int centiPawnsRel = (int)(minimax_ai.d_best_move_score_rel * 100.0);
    int centiPawnsRel = (int)convert_to_CP(minimax_ai.d_best_move_score_rel);

    std::cout << "info string testing\n";
    std::cout << "info"
            << " depth " << minimax_ai.max_attained_depth
            << " seldepth " << minimax_ai.max_attained_qdepth
            << " score cp " << centiPawnsRel
            << " nodes " << nodesSeen
            << " nps " << nps
            << "\n";


    // Show move
    iMovesInGame++;

    sout << move_str_alebriac << " SENDING bestmove " << move_str << " go_id=" << search_thread.go_id << endl;

    std::cout << "bestmove " << move_str << "\n";
    std::cout.flush();

    // // cerr << "\nPly " << ply << " "
    // //      << utility::representation::color_to_string(move.color)
    // //      << " move: " << move_to_uci(move) << endl;
    std::cout.flush();
}

static bool extract_pending_line(string& pending_input, string& line)
{
    size_t newline = pending_input.find('\n');
    if (newline == string::npos) {
        return false;
    }

    line = pending_input.substr(0, newline);
    pending_input.erase(0, newline + 1);

    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    return true;
}

static bool try_read_uci_line(std::string& line, bool& input_closed)
{
    static string pending_input;

    if (extract_pending_line(pending_input, line)) {
        return true;
    }

#ifdef _WIN32
    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    if (stdin_handle == INVALID_HANDLE_VALUE || stdin_handle == nullptr) {
        input_closed = true;
        return false;
    }

    DWORD file_type = GetFileType(stdin_handle);
    if (file_type == FILE_TYPE_CHAR) {
        // Nonblocking console check: _getch() is called only after _kbhit() says input exists.
        while (_kbhit()) {
            int ch = _getch();
            if (ch == '\r') {
                continue;
            }
            pending_input.push_back(static_cast<char>(ch));
            if (ch == '\n') {
                return extract_pending_line(pending_input, line);
            }
        }
        return false;
    }

    DWORD bytes_available = 0;
    // Nonblocking pipe check: cutechess talks to us through stdin; return if no bytes are ready.
    if (!PeekNamedPipe(stdin_handle, nullptr, 0, nullptr, &bytes_available, nullptr)) {
        input_closed = GetLastError() == ERROR_BROKEN_PIPE || GetLastError() == ERROR_HANDLE_EOF;
        return false;
    }

    if (bytes_available == 0) {
        return false;
    }

    vector<char> buffer(bytes_available);
    DWORD bytes_read = 0;
    // Reads only the bytes PeekNamedPipe() reported as available above.
    if (!ReadFile(stdin_handle, buffer.data(), bytes_available, &bytes_read, nullptr) || bytes_read == 0) {
        input_closed = true;
        return false;
    }

    pending_input.append(buffer.data(), bytes_read);
    return extract_pending_line(pending_input, line);
#else
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    timeval timeout{};
    int ready = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);
    if (ready <= 0) {
        return false;
    }

    char buffer[4096];
    // Reads only after select() reported stdin as ready.
    ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            input_closed = true;
        }
        return false;
    }

    if (bytes_read == 0) {
        input_closed = true;
        return false;
    }

    pending_input.append(buffer, static_cast<size_t>(bytes_read));
    return extract_pending_line(pending_input, line);
#endif
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

    // Manage three time repetition
    engine.push_to_three_time_rep_stack(move);

}

static string move_to_uci(const Move& move)
{
    const ull movefrom = utility::bit::square_to_bitboard(move.fromSQ);
    const ull moveto = utility::bit::square_to_bitboard(move.toSQ);

    string move_uci = utility::representation::bitboard_to_acn_conversion(movefrom)
                    + utility::representation::bitboard_to_acn_conversion(moveto);

    if (move.promotion != Piece::NONE) {
        move_uci += utility::representation::piece_to_charactor(move.promotion);
    }

    return move_uci;
}

static bool make_uci_move(Engine& engine, const string& move_uci)
{
    vector<Move> legal_moves;
    engine.get_legal_moves_fast(engine.game_board.turn, false, false, legal_moves);

    for (const Move& move : legal_moves) {
        if (move_to_uci(move) == move_uci) {
            make_engine_move(engine, move);
            return true;
        }
    }

    sout << "Invalid UCI move for current position: " << move_uci << endl;
    return false;
}

// Is old_moves a prefix list to new_moves?
static bool is_prefix(const vector<string>& old_moves, const vector<string>& new_moves)
{
    if (old_moves.size() > new_moves.size()) {
        return false;
    }

    for (size_t i = 0; i < old_moves.size(); i++) {
        if (old_moves[i] != new_moves[i]) {
            return false;
        }
    }

    return true;
}

static bool parse_position_command(const string& line, string& new_base, vector<string>& new_moves)
{
    istringstream input(line);
    vector<string> tokens;
    string token;

    while (input >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() < 2 || tokens[0] != "position") {
        return false;
    }

    size_t next_token = 0;

    if (tokens[1] == "startpos") {
        new_base = "startpos";
        next_token = 2;
    } else if (tokens[1] == "fen") {
        if (tokens.size() < 8) {
            return false;
        }

        new_base = tokens[2];
        for (size_t i = 3; i < 8; i++) {
            new_base += " " + tokens[i];
        }
        next_token = 8;
    } else {
        return false;
    }

    new_moves.clear();

    if (next_token == tokens.size()) {
        return true;
    }

    if (tokens[next_token] != "moves") {
        return false;
    }

    new_moves.assign(tokens.begin() + next_token + 1, tokens.end());
    return true;
}

static bool create_position(const string& base,
                            const vector<string>& moves,
                            Engine*& engine,
                            MinimaxAI*& minimax_ai)
{
    static const string STARTPOS_FEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    const string& fen = base == "startpos" ? STARTPOS_FEN : base;
    Engine* new_engine = new Engine(fen);
    MinimaxAI* new_minimax_ai = new MinimaxAI(*new_engine);

    for (const string& move_uci : moves) {
        if (!make_uci_move(*new_engine, move_uci)) {
            delete new_minimax_ai;
            delete new_engine;
            return false;
        }
    }

    delete minimax_ai;
    delete engine;
    engine = new_engine;
    minimax_ai = new_minimax_ai;
    return true;
}

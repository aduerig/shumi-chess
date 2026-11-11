#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "json.hpp"
#include "engine.hpp"
#include "minimax.hpp"
#include "score.hpp"
#include "weights.hpp"

using json = nlohmann::json;
using namespace ShumiChess;
using namespace std::chrono_literals;

namespace {
constexpr const char* START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
std::mutex output_mutex;
std::ostream protocol(std::cout.rdbuf());

void emit(const json& value) {
    std::lock_guard<std::mutex> guard(output_mutex);
    protocol << value.dump() << '\n' << std::flush;
}

std::string square_name(int square) {
    return {static_cast<char>('h' - square % 8), static_cast<char>('1' + square / 8)};
}

std::string uci(const Move& move) {
    if (move.piece_type == Piece::NONE) return "";
    std::string result = square_name(move.fromSQ) + square_name(move.toSQ);
    switch (move.promotion) {
        case Piece::QUEEN: result += 'q'; break;
        case Piece::ROOK: result += 'r'; break;
        case Piece::BISHOP: result += 'b'; break;
        case Piece::KNIGHT: result += 'n'; break;
        default: break;
    }
    return result;
}

int bounded_int(const json& request, const char* key, int low, int high, int fallback) {
    const auto found = request.find(key);
    if (found == request.end()) return fallback;
    if (!found->is_number_integer()) throw std::runtime_error(std::string(key) + " must be an integer.");
    const auto value = found->get<long long>();
    if (value < low || value > high) throw std::runtime_error(std::string(key) + " is outside the supported range.");
    return static_cast<int>(value);
}

std::vector<std::string> fields(const std::string& text) {
    std::istringstream stream(text);
    std::vector<std::string> result;
    for (std::string field; stream >> field;) result.push_back(field);
    return result;
}

// The core FEN constructor assumes valid input and uses assertions. Check all
// external input before reaching it; validation does not replace move generation.
std::string validate_fen(const std::string& input) {
    if (input.size() > 200) throw std::runtime_error("FEN is too long.");
    const auto part = fields(input);
    if (part.size() != 6) throw std::runtime_error("FEN needs six fields: position, turn, castling, en passant, and two counters.");
    if (part[1] != "w" && part[1] != "b") throw std::runtime_error("FEN turn must be w or b.");
    char board[8][8]{};
    int row = 0, column = 0, kings[2]{}, pawns[2]{}, pieces[2]{};
    int king_row[2]{}, king_col[2]{};
    for (char token : part[0]) {
        if (token == '/') {
            if (column != 8 || ++row > 7) throw std::runtime_error("Each FEN rank must contain eight squares.");
            column = 0;
        } else if (token >= '1' && token <= '8') {
            column += token - '0';
            if (column > 8) throw std::runtime_error("A FEN rank contains too many squares.");
        } else {
            if (std::string("prnbqkPRNBQK").find(token) == std::string::npos || column >= 8)
                throw std::runtime_error("Invalid FEN piece or rank.");
            board[row][column] = token;
            const int side = std::isupper(static_cast<unsigned char>(token)) ? 0 : 1;
            ++pieces[side];
            if (std::tolower(static_cast<unsigned char>(token)) == 'k') {
                ++kings[side]; king_row[side] = row; king_col[side] = column;
            }
            if (std::tolower(static_cast<unsigned char>(token)) == 'p') {
                ++pawns[side];
                if (row == 0 || row == 7) throw std::runtime_error("Pawns cannot occupy the first or eighth rank.");
            }
            ++column;
        }
    }
    if (row != 7 || column != 8) throw std::runtime_error("FEN must describe eight complete ranks.");
    if (kings[0] != 1 || kings[1] != 1) throw std::runtime_error("The position needs exactly one king of each color.");
    if (pieces[0] > 16 || pieces[1] > 16 || pawns[0] > 8 || pawns[1] > 8)
        throw std::runtime_error("The position has too many pieces or pawns.");
    if (std::abs(king_row[0] - king_row[1]) <= 1 && std::abs(king_col[0] - king_col[1]) <= 1)
        throw std::runtime_error("The kings cannot be adjacent.");
    if (part[2] != "-") {
        std::string seen;
        for (char c : part[2]) {
            if (std::string("KQkq").find(c) == std::string::npos || seen.find(c) != std::string::npos)
                throw std::runtime_error("Invalid castling rights.");
            seen += c;
            const bool white = c == 'K' || c == 'Q';
            const int r = white ? 7 : 0;
            const int file = c == 'K' || c == 'k' ? 7 : 0;
            if (board[r][4] != (white ? 'K' : 'k') || board[r][file] != (white ? 'R' : 'r'))
                throw std::runtime_error("Castling rights require the king and rook on their starting squares.");
        }
    }
    if (part[3] != "-") {
        if (part[3].size() != 2 || part[3][0] < 'a' || part[3][0] > 'h' ||
            part[3][1] != (part[1] == "w" ? '6' : '3')) throw std::runtime_error("Invalid en passant square.");
        const int r = 8 - (part[3][1] - '0'), c = part[3][0] - 'a';
        const bool white_turn = part[1] == "w";
        if (board[r][c] || board[r + (white_turn ? 1 : -1)][c] != (white_turn ? 'p' : 'P') ||
            board[r + (white_turn ? -1 : 1)][c]) throw std::runtime_error("En passant does not match the pawn position.");
    }
    for (int index : {4, 5}) {
        if (part[index].empty() || part[index].size() > 3 ||
            !std::all_of(part[index].begin(), part[index].end(), [](unsigned char c) { return std::isdigit(c); }))
            throw std::runtime_error("FEN counters must be integers.");
        const int value = std::stoi(part[index]);
        if (value > 255 || (index == 5 && value == 0))
            throw std::runtime_error("This engine accepts FEN counters up to 255; the move number starts at 1.");
    }
    std::string result;
    for (const auto& field : part) { if (!result.empty()) result += ' '; result += field; }
    return result;
}

class Host {
    std::unique_ptr<Engine> engine = std::make_unique<Engine>();
    std::unique_ptr<MinimaxAI> ai = std::make_unique<MinimaxAI>(*engine);
    std::thread search_thread;
    std::atomic<bool> searching{false};
    std::atomic<bool> stop_requested{false};
    std::thread stop_thread;
    std::string initial_fen = START_FEN;
    int initial_move = 1;
    bool initial_black = false;
    json moves = json::array();
    int search_id = 0;
    int search_sign = 1;
    int last_player = -1, last_features = -1;
    bool model_loaded = false;
    int last_depth = 0;

    bool in_check() {
        return engine->game_board.turn == WHITE ? engine->is_king_in_check_t<WHITE>() : engine->is_king_in_check_t<BLACK>();
    }

    std::vector<Move> legal() {
        std::vector<Move> result;
        engine->get_legal_moves_fast(engine->game_board.turn, false, false, result);
        return result;
    }

    static void progress(const MinimaxAI::thread_callback_structure& data, void* context) {
        auto& host = *static_cast<Host*>(context);
        host.last_depth = data.depth;
        emit({{"event", "progress"}, {"searchId", host.search_id}, {"depth", data.depth},
              {"elapsedMs", data.elapsed_time_msecc}, {"nodes", data.nodes_so_far},
              {"scoreCp", convert_to_CP(data.best_scoree) * host.search_sign}});
    }

    std::string result_text(int result) {
        return result == WHITEWIN ? "1-0" : result == BLACKWIN ? "0-1" : result == DRAW ? "1/2-1/2" : "*";
    }

public:
    Host() {
        const auto model_path = std::filesystem::path(SHUMI_REPO_ROOT) / NNUE_model_file;
        model_loaded = ai->nnue.load_parameters(model_path);
        ai->thread_callback = progress;
        ai->thread_callback_user_data = this;
        // Poll the atomic stop flag outside the engine thread. Reasserting the
        // request closes the race with minimax's initialization clearing it.
        stop_thread = std::thread([this] {
            while (!closing.load()) {
                if (stop_requested.load() && searching.load()) ai->stop_calculation.store(true);
                std::this_thread::sleep_for(2ms);
            }
        });
    }
    std::atomic<bool> closing{false};
    ~Host() {
        stop();
        closing = true;
        if (stop_thread.joinable()) stop_thread.join();
    }

    void stop() {
        stop_requested = true;
        if (search_thread.joinable()) search_thread.join();
        stop_requested = false;
    }

    json snapshot() {
        auto available = legal();
        engine->game_board.compute_bits_in();
        int reason = 0;
        const int result = engine->is_game_over(static_cast<int>(available.size()), &reason);
        const char* reasons[] = {"", "Stalemate", "Threefold repetition", "Fifty-move rule", "Insufficient material", "Agreement", "Move limit"};
        json legal_moves = json::array();
        for (const auto& move : available) legal_moves.push_back(uci(move));
        const int move_number = initial_move + (static_cast<int>(moves.size()) + (initial_black ? 1 : 0)) / 2;
        std::string fen = engine->game_board.to_fen();
        fen = fen.substr(0, fen.find_last_of(' ') + 1) + std::to_string(move_number);
        std::ostringstream pgn;
        pgn << "[Event \"Shumi Chess\"]\n[Result \"" << result_text(result) << "\"]\n";
        if (initial_fen != START_FEN) pgn << "[SetUp \"1\"]\n[FEN \"" << initial_fen << "\"]\n";
        pgn << '\n';
        for (size_t i = 0; i < moves.size(); ++i) {
            const auto& record = moves[i];
            if (record["color"] == "w") pgn << record["number"].get<int>() << ". ";
            else if (i == 0) pgn << record["number"].get<int>() << "... ";
            pgn << record["san"].get<std::string>() << ' ';
        }
        pgn << result_text(result);
        return {{"fen", fen}, {"turn", engine->game_board.turn == WHITE ? "w" : "b"},
                {"legalMoves", legal_moves}, {"inCheck", in_check()}, {"result", result_text(result)},
                {"drawReason", reason >= 0 && reason <= 6 ? reasons[reason] : "Draw"},
                {"moves", moves}, {"pgn", pgn.str()}, {"moveNumber", move_number},
                {"phase", str_from_GamePhase(ai->phase_of_game_full())},
                {"whiteTimeMs", engine->game_white_time_msec}, {"blackTimeMs", engine->game_black_time_msec},
                {"nnueAvailable", model_loaded}, {"defaultFeatures", _DEFAULT_FEATURES_MASK}};
    }

    json execute(const json& request) {
        const std::string command = request.at("command").get<std::string>();
        if (command == "stop") { stop(); return {{"stopped", true}}; }
        if (command == "quit") { stop(); closing = true; return {{"closed", true}}; }
        if (searching.load()) throw std::runtime_error("The engine is searching. Stop it before changing or reading the position.");
        if (search_thread.joinable()) search_thread.join();
        if (command == "snapshot") return snapshot();
        if (command == "reset") {
            std::string fen = request.value("fen", std::string(START_FEN));
            if (fen.empty()) fen = START_FEN;
            fen = validate_fen(fen);
            // Validate on a temporary engine so errors leave the current game intact.
            auto candidate = std::make_unique<Engine>(fen);
            const bool illegal = candidate->game_board.turn == WHITE
                ? candidate->is_king_in_check_t<BLACK>() : candidate->is_king_in_check_t<WHITE>();
            if (illegal) throw std::runtime_error("The side that just moved cannot have left its king in check.");
            const int random_moves = bounded_int(request, "random", 0, 100, 0);
            engine->reset_engine(fen);
            engine->set_random_on_next_move(random_moves);
            ai->TTable2.clear();
            ai->pawn_file_info.clear();
            initial_fen = fen;
            initial_move = std::stoi(fields(fen)[5]);
            initial_black = engine->game_board.turn == BLACK;
            moves = json::array();
            return snapshot();
        }
        if (command == "move") {
            const auto text = request.at("move").get<std::string>();
            auto available = legal();
            const auto found = std::find_if(available.begin(), available.end(), [&](const Move& m) { return uci(m) == text; });
            if (found == available.end()) throw std::runtime_error("That move is not legal in the current position.");
            if (snapshot()["result"] != "*") throw std::runtime_error("This game has ended.");
            Move move = *found;
            const bool white = engine->game_board.turn == WHITE;
            const int number = initial_move + (static_cast<int>(moves.size()) + (initial_black ? 1 : 0)) / 2;
            engine->gamePGN.addMe(move, *engine);
            std::string san = engine->move_string;
            while (!san.empty() && std::isspace(static_cast<unsigned char>(san.back()))) san.pop_back();
            engine->users_last_move = move;
            ++engine->ply_so_far;
            if (white) engine->pushMove_t<WHITE>(move); else engine->pushMove_t<BLACK>(move);
            engine->push_to_three_time_rep_stack(move);
            ai->playground(engine->game_phase);
            const bool check = in_check();
            if (check) san += legal().empty() ? "#" : "+";
            moves.push_back({{"uci", text}, {"san", san}, {"color", white ? "w" : "b"}, {"number", number}});
            return snapshot();
        }
        if (command == "evaluate") {
            const int player = bounded_int(request, "player", 0, 3, UNCLE_SHUMI);
            if (player == NNUE && !model_loaded) throw std::runtime_error("The configured NNUE model could not be loaded.");
            const auto person = static_cast<EvalPersons>(player);
            const int score = ai->evaluate_board_t<WHITE>(person);
            return {{"scoreCp", score}};
        }
        if (command == "search") {
            const int depth = bounded_int(request, "depth", 1, 30, 9);
            const int milliseconds = bounded_int(request, "time", 0, 3600000, 5000);
            const int player = bounded_int(request, "player", 0, 3, UNCLE_SHUMI);
            const int features = bounded_int(request, "features", 0, 2147483647, _DEFAULT_FEATURES_MASK);
            if (player == NNUE && !model_loaded) throw std::runtime_error("The configured NNUE model could not be loaded.");
            if (snapshot()["result"] != "*") throw std::runtime_error("This game has ended.");
            if (player != last_player || features != last_features) {
                ai->TTable2.clear(); ai->pawn_file_info.clear();
                last_player = player; last_features = features;
            }
            search_id = bounded_int(request, "id", 1, 2147483647, 1);
            search_sign = engine->game_board.turn == WHITE ? 1 : -1;
            last_depth = 0;
            stop_requested = false;
            searching = true;
            search_thread = std::thread([this, depth, milliseconds, player, features] {
                try {
                    const auto start = std::chrono::steady_clock::now();
                    const auto result = ai->get_move_iterative_deepening(static_cast<ull>(milliseconds) + 1ULL, depth, player, 0, features);
                    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
                    const json payload = {{"move", uci(std::get<1>(result))},
                        {"scoreCp", convert_to_CP(ai->d_best_move_score_rel) * search_sign},
                        {"depth", last_depth}, {"nodes", ai->nodes_visited}, {"elapsedMs", elapsed}};
                    searching = false;
                    emit({{"id", search_id}, {"ok", true}, {"data", payload}});
                } catch (const std::exception& error) {
                    searching = false;
                    emit({{"id", search_id}, {"ok", false}, {"error", error.what()}});
                }
            });
            return nullptr; // Search completes asynchronously with the same request id.
        }
        throw std::runtime_error("Unknown engine command.");
    }
};
} // namespace

int main() {
    // Preserve a dedicated protocol stream; core diagnostic output is never JSON.
    std::cout.rdbuf(std::cerr.rdbuf());
    try {
        Host host;
        emit({{"event", "ready"}, {"protocol", 1}, {"data", host.snapshot()}});
        for (std::string line; !host.closing.load() && std::getline(std::cin, line);) {
            int id = 0;
            try {
                if (line.size() > 8192) throw std::runtime_error("Engine command is too large.");
                const auto request = json::parse(line);
                id = bounded_int(request, "id", 1, 2147483647, 0);
                const auto response = host.execute(request);
                if (!response.is_null()) emit({{"id", id}, {"ok", true}, {"data", response}});
            } catch (const std::exception& error) {
                emit({{"id", id}, {"ok", false}, {"error", error.what()}});
            }
        }
    } catch (const std::exception& error) {
        emit({{"event", "fatal"}, {"error", error.what()}});
        return 1;
    }
    return 0;
}

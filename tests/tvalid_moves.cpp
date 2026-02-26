// #include <bits/stdc++.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "engine.hpp"
#include "globals.hpp"
#include "utility.hpp"

using namespace std;
namespace fs = std::filesystem;

typedef unordered_map<int, vector<string>> fen_map;

void recurse_moves_and_fill_fens(vector<string>& fen_holder, int depth, int max_depth, ShumiChess::Engine& engine) {
    if (depth > max_depth) {
        return;
    }
    vector<ShumiChess::Move> legal_moves = engine.get_legal_moves();
    for (ShumiChess::Move move : legal_moves) {

        string before_fen = engine.game_board.to_fen();
        engine.pushMove(move);

        recurse_moves_and_fill_fens(fen_holder, depth + 1, max_depth, engine);
        if (depth == max_depth) {
            fen_holder.push_back(engine.game_board.to_fen());
        }
        engine.popMove();
        string after_fen = engine.game_board.to_fen();

        if (before_fen != after_fen) {
            FAIL() << "At depth " << depth << " fens DID NOT MATCH AFTER POP() "
                   << "Before: " << before_fen << ", After: " << after_fen << endl;
        }
    }
}

vector<string> get_certain_depth_fens_from_engine(string starting_fen, int depth) {
    vector<string> fen_holder;
    ShumiChess::Engine test_engine(starting_fen);
    recurse_moves_and_fill_fens(fen_holder, 1, depth, test_engine);
    return fen_holder;
}

pair<string, vector<string>> get_fens_from_file(fs::path filepath) {
    vector<string> fen_holder;
    ifstream myfile(filepath);
    if (!myfile.is_open()) {
        // TODO find better way to have this function fail
        cout << "ERROR: Could not open file: " << filepath << ", exiting..." << endl;
        exit(1);
    }
        
    int depth = 0;
    string line;
    string starting_fen;
    if (getline (myfile, line)) {
        if (utility::our_string::starts_with(line, "Starting Fen:")) {
            vector<string> splitted = utility::our_string::split(line, ":");
            starting_fen = splitted.back();
            utility::our_string::trim(starting_fen); // in place
        }
    }
    
    // all other fens in the file
    while (getline (myfile, line)) {
        if (line != "") {
            fen_holder.push_back(line);
        }
    }

    myfile.close();
    return make_pair(starting_fen, fen_holder);
}

vector<string> get_filenames_to_test_positions(fs::path folder_to_search) {
    vector<string>  all_filenames;
    for(auto& p: fs::directory_iterator(folder_to_search)) {
        // cout << "found path and adding: " << p.path() << endl;
        all_filenames.push_back(p.path().string());
    }
    sort(all_filenames.begin(), all_filenames.end());
    return all_filenames;
}

fs::path test_data_path = TEST_DATA_DIR;
vector<string>  test_filenames = get_filenames_to_test_positions(test_data_path);

// uncomment if need just one file
// vector<string>  test_filenames = vector<string>{"tests/test_data/rooks_depth_1.dat"};


class LegalPositionsByDepth : public testing::TestWithParam<string> {}; 
TEST_P(LegalPositionsByDepth, LegalPositionsByDepth) {
    // ? seg faults when i pass a fs::path into here, idk why
    fs::path local_filepath = fs::path(GetParam());
    string filename = local_filepath.stem().string();
    string string_depth = utility::our_string::split(filename, "_").back();

    // cout << "depth of file: " << local_filepath.filename() << ", is: " << string_depth << endl;
    int depth = stoi(string_depth);
    
    pair<string, vector<string>> fen_info_pair = get_fens_from_file(local_filepath);
    string starting_fen = fen_info_pair.first;
    vector<string> fens_from_file = fen_info_pair.second;

    // construct baseline map from the known fens by depth
    unordered_map<string, int> baseline_fens;
    for (string i : fens_from_file) {
        if (baseline_fens.find(i) == baseline_fens.end()) {
            baseline_fens[i] = 0;
        }
        // cout << "Baseline fen: " << i << endl;
        // utility::representation::print_gameboard(ShumiChess::GameBoard(i));
        baseline_fens[i]++;
    }

    // construct map based on ShumiChess engine
    vector<string> fen_data_engine = get_certain_depth_fens_from_engine(starting_fen, depth);
    unordered_map<string, int> shumi_fens;
    for (string i : fen_data_engine) {
        if (baseline_fens.find(i) == baseline_fens.end()) {
            shumi_fens[i] = 0;
        }
        // cout << "Shumi fen: " << i << endl;
        // utility::representation::print_gameboard(ShumiChess::GameBoard(i));
        shumi_fens[i]++;
    }

    // std::cout << "Size of shumi_fens: " << shumi_fens.size() << std::endl;
    // std::cout << "Size of baseline_fens: " << baseline_fens.size() << std::endl;

    // compare baseline and shumichess
    for (const auto& pair : baseline_fens) {
        string baseline_fen = pair.first;
        int baseline_times_appear = pair.second;
        
        if (shumi_fens.find(baseline_fen) == shumi_fens.end()) {
            ShumiChess::Engine test_engine1(baseline_fen);
            utility::representation::print_gameboard(test_engine1.game_board);
            cout << "Trying to find matching board positions (not perfect fen match)..." << endl;
            string just_position = utility::our_string::split(baseline_fen, " ")[0];
            for (const auto& pair2 : shumi_fens) {
                string fenner = pair2.first;
                int nothingmatters = pair2.second;
                string just_position2 = utility::our_string::split(fenner, " ")[0];
                if (just_position == just_position2) {
                    cout << "generated matching board found, fen is: " << fenner << ", board position is" << endl;
                    ShumiChess::Engine test_engine8(fenner);
                    utility::representation::print_gameboard(test_engine8.game_board);
                }
            }

            FAIL() << "At depth " << depth << " fen '" << baseline_fen <<
                "' was not found in positions generated by ShumiChess, bitboard rep of the missing fen above" << endl;
        }

        int shumi_times_appear = shumi_fens[baseline_fen];
        if (baseline_times_appear != shumi_times_appear) {
            ShumiChess::Engine test_engine2(baseline_fen);
            utility::representation::print_gameboard(test_engine2.game_board);
            FAIL() << "At depth " << depth << " fen '" << baseline_fen <<
                "' was found " << shumi_times_appear << " times in ShumiChess, and " << 
                baseline_times_appear << " in the baseline, bitboard rep above" <<  endl;
        }
    }

    for (const auto& pair : shumi_fens) {
        string shumi_fen = pair.first;
        int shumi_times_appear = pair.second;
        
        if (baseline_fens.find(shumi_fen) == baseline_fens.end()) {
            ShumiChess::Engine test_engine3(shumi_fen);
            utility::representation::print_gameboard(test_engine3.game_board);
            FAIL() << "At depth " << depth << " fen '" << shumi_fen <<
                "' was found in positions generated by ShumiChess but NOT by the baseline, bitboard rep above" << endl;
        }
    }
}
INSTANTIATE_TEST_SUITE_P(LegalPositionsByDepthParam, LegalPositionsByDepth, testing::ValuesIn(test_filenames));




vector<string> fens_to_test_push_pop = {
    "rnb1kbnr/pppp3p/1q4p1/4p3/8/2NPpN2/PPP1BPPP/R1BQ2KR w kq - 0 8",
};

class ValidMoves : public testing::TestWithParam<string> {}; 
TEST_P(ValidMoves, PushPopFenValidation) {
    using namespace ShumiChess;

    Engine test_engine;
    test_engine.reset_engine(GetParam());

    // get all legal moves
    auto legal_moves = test_engine.get_legal_moves();

    for (const auto& move : legal_moves) {
        string before_fen = test_engine.game_board.to_fen();
        test_engine.pushMove(move);
        string in_between_fen = test_engine.game_board.to_fen();
        test_engine.popMove();
        string after_fen = test_engine.game_board.to_fen();
        if (before_fen != after_fen) {
            utility::representation::cout_move_info(move);
            FAIL() << "FEN mismatch detected:\n"
                   << "Before: " << before_fen << "\n"
                   << "In Between: " << in_between_fen << "\n"
                   << "After: " << after_fen << "\n";
        }
    }
}

INSTANTIATE_TEST_SUITE_P(ValidMoves, ValidMoves, testing::ValuesIn(fens_to_test_push_pop));

#include <iostream>
#include <fstream>
#include <unordered_map>

#include <gtest/gtest.h>
#include <engine.hpp>
#include <utility.hpp>
#include <globals.hpp>

using namespace std;

typedef unordered_map<int, vector<string>> fen_map;

TEST(Setup, WhiteGoesFirst) {
    ShumiChess::Engine test_engine;
    test_engine.game_board.turn == ShumiChess::Color::WHITE;
}

// using qperft to determine number of legal moves by depth
// https://home.hccnet.nl/h.g.muller/dwnldpage.html
// perft( 1)=           20
// perft( 2)=          400
// perft( 3)=         8902
// perft( 4)=       197281
// perft( 5)=      4865609
// perft( 6)=    119060324

TEST(ValidMoves, LengthOfValidMovesDepth1) {
    ShumiChess::Engine test_engine;
    int total_legal_moves = test_engine.get_legal_moves().size();
    ASSERT_EQ(20, total_legal_moves);
}

// TODO need to calculate length of total legal moves by depth, refactor using matts method
// TEST(ValidMoves, LengthOfValidMovesDepth2) {
//     ShumiChess::Engine test_engine;
//     int total_legal_moves = test_engine.get_legal_moves().size();
//     ASSERT_EQ(400, total_legal_moves);
// }

// TEST(ValidMoves, LengthOfValidMovesDepth3) {
//     ShumiChess::Engine test_engine;
//     int total_legal_moves = test_engine.get_legal_moves().size();
//     ASSERT_EQ(8902, total_legal_moves);
// }

// TEST(ValidMoves, LengthOfValidMovesDepth4) {
//     ShumiChess::Engine test_engine;
//     int total_legal_moves = test_engine.get_legal_moves().size();
//     ASSERT_EQ(197281, total_legal_moves);
// }

// TEST(ValidMoves, LengthOfValidMovesDepth5) {
//     ShumiChess::Engine test_engine;
//     int total_legal_moves = test_engine.get_legal_moves().size();
//     ASSERT_EQ(4865609, total_legal_moves);
// }

// TEST(ValidMoves, LengthOfValidMovesDepth6) {
//     ShumiChess::Engine test_engine;
//     int total_legal_moves = test_engine.get_legal_moves().size();
//     ASSERT_EQ(119060324, total_legal_moves);
// }

/////////////////////////////////
// FENS BY DEPTH
/////////////////////////////////

void recurse_moves_and_fill_fens(fen_map& fen_holder, int depth, ShumiChess::Engine& engine) {
    if (depth <= 0) {
        return;
    }

    vector<ShumiChess::Move> legal_moves = engine.get_legal_moves();
    for (auto move : legal_moves) {
        engine.push(move);
        recurse_moves_and_fill_fens(fen_holder, depth - 1, engine);
        fen_holder[depth].push_back(engine.game_board.to_fen());
        engine.pop();
    }
}

fen_map get_fens_by_depth_from_engine() {
    fen_map fen_holder;
    ShumiChess::Engine test_engine;
    
    recurse_moves_and_fill_fens(fen_holder, 3, test_engine);

    return fen_holder;
}

fen_map get_fens_by_depth_from_file(string test_filename) {
    fen_map fen_holder;
    ifstream myfile(test_filename);
    if (!myfile.is_open()) {
        // TODO find better way to have this function fail
        assert(0 == 1);
    }
    
    int depth = 0;
    string line;
    while (getline (myfile, line)) {
        if (utility::string::starts_with(line, "DEPTH")) {
            vector<string> splitted = utility::string::split(line, ":");
            depth = stoi(splitted.back());
            continue;
        }
        fen_holder[depth].push_back(line);
    }
    myfile.close();
    return fen_holder;
}

vector<int> get_keys_from_map(fen_map map) {
    vector<int> keys;
    for (const auto& key_and_value : map) {
        keys.push_back(key_and_value.first);
    }
    return keys;
}

// TODO delete once not used 
// vector<pair<int, fen_map>> generate_data_pairs(fen_map map) {
//     vector<pair<int, fen_map>> data_pairs;
//     for (const auto& key_and_value : map) {
//         auto data_pair = make_pair(key_and_value.first, map);
//         data_pairs.push_back(data_pair);
//     }
//     return data_pairs;
// }

string test_filename = "tests/test_data/legal_positions_by_depth.dat";
fen_map fens_by_depth = get_fens_by_depth_from_file(test_filename);
vector<int> depths = get_keys_from_map(fens_by_depth);

class LegalPositionsByDepth : public testing::TestWithParam<int> {}; 
TEST_P(LegalPositionsByDepth, LegalPositionsByDepth) {
    int depth = GetParam(); 

    // construct baseline map from the known fens by depth
    vector<string> fen_data_baseline = get_fens_by_depth_from_file(test_filename)[depth];
    unordered_map<string, int> baseline_fens;
    for (string i : fen_data_baseline) {
        if (baseline_fens.find(i) == baseline_fens.end()) {
            baseline_fens[i] = 0;
        }
        baseline_fens[i]++;
    } 

    // construct map based on ShumiChess engine
    vector<string> fen_data_engine = get_fens_by_depth_from_engine()[depth];
    unordered_map<string, int> shumi_fens;
    for (string i : fen_data_engine) {
        if (baseline_fens.find(i) == baseline_fens.end()) {
            shumi_fens[i] = 0;
        }
        shumi_fens[i]++;
    } 

    // compare baseline and shumichess
    for (const auto& pair : baseline_fens) {
        string baseline_fen = pair.first;
        int baseline_times_appear = pair.second;
        
        if (shumi_fens.find(baseline_fen) == shumi_fens.end()) {
            FAIL() << "At depth " << depth << " fen '" << baseline_fen <<
                "' was not found in positions generated by ShumiChess" << endl;
        }

        int shumi_times_appear = shumi_fens[baseline_fen];
        if (baseline_times_appear != shumi_times_appear) {
            FAIL() << "At depth " << depth << " fen '" << baseline_fen <<
                "' was found " << shumi_times_appear << " times in ShumiChess, and " << 
                baseline_times_appear << " in the baseline" <<  endl;
        }
    }

    for (const auto& pair : shumi_fens) {
        string shumi_fen = pair.first;
        int shumi_times_appear = pair.second;
        
        if (baseline_fens.find(shumi_fen) == shumi_fens.end()) {
            FAIL() << "At depth " << depth << " fen '" << shumi_fen <<
                "' was found in positions generated by ShumiChess but NOT by the baseline" << endl;
        }
    }
}
INSTANTIATE_TEST_CASE_P(LegalPositionsByDepthParam, LegalPositionsByDepth, testing::ValuesIn(depths));
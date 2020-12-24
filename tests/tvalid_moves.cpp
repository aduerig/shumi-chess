#include <gtest/gtest.h>
#include <engine.hpp>

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

TEST(ValidMoves, LengthOfValidMovesDepth2) {
    ShumiChess::Engine test_engine;
    int total_legal_moves = test_engine.get_legal_moves().size();
    ASSERT_EQ(400, total_legal_moves);
}

TEST(ValidMoves, LengthOfValidMovesDepth3) {
    ShumiChess::Engine test_engine;
    int total_legal_moves = test_engine.get_legal_moves().size();
    ASSERT_EQ(8902, total_legal_moves);
}

TEST(ValidMoves, LengthOfValidMovesDepth4) {
    ShumiChess::Engine test_engine;
    int total_legal_moves = test_engine.get_legal_moves().size();
    ASSERT_EQ(197281, total_legal_moves);
}

TEST(ValidMoves, LengthOfValidMovesDepth5) {
    ShumiChess::Engine test_engine;
    int total_legal_moves = test_engine.get_legal_moves().size();
    ASSERT_EQ(4865609, total_legal_moves);
}

TEST(ValidMoves, LengthOfValidMovesDepth6) {
    ShumiChess::Engine test_engine;
    int total_legal_moves = test_engine.get_legal_moves().size();
    ASSERT_EQ(119060324, total_legal_moves);
}


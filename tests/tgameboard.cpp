#include <gtest/gtest.h>
#include <string>

#include "gameboard.hpp"

using std::string;

TEST(FenNotationConstructor, badFen) {
    ASSERT_DEATH(ShumiChess::GameBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"), "^(Assertion failed:).*");
}

TEST(Constructors, defaultMatchesFenStart) {
    ASSERT_EQ(ShumiChess::GameBoard(), ShumiChess::GameBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
}

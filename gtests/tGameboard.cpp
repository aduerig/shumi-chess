#include <gtest/gtest.h>
#include <string>

#include "Gameboard.hpp"

using std::string;

TEST(Constructors, defaultMatchesFenStart) {
    ASSERT_EQ(GameBoard(), GameBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
}

TEST(FenNotationConstructor, badFen) {
    ASSERT_DEATH(GameBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"), "Assertion failed");
}
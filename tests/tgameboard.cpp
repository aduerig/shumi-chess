#include <gtest/gtest.h>
#include <string>

#include "gameboard.hpp"

using std::string;

namespace ShumiChess {
    // Equality operator for gameboards
    bool operator==(const GameBoard& a, const GameBoard& b) {
        return a.black_pawns   == b.black_pawns &&
               a.white_pawns   == b.white_pawns &&
               a.black_rooks   == b.black_rooks &&
               a.white_rooks   == b.white_rooks &&
               a.black_knights == b.black_knights &&
               a.white_knights == b.white_knights &&
               a.black_bishops == b.black_bishops &&
               a.white_bishops == b.white_bishops &&
               a.black_queens  == b.black_queens &&
               a.white_queens  == b.white_queens &&
               a.black_king    == b.black_king &&
               a.white_king    == b.white_king &&
               a.turn          == b.turn;
    }
}

TEST(FenNotationConstructor, BadFen) {
    ASSERT_DEATH(ShumiChess::GameBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"), "Assertion.*failed");
}

TEST(Constructors, DefaultMatchesFenStart) {
    ASSERT_EQ(ShumiChess::GameBoard(), ShumiChess::GameBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
}

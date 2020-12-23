#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <vector>
#include <utility>

#include "gameboard.hpp"

using namespace std;

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

class BadFenConstructor : public testing::TestWithParam<string> {};
vector<string> bad_fen_list = { "banana",
                                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",
                                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR white - - -" };
TEST_P(BadFenConstructor, AssertBadFenFails) {
    const string fen_notation = GetParam();
    ASSERT_DEATH(ShumiChess::GameBoard(fen_notation), "Assertion.*failed");
}
INSTANTIATE_TEST_CASE_P(BadFenConstructorParam, BadFenConstructor, testing::ValuesIn(bad_fen_list));

typedef tuple<ull, ull, ull, ull, ull, ull, ull, ull, ull, ull, ull, ull, ShumiChess::Color> board_tuple;
class tGameboardFenNotation : public testing::TestWithParam<pair<board_tuple, string>> {};
TEST_P(tGameboardFenNotation, DefaultMatchesFenStart) {
    const auto test_pair = GetParam();

    ShumiChess::GameBoard fen_board {test_pair.second};
    ShumiChess::GameBoard bit_board;
    bit_board.black_pawns = get<0>(test_pair.first);
    bit_board.white_pawns = get<1>(test_pair.first);
    bit_board.black_rooks = get<2>(test_pair.first);
    bit_board.white_rooks = get<3>(test_pair.first);
    bit_board.black_knights = get<4>(test_pair.first);
    bit_board.white_knights = get<5>(test_pair.first);
    bit_board.black_bishops = get<6>(test_pair.first);
    bit_board.white_bishops = get<7>(test_pair.first);
    bit_board.black_queens = get<8>(test_pair.first);
    bit_board.white_queens = get<9>(test_pair.first);
    bit_board.black_king = get<10>(test_pair.first);
    bit_board.white_king = get<11>(test_pair.first);
    bit_board.turn = get<12>(test_pair.first);

    ASSERT_EQ(fen_board, bit_board);
}
INSTANTIATE_TEST_CASE_P(tGameboardFenNotationParams,  
                        tGameboardFenNotation,  
                        testing::Values( make_pair(make_tuple(71776119061217280ULL, 65280ULL, 9295429630892703744ULL, 129ULL, 4755801206503243776ULL, 66ULL, 2594073385365405696ULL, 36ULL, 1152921504606846976ULL, 16ULL, 576460752303423488ULL, 8ULL, ShumiChess::Color::WHITE),
                                                   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") ));  

// TEST(Constructors, FenRepresentationCorrect) {
//     ASSERT_EQ(ShumiChess::GameBoard(), ShumiChess::GameBoard("r3k2r/pp1n2pp/2p2q2/b2p1n2/BP1Pp3/P1N2P2/2PB2PP/R2Q1RK1 w kq b3 0 13"));
// }

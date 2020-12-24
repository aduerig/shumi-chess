#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <vector>
#include <utility>

#include "gameboard.hpp"

using namespace std;

class BadFenConstructor : public testing::TestWithParam<string> {};
void create_gameboard(string fen) {
    const ShumiChess::GameBoard test_board {fen};
}
vector<string> bad_fen_list = { "banana",
                                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",
                                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR white - - -",
                                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR - kqKQs - -",
                                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR - - e77 - -" };
TEST_P(BadFenConstructor, AssertBadFenFails) {
    ASSERT_DEATH(create_gameboard(GetParam()), "Assertion.*failed");
}
INSTANTIATE_TEST_CASE_P(BadFenConstructorParam, BadFenConstructor, testing::ValuesIn(bad_fen_list));

typedef tuple<ull, ull, ull, ull, ull, ull, ull, ull, ull, ull, ull, ull, ShumiChess::Color, uint8_t, uint8_t, ull, int, int> board_tuple;
class tGameboardFenNotation : public testing::TestWithParam<pair<board_tuple, string>> {};
TEST_P(tGameboardFenNotation, FenMatchesBitBoard) {
    const auto test_pair = GetParam();

    ShumiChess::GameBoard fen_board {test_pair.second};

    EXPECT_EQ(fen_board.black_pawns, get<0>(test_pair.first));
    EXPECT_EQ(fen_board.white_pawns, get<1>(test_pair.first));
    EXPECT_EQ(fen_board.black_rooks, get<2>(test_pair.first));
    EXPECT_EQ(fen_board.white_rooks, get<3>(test_pair.first));
    EXPECT_EQ(fen_board.black_knights, get<4>(test_pair.first));
    EXPECT_EQ(fen_board.white_knights, get<5>(test_pair.first));
    EXPECT_EQ(fen_board.black_bishops, get<6>(test_pair.first));
    EXPECT_EQ(fen_board.white_bishops, get<7>(test_pair.first));
    EXPECT_EQ(fen_board.black_queens, get<8>(test_pair.first));
    EXPECT_EQ(fen_board.white_queens, get<9>(test_pair.first));
    EXPECT_EQ(fen_board.black_king, get<10>(test_pair.first));
    EXPECT_EQ(fen_board.white_king, get<11>(test_pair.first));
    EXPECT_EQ(fen_board.turn, get<12>(test_pair.first));
    EXPECT_EQ(fen_board.black_castle, get<13>(test_pair.first));
    EXPECT_EQ(fen_board.white_castle, get<14>(test_pair.first));
    EXPECT_EQ(fen_board.en_passant, get<15>(test_pair.first));
    EXPECT_EQ(fen_board.halfmove, get<16>(test_pair.first));
    EXPECT_EQ(fen_board.fullmove, get<17>(test_pair.first));
}

INSTANTIATE_TEST_CASE_P(tGameboardFenNotationParams,  
                        tGameboardFenNotation,  
                        testing::Values( make_pair(make_tuple(71776119061217280ULL, 65280ULL, 9295429630892703744ULL, 129ULL, 4755801206503243776ULL, 66ULL, 2594073385365405696ULL, 36ULL, 1152921504606846976ULL, 16ULL, 576460752303423488ULL, 8ULL, ShumiChess::Color::WHITE, 3, 3, 0ULL, 0, 1),
                                                   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
                                         make_pair(make_tuple(62769057245429760ULL, 134280960ULL, 9295429630892703744ULL, 129ULL, 4755801206503243776ULL, 66ULL, 2594073385365405696ULL, 36ULL, 1152921504606846976ULL, 16ULL, 576460752303423488ULL, 8ULL, ShumiChess::Color::WHITE, 3, 3, 1ULL << 45, 0, 2),
                                                   "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2"),
                                         make_pair(make_tuple(0ULL, 2048ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 576460752303423488ULL, 8ULL, ShumiChess::Color::WHITE, 0, 0, 0, 5, 39),
                                                   "4k3/8/8/8/8/8/4P3/4K3 w - - 5 39"),
                                         make_pair(make_tuple(19888039337656320ULL, 584419722240ULL, 9223373136366403584ULL, 132ULL, 4503599627370496ULL, 4456448ULL, 2251799947902976ULL, 2048ULL, 1152921504606846976ULL, 4096ULL, 576460752303423488ULL, 512ULL, ShumiChess::Color::WHITE, 2, 0, 0, 2, 17),
                                                   "r2qk3/1p1nbpp1/p1p1p2r/P2pP2p/3Pb1P1/1NP2N1P/1P1QBPK1/R4R2 w q - 2 17"),
                                        // make_pair(make_tuple(bp, wp, br, wr, bn, wn, bb, wb, bq, wq, bk, wk, ShumiChess::Color::BLACK, 0, 0, 0, 1, 41),
                                                //    "R7/3P1pkp/3Nnp2/6p1/2B5/7P/1Pr5/7K b - - 1 41"),

                                                   make_pair(make_tuple(62769057245429760ULL, 134280960ULL, 9295429630892703744ULL, 129ULL, 4755801206503243776ULL, 66ULL, 2594073385365405696ULL, 36ULL, 1152921504606846976ULL, 16ULL, 576460752303423488ULL, 8ULL, ShumiChess::Color::WHITE, 3, 3, 1ULL << 45, 0, 2),
                                                   "rnb1kb1r/pppp1p1p/5qp1/8/2P5/1P3P2/P2PPP1P/RN1QKB1R w KQkq - 0 7"),
                                                   make_pair(make_tuple(62769057245429760ULL, 134280960ULL, 9295429630892703744ULL, 129ULL, 4755801206503243776ULL, 66ULL, 2594073385365405696ULL, 36ULL, 1152921504606846976ULL, 16ULL, 576460752303423488ULL, 8ULL, ShumiChess::Color::WHITE, 3, 3, 1ULL << 45, 0, 2),
                                                   "r1bq1rk1/1p1nbppp/p3p2B/4P3/3N2Q1/1PN5/1PP3PP/R3K2R b KQ - 4 14") ));
                                                    
                                                    ull test_val = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
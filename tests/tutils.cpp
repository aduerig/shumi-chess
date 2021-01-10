#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "globals.hpp"
#include "utility.hpp"

using namespace std;

TEST(InverseColor, GetOpposingColorTest) {
    EXPECT_EQ(utility::representation::get_opposite_color(ShumiChess::Color::WHITE),
              ShumiChess::Color::BLACK);
    ASSERT_EQ(utility::representation::get_opposite_color(ShumiChess::Color::BLACK),
              ShumiChess::Color::WHITE);
}

TEST(MoveStringConversion, MoveToString1) {
    // squares 0 (h1), and 1 (g1)
    ShumiChess::Move test_move = {ShumiChess::Color::WHITE, ShumiChess::Piece::PAWN, 1ULL << 0, 1ULL << 1};
    string converted = utility::representation::move_to_string(test_move);
    ASSERT_EQ("h1g1", converted);
}

TEST(MoveStringConversion, MoveToString2) {
    // squares 8 (h2), and 63 (a8)
    ShumiChess::Move test_move = {ShumiChess::Color::WHITE, ShumiChess::Piece::PAWN, 1ULL << 8, 1ULL << 63};
    string converted = utility::representation::move_to_string(test_move);
    ASSERT_EQ("h2a8", converted);
}


TEST(BitTests, LsbAndPop) {
    ull test_board        = 0b00000000'11101101'00000000'00000000'00000001'00000000'00000000'00000010;
    ull expected_board    = 0b00000000'11101101'00000000'00000000'00000001'00000000'00000000'00000000;
    ull expected_popped   = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000010;
    ull popped_square = utility::bit::lsb_and_pop(test_board);
    EXPECT_EQ(expected_popped, popped_square);
    ASSERT_EQ(expected_board, test_board);
}



typedef pair<string, ull> acn_to_bitboard_test_type;
class tAcnBitboardConversion : public testing::TestWithParam<acn_to_bitboard_test_type> {};
TEST_P(tAcnBitboardConversion, AcnToBitboardConversion) {
    const auto acn_notation = get<0>(GetParam());
    const auto expected_numeric = get<1>(GetParam());

    ASSERT_EQ(utility::representation::acn_to_bitboard_conversion(acn_notation), expected_numeric);
}
vector<acn_to_bitboard_test_type> acn_to_bitboard_test_data = { make_pair("a1", 1ull << 7),
                                                                make_pair("b2", 1ull << 14),
                                                                make_pair("c3", 1ull << 21),
                                                                make_pair("d4", 1ull << 28),
                                                                make_pair("e5", 1ull << 35),
                                                                make_pair("f6", 1ull << 42),
                                                                make_pair("g7", 1ull << 49),
                                                                make_pair("h8", 1ull << 56),
                                                                make_pair("c1", 1ull << 5),
                                                                make_pair("c2", 1ull << 13),
                                                                make_pair("c4", 1ull << 29),
                                                                make_pair("c5", 1ull << 37),
                                                                make_pair("c6", 1ull << 45),
                                                                make_pair("c7", 1ull << 53),
                                                                make_pair("c8", 1ull << 61),
                                                                make_pair("a5", 1ull << 39),
                                                                make_pair("b5", 1ull << 38),
                                                                make_pair("d5", 1ull << 36),
                                                                make_pair("f5", 1ull << 34),
                                                                make_pair("g5", 1ull << 33),
                                                                make_pair("h5", 1ull << 32) };
INSTANTIATE_TEST_CASE_P(tAcnBitboardConversionParam,  
                        tAcnBitboardConversion,  
                        testing::ValuesIn( acn_to_bitboard_test_data ));

// opposite of above
typedef pair<ull, string> bitboard_to_acn_test_type;
class tAcnBitboardConversion2 : public testing::TestWithParam<bitboard_to_acn_test_type> {};
TEST_P(tAcnBitboardConversion2, BitboardToAcnConversion) {
    const auto bitboard = get<0>(GetParam());
    const auto expected_acn = get<1>(GetParam());

    ASSERT_EQ(expected_acn, utility::representation::bitboard_to_acn_conversion(bitboard));
}
vector<bitboard_to_acn_test_type> bitboard_to_acn_test_data = { make_pair(1ull << 7, "a1"),
                                                                make_pair(1ull << 14, "b2"),
                                                                make_pair(1ull << 21, "c3"),
                                                                make_pair(1ull << 28, "d4"),
                                                                make_pair(1ull << 35, "e5"),
                                                                make_pair(1ull << 42, "f6"),
                                                                make_pair(1ull << 49, "g7"),
                                                                make_pair(1ull << 56, "h8"),
                                                                make_pair(1ull << 5, "c1"),
                                                                make_pair(1ull << 13, "c2"),
                                                                make_pair(1ull << 29, "c4"),
                                                                make_pair(1ull << 37, "c5"),
                                                                make_pair(1ull << 45, "c6"),
                                                                make_pair(1ull << 53, "c7"),
                                                                make_pair(1ull << 61, "c8"),
                                                                make_pair(1ull << 39, "a5"),
                                                                make_pair(1ull << 38, "b5"),
                                                                make_pair(1ull << 36, "d5"),
                                                                make_pair(1ull << 34, "f5"),
                                                                make_pair(1ull << 33, "g5"),
                                                                make_pair(1ull << 32, "h5") };
INSTANTIATE_TEST_CASE_P(tAcnBitboardConversionParam2,  
                        tAcnBitboardConversion2,  
                        testing::ValuesIn( bitboard_to_acn_test_data ));

TEST(tStringSplit, DefaultDelim) {
    string query = "a b c";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query));
}

typedef tuple<string, string, vector<string>> string_split_test_type;
class tStringSplit : public testing::TestWithParam<string_split_test_type> {};
TEST_P(tStringSplit, StringSplitMultipleDelim) {
    const auto query = get<0>(GetParam());
    const auto delim = get<1>(GetParam());
    const auto sol = get<2>(GetParam());

    ASSERT_EQ(sol, utility::string::split(query, delim));
}
vector<string_split_test_type> string_split_test_data = { make_tuple("a/b/c", "/", vector<string>{"a", "b", "c"}),
                                                          make_tuple("a->b->c", "->", vector<string>{"a", "b", "c"}), 
                                                          make_tuple("a->b->c->", "->", vector<string>{"a", "b", "c"}),
                                                          make_tuple("a->b->->c->", "->", vector<string>{"a", "b", "c"}),
                                                          make_tuple("a->b->c->->", "->", vector<string>{"a", "b", "c"}),
                                                          make_tuple("abc", "->", vector<string>{"abc"}),
                                                          make_tuple("This is | a test | with spaces.", "|", vector<string>{"This is ", " a test ", " with spaces."}) };
INSTANTIATE_TEST_CASE_P(tStringSplitParam,  
                        tStringSplit,  
                        testing::ValuesIn( string_split_test_data ));

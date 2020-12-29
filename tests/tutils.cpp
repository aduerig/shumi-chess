#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "globals.hpp"
#include "utility.hpp"

using namespace std;



TEST(MoveStringConversion, MoveToString1) {
    // squares 0 (h1), and 1 (g1)
    ShumiChess::Move test_move = {1ULL << 0, 1ULL << 1, ShumiChess::Piece::PAWN, ShumiChess::Color::WHITE};
    string converted = utility::representation::move_to_string(test_move);
    ASSERT_EQ("h1g1", converted);
}

TEST(MoveStringConversion, MoveToString2) {
    // squares 8 (h2), and 63 (a8)
    ShumiChess::Move test_move = {1ULL << 8, 1ULL << 63, ShumiChess::Piece::PAWN, ShumiChess::Color::WHITE};
    string converted = utility::representation::move_to_string(test_move);
    ASSERT_EQ("h2a8", converted);
}


TEST(BitTests, LsbAndPop) {
    ull test_board        = 0b00000000'11101101'00000000'00000000'00000001'00000000'00000000'00000010;
    ull expected_board    = 0b00000000'11101101'00000000'00000000'00000001'00000000'00000000'00000000;
    ull expected_popped   = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000010;
    ull popped_square = utility::representation::lsb_and_pop(test_board);
    ASSERT_EQ(expected_popped, popped_square);
    ASSERT_EQ(expected_board, test_board);
}



typedef pair<string, ull> acn_to_bit_test_type;
class tAncBitConversion : public testing::TestWithParam<acn_to_bit_test_type> {};
TEST_P(tAncBitConversion, AncToBitboardConversion) {
    const auto anc_notation = get<0>(GetParam());
    const auto expected_numeric = get<1>(GetParam());

    ASSERT_EQ(utility::representation::acn_to_bit_conversion(anc_notation), expected_numeric);
}
vector<acn_to_bit_test_type> anc_to_bit_test_data = { make_pair("a1", 1ull << 7),
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
INSTANTIATE_TEST_CASE_P(tAncBitConversionParam,  
                        tAncBitConversion,  
                        testing::ValuesIn( anc_to_bit_test_data ));

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

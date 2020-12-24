#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <vector>

#include <utility.hpp>

using namespace std;

typedef tuple<string, string, vector<string>> string_split_test_param;
class tStringSplit : public testing::TestWithParam<string_split_test_param> {};
TEST_P(tStringSplit, StringSplitMultipleDelim) {
    const auto query = get<0>(GetParam());
    const auto delim = get<1>(GetParam());
    const auto sol = get<2>(GetParam());

    ASSERT_EQ(sol, utility::string::split(query, delim));
}
vector<string_split_test_param> string_split_test_data = { make_tuple("a/b/c", "/", {"a", "b", "c"}),
                                                           make_tuple("a->b->c", "->", {"a", "b", "c"}), 
                                                           make_tuple("a->b->c->", "->", {"a", "b", "c"}),
                                                           make_tuple("a->b->->c->", "->", {"a", "b", "c"}),
                                                           make_tuple("a->b->c->->", "->", {"a", "b", "c"}),
                                                           make_tuple("abc", "->", {"abc"}),
                                                           make_tuple("This is | a test | with spaces.", "|", {"This is ", " a test ", " with spaces."}) };
INSTANTIATE_TEST_CASE_P(tStringSplitParam,  
                        tStringSplit,  
                        testing::ValuesIn( string_split_test_data ));

TEST(tStringSplit, DefaultDelim) {
    string query = "a b c";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query));
}
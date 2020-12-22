#include <gtest/gtest.h>
#include <string>
#include <vector>

#include <utility.hpp>

using std::string;
using std::vector; 

TEST(SanityTests, TrueIsTrue) {
    ASSERT_EQ(true, true);
}

TEST(StringSplit, BasicDelim) {
    string query = "a/b/c";
    string del = "/";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, DefaultDelim) {
    string query = "a b c";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query));
}

TEST(StringSplit, MultiDelim) {
    string query = "a->b->c";
    string del = "->";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, EndDelim) {
    string query = "a->b->c->";
    string del = "->";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, DoubleDelim) {
    string query = "a->b->->c->";
    string del = "->";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, DoubleEndDelim) {
    string query = "a->b->c->->";
    string del = "->";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, DelimNoAppear) {
    string query = "abc";
    string del = "->";
    const vector<string> sol = {"abc"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, SpacesWNonSpaceDelim) {
    string query = "This is | a test | with spaces.";
    string del = "|";
    const vector<string> sol = {"This is ", " a test ", " with spaces."};
    ASSERT_EQ(sol, utility::string::split(query, del));
}
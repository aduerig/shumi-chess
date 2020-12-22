#include <gtest/gtest.h>
#include <string>
#include <vector>

#include <utility.hpp>

using std::string;
using std::vector; 

TEST(SanityTests, TrueIsTrue) {
    ASSERT_EQ(true, true);
}

TEST(StringSplit, basicDelim) {
    string query = "a/b/c";
    string del = "/";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, defaultDelim) {
    string query = "a b c";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query));
}

TEST(StringSplit, multiDelim) {
    string query = "a->b->c";
    string del = "->";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, endDelim) {
    string query = "a->b->c->";
    string del = "->";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, doubleDelim) {
    string query = "a->b->->c->";
    string del = "->";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, doubleEndDelim) {
    string query = "a->b->c->->";
    string del = "->";
    const vector<string> sol = {"a", "b", "c"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, delimNoAppear) {
    string query = "abc";
    string del = "->";
    const vector<string> sol = {"abc"};
    ASSERT_EQ(sol, utility::string::split(query, del));
}

TEST(StringSplit, spacesWNonSpaceDelim) {
    string query = "This is | a test | with spaces.";
    string del = "|";
    const vector<string> sol = {"This is ", " a test ", " with spaces."};
    ASSERT_EQ(sol, utility::string::split(query, del));
}
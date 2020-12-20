#include <gtest/gtest.h>

// #include "../src/engine.hpp"
#include <engine.hpp>

TEST(SanityTests, TrueIsTrue) {
    ASSERT_EQ(true, true);
}

TEST(SanityTests, CanICallLibrary) {
    ASSERT_EQ(return_28(), 28);
}
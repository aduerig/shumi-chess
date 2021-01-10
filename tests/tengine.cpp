#include <gtest/gtest.h>
#include <engine.hpp>
#include <globals.hpp>

TEST(Setup, WhiteGoesFirst) {
    ShumiChess::Engine test_engine;
    ASSERT_EQ(test_engine.game_board.turn, ShumiChess::Color::WHITE);
}

TEST(EngineMoveStorage, PushingMoves) {

}
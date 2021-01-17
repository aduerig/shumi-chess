#include <gtest/gtest.h>

#include "engine.hpp"
#include "gameboard.hpp"
#include "globals.hpp"
#include "test_helper_fcns.hpp"

TEST(Setup, WhiteGoesFirst) {
    ShumiChess::Engine test_engine;
    ASSERT_EQ(test_engine.game_board.turn, ShumiChess::Color::WHITE);
}

TEST(EngineMoveStorage, PushingMoves) {
    using namespace ShumiChess;

    Engine test_engine;
    GameBoard expected_board;
    EXPECT_EQ(GameBoard(), test_engine.game_board);

    auto temp_move_0 = (Move{WHITE, PAWN, 1ULL<<11, 1ULL<<27});
    temp_move_0.en_passent = 1ULL<<19;
    test_engine.push(temp_move_0);
    EXPECT_EQ(GameBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<57, 1ULL<<42});
    EXPECT_EQ(GameBoard("rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2"), test_engine.game_board);

    test_engine.push(Move{WHITE, PAWN, 1ULL<<27, 1ULL<<35});
    EXPECT_EQ(GameBoard("rnbqkb1r/pppppppp/5n2/4P3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2"), test_engine.game_board);

    auto temp_move_1 = Move{BLACK, PAWN, 1ULL<<52, 1ULL<<36};
    temp_move_1.en_passent = 1ULL<<44;
    test_engine.push(temp_move_1);
    EXPECT_EQ(GameBoard("rnbqkb1r/ppp1pppp/5n2/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3"), test_engine.game_board);
    
    auto temp_move_20 = Move{WHITE, PAWN, 1ULL<<35, 1ULL<<44, PAWN};
    temp_move_20.is_en_passent_capture = 1;
    test_engine.push(temp_move_20);
    expected_board = GameBoard("rnbqkb1r/ppp1pppp/3P1n2/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
    EXPECT_EQ(expected_board, test_engine.game_board); //!Black pawns wrong

    test_engine.push(Move{BLACK, PAWN, 1ULL<<48, 1ULL<<40});
    EXPECT_EQ(GameBoard("rnbqkb1r/ppp1pp1p/3P1np1/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 4"), test_engine.game_board);

    test_engine.push(Move{WHITE, PAWN, 1ULL<<44, 1ULL<<53, PAWN});
    EXPECT_EQ(GameBoard("rnbqkb1r/ppP1pp1p/5np1/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 4"), test_engine.game_board);

    test_engine.push(Move{BLACK, BISHOP, 1ULL<<58, 1ULL<<40});
    EXPECT_EQ(GameBoard("rnbqk2r/ppP1pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 1 5"), test_engine.game_board);

    test_engine.push(Move{WHITE, PAWN, 1ULL<<53, 1ULL<<62, KNIGHT, QUEEN});
    EXPECT_EQ(GameBoard("rQbqk2r/pp2pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 5"), test_engine.game_board);

    auto temp_move_2 = Move{BLACK, KING, 1ULL<<59, 1ULL<<57};
    temp_move_2.black_castle = 0;
    test_engine.push(temp_move_2);
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR w KQ - 1 6"), test_engine.game_board);

    test_engine.push(Move{WHITE, PAWN, 1ULL<<15, 1ULL<<23});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P7/1PPP1PPP/RNBQKBNR b KQ - 0 6"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<42, 1ULL<<36});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P7/1PPP1PPP/RNBQKBNR w KQ - 1 7"), test_engine.game_board);

    test_engine.push(Move{WHITE, PAWN, 1ULL<<12, 1ULL<<20});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/1PP2PPP/RNBQKBNR b KQ - 0 7"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<36, 1ULL<<42});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/1PP2PPP/RNBQKBNR w KQ - 1 8"), test_engine.game_board);

    auto temp_move_3 = Move{WHITE, ROOK, 1ULL<<7, 1ULL<<17};
    temp_move_3.white_castle = 1;
    test_engine.push(temp_move_3);
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPP2PPP/1NBQKBNR b K - 2 8"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<42, 1ULL<<36});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPP2PPP/1NBQKBNR w K - 3 9"), test_engine.game_board);

    test_engine.push(Move{WHITE, QUEEN, 1ULL<<4, 1ULL<<11});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPP1QPPP/1NB1KBNR b K - 4 9"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<36, 1ULL<<42});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPP1QPPP/1NB1KBNR w K - 5 10"), test_engine.game_board);
    
    test_engine.push(Move{WHITE, BISHOP, 1ULL<<5, 1ULL<<12});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPPBQPPP/1N2KBNR b K - 6 10"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<42, 1ULL<<36});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPPBQPPP/1N2KBNR w K - 7 11"), test_engine.game_board);
    
    test_engine.push(Move{WHITE, QUEEN, 1ULL<<11, 1ULL<<35});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P2P4/RPPB1PPP/1N2KBNR b K - 8 11"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<36, 1ULL<<42});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P2P4/RPPB1PPP/1N2KBNR w K - 9 12"), test_engine.game_board);

    test_engine.push(Move{WHITE, BISHOP, 1ULL<<12, 1ULL<<21});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP4/RPP2PPP/1N2KBNR b K - 10 12"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<42, 1ULL<<36});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP4/RPP2PPP/1N2KBNR w K - 11 13"), test_engine.game_board);

    test_engine.push(Move{WHITE, PAWN, 1ULL<<8, 1ULL<<16});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP3P/RPP2PP1/1N2KBNR b K - 0 13"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<36, 1ULL<<42});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP3P/RPP2PP1/1N2KBNR w K - 1 14"), test_engine.game_board);
    
    test_engine.push(Move{WHITE, KING, 1ULL<<3, 1ULL<<11});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP3P/RPP1KPP1/1N3BNR b - - 2 14"), test_engine.game_board);

    test_engine.push(Move{BLACK, KNIGHT, 1ULL<<42, 1ULL<<36});
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP3P/RPP1KPP1/1N3BNR w - - 3 15"), test_engine.game_board);

    test_engine.push(Move{WHITE, QUEEN, 1ULL<<35, 1ULL<<56});
    EXPECT_EQ(GameBoard("rQbq1rkQ/pp2pp1p/6pb/3n4/8/P1BP3P/RPP1KPP1/1N3BNR b - - 4 15"), test_engine.game_board);
}
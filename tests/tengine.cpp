#include <gtest/gtest.h>

#include <stack>

#include "engine.hpp"
#include "gameboard.hpp"
#include "globals.hpp"
#include "test_helper_fcns.hpp"

TEST(Setup, WhiteGoesFirst) {
    ShumiChess::Engine test_engine;
    ASSERT_EQ(test_engine.game_board.turn, ShumiChess::Color::WHITE);
}

vector<string> white_in_check_black_is_safe_fens = {"7k/8/8/3p4/4K3/8/8/8 w - - 0 1",
                                                    "8/5k2/5P2/3p4/4K3/8/8/8 w - - 0 1",
                                                    "8/8/3kr3/8/4K3/8/8/8 w - - 0 1",
                                                    "8/R7/3kr3/R7/3rK3/R7/3R4/2R1R3 w - - 0 1",
                                                    "8/8/2k5/8/6n1/4K3/8/8 w - - 0 1",
                                                    "8/8/2k5/5n2/8/4K3/8/8 w - - 0 1",
                                                    "8/8/2k5/3n4/8/4K3/8/8 w - - 0 1",
                                                    "8/8/2k5/8/2n5/4K3/8/8 w - - 0 1",
                                                    "8/8/2k5/8/8/4K3/2n5/8 w - - 0 1",
                                                    "8/8/2k5/8/8/4K3/8/3n4 w - - 0 1",
                                                    "8/8/2k5/8/8/4K3/8/5n2 w - - 0 1",
                                                    "8/8/2k5/8/8/4K3/6n1/8 w - - 0 1",
                                                    "8/1NNN4/1NkN4/1NNN4/3n4/4K3/6n1/8 w - - 0 1",
                                                    "8/8/2k5/8/8/4K3/5b2/8 w - - 0 1",
                                                    "8/8/B1k5/4B3/2BB4/4K3/5b2/8 w - - 0 1",
                                                    "8/8/2k5/4q3/8/4K3/8/8 w - - 0 1",
                                                    "8/7Q/2k5/Q4Q2/1Q6/3QK3/8/6q1 w - - 0 1"};
class IsKingInCheck : public testing::TestWithParam<string> {}; 
TEST_P(IsKingInCheck, IsKingInCheck) {
    using namespace ShumiChess;
    Engine black_safe_white_check_engine{GetParam()};

    //string debugStr;
    ASSERT_TRUE (black_safe_white_check_engine.is_king_in_check2_t<Color::WHITE>());
    ASSERT_FALSE(black_safe_white_check_engine.is_king_in_check2_t<Color::BLACK>());

}
INSTANTIATE_TEST_SUITE_P(IsKingInCheck, IsKingInCheck, testing::ValuesIn(white_in_check_black_is_safe_fens));

TEST(EngineMoveStorage, PushingMoves) {
    using namespace ShumiChess;

    Engine test_engine;
    EXPECT_EQ(GameBoard(), test_engine.game_board);

    Move temp_move_0 = MoveSet(WHITE, PAWN, 1ULL <<11, 1ULL <<27);
    temp_move_0.en_passant_rights = 1ULL <<19;
    test_pushMove(test_engine,temp_move_0);
    EXPECT_EQ(GameBoard("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<57, 1ULL <<42));
    utility::representation::highlight_board_differences(GameBoard("rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2"), test_engine.game_board);
    EXPECT_EQ(GameBoard("rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<27, 1ULL <<35));
    EXPECT_EQ(GameBoard("rnbqkb1r/pppppppp/5n2/4P3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2"), test_engine.game_board);

    auto temp_move_1 = MoveSet(BLACK, PAWN, 1ULL <<52, 1ULL <<36);
    temp_move_1.en_passant_rights = 1ULL <<44;
    test_pushMove(test_engine,temp_move_1);
    EXPECT_EQ(GameBoard("rnbqkb1r/ppp1pppp/5n2/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3"), test_engine.game_board);
    
    auto temp_move_20 = MoveSet2(WHITE, PAWN, 1ULL <<35, 1ULL <<44, PAWN);
    temp_move_20.is_en_passent_capture = 1;
    test_pushMove(test_engine,temp_move_20);
    EXPECT_EQ(GameBoard("rnbqkb1r/ppp1pppp/3P1n2/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, PAWN, 1ULL <<49, 1ULL <<41));
    EXPECT_EQ(GameBoard("rnbqkb1r/ppp1pp1p/3P1np1/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 4"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet2(WHITE, PAWN, 1ULL <<44, 1ULL <<53, PAWN));
    EXPECT_EQ(GameBoard("rnbqkb1r/ppP1pp1p/5np1/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 4"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, BISHOP, 1ULL <<58, 1ULL <<40));
    EXPECT_EQ(GameBoard("rnbqk2r/ppP1pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 1 5"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet3(WHITE, PAWN, 1ULL <<53, 1ULL <<62, KNIGHT, QUEEN));
    EXPECT_EQ(GameBoard("rQbqk2r/pp2pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 5"), test_engine.game_board);

    auto temp_move_2 = MoveSet(BLACK, KING, 1ULL <<59, 1ULL <<57);
    temp_move_2.black_castle_rights = 0;
    temp_move_2.is_castle_move = true;
    test_pushMove(test_engine,temp_move_2);
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR w KQ - 1 6"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<15, 1ULL <<23));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P7/1PPP1PPP/RNBQKBNR b KQ - 0 6"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P7/1PPP1PPP/RNBQKBNR w KQ - 1 7"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<12, 1ULL <<20));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/1PP2PPP/RNBQKBNR b KQ - 0 7"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<36, 1ULL <<42));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/1PP2PPP/RNBQKBNR w KQ - 1 8"), test_engine.game_board);

    auto temp_move_3 = MoveSet(WHITE, ROOK, 1ULL <<7, 1ULL <<15);
    temp_move_3.white_castle_rights = 1;
    test_pushMove(test_engine,temp_move_3);
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPP2PPP/1NBQKBNR b K - 2 8"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPP2PPP/1NBQKBNR w K - 3 9"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(WHITE, QUEEN, 1ULL <<4, 1ULL <<11));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPP1QPPP/1NB1KBNR b K - 4 9"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<36, 1ULL <<42));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPP1QPPP/1NB1KBNR w K - 5 10"), test_engine.game_board);
    
    test_pushMove(test_engine,MoveSet(WHITE, BISHOP, 1ULL <<5, 1ULL <<12));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPPBQPPP/1N2KBNR b K - 6 10"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPPBQPPP/1N2KBNR w K - 7 11"), test_engine.game_board);
    
    test_pushMove(test_engine,MoveSet(WHITE, QUEEN, 1ULL <<11, 1ULL <<35));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P2P4/RPPB1PPP/1N2KBNR b K - 8 11"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<36, 1ULL <<42));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P2P4/RPPB1PPP/1N2KBNR w K - 9 12"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(WHITE, BISHOP, 1ULL <<12, 1ULL <<21));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP4/RPP2PPP/1N2KBNR b K - 10 12"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP4/RPP2PPP/1N2KBNR w K - 11 13"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<8, 1ULL <<16));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP3P/RPP2PP1/1N2KBNR b K - 0 13"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<36, 1ULL <<42));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP3P/RPP2PP1/1N2KBNR w K - 1 14"), test_engine.game_board);
    
    auto temp_move_4 = MoveSet(WHITE, KING, 1ULL <<3, 1ULL <<11);
    temp_move_4.white_castle_rights = 0;
    test_pushMove(test_engine,temp_move_4);
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP3P/RPP1KPP1/1N3BNR b - - 2 14"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    EXPECT_EQ(GameBoard("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP3P/RPP1KPP1/1N3BNR w - - 3 15"), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(WHITE, QUEEN, 1ULL <<35, 1ULL <<56));
    EXPECT_EQ(GameBoard("rQbq1rkQ/pp2pp1p/6pb/3n4/8/P1BP3P/RPP1KPP1/1N3BNR b - - 4 15"), test_engine.game_board);
}

TEST(EngineMoveStorage, PushPopMini) {
    using namespace ShumiChess;

    Engine test_engine;
    std::stack<GameBoard> expected_game_history;

    Move temp_move_0 = (MoveSet(WHITE, PAWN, 1ULL <<11, 1ULL <<27));
    temp_move_0.en_passant_rights = 1ULL <<19;
    test_pushMove(test_engine,temp_move_0);
    expected_game_history.emplace("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    EXPECT_EQ(expected_game_history.top(), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<57, 1ULL <<42));
    expected_game_history.emplace("rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2");
    EXPECT_EQ(expected_game_history.top(), test_engine.game_board);

    auto temp_move_1 = MoveSet(WHITE, KING, 1ULL <<3, 1ULL <<11);
    temp_move_1.white_castle_rights = 0;
    test_pushMove(test_engine,temp_move_1);
    expected_game_history.emplace("rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPPKPPP/RNBQ1BNR b kq - 2 2");
    EXPECT_EQ(expected_game_history.top(), test_engine.game_board);

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<57));
    expected_game_history.emplace("rnbqkbnr/pppppppp/8/8/4P3/8/PPPPKPPP/RNBQ1BNR w kq - 3 3");
    EXPECT_EQ(expected_game_history.top(), test_engine.game_board);

    while (!expected_game_history.empty())
    {
        EXPECT_EQ(expected_game_history.top(), test_engine.game_board);
        utility::representation::highlight_board_differences(expected_game_history.top(), test_engine.game_board);
        test_popMove(test_engine);
        expected_game_history.pop();
    }
}

TEST(EngineMoveStorage, EngineZobristConsistancy) {
    using namespace ShumiChess;
    Engine test_engine;

    uint64_t starting_zobrist = test_engine.game_board.zobrist_key;
    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<11, 1ULL <<27));
    uint64_t mid_zobrist = test_engine.game_board.zobrist_key;
    test_popMove(test_engine);

    uint64_t ending_zobrist = test_engine.game_board.zobrist_key;

    EXPECT_EQ(starting_zobrist, ending_zobrist);
}

//TODO use a different game to test pop (this one is same game as pushMove), more variety
//? ^ Test different castling rook
//? Maybe just more mini tests
TEST(EngineMoveStorage, PoppingMoves) {
    using namespace ShumiChess;

    Engine test_engine;
    std::stack<GameBoard> expected_game_history;

    auto temp_move_0 = (MoveSet(WHITE, PAWN, 1ULL <<11, 1ULL <<27));
    temp_move_0.en_passant_rights = 1ULL <<19;
    test_pushMove(test_engine,temp_move_0);
    expected_game_history.emplace("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<57, 1ULL <<42));
    expected_game_history.emplace("rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2");

    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<27, 1ULL <<35));
    expected_game_history.emplace("rnbqkb1r/pppppppp/5n2/4P3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");

    auto temp_move_1 = MoveSet(BLACK, PAWN, 1ULL <<52, 1ULL <<36);
    temp_move_1.en_passant_rights = 1ULL <<44;
    test_pushMove(test_engine,temp_move_1);
    expected_game_history.emplace("rnbqkb1r/ppp1pppp/5n2/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
    
    auto temp_move_20 = MoveSet2(WHITE, PAWN, 1ULL <<35, 1ULL <<44, PAWN);
    temp_move_20.is_en_passent_capture = 1;
    test_pushMove(test_engine,temp_move_20);
    expected_game_history.emplace("rnbqkb1r/ppp1pppp/3P1n2/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");

    test_pushMove(test_engine,MoveSet(BLACK, PAWN, 1ULL <<49, 1ULL <<41));
    expected_game_history.emplace("rnbqkb1r/ppp1pp1p/3P1np1/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 4");

    test_pushMove(test_engine,MoveSet2(WHITE, PAWN, 1ULL <<44, 1ULL <<53, PAWN));
    expected_game_history.emplace("rnbqkb1r/ppP1pp1p/5np1/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 4");

    test_pushMove(test_engine,MoveSet(BLACK, BISHOP, 1ULL <<58, 1ULL <<40));
    expected_game_history.emplace("rnbqk2r/ppP1pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR w KQkq - 1 5");

    test_pushMove(test_engine,MoveSet3(WHITE, PAWN, 1ULL <<53, 1ULL <<62, KNIGHT, QUEEN));
    expected_game_history.emplace("rQbqk2r/pp2pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 5");

    auto temp_move_2 = MoveSet(BLACK, KING, 1ULL <<59, 1ULL <<57);
    temp_move_2.black_castle_rights = 0;
    temp_move_2.is_castle_move = true;
    test_pushMove(test_engine,temp_move_2);
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/8/8/8/PPPP1PPP/RNBQKBNR w KQ - 1 6");

    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<15, 1ULL <<23));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/8/8/P7/1PPP1PPP/RNBQKBNR b KQ - 0 6");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3n4/8/P7/1PPP1PPP/RNBQKBNR w KQ - 1 7");

    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<12, 1ULL <<20));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/1PP2PPP/RNBQKBNR b KQ - 0 7");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<36, 1ULL <<42));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/1PP2PPP/RNBQKBNR w KQ - 1 8");

    auto temp_move_3 = MoveSet(WHITE, ROOK, 1ULL <<7, 1ULL <<15);
    temp_move_3.white_castle_rights = 1;
    test_pushMove(test_engine,temp_move_3);
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPP2PPP/1NBQKBNR b K - 2 8");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPP2PPP/1NBQKBNR w K - 3 9");

    test_pushMove(test_engine,MoveSet(WHITE, QUEEN, 1ULL <<4, 1ULL <<11));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPP1QPPP/1NB1KBNR b K - 4 9");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<36, 1ULL <<42));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPP1QPPP/1NB1KBNR w K - 5 10");
    
    test_pushMove(test_engine,MoveSet(WHITE, BISHOP, 1ULL <<5, 1ULL <<12));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/8/8/P2P4/RPPBQPPP/1N2KBNR b K - 6 10");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3n4/8/P2P4/RPPBQPPP/1N2KBNR w K - 7 11");
    
    test_pushMove(test_engine,MoveSet(WHITE, QUEEN, 1ULL <<11, 1ULL <<35));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P2P4/RPPB1PPP/1N2KBNR b K - 8 11");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<36, 1ULL <<42));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P2P4/RPPB1PPP/1N2KBNR w K - 9 12");

    test_pushMove(test_engine,MoveSet(WHITE, BISHOP, 1ULL <<12, 1ULL <<21));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP4/RPP2PPP/1N2KBNR b K - 10 12");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP4/RPP2PPP/1N2KBNR w K - 11 13");

    test_pushMove(test_engine,MoveSet(WHITE, PAWN, 1ULL <<8, 1ULL <<16));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP3P/RPP2PP1/1N2KBNR b K - 0 13");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<36, 1ULL <<42));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP3P/RPP2PP1/1N2KBNR w K - 1 14");
    
    auto temp_move_4 = MoveSet(WHITE, KING, 1ULL <<3, 1ULL <<11);
    temp_move_4.white_castle_rights = 0;
    test_pushMove(test_engine,temp_move_4);
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/5npb/4Q3/8/P1BP3P/RPP1KPP1/1N3BNR b - - 2 14");

    test_pushMove(test_engine,MoveSet(BLACK, KNIGHT, 1ULL <<42, 1ULL <<36));
    expected_game_history.emplace("rQbq1rk1/pp2pp1p/6pb/3nQ3/8/P1BP3P/RPP1KPP1/1N3BNR w - - 3 15");

    test_pushMove(test_engine,MoveSet(WHITE, QUEEN, 1ULL <<35, 1ULL <<56));
    expected_game_history.emplace("rQbq1rkQ/pp2pp1p/6pb/3n4/8/P1BP3P/RPP1KPP1/1N3BNR b - - 4 15");

    while (!expected_game_history.empty())
    {
        EXPECT_EQ(expected_game_history.top(), test_engine.game_board);
        utility::representation::highlight_board_differences(expected_game_history.top(), test_engine.game_board);
        test_popMove(test_engine);
        expected_game_history.pop();
    }
}
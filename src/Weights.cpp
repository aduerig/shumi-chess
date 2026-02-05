

#include "weights.hpp"


int EvalWghts[LAST_VALUE+1] = {0};

/*

void init_eval_weights()
{
    EvalWghts[HAS_CASTLED]              = EvalW::HAS_CASTLED_WGHT;
    EvalWghts[CAN_CASTLE]               = EvalW::CAN_CASTLE_WGHT;
    EvalWghts[ISOLANI]                  = EvalW::ISOLANI_WGHT;
    EvalWghts[ISOLANI_ROOK]             = EvalW::ISOLANI_ROOK_WGHT;
    EvalWghts[PAWN_HOLE]                = EvalW::PAWN_HOLE_WGHT;
    EvalWghts[KNIGHT_HOLE]              = EvalW::KNIGHT_HOLE_WGHT;
    EvalWghts[DOUBLED]                  = EvalW::DOUBLED_WGHT;
    EvalWghts[DOUBLED_ROOK]             = EvalW::DOUBLED_ROOK_WGHT;
    EvalWghts[DOUBLED_OPEN_FILE]        = EvalW::DOUBLED_OPEN_FILE_WGHT;
    EvalWghts[PASSED_PAWN_SLOPE]        = EvalW::PASSED_PAWN_SLOPE_WGHT;
    EvalWghts[PASSED_PAWN_YINRCPT]      = EvalW::PASSED_PAWN_YINRCPT_WGHT;
    EvalWghts[PAWN_ON_CTR_DEF]          = EvalW::PAWN_ON_CTR_DEF_WGHT;
    EvalWghts[PAWN_ON_CTR_OFF]          = EvalW::PAWN_ON_CTR_OFF_WGHT;
    EvalWghts[PAWN_ON_ADV_CTR]          = EvalW::PAWN_ON_ADV_CTR_WGHT;
    EvalWghts[PAWN_ON_ADV_FLK]          = EvalW::PAWN_ON_ADV_FLK_WGHT;
    EvalWghts[KNIGHT_ON_CTR]            = EvalW::KNIGHT_ON_CTR_WGHT;
    EvalWghts[BISHOP_ON_CTR]            = EvalW::BISHOP_ON_CTR_WGHT;
    EvalWghts[TWO_BISHOPS]              = EvalW::TWO_BISHOPS_WGHT;
    EvalWghts[QUEEN_OUT_EARLY]          = EvalW::QUEEN_OUT_EARLY_WGHT;
    EvalWghts[BISHOP_PATTERN]           = EvalW::BISHOP_PATTERN_WGHT;
    EvalWghts[F_PAWN_MOVED_EARLY]       = EvalW::F_PAWN_MOVED_EARLY_WGHT;
    EvalWghts[ROOK_CONNECTED]           = EvalW::ROOK_CONNECTED_WGHT;
    EvalWghts[ROOK_ON_OPEN_FILE]        = EvalW::ROOK_ON_OPEN_FILE;
    EvalWghts[KING_ON_FILE]             = EvalW::KING_ON_FILE_WGHT;
    EvalWghts[MAJOR_ON_RANK7]           = EvalW::MAJOR_ON_RANK7_WGHT;
    EvalWghts[MAJOR_ON_RANK8]           = EvalW::MAJOR_ON_RANK8_WGHT;
    EvalWghts[KNIGHT_ON_EDGE]           = EvalW::KNIGHT_ON_EDGE_WGHT;
    EvalWghts[KING_EDGE]                = EvalW::KING_EDGE_WGHT;
    EvalWghts[KINGS_CLOSE_TOGETHER]     = EvalW::KINGS_CLOSE_TOGETHER_WGHT;
    EvalWghts[ATTACKERS_ON_KING]        = EvalW::ATTACKERS_ON_KING_WGHT;
    EvalWghts[CENTER_OCCUPY_PIECES]     = EvalW::CENTER_OCCUPY_PIECES_WGHT;
    EvalWghts[DEVELOPMENT_OPENING]      = EvalW::DEVELOPMENT_OPENING;
}

*/

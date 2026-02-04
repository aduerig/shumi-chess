#pragma once

// All values in integer centipawns. Positive values are bonus, negative values are penelties 
namespace EvalW
{
    // can_castle priviledge prevents stupid king wandering.
    // has_castled bonus must be > can_castle priviledge or it will never castle.
    constexpr int HAS_CASTLED_WGHT = 150;
    constexpr int CAN_CASTLE_WGHT = 35;
    
    // Isolated pawns.
    //      One count for each instance
    //      1.5 times as bad for isolated pawns on open files
    constexpr int ISOLANI_WGHT  = -18;
    constexpr int ISOLANI_ROOK_WGHT  = -18;    // Rook pawns can be isolated too

    // Backward pawns (holes)
    constexpr int HOLE_WGHT = -18;             // A hole is the square direclty ahead of a backward pawn.
    constexpr int KNIGHT_HOLE_WGHT = -25;

    // Doubled pawns
    constexpr int DOUBLED_WGHT      = -28;
    constexpr int DOUBLED_ROOK_WGHT = -33;          // Doubled pawn or rook file
    constexpr int DOUBLED_OPEN_FILE_WGHT   = -20;   // Extra penalty per "extra pawn" if the file is open of enemy pawns

    // Passed pawns
    // base = PASSED_PAWN_SLOPE_WGHT*adv*adv + PASSED_PAWN_YINRCPT_WGHT;
    //    adv   new: 5*adv*adv + 20     old (hand-tuned)
    //    ----------------------------------------------
    //    1        25                  25    // 2nd rank
    //    2        40                  25    // 3rd rank
    //    3        65                  45    // 4th rank
    //    4       100                  82    // 5th rank
    //    5       145                 151    // 6th rank
    //    6       200                 200    // 7th rank
    constexpr int PASSED_PAWN_SLOPE_WGHT = 5;
    constexpr int PASSED_PAWN_YINRCPT_WGHT = 20;

    // Pawn controlling center squares:
    constexpr int CTR_DEF_WGHT = 25;        // center e4,d4 (white); e5,d5, (black) "defensive" center squares
    constexpr int CTR_OFF_WGHT = 35;        // center e4,d4 (black); e5,d5, (white) "offensive" center squares
    constexpr int ADV_CTR_WGHT = 20;        // "advanced center" e6,d6 (White); or e3,d3 (Black)
    constexpr int ADV_FLK_WGHT = 10;        // "advanced flank" c4,f4 (black); c5,f5 (White)

    constexpr int KNIGHT_ON_CTR_WGHT = 20;  // Knight controlling center squares
    constexpr int BISHOP_ON_CTR_WGHT = 20;  // Bishop controlling center squares

    constexpr int TWO_BISHOPS_WGHT = 15;    // 2 or more bishops (only one bonus for multiple bishops)

    constexpr int QUEEN_OUT_EARLY_WGHT = -400;   // for center squares only
    constexpr int BISHOP_PATTERN_WGHT = -50;     // stupid king bishop blocking queen pawn (on d3 or d6)

    constexpr int ROOK_CONNECTED_WGHT = 120;    // if any connected rook pair exists (one bonus only)

    // Rooks on open or semi open files
    constexpr int FILE_STATUS_WGHT     = 10;   // open=2x, semi-open=1x
    constexpr int KING_ON_FILE_WGHT    = 8;    // extra per rook if enemy king on same file (even if pieces beweent he king ans rook)

    constexpr int RANK7_WGHT = 20;             // Rook or queen on 7th rank
    constexpr int RANK8_WGHT = 10;             // Rook or queen on 8th rank

    constexpr int KNIGHT_ON_EDGE_WGHT = -10;     // knight on edge penatly (doubled if knight in corner)
    
    constexpr int KING_EDGE_WGHT = 40;              // Only in ending, to force enemy king to edge

    constexpr int KINGS_CLOSE_TOGETHER_WGHT = 30;   // Only in ending, to force enemy king to edge

    // Attackers are NOT kings. Otherwise everybody else.
    constexpr int ATTACKERS_ON_KING_WGHT = 20;      // For each square at or around the king box.

    constexpr int BASE_CP_WGHT = 24;  // Used only in CRAZY_IVAN

}

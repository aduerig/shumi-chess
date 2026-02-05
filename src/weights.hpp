#pragma once


void init_eval_weights();

// All values in integer centipawns. Positive values are bonus, negative values are penelties 
namespace EvalW
{
    // can_castle privilege prevents stupid king wandering.
    // has_castled bonus must be > can_castle privilege or it will never castle.
    constexpr int HAS_CASTLED_WGHT = 110;
    constexpr int CAN_CASTLE_WGHT = 35;
    
    // Isolated pawns.
    //      One count for each instance
    //      1.5 times as bad for isolated pawns on open files
    constexpr int ISOLANI_WGHT  = -18;
    constexpr int ISOLANI_ROOK_WGHT  = -18;    // Rook pawns can be isolated too

    // Backward pawns (holes)
    constexpr int PAWN_HOLE_WGHT = -18;             // A hole is the square direclty ahead of a backward pawn.
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
    constexpr int PAWN_ON_CTR_DEF_WGHT = 25;        // center e4,d4 (white); e5,d5, (black) "defensive" center squares
    constexpr int PAWN_ON_CTR_OFF_WGHT = 35;        // center e4,d4 (black); e5,d5, (white) "offensive" center squares
    constexpr int PAWN_ON_ADV_CTR_WGHT = 20;        // "advanced center" e6,d6 (White); or e3,d3 (Black)
    constexpr int PAWN_ON_ADV_FLK_WGHT = 10;        // "advanced flank" c4,f4 (black); c5,f5 (White)

    constexpr int KNIGHT_ON_CTR_WGHT = 16;  // Knight controlling center squares
    constexpr int BISHOP_ON_CTR_WGHT = 19;  // Bishop controlling center squares

    constexpr int TWO_BISHOPS_WGHT = 16;    // 2 or more bishops (only one bonus for multiple bishops)

    // Weird conditions to stop bad moves in the opening
    constexpr int QUEEN_OUT_EARLY_WGHT = -50;   // for center squares only
    constexpr int BISHOP_PATTERN_WGHT = -60;     // stupid bishop blocking king/queen pawn (on d3 or d6)
    constexpr int F_PAWN_MOVED_EARLY_WGHT = -32;     // only in opening. Boo hoo, no Bird opening.

    constexpr int ROOK_CONNECTED_WGHT = 120;    // if any connected rook pair exists (one bonus only)

    // Rooks on open or semi open files
    constexpr int ROOK_ON_OPEN_FILE    = 10;    // open=2x, semi-open=1x
    constexpr int KING_ON_FILE_WGHT    = 8;     // extra per rook if enemy king on same file (even if pieces beweent he king ans rook)

    constexpr int MAJOR_ON_RANK7_WGHT = 20;             // Rook or queen on 7th rank
    constexpr int MAJOR_ON_RANK8_WGHT = 10;             // Rook or queen on 8th rank

    constexpr int KNIGHT_ON_EDGE_WGHT = -10;     // knight on edge penatly (doubled if knight in corner)
    
    constexpr int KING_EDGE_WGHT = 40;              // Only in ending, to force enemy king to edge

    constexpr int KINGS_CLOSE_TOGETHER_WGHT = 30;   // Only in ending, to force enemy king to edge

    // Attackers are NOT kings. Otherwise everybody else.
    constexpr int ATTACKERS_ON_KING_WGHT = 20;      // For each square at or around the king box.
    
    constexpr int DEVELOPMENT_OPENING = 5;


    constexpr int CENTER_OCCUPY_PIECES_WGHT = 24;  // Used only in CRAZY_IVAN. Doesnt count pawns or kings.

}

enum WghtIndxs
{
    HAS_CASTLED = 0,
    CAN_CASTLE,
    ISOLANI,
    ISOLANI_ROOK,
    PAWN_HOLE,
    KNIGHT_HOLE,
    DOUBLED,
    DOUBLED_ROOK,
    DOUBLED_OPEN_FILE,
    PASSED_PAWN_SLOPE,
    PASSED_PAWN_YINRCPT,
    PAWN_ON_CTR_DEF,
    PAWN_ON_CTR_OFF,
    PAWN_ON_ADV_CTR,
    PAWN_ON_ADV_FLK,
    KNIGHT_ON_CTR,
    BISHOP_ON_CTR,
    TWO_BISHOPS,
    QUEEN_OUT_EARLY,
    BISHOP_PATTERN,
    F_PAWN_MOVED_EARLY,
    ROOK_CONNECTED,
    ROOK_ON_OPEN_FILE,
    KING_ON_FILE,
    MAJOR_ON_RANK7,
    MAJOR_ON_RANK8,
    KNIGHT_ON_EDGE,
    KING_EDGE,
    KINGS_CLOSE_TOGETHER,
    ATTACKERS_ON_KING,
    CENTER_OCCUPY_PIECES,
    DEVELOPMENT_OPENING,
    LAST_VALUE              // I must be last
};


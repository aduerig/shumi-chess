#pragma once


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



// All values in integer centipawns. Positive values are bonus, negative values are penelties
class Weights
{
public:

    // can_castle privilege prevents stupid king wandering.
    // has_castled bonus must be > can_castle privilege or it will never castle.
    // castling is not actual "castling". It means getting the king to the side (on back rank), without trapping a rook.
    static constexpr int HAS_CASTLED_WGHT = 110;
    static constexpr int CAN_CASTLE_WGHT = 35;

    // Isolated pawns.
    //      One count for each instance.
    //      1.5 times as bad for isolated pawns on open files.
    static constexpr int ISOLANI_WGHT  = -18;
    static constexpr int ISOLANI_ROOK_WGHT  = -17;    // Rook pawns can be isolated too.

    // Backward pawns (holes)
    static constexpr int PAWN_HOLE_WGHT = -18;          // A hole is the square direclty ahead of a backward pawn.
    static constexpr int KNIGHT_HOLE_WGHT = -25;        // A knight sitting in a hole.

    // Doubled pawns
    static constexpr int DOUBLED_WGHT      = -25;           // One slam for each pawn more than one on a file
    static constexpr int DOUBLED_ROOK_WGHT = -29;           // Same, but doubled pawn on rook file
    static constexpr int DOUBLED_OPEN_FILE_WGHT   = -20;    // Extra penalty per "extra pawn" if the file is open of enemy pawns

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
    static constexpr int PASSED_PAWN_SLOPE_WGHT = 5;
    static constexpr int PASSED_PAWN_YINRCPT_WGHT = 20;

    // Pawn controlling center squares: (one per qualifiing pawn)
    static constexpr int PAWN_ON_CTR_DEF_WGHT = 23;     // center e4,d4 (white); e5,d5, (black) "defensive" center squares
    static constexpr int PAWN_ON_CTR_OFF_WGHT = 35;     // center e4,d4 (black); e5,d5, (white) "offensive" center squares
    static constexpr int PAWN_ON_ADV_CTR_WGHT = 22;     // "advanced center" e6,d6 (White); or e3,d3 (Black)
    static constexpr int PAWN_ON_ADV_FLK_WGHT = 10;     // "advanced flank" c4,f4 (black); c5,f5 (White)

    static constexpr int KNIGHT_ON_CTR_WGHT = 16;  // Knight controlling center squares (per square)
    static constexpr int BISHOP_ON_CTR_WGHT = 19;  // Bishop controlling center squares (per square)

    static constexpr int TWO_BISHOPS_WGHT = 16;    // 2 or more bishops (only one bonus per side)

    // Weird conditions to stop stupid moves in the opening
    static constexpr int QUEEN_OUT_EARLY_WGHT = -120;   // for center squares only
    static constexpr int BISHOP_PATTERN_WGHT = -110;     // stupid bishop blocking king/queen pawn (on d3,e3 or d6,e6)
    static constexpr int F_PAWN_MOVED_EARLY_WGHT = -30; // only in opening. Boo hoo, no Bird opening.
    static constexpr int DEVELOPMENT_OPENING = 11;      // Opening only.  Only minor pieces, off theier starting square.

    static constexpr int ROOK_CONNECTED_WGHT = 90;     // if any connected rook pair exists (one bonus only)

    // Rooks on open or semi open files
    static constexpr int ROOK_ON_OPEN_FILE    = 10;     // open=2x, semi-open=1x
    static constexpr int KING_ON_FILE_WGHT    = 8;      // extra per rook if enemy king on same file (even if pieces beweent he king ans rook)

    static constexpr int MAJOR_ON_RANK7_WGHT = 20;             // Rook or queen on 7th rank
    static constexpr int MAJOR_ON_RANK8_WGHT = 10;             // Rook or queen on 8th rank

    static constexpr int KNIGHT_ON_EDGE_WGHT = -10;     // knight on edge penatly (doubled if knight in corner)

    static constexpr int KING_EDGE_WGHT = 40;              // Only in ending, to force enemy king to edge

    static constexpr int KINGS_CLOSE_TOGETHER_WGHT = 30;   // Only in ending, to force enemy king to edge

    // Attackers are NOT kings. Otherwise everybody else.
    static constexpr int ATTACKERS_ON_KING_WGHT = 20;      // For each square (per square) at or around the king box. Includes the king square itself.

    static constexpr int CENTER_OCCUPY_PIECES_WGHT = 24;  // Used only in CRAZY_IVAN. Doesnt count pawns or kings.

    // Only member functions:
    Weights();                      // Constructer loads up the aW array.

    //int GetWeight(int wghtIndx) const;
    inline int GetWeight(int wghtIndx) const {
        return aW[wghtIndx];
    }

private:
    int aW[LAST_VALUE+1];   // sized safely; we only use [0..LAST_VALUE-1]
};





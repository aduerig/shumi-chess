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
    //F_PAWN_MOVED_EARLY,
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
    DEVELOPMENT_OPENINGK,
    DEVELOPMENT_OPENINGB,
    PASSED_PAWN_CONNECTED,
    ISOLANI_OPEN_FILE,
    KING_CENTER_LATE,
    KEEP_ROOKS_WHEN_DOWN_PAWN,
    NO_MOVE_SAME_TWICE,
    PAWN_HOLE_OPEN_FILE,
    OPPOSITE_BISHOPS,
    BLOCKED_HOME_BISHOP,
    TRADE_MAX_BONUS,
    TRADE_ADVANTAGE_CAP,
    LAST_VALUE              // I must be last in this list
};

constexpr double VOLUME_CONTROL = 0.63;

// All values in integer centipawns. Positive values are bonus, negative values are penelties
class Weights
{
private:

    // can_castle privilege prevents stupid king wandering.
    // has_castled bonus must be > can_castle privilege*2 or it will never castle.
    // castling is not actual "castling". It means getting the king to the side (on back rank), without trapping a rook.
    static constexpr int HAS_CASTLED_WGHT = 200;
    static constexpr int CAN_CASTLE_WGHT = 50;

    // Isolated pawns.
    //      One count for each instance.
    //      1.5 times as bad for isolated pawns on open files (ragrdless of wether a rook/quuen is on the file)
    static constexpr int ISOLANI_WGHT  = -18;
    static constexpr int ISOLANI_ROOK_WGHT  = -17;    // Rook pawns can be isolated too.
    static constexpr int ISOLANI_OPEN_FILE_WGHT  = -15;


    // Backward pawns (holes)
    static constexpr int PAWN_HOLE_WGHT = -30;        // A hole is the square direclty ahead of a backward pawn.
    static constexpr int KNIGHT_HOLE_WGHT = -25;      // A knight sitting in a hole.
    static constexpr int PAWN_HOLE_OPEN_FILE_WGHT = -15;    // also applies to the backward pawn behind the hole

    // Doubled pawns
    static constexpr int DOUBLED_WGHT      = -19;           // One slam for each pawn more than one on a file
    static constexpr int DOUBLED_ROOK_WGHT = -19;           // Same, but doubled pawn on rook file
    static constexpr int DOUBLED_OPEN_FILE_WGHT   = -18;    // Extra penalty per "extra pawn" if the file is open of enemy pawns

    // Passed pawns
    static constexpr int PASSED_PAWN_SLOPE_WGHT   = 9;     // Actually this is a quadratic, not a line
    static constexpr int PASSED_PAWN_YINRCPT_WGHT = 25;
    // bonus = PASSED_PAWN_SLOPE_WGHT * (adv-1)*(adv-1)  +  PASSED_PAWN_YINRCPT_WGHT;
    //    adv   new: 11*(adv-1)^2 + 30
    //    --------------------------------
    //    1        30     // 2nd rank
    //    2        41     // 3rd rank
    //    3        74     // 4th rank
    //    4       129     // 5th rank
    //    5       206     // 6th rank
    //    6       305     // 7th rank
    

    static constexpr int PASSED_PAWN_CONNECTED_WGHT = 4;   // Multiplied by passed pawn bonus, divide it all by 3

    // Pawn controlling center squares: (one per qualifiing pawn)
    static constexpr int PAWN_ON_CTR_DEF_WGHT = 24;     // center e4,d4 (white); and e5,d5, (black) "defensive" center squares
    static constexpr int PAWN_ON_CTR_OFF_WGHT = 38;     // center e5,d5 (white); and e4,d4, (white) "offensive" center squares
    static constexpr int PAWN_ON_ADV_CTR_WGHT = 24;     // "advanced center" e6,d6,e7,d7 (White); or e3,d3,e2,d2 (Black)
    static constexpr int PAWN_ON_ADV_FLK_WGHT = 10;      // "advanced flank" c5,f5 (White), c4,f4 (black); 

    static constexpr int KNIGHT_ON_CTR_WGHT = 14;  // Knight controlling center squares (per square)
    static constexpr int BISHOP_ON_CTR_WGHT = 23;  // Bishop controlling center squares (per square) (here we can look through other pieces)

    static constexpr int TWO_BISHOPS_WGHT = 24;    // 2 or more bishops (only one bonus per side)

    // Weird conditions to stop stupid moves in the opening
    static constexpr int QUEEN_OUT_EARLY_WGHT = -40;    // for landing on center squares only. only in opening.
    static constexpr int BISHOP_PATTERN_WGHT = -150;    // stupid bishop blocking king/queen pawn (on d3,e3 or d6,e6). Only in opening.
    //static constexpr int F_PAWN_MOVED_EARLY_WGHT = 0; // only in opening. Boo hoo, no Bird opening.
    static constexpr int BLOCKED_HOME_BISHOP_WGHT = 10; // only in opening. Bishop on home square blocked by 2 pawns
    
    static constexpr int DEVELOPMENT_OPENINGK_WGHT = 14;  // Opening only.  Counts knights, off their starting square.
    static constexpr int DEVELOPMENT_OPENINGB_WGHT = 22;  // Opening only.  Counts bishops, off their starting square.

    static constexpr int ROOK_CONNECTED_WGHT = 100;      // if any connected rook pair exists (one bonus only)

    // Rooks on open or semi open files
    static constexpr int ROOK_ON_OPEN_FILE_WGHT = 21;     // open=2x, semi-open=1x
    static constexpr int KING_ON_FILE_WGHT    = 14;      // extra per rook if enemy king on same file (even if pieces between he king and rook)

    static constexpr int MAJOR_ON_RANK7_WGHT = 28;      // Rook or queen on 7th rank (if 2 major then 3 times)
    static constexpr int MAJOR_ON_RANK8_WGHT = 10;      // Rook or queen on 8th rank (if 2 major then 3 times)

    static constexpr int KNIGHT_ON_EDGE_WGHT = -12;     // knight on edge penatly (doubled if knight in corner)

    static constexpr int KING_EDGE_WGHT = 40;              // Only in ending, to force enemy king to edge

    static constexpr int KING_CENTER_LATE_WGHT = 22;       // Only in ending, 

    static constexpr int KINGS_CLOSE_TOGETHER_WGHT = 30;   // Only in ending, to force enemy king to edge

    // Attackers are NOT kings. Otherwise everybody else.
    static constexpr int ATTACKERS_ON_KING_WGHT = 20;      // For each square (per square) at or around the king box. Includes the king square itself.

    static constexpr int CENTER_OCCUPY_PIECES_WGHT = 24;  // Used only in CRAZY_IVAN. Doesnt count pawns or kings.

    static constexpr int KEEP_ROOKS_WHEN_DOWN_PAWN_WGHT = 10;
    static constexpr int TRADE_MAX_BONUS_WGHT = 120;
    static constexpr int TRADE_ADVANTAGE_CAP_WGHT = 200;



    static constexpr int NO_MOVE_SAME_TWICE_WGHT = -20; // NOT USED

    static constexpr int OPPOSITE_BISHOPS_WGHT = 50; //   subtracted from the ahead side
    

public:

    // Only member functions:
    Weights();                      // Constructer loads up the aW array.

    inline int GetWeight(int wghtIndx) const {
        return aW[wghtIndx];
    }

    void multiply_weights(double dMult);


private:
    int aW[LAST_VALUE+1];   // sized safely; we only use [0..LAST_VALUE-1]
};








#pragma once


// #include "engine.hpp"
// #include "gameboard.hpp"

//
// A minimal description of a position in the format the Syzygy probe
// library will want (conceptually "FEN without fullmove number").
struct endgameTablePos
{
    int stm;              // Side to move: 0 = white, 1 = black

    int piece_count;      // Number of pieces in this TB position.
                          // For KQvK / KRvK this will always be 3.

    int pieces[3];        // Piece type codes expected by the TB library.
                          // e.g. (KING, QUEEN, KING) or (KING, ROOK, KING).
                          // Exact numeric values will come from the probe
                          // library header; for now you can use placeholder
                          // constants and map later.

    int squares[3];       // Square indices 0..63 in **a1 = 0** format.
                          // IMPORTANT: you use h1 = 0 internally, so when
                          // you fill this array you must convert from your
                          // h1=0 to a1=0 (flip files).

    int castling_rights;  // Bitmask of castling rights (Syzygy style).
                          // For KQvK / KRvK it will always be 0.

    int ep_square;        // En-passant target square 0..63, or -1 if none.
                          // For KQvK / KRvK: always -1.

    int rule50;           // Half-move clock for the 50-move rule (0..100+).
};


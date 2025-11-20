/*
 * (c) 2015 basil, all rights reserved,
 * Modifications Copyright (c) 2016-2019 by Jon Dart
 * Modifications Copyright (c) 2020-2024 by Andrew Grant
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

/*
 * You are in charge of defining each of these macros. The macros already
 * defined here are simply an example of what to do. This configuration is
 * used by Ethereal to implement Pyrrhic.
 *
 * See Ethereal's source <https://github.com/AndyGrant/Ethereal> if it is
 * not readily clear what these definfitions mean. The relevant files are
 * are the ones included below.
 *
 * Note that for the Pawn Attacks, we invert the colour. This is because
 * Pyrrhic defines White as 1, where as Ethereal (any many others) choose
 * to define White as 0 and Black as 1.
 */



// #include "../attacks.h"
// #include "../bitboards.h"

// #define PYRRHIC_POPCOUNT(x)              (popcount(x))
// #define PYRRHIC_LSB(x)                   (getlsb(x))
// #define PYRRHIC_POPLSB(x)                (poplsb(x))

// #define PYRRHIC_PAWN_ATTACKS(sq, c)      (pawnAttacks(!c, sq))
// #define PYRRHIC_KNIGHT_ATTACKS(sq)       (knightAttacks(sq))
// #define PYRRHIC_BISHOP_ATTACKS(sq, occ)  (bishopAttacks(sq, occ))
// #define PYRRHIC_ROOK_ATTACKS(sq, occ)    (rookAttacks(sq, occ))
// #define PYRRHIC_QUEEN_ATTACKS(sq, occ)   (queenAttacks(sq, occ))
// #define PYRRHIC_KING_ATTACKS(sq)         (kingAttacks(sq))

//#include "../attacks.h"
//#include "../bitboards.h"   // Ethereal-specific – not used in ShumiChess

#include <stdint.h>

/*
 * Minimal, engine-agnostic configuration for Pyrrhic.
 * We define simple popcount/lsb/poplsb helpers here so Pyrrhic
 * does not depend on any external engine headers.
 *
 * NOTE: These are intentionally simple and not super-optimized.
 * For KQK / KRK endgames, this is perfectly fine.
 */

static inline int pyrrhic_popcount_u64(uint64_t x)
{
    int c = 0;
    while (x)
    {
        x &= (x - 1);
        ++c;
    }
    return c;
}

static inline int pyrrhic_lsb_u64(uint64_t x)
{
    if (!x) return -1;  // convention: -1 if empty
    int idx = 0;
    while ((x & 1ull) == 0ull)
    {
        x >>= 1;
        ++idx;
    }
    return idx;
}

static inline uint64_t pyrrhic_poplsb_u64(uint64_t *x)
{
    uint64_t bb = *x;
    if (!bb) return 0ull;
    uint64_t lsb = bb & (~bb + 1ull); // bb & -bb
    *x = bb ^ lsb;
    return lsb;
}

#define PYRRHIC_POPCOUNT(x)              pyrrhic_popcount_u64((uint64_t)(x))
#define PYRRHIC_LSB(x)                   pyrrhic_lsb_u64((uint64_t)(x))
#define PYRRHIC_POPLSB(x)                pyrrhic_poplsb_u64((uint64_t *)&(x))

/*
 * Attack macros:
 * For now, we do NOT use Pyrrhic’s internal move-generation,
 * so these can be left as stubs. If Pyrrhic later complains or
 * we want to enable its internal checks, we can wire these up
 * properly to ShumiChess’s bitboard attack routines.
 */

#define PYRRHIC_PAWN_ATTACKS(sq, c)      (0ull)
#define PYRRHIC_KNIGHT_ATTACKS(sq)       (0ull)
#define PYRRHIC_BISHOP_ATTACKS(sq, occ)  (0ull)
#define PYRRHIC_ROOK_ATTACKS(sq, occ)    (0ull)
#define PYRRHIC_QUEEN_ATTACKS(sq, occ)   (0ull)
#define PYRRHIC_KING_ATTACKS(sq)         (0ull)

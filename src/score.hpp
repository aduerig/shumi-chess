
#pragma once

#ifdef SHUMI_FORCE_ASSERTS
    #undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <cmath>


#define SCORE_AS_INT

#ifndef SCORE_AS_INT

    // This code is no lonber valid, after making Score a 16 bit integer
    typedef double Score;

    #define ZERO_SCORE 0.0
    #define ONE_PAWN 1.0
    #define ONE_CENTIPAWN 0.01


    // Explicit centipawn integer type (32-bit).
    //typedef int32_t CP;

    // Centipawn to pawn conversions
    // these lines must be:
    //  inline int convert_to_CP(Score dd) {return (int)( (dd * 100.0) + (dd >= 0.0 ? 0.5 : -0.5) );}
    //  inline Score convert_from_CP(int ii) {return (static_cast<Score>(ii) / 100.0);}
    //
    inline int convert_to_CP(Score dd) {return (int)( (dd * 100.0) + (dd >= 0.0 ? 0.5 : -0.5) );}
    // inline Score convert_from_CP(int ii) {return (static_cast<Score>(ii) / 100.0);}
    inline Score convert_from_CP(int ii)
    {   // temporary routine to remove noise from multiplication and rounding
        // exact division to get pawns
        Score s = static_cast<Score>(ii) / 100.0; 
        // quantize to 2 decimal places to remove extra precision/noise
        s = std::floor(s * 100.0 + 0.5) / 100.0;
        return s;
    }

    inline double convert_to_pawns(double ii) {return (static_cast<double>(ii));}

    inline constexpr Score VERY_SMALL_SCORE = 0.00001;   // pawns
    inline constexpr Score HUGE_SCORE       = 10000.0;   // pawns

    inline bool IS_MATE_SCORE(Score x) {return std::abs(x) > (HUGE_SCORE - 200.0);}  // pawns. Why 200? This would be a mate in 100.

    inline constexpr Score ABORT_SCORE     = HUGE_SCORE + 1.0;  // abort analysis
    inline constexpr Score ONLY_MOVE_SCORE = HUGE_SCORE + 2.0;  // short-circuit when only one legal move

    static const char* fmtPos = "%+.2f";
    static const char* fmtNeg = "%.2f";
    static const char* fmtMain = "%.3f";


#else

    //typedef int Score;
    typedef std::int16_t Score;

    #define ZERO_SCORE 0
    #define ONE_PAWN 100
    #define ONE_CENTIPAWN 1


    // Note: clean me up, remove me after testing
    inline int convert_to_CP(Score dd) {return (int)dd;}
    inline Score convert_from_CP(int ii)
    {
        // Verify return from evaluation, in anticipation of Score becoming 16 bit.
        assert(ii >= -32767 && ii <= 32767);
        return static_cast<Score>(ii);
    }


    // Used only for displays to humans. convert_from_CP() used to do this, but now Shumi is CP internally everywhere.
    inline double convert_to_pawns(int ii) {return (static_cast<double>(ii) / 100.0);}

    static const char* fmtPos = "%+d";
    static const char* fmtNeg = "%d";
    static const char* fmtMain = "%d";

    inline constexpr Score VERY_SMALL_SCORE = 0;   // centi-pawns
    //
    // HUGE_SCORE = 30,000 centipawns (300 pawns).
    //      Chosen to fit within a signed 16-bit Score (-32,768 to +32,767).
    //      Even an impossible army of 15 queens (every pawn promotes) and a king -  has only 13,500 cp
    //      in queen material. This leaves substantial room for positional scores
    //      while keeping mate scores far above ordinary evaluations.
    //
    // "all Score arithmetic must be checked for 16-bit overflow"
    //
    inline constexpr Score HUGE_SCORE = 30'000;       // Was 10000.0 pawns, now 1,000,000 centpawns
    
    //  MATE_CODING_BUFFER: I suggest 1,000 centipawns, allowing room for mate distances up to approximately 1,000 plies.
    inline constexpr Score MATE_CODING_BUFFER = 1'000;   // // 20000 centpawns = 200 pawns

    // Leave room for "mate marker"
    inline bool IS_MATE_SCORE(Score x) { return std::abs(x) > (HUGE_SCORE - MATE_CODING_BUFFER); } 

    inline constexpr Score ABORT_SCORE = HUGE_SCORE + 1; 
    inline constexpr Score ONLY_MOVE_SCORE = HUGE_SCORE + 2; 

#endif

//
// The incoming score has had a "mate marker" added to it. Here we remove that marker, so now
// its a real "Score" that can be comraed to other Scores.
//
// inline Score mate_score_to_TT(Score score, int level)
// {
//     assert(level >= 0);
//     assert ( (score >= -HUGE_SCORE) && (score <= HUGE_SCORE) );

//     if (IS_MATE_SCORE(score)) {
//         if (score > 0) {
//             // Remove the "distance from the current root".
//             return score + level;
//         } else {
//             // Remove the distance from the current root.
//             return score - level;
//         }
//     }

//     return score;
// }

inline Score mate_score_to_TT(Score score, int level)
{
    assert(level >= 0);
    assert(score >= -HUGE_SCORE && score <= HUGE_SCORE);

    int adjusted = score;

    if (IS_MATE_SCORE(score)) {
        adjusted = (score > 0)
            ? adjusted + level
            : adjusted - level;
    }

    assert ( (adjusted >= -HUGE_SCORE) && (adjusted <= HUGE_SCORE) );
    return (Score)(adjusted);
}

inline Score mate_score_from_TT(Score score, int level)
{
    if (IS_MATE_SCORE(score)) {
        if (score > 0) {
            // Apply the distance from the new root.
            return score - level;
        } else {
            // Apply the distance from the new root.
            return score + level;
        }
    }

    return score;
}

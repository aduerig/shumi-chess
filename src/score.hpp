
#pragma once

//#define SCORE_AS_INT

#ifndef SCORE_AS_INT

    typedef double Score;

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

    inline constexpr Score VERY_SMALL_SCORE = 1.0e-5;   // pawns (0.01 centipawns)
    inline constexpr Score HUGE_SCORE       = 10000.0;  // pawns

    inline bool IS_MATE_SCORE(Score x) {return std::abs(x) > (HUGE_SCORE - 200.0);}  // pawns. Why 200? This would be a mate in 100.

    inline constexpr Score ABORT_SCORE     = HUGE_SCORE + 1.0;  // abort analysis
    inline constexpr Score ONLY_MOVE_SCORE = HUGE_SCORE + 2.0;  // short-circuit when only one legal move

    static const char* fmtPos = "%+.2f";

    static const char* fmtNeg = "%.2f";

    static const char* fmtMain = "%.3f";

    inline double convert_to_pawns(double ii) {return (static_cast<double>(ii));}


#else

    typedef int Score;

    // Note: clean me up, remove me after testing
    inline int convert_to_CP(Score dd) {return (int)dd;}
    inline Score convert_from_CP(int ii) {return (static_cast<Score>(ii));}


    // Used only for displays to humans. convert_from_CP() used to do this, but now Shumi is CP internally everywhere.
    inline double convert_to_pawns(int ii) {return (static_cast<double>(ii) / 100.0);}

    static const char* fmtPos = "%+d";

    static const char* fmtNeg = "%d";

    static const char* fmtMain = "%d";


    inline constexpr Score VERY_SMALL_SCORE = 1;       // Was 0.00001 pawns, now 1 centpawn
    inline constexpr Score HUGE_SCORE = 1000000;       // Was 10000.0 pawns, now 1,000,000 centpawns

    inline bool IS_MATE_SCORE(Score x) { return std::abs(x) > (HUGE_SCORE - 20000); } // 20000 centpawns = 200 pawns

    inline constexpr Score ABORT_SCORE = HUGE_SCORE + 1; 
    inline constexpr Score ONLY_MOVE_SCORE = HUGE_SCORE + 2; 

#endif
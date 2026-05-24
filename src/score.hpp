
#ifndef SCORE_HPP
#define SCORE_HPP

// Keep your existing Score type for now (double). Change this later
//   to 'typedef int32_t Score;' when you're ready to flip.
typedef double Score;

/* Explicit centipawn integer type (32-bit). */
typedef int32_t CP;



// Centipawn to pawn conversions
// these lines must be:
//  inline CP convert_to_CP(Score dd) {return (int)( (dd * 100.0) + (dd >= 0.0 ? 0.5 : -0.5) );}
//  inline Score convert_from_CP(int ii) {return (static_cast<Score>(ii) / 100.0);}
//
inline CP convert_to_CP(Score dd) {return (int)( (dd * 100.0) + (dd >= 0.0 ? 0.5 : -0.5) );}
inline Score convert_from_CP(int ii) {return (static_cast<Score>(ii) / 100.0);}
// inline Score convert_from_CP(int ii)
// {
//     // exact division to get pawns
//     Score s = static_cast(ii) / 100.0;
//     // quantize to 2 decimal places to remove extra precision/noise
//     s = std::floor(s * 100.0 + 0.5) / 100.0;
//     return s;
// }

#define CP_PER_PAWN 100



inline constexpr Score VERY_SMALL_SCORE = 1.0e-5;   // pawns (0.01 centipawns)
inline constexpr Score HUGE_SCORE       = 10000.0;  // pawns

inline bool IS_MATE_SCORE(Score x) {return std::abs(x) > (HUGE_SCORE - 200.0);}  // pawns. Why 200? This would be a mate in 100.

inline constexpr Score ABORT_SCORE     = HUGE_SCORE + 1.0;  // abort analysis
inline constexpr Score ONLY_MOVE_SCORE = HUGE_SCORE + 2.0;  // short-circuit when only one legal move





#endif /* SCORE_HPP */
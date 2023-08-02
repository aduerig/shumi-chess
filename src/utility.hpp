#pragma once

//TODO shares a name with builtin lib
#include <algorithm> 
#include <cctype>
#include <iterator>
#include <locale>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "globals.hpp"
#include "gameboard.hpp"

using namespace std;

namespace utility {

enum class AColor {
    RESET = 0,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    BRIGHT_BLACK = 90,
    BRIGHT_RED = 91,
    BRIGHT_GREEN = 92,
    BRIGHT_YELLOW = 93,
    BRIGHT_BLUE = 94,
    BRIGHT_MAGENTA = 95,
    BRIGHT_CYAN = 96,
    BRIGHT_WHITE = 97
};

inline std::string colorize(AColor color, const std::string& text) {
    return "\033[1;" + std::to_string(static_cast<int>(color)) + "m" + text + "\033[0m";
}

namespace bit {

inline ull bitshift_by_color(ull bitboard, ShumiChess::Color color, int amount) {
    if (color == ShumiChess::WHITE) {
        return bitboard << amount;
    }
    return bitboard >> amount;
};

// inline ull lsb_and_pop(ull& bitboard) {
//     ull lsb_fast = 1ULL << (__builtin_ffsll(bitboard) - 1);
//     bitboard = bitboard & (~lsb_fast);
//     return lsb_fast;
// };

inline ull lsb_and_pop(ull& bitboard) {
    ull lsb_fast = 1ULL << __builtin_ctzll(bitboard);
    bitboard ^= lsb_fast;
    return lsb_fast;
};

inline ull lsb_and_pop_to_square(ull& bitboard) {
    int square = __builtin_ctzll(bitboard);
    ull lsb_fast = 1ULL << square;
    bitboard ^= lsb_fast;
    return square;
};


inline int square_to_bitboard(int square) {
    return 1ULL << square;
};

// both the asm instructions return 64 if bitboard == 0 
inline int bitboard_to_lowest_square(ull bitboard) {
    return __builtin_ctzll(bitboard);
};
// !TODO is this an instruction? https://www.felixcloutier.com/x86/bsr.html
// talked about here: https://www.felixcloutier.com/x86/bsr.html
inline int bitboard_to_highest_square(ull bitboard) {
    // idk if this is optimal, but it works, from chatgpt tbh
    return 63 - (__builtin_clzll(bitboard | 1));
};


inline ull lowest_bitboard(ull bitboard) {
    // i think this is safe to comment
    // if (bitboard == 0ULL) {
    //     return 0ULL;
    // }
    return 1ULL << bitboard_to_lowest_square(bitboard);
};

inline ull highest_bitboard(ull bitboard) {
    if (bitboard == 0ULL) {
        return 0ULL;
    }
    return 1ULL << bitboard_to_highest_square(bitboard);
};

} // end namespace bit

namespace representation {

ull acn_to_bitboard_conversion(const std::string&);
std::string bitboard_to_acn_conversion(ull);


constexpr ShumiChess::Color opposite_color_constexpr(ShumiChess::Color color) {
    return (color == ShumiChess::WHITE) ? ShumiChess::BLACK : ShumiChess::WHITE;
}

inline const ShumiChess::Color opposite_color(const ShumiChess::Color color) {
    return (ShumiChess::Color) (1 - (int) color);
}


inline const string color_str(const ShumiChess::Color color) {
    return color == ShumiChess::WHITE ? "white" : "black";
}


extern std::array<std::string, 8> row_to_letter;

// assumes layout is:
// lower right is 2^0
// lower left is 2^7
// upper right is 2^56
// upper left is 2^63
inline std::string square_to_position_string(ull square) {
    for (int i = 1; i <= 8; i++) {
        for (int j = 1; j <= 8; j++) {
            if (1ULL & square) {
                return row_to_letter[(9 - j) - 1] + to_string(i);
            }
            square = square >> 1;
        }
    }
    return "error";
};

inline std::string move_to_string(ShumiChess::Move move) {
    return square_to_position_string(move.from) + square_to_position_string(move.to);
};
struct MoveHash {
    std::size_t operator()(const ShumiChess::Move &m) const {
        int ok1 = utility::bit::bitboard_to_lowest_square(m.from);
        int ok2 = utility::bit::bitboard_to_lowest_square(m.to);
        return (std::size_t) ok1 + (ok2 * 64);
        // return std::hash<int>{ok1 + (ok2 * 64)}();
        // return std::hash<std::string>{}(move_to_string(m));
    }
};
std::string bitboard_to_string(ull);
void print_bitboard(ull);
std::string gameboard_to_string(ShumiChess::GameBoard);
void print_gameboard(ShumiChess::GameBoard);
std::string stringify(ShumiChess::Piece);
std::string square_to_position_string(ull);

} // end namespace representation

// ? should we really clash in namespace name here
namespace our_string {

// Split an input string according to some string delimiter.
// Uses space by default. 
std::vector<std::string> split(const std::string&, const std::string& = " ");
std::string join(const vector<std::string>&, const char*);
bool starts_with(const std::string& main_str, const std::string& smaller_str);

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

} // end namespace string

} // end namespace utility
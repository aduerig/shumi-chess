#pragma once


#ifdef _MSC_VER  // Check if we are using the Microsoft Visual C++ compiler
#include <intrin.h> // Include the header for MSVC intrinsics


inline int signOf(int x) {
    return (x > 0) - (x < 0);
}

// MSVC doesn't have __builtin_ctzll, so we create our own version
// using the _BitScanForward64 intrinsic.
// __builtin_ctzll returns the number of trailing zeros in the binary representation of a 64-bit integer
inline int __builtin_ctzll(unsigned long long x) {
    unsigned long index;
    // Finds the index (0–63) of the least-significant set bit in a 64-bit mask (i.e., count of trailing zeros until the first 1).
    if (_BitScanForward64(&index, x)) {
        return (int)index;
    }
    return 64; // GCC's behavior for 0 is undefined, 64 is a common fallback
}

// MSVC doesn't have __builtin_clzll, so we create our own version
// using the _BitScanReverse64 intrinsic.
// __builtin_clzll returns the number of leading zeros in the binary representation of a 64-bit integer
inline int __builtin_clzll(unsigned long long x) {
    unsigned long index;
    if (_BitScanReverse64(&index, x)) {
        return 63 - (int)index;
    }
    return 64; // GCC's behavior for 0 is undefined, 64 is a common fallback
}

#endif // _MSC_VER

//TODO shares a name with builtin lib
#include <algorithm> 
#include <bitset>
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

// This function does 2 things:
//    1. Returns only the least significant bit (LSB) as a bitboard — all higher bits are zero.
//    2. The lsb is zeroed on the input "bitboard"
inline ull lsb_and_pop(ull& bitboard) {
    //__builtin_ctzll returns the number of trailing zeros in the binary representation of a 64-bit integer
    ull lsb_fast = 1ULL << __builtin_ctzll(bitboard);
    bitboard = bitboard & (~lsb_fast);
    return lsb_fast;
};

// It finds the index (0–63) of the least-significant 1-bit in bitboard, clears that bit in the original variable, and returns the index.
inline ull lsb_and_pop_to_square(ull& bitboard) {
    //__builtin_ctzll returns the number of trailing zeros in the binary representation of a 64-bit integer
    int square = __builtin_ctzll(bitboard);
    ull lsb_fast = 1ULL << square;
    bitboard = bitboard & (~lsb_fast);
    return square;
};

inline int square_to_bitboard(int square) {
    return 1ULL << (square - 1);
};

// Returns the number of trailing zeros in the binary representation of a 64-bit integer.
// (how many zeros are at the right end of the binary number, before you hit the first 1 bit)
// Returns 64 if bitboard == 0.
// BUT for a "h1=0" system like this, this means scanning from h1 to a1, h2 to a2, and so on to a8.
inline int bitboard_to_lowest_square(ull bitboard) {
    return __builtin_ctzll(bitboard);
};

// Returns the number of leading zeros in the binary representation of a 64-bit integer.
// Returns 64 if bitboard == 0.
// !TODO is this an instruction? https://www.felixcloutier.com/x86/bsr.html
// talked about here: https://www.felixcloutier.com/x86/bsr.html
inline int bitboard_to_highest_square(ull bitboard) {
    // idk if this is optimal, but it works, from chatgpt tbh
    return 63 - (__builtin_clzll(bitboard | 1));
};

} // end namespace bit

namespace representation {

ull acn_to_bitboard_conversion(const std::string&);
std::string bitboard_to_acn_conversion(ull);

//
//  Which one is fastest? This is called everywhere. Chatbot says they are the same speed. But only
//  one of them does not require colors to be "0" and "1".
// inline constexpr Color ShumiChess::opposite_color(const ShumiChess::Color color) {
//     return static_cast<Color>(static_cast<std::uint8_t>(color) ^ 1);   // I depend on WHITE=0, BLACK=1
// }
// inline constexpr Color ShumiChess::opposite_color(const ShumiChess::Color color) {
//     return (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
// }
// Or what about this:
// inline constexpr ShumiChess::Color opposite_color(Color c) {
//     return static_cast<Color>(static_cast<unsigned>(c) ^ 1u);
// }
inline constexpr ShumiChess::Color opposite_color(const ShumiChess::Color color) {
    return (ShumiChess::Color) (1 - (int) color);                      // I depend on WHITE=0, BLACK=1
}


// inline constexpr string color_str(const ShumiChess::Color color) {
//     return color == ShumiChess::WHITE ? "white" : "black";
// }


extern std::array<std::string, 8> row_to_letter;

// assumes layout is (h1=0):
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


inline std::string piece_to_string(ShumiChess::Piece piece) {
    switch (piece) {
        case ShumiChess::Piece::NONE:   return "None";
        case ShumiChess::Piece::PAWN:   return "Pawn";
        case ShumiChess::Piece::KNIGHT: return "Knight";
        case ShumiChess::Piece::BISHOP: return "Bishop";
        case ShumiChess::Piece::ROOK:   return "Rook";
        case ShumiChess::Piece::QUEEN:  return "Queen";
        case ShumiChess::Piece::KING:   return "King";
        default:                        return "Unknown Piece";
    }
}

inline std::string color_to_string(ShumiChess::Color color) {
    switch (color) {
        case ShumiChess::Color::WHITE: return "White";
        case ShumiChess::Color::BLACK: return "Black";
        default:                       return "Unknown Color";
    }
}

inline void cout_move_info(const ShumiChess::Move& move) {
    auto print_if_not_none = [](const std::string& label, ShumiChess::Piece piece) {
        if (piece != ShumiChess::Piece::NONE) {
            std::cout << label << piece_to_string(piece) << std::endl;
        }
    };

    std::cout << "--- Move Details for " << move_to_string(move) << " ---" << std::endl;
    std::cout << "Player: " << color_to_string(move.color) << std::endl;
    std::cout << "Piece: " << piece_to_string(move.piece_type) << std::endl;
    
    // Print optional information only if it's relevant
    print_if_not_none("Capture: ", move.capture);
    print_if_not_none("Promotion: ", move.promotion);
    
    // Use std::boolalpha to print booleans as "true" or "false"
    std::cout << std::boolalpha;
    std::cout << "Is Castle: " << move.is_castle_move << std::endl;
    std::cout << "Is En Passant Capture: " << move.is_en_passent_capture << std::endl;
    
    // Print en passant target square if it exists
    if (move.en_passant_rights != 0) {
        std::cout << "En Passant Target: " << square_to_position_string(move.en_passant_rights) << std::endl;
    }

    // Use std::bitset to clearly show castling rights (1 = available, 0 = unavailable)
    // Assumes bit 1 is Kingside and bit 0 is Queenside
    std::cout << "White Castle Rights (KQ): " << std::bitset<2>(move.white_castle_rights) << std::endl;
    std::cout << "Black Castle Rights (kq): " << std::bitset<2>(move.black_castle_rights) << std::endl;
    std::cout << "----------------------------------" << std::endl;
}


struct MoveHash {
    std::size_t operator()(const ShumiChess::Move &m) const {
        return std::hash<std::string>{}(move_to_string(m));
    }
};
std::string bitboard_to_string(ull);
void print_bitboard(ull);
std::string gameboard_to_string(ShumiChess::GameBoard);
std::string gameboard_to_string2(ShumiChess::GameBoard);
std::string colorize_board_string(const std::string& plain);
std::string widen_board(const std::string& plain);
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
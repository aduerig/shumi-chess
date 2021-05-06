#include <gtest/gtest.h>

#include "gameboard.hpp"
#include "globals.hpp"

namespace ShumiChess {
bool operator==(const ShumiChess::GameBoard& a, const ShumiChess::GameBoard& b) {
    return (a.black_pawns == b.black_pawns &&
            a.white_pawns == b.white_pawns &&
            a.black_rooks == b.black_rooks &&
            a.white_rooks == b.white_rooks &&
            a.black_knights == b.black_knights &&
            a.white_knights == b.white_knights &&
            a.black_bishops == b.black_bishops &&
            a.white_bishops == b.white_bishops &&
            a.black_queens == b.black_queens &&
            a.white_queens == b.white_queens &&
            a.black_king == b.black_king &&
            a.white_king == b.white_king &&
            a.turn == b.turn &&
            a.black_castle == b.black_castle &&
            a.white_castle == b.white_castle &&
            a.en_passant == b.en_passant &&
            a.halfmove == b.halfmove &&
            a.fullmove == b.fullmove);
}
}

namespace utility::representation {
void highlight_board_differences(const ShumiChess::GameBoard& a, const ShumiChess::GameBoard& b) {
    if (a.black_pawns != b.black_pawns) {
        cout << "Black Pawns" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.black_pawns);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.black_pawns);
        cout << endl;
    }
    if (a.white_pawns != b.white_pawns) {
        cout << "White Pawns" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.white_pawns);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.white_pawns);
        cout << endl;
    }
    if (a.black_rooks != b.black_rooks) {
        cout << "Black Rooks" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.black_rooks);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.black_rooks);
        cout << endl;
    }
    if (a.white_rooks != b.white_rooks) {
        cout << "White Rooks" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.white_rooks);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.white_rooks);
        cout << endl;
    }
    if (a.black_knights != b.black_knights) {
        cout << "Black Knights" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.black_knights);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.black_knights);
        cout << endl;
    }
    if (a.white_knights != b.white_knights) {
        cout << "White Knights" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.white_knights);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.white_knights);
        cout << endl;
    }
    if (a.black_bishops != b.black_bishops) {
        cout << "Black Bishops" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.black_bishops);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.black_bishops);
        cout << endl;
    }
    if (a.white_bishops != b.white_bishops) {
        cout << "White Bishops" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.white_bishops);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.white_bishops);
        cout << endl;
    }
    if (a.black_queens != b.black_queens) {
        cout << "Black Queens" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.black_queens);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.black_queens);
        cout << endl;
    }
    if (a.white_queens != b.white_queens) {
        cout << "White Queens" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.white_queens);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.white_queens);
        cout << endl;
    }
    if (a.black_king != b.black_king) {
        cout << "Black King" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.black_king);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.black_king);
        cout << endl;
    }
    if (a.white_king != b.white_king) {
        cout << "White King" << endl;
        cout << "Board 1:";
        utility::representation::print_bitboard(a.white_king);
        cout << endl << "Board 2:";
        utility::representation::print_bitboard(b.white_king);
        cout << endl;
    }
    if (a.turn != b.turn) {
        cout << "Board 1 Turn: " << a.turn << endl;
        cout << "Board 2 Turn: " << b.turn << endl;
    }
    if (a.black_castle != b.black_castle) {
        cout << "Board 1 Black Castle: " << static_cast<int>(a.black_castle) << endl;
        cout << "Board 2 Black Castle: " << static_cast<int>(b.black_castle) << endl;
    }
    if (a.white_castle != b.white_castle) {
        cout << "Board 1 White Castle: " << static_cast<int>(a.white_castle) << endl;
        cout << "Board 2 White Castle: " << static_cast<int>(b.white_castle) << endl;
    }
    if (a.en_passant != b.en_passant) {
        cout << "Board 1 EnPassant: " << a.en_passant << endl;
        cout << "Board 2 EnPassant: " << b.en_passant << endl;
    }
    if (a.halfmove != b.halfmove) {
        cout << "Board 1 Halfmove: " << static_cast<int>(a.halfmove) << endl;
        cout << "Board 2 Halfmove: " << static_cast<int>(b.halfmove) << endl;
    }
    if (a.fullmove != b.fullmove) {
        cout << "Board 1 FullMove: " << static_cast<int>(a.fullmove) << endl;
        cout << "Board 2 FullMove: " << static_cast<int>(b.fullmove) << endl;
    }
}
}
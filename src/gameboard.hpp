#pragma once

#include <string>
#include <iostream>

#include "globals.hpp"

namespace ShumiChess {
// TODO think about copy and move constructors.
// ? Will we ever want to copy?
class GameBoard {
    public:
        // Piece locations
        ull black_pawns {};
        ull white_pawns {};

        ull black_rooks {};
        ull white_rooks {};

        ull black_knights {};
        ull white_knights {};

        ull black_queens {};
        ull white_queens {};

        ull black_king {};
        ull white_king {};

        // other information about the board state
        Color turn;

        uint64_t zobrist_key {0};

        // Constructors
        explicit GameBoard();
        explicit GameBoard(const std::string&);

        // Member methods
        const std::string to_fen();

        void set_zobrist();

        template <Piece p>
        inline ull get_pieces() {
            if constexpr (p == Piece::PAWN) { return black_pawns | white_pawns; }
            if constexpr (p == Piece::ROOK) { return black_rooks | white_rooks; }
            if constexpr (p == Piece::KNIGHT) { return black_knights | white_knights; }
            if constexpr (p == Piece::QUEEN) { return black_queens | white_queens; }
            if constexpr (p == Piece::KING) { return black_king | white_king; }
        };

        template <Piece p, Color c>  
        inline ull get_pieces() {
            if constexpr (p == Piece::PAWN && c == Color::BLACK) { return black_pawns; }
            if constexpr (p == Piece::PAWN && c == Color::WHITE) { return white_pawns; }   
            if constexpr (p == Piece::ROOK && c == Color::BLACK) { return black_rooks; }
            if constexpr (p == Piece::ROOK && c == Color::WHITE) { return white_rooks; }
            if constexpr (p == Piece::KNIGHT && c == Color::BLACK) { return black_knights; }
            if constexpr (p == Piece::KNIGHT && c == Color::WHITE) { return white_knights; }     
            if constexpr (p == Piece::QUEEN && c == Color::BLACK) { return black_queens; }
            if constexpr (p == Piece::QUEEN && c == Color::WHITE) { return white_queens; } 
            if constexpr (p == Piece::KING && c == Color::BLACK) { return black_king; }
            if constexpr (p == Piece::KING && c == Color::WHITE) { return white_king; }
            if constexpr (p == Piece::NONE) { return 0ULL; }
        }

        template <Piece p>
        inline ull get_pieces(Color c) {
            if (c == Color::BLACK) { 
                return get_pieces<p, Color::BLACK>(); 
            }
            return get_pieces<p, Color::WHITE>(); 
        };

        template <Color c>
        inline ull get_pieces() {
            if constexpr (c == Color::WHITE) { return white_pawns | white_rooks | white_knights | white_queens | white_king; }   
            if constexpr (c == Color::BLACK) { return black_pawns | black_rooks | black_knights | black_queens | black_king; }
        }

        // ull get_pieces();
        // ull get_pieces(Color);
        // ull get_pieces(Piece);
        // ull get_pieces(Color, Piece);

        inline ull get_pieces(Color color) {
            if (color == Color::WHITE) {
                return white_pawns | white_rooks | white_knights | white_queens | white_king;
            }
            else {
                return black_pawns | black_rooks | black_knights | black_queens | black_king;
            }
        }

        inline ull get_pieces(Piece piece_type) {
            if (piece_type == Piece::PAWN) {
                return black_pawns | white_pawns;
            }
            else if (piece_type == Piece::ROOK) {
                return black_rooks | white_rooks;
            }
            else if (piece_type == Piece::KNIGHT) {
                return black_knights | white_knights;
            }
            else if (piece_type == Piece::QUEEN) {
                return black_queens | white_queens;
            }
            return black_king | white_king;
        }

        inline ull get_pieces(Color color, Piece piece_type) {
            return get_pieces(piece_type) & get_pieces(color);
        }

        inline ull get_pieces() {
            return white_pawns | white_rooks | white_knights | white_queens | white_king | 
                black_pawns | black_rooks | black_knights | black_queens | black_king;
        }

        // Piece get_piece_type_on_bitboard_using_templates(ull bitboard);
        Piece get_piece_type_on_bitboard(ull);
        Color get_color_on_bitboard(ull);
};
} // end namespace ShumiChess

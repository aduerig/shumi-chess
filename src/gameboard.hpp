#pragma once

#include <string>
#include <iostream>

//#define NDEBUG         // Define (uncomment) this to disable asserts
#undef NDEBUG
#include <assert.h>

#include "globals.hpp"

namespace ShumiChess {
// TODO think about copy and move constructors.
// ? Will we ever want to copy?
class GameBoard {
    public:

        // Piece location bitboards
        ull black_pawns {};
        ull white_pawns {};

        ull black_rooks {};
        ull white_rooks {};

        ull black_knights {};
        ull white_knights {};

        ull black_bishops {};
        ull white_bishops {};

        ull black_queens {};
        ull white_queens {};

        ull black_king {};
        ull white_king {};

        // other information about the board state
        Color turn;

        //1<<1 for queenside, 1<<0 for kingside (other bits not used)
        uint8_t black_castle = 0b00000000;
        uint8_t white_castle = 0b00000000;

        //square behind enpassantable pawn after moving
        //0 is impossible state in conventional game
        // ? is this right way to represent this
        // ? do we care about non standard gameboards / moves
        // ? would one consider an extra bit for if we should look at this val
        // ? what about an std::optional https://stackoverflow.com/questions/23523184/overhead-of-stdoptionalt
        ull en_passant {0}; 

        uint64_t zobrist_key {0};

        // move clocks
        uint8_t halfmove;  // Used only to apply the "fifty-move draw" rule in chess
        uint8_t fullmove;  // Used only for display purposes.

        // Constructors
        explicit GameBoard();
        explicit GameBoard(const std::string&);

        // Member methods
        const std::string to_fen();

        void set_zobrist();

        template <Piece p>
        inline ull get_pieces_template() {
            if constexpr (p == Piece::PAWN) { return black_pawns | white_pawns; }
            if constexpr (p == Piece::ROOK) { return black_rooks | white_rooks; }
            if constexpr (p == Piece::KNIGHT) { return black_knights | white_knights; }
            if constexpr (p == Piece::BISHOP) { return black_bishops | white_bishops; }
            if constexpr (p == Piece::QUEEN) { return black_queens | white_queens; }
            if constexpr (p == Piece::KING) { return black_king | white_king; }
        };

        template <Piece p, Color c>  
        inline ull get_pieces_template() {
            if constexpr (p == Piece::PAWN && c == Color::BLACK) { return black_pawns; }
            if constexpr (p == Piece::PAWN && c == Color::WHITE) { return white_pawns; }   
            if constexpr (p == Piece::ROOK && c == Color::BLACK) { return black_rooks; }
            if constexpr (p == Piece::ROOK && c == Color::WHITE) { return white_rooks; }
            if constexpr (p == Piece::KNIGHT && c == Color::BLACK) { return black_knights; }
            if constexpr (p == Piece::KNIGHT && c == Color::WHITE) { return white_knights; }     
            if constexpr (p == Piece::BISHOP && c == Color::BLACK) { return black_bishops; }
            if constexpr (p == Piece::BISHOP && c == Color::WHITE) { return white_bishops; }    
            if constexpr (p == Piece::QUEEN && c == Color::BLACK) { return black_queens; }
            if constexpr (p == Piece::QUEEN && c == Color::WHITE) { return white_queens; } 
            if constexpr (p == Piece::KING && c == Color::BLACK) { return black_king; }
            if constexpr (p == Piece::KING && c == Color::WHITE) { return white_king; }
        }; 

        template <Piece p>
        inline ull get_pieces_template(Color c) {
            if (c == Color::BLACK) { 
                return get_pieces_template<p, Color::BLACK>(); 
            }
            return get_pieces_template<p, Color::WHITE>(); 
        };

        template <Color c>
        inline ull get_pieces_template() {
            if constexpr (c == Color::WHITE) { return white_pawns | white_rooks | white_knights | white_bishops | white_queens | white_king; }   
            else if constexpr (c == Color::BLACK) { return black_pawns | black_rooks | black_knights | black_bishops | black_queens | black_king; }
            else {assert(0);return 0;}
        }

        inline ull get_pieces(Color color) {
            if (color == Color::WHITE) {
                return white_pawns | white_rooks | white_knights | 
                    white_bishops | white_queens | white_king;
            }
            else if (color == Color::BLACK) {
                return black_pawns | black_rooks | black_knights | 
                    black_bishops | black_queens | black_king;
            }
            else {
                std::cout << "Unexpected color in get_pieces 1: " << color << std::endl;
                assert(0);
                return 0;
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
            else if (piece_type == Piece::BISHOP) {
                return black_bishops | white_bishops;
            }
            else if (piece_type == Piece::QUEEN) {
                return black_queens | white_queens;
            }
            else if (piece_type == Piece::KING) {
               return black_king | white_king;
            }
            else {
                std::cout << "Unexpected piece type in get_pieces 2: " << piece_type << std::endl;
                assert(0);
                return 0;
            }
        }

        inline ull get_pieces(Color color, Piece piece_type) {
            return get_pieces(piece_type) & get_pieces(color);
        }

        // Returns a bitboard
        inline ull get_pieces() {
            return white_pawns | white_rooks | white_knights | white_bishops | white_queens | white_king | 
                black_pawns | black_rooks | black_knights | black_bishops | black_queens | black_king;
        }

        // Piece get_piece_type_on_bitboard_using_templates(ull bitboard);
        Piece get_piece_type_on_bitboard(ull);
        Color get_color_on_bitboard(ull);

        bool are_bit_boards_valid() const;
        
        bool king_coords(Color c, double& centerness) const;

        bool rook_connectiveness(Color c, double& connectiveness) const;

};
} // end namespace ShumiChess

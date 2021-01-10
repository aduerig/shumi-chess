#include "gameboard.hpp"

#include <assert.h>
#include <math.h>
#include <vector>

#include "utility.hpp"

namespace ShumiChess {
GameBoard::GameBoard() : 
    black_pawns(0b00000000'11111111'00000000'00000000'00000000'00000000'00000000'00000000),
    white_pawns(0b00000000'00000000'00000000'00000000'00000000'00000000'11111111'00000000),
    black_rooks(0b10000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_rooks(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000001),
    black_knights(0b01000010'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_knights(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'01000010),
    black_bishops(0b00100100'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_bishops(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00100100),
    black_queens(0b00010000'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_queens(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00010000),
    black_king(0b00001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_king(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001000),
    turn(WHITE),
    black_castle(0b00000011),
    white_castle(0b00000011),
    en_passant(0),
    halfmove(0),
    fullmove(1) {
}

GameBoard::GameBoard(const std::string& fen_notation) {
    const std::vector<std::string> fen_components = utility::string::split(fen_notation);
    
    assert(fen_components.size() == 6);
    assert(fen_components[1].size() == 1);
    assert(fen_components[2].size() <= 4);
    assert(fen_components[3].size() <= 2);


    int square_counter = 64;
    for (const char token : fen_components[0]) {
        if (token != '/') { --square_counter; }
        if (token >= 49 && token <= 56) {
            //Token is 1-8
            square_counter -= token-49; //Purposely subtract 1 too few as we always sub 1 to start.
        } else if (token == 'p') {
            this->black_pawns |= 1ULL << square_counter;
        } else if (token == 'P') {
            this->white_pawns |= 1ULL << square_counter;
        } else if (token == 'r') {
            this->black_rooks |= 1ULL << square_counter;
        } else if (token == 'R') {
            this->white_rooks |= 1ULL << square_counter;
        } else if (token == 'n') {
            this->black_knights |= 1ULL << square_counter;
        } else if (token == 'N') {
            this->white_knights |= 1ULL << square_counter;
        } else if (token == 'b') {
            this->black_bishops |= 1ULL << square_counter;
        } else if (token == 'B') {
            this->white_bishops |= 1ULL << square_counter;
        } else if (token == 'q') {
            this->black_queens |= 1ULL << square_counter;
        } else if (token == 'Q') {
            this->white_queens |= 1ULL << square_counter;
        } else if (token == 'k') {
            this->black_king |= 1ULL << square_counter;
        } else if (token == 'K') {
            this->white_king |= 1ULL << square_counter;
        }
    }
    
    assert(square_counter == 0);

    this->turn = fen_components[1] == "w" ? ShumiChess::WHITE : ShumiChess::BLACK;

    for (const char token : fen_components[2]) {
        switch (token)
        {
        case 'k':
            this->black_castle |= 1;
            break;
        case 'q':
            this->black_castle |= 2;
            break;
        case 'K':
            this->white_castle |= 1;
            break;
        case 'Q':
            this->white_castle |= 2; 
            break;
        }
    }

    if (fen_components[3] != "-") { 
        this->en_passant = utility::representation::acn_to_bitboard_conversion(fen_components[3]);
    }

    this->halfmove = std::stoi(fen_components[4]);
    this->fullmove = std::stoi(fen_components[5]);

    assert(square_counter == 0);
    
    this->turn = fen_components[1] == "w" ? ShumiChess::WHITE : ShumiChess::BLACK;
}

// fields for fen are:
// piece placement, current colors turn, castling avaliablity, enpassant, halfmove number (fifty move rule), total moves 
const std::string GameBoard::to_fen() {
    vector<string> fen_components;

    // TODO: piece placement
    fen_components.push_back("");

    // current turn
    string color_rep = "w";
    if (turn == Color::BLACK) {
        color_rep = "b";
    }
    fen_components.push_back(color_rep);

    // TODO: castling
    fen_components.push_back("");

    // TODO: enpassant
    fen_components.push_back("");
    
    // TODO: halfmove number (fifty move rule)
    fen_components.push_back("");

    // TODO: total moves
    fen_components.push_back("");

    // returns string joined by spaces
    return utility::string::join(fen_components, " ");
}

ull GameBoard::get_pieces(Color color) {
    if (color == Color::WHITE) {
        return white_pawns | white_rooks | white_knights | 
               white_bishops | white_queens | white_king;
    }
    else if (color == Color::BLACK) {
        return black_pawns | black_rooks | black_knights | 
               black_bishops | black_queens | black_king;
    }
    assert(false);
}

ull GameBoard::get_pieces(Piece piece_type) {
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
    assert(false);
}

ull GameBoard::get_pieces(Color color, Piece piece_type) {
    return get_pieces(piece_type) & get_pieces(color);
}

ull GameBoard::get_pieces() {
    return get_pieces(Color::WHITE) & get_pieces(Color::BLACK);
}
} // end namespace ShumiChess
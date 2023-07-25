
#include <assert.h>
#include <math.h>
#include <vector>

#include "gameboard.hpp"
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

    unordered_map<ull, char> piece_to_letter = {
        {Piece::BISHOP, 'b'},
        {Piece::KING, 'k'},
        {Piece::KNIGHT, 'n'},
        {Piece::PAWN, 'p'},
        {Piece::ROOK, 'r'},
        {Piece::QUEEN, 'q'},
    };

    vector<string> piece_positions;
    for (int i = 7; i > -1; i--) {
        string poses;
        int compressed = 0;
        for (int j = 0; j < 8; j++) {
            ull bitboard_of_square = 1ULL << ((i * 8) + (7 - j));
            Piece piece_found = get_piece_type_on_bitboard(bitboard_of_square);
            if (piece_found == Piece::NONE) {
                compressed += 1;
            }
            else {
                if (compressed) {
                    poses += to_string(compressed);
                    compressed = 0;
                }
                if (get_color_on_bitboard(bitboard_of_square) == Color::WHITE) {
                    poses += toupper(piece_to_letter[piece_found]);
                }
                else {
                    poses += piece_to_letter[piece_found];
                }
            }
        }
        if (compressed) {
            poses += to_string(compressed);
        }
        piece_positions.push_back(poses);
    }
    fen_components.push_back(utility::string::join(piece_positions, "/"));

    // current turn
    string color_rep = "w";
    if (turn == Color::BLACK) {
        color_rep = "b";
    }
    fen_components.push_back(color_rep);

    // TODO: castling
    string castlestuff;
    if (0b00000001 & white_castle) {
        castlestuff += 'K';
    }
    if (0b00000010 & white_castle) {
        castlestuff += 'Q';
    }
    if (0b00000001 & black_castle) {
        castlestuff += 'k';
    }
    if (0b00000010 & black_castle) {
        castlestuff += 'q';
    }
    if (castlestuff.empty()) {
        castlestuff = "-";
    }
    fen_components.push_back(castlestuff);

    // TODO: enpassant
    string enpassant_info = "-";
    if (en_passant != 0) {
        enpassant_info = utility::representation::bitboard_to_acn_conversion(en_passant);
    }
    fen_components.push_back(enpassant_info);
    
    // TODO: halfmove number (fifty move rule)
    fen_components.push_back(to_string(halfmove));

    // TODO: total moves
    fen_components.push_back(to_string(fullmove));

    // returns string joined by spaces
    return utility::string::join(fen_components, " ");
}


Piece GameBoard::get_piece_type_on_bitboard(ull bitboard) {
    vector<Piece> all_piece_types = { Piece::PAWN, Piece::ROOK, Piece::KNIGHT, Piece::BISHOP, Piece::QUEEN, Piece::KING };
    for (auto piece_type : all_piece_types) {
        if (get_pieces(piece_type) & bitboard) {
            return piece_type;
        }
    }
    return Piece::NONE;
}

Color GameBoard::get_color_on_bitboard(ull bitboard) {
    if (get_pieces(Color::WHITE) & bitboard) {
        return Color::WHITE;
    }
    return Color::BLACK;
    // vector<Piece> all_piece_types = { Piece::PAWN, Piece::ROOK, Piece::KNIGHT, Piece::BISHOP, Piece::QUEEN, Piece::KING };
    // vector<Color> all_colors = {Color::WHITE, Color::BLACK};
    // for (auto piece_type : all_piece_types) {
    //     for (auto color : all_colors) {
    //         if (get_pieces(color, piece_type) & bitboard) {
    //             return color;
    //         }
    //     }
    // }
    // assert(false);
    // // TODO remove this, i'm just putting it here because it prevents a warning
    // return Color::WHITE;
}
} // end namespace ShumiChess
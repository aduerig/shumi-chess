
#include <assert.h>
#include <math.h>
#include <vector>

#include "gameboard.hpp"
#include "utility.hpp"

using namespace std;

namespace ShumiChess {
GameBoard::GameBoard() : 
    black_pawns(0b00000000'00000000'00000000'00000000'01100000'00010000'00010000'00000000),
    white_pawns(0b00000000'00001000'00001000'00000110'00000000'00000000'00000000'00000000),
    black_rooks(0b00000000'00000000'00000000'00000000'00000000'00100000'00000000'00000000),
    white_rooks(0b00000000'00000000'00000100'00000000'00000000'00000000'00000000'00000000),
    black_knights(0b00000000'00000000'00000000'00000000'00000000'01000000'00100000'00000000),
    white_knights(0b00000000'00000100'00000010'00000000'00000000'00000000'00000000'00000000),
    black_queens(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000000),
    white_queens(0b00000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    black_king(0b00000000'00000000'00000000'00000000'00000000'00000000'01000000'00000000),
    white_king(0b00000000'00000010'00000000'00000000'00000000'00000000'00000000'00000000),
    turn(BLACK) {
    ShumiChess::initialize_zobrist();
    set_zobrist();
}

GameBoard::GameBoard(const std::string& fen_notation) {
    const std::vector<std::string> fen_components = utility::our_string::split(fen_notation);
    
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

    assert(square_counter == 0);
    
    this->turn = fen_components[1] == "w" ? ShumiChess::WHITE : ShumiChess::BLACK;
    ShumiChess::initialize_zobrist();
    set_zobrist();
}

void GameBoard::set_zobrist() {
    for (int color_int = 0; color_int < 2; color_int++) {
        Color color = static_cast<Color>(color_int);
        for (int j = 0; j < 5; j++) {
            Piece piece_type = static_cast<Piece>(j);
            ull bitboard = get_pieces(color, piece_type);
            while (bitboard) {
                int square = utility::bit::lsb_and_pop_to_square(bitboard);
                zobrist_key ^= zobrist_piece_square[piece_type + color * 5][square];
            }
        }
    }

    if (turn == Color::BLACK) {
        zobrist_key ^= zobrist_side;
    }
}

// fields for fen are:
// piece placement, current colors turn, castling avaliablity, enpassant, halfmove number (fifty move rule), total moves 
const string GameBoard::to_fen() {
    vector<string> fen_components;

    unordered_map<ull, char> piece_to_letter = {
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
    fen_components.push_back(utility::our_string::join(piece_positions, "/"));

    // current turn
    string color_rep = "w";
    if (turn == Color::BLACK) {
        color_rep = "b";
    }
    fen_components.push_back(color_rep);

    // castling avaliablity + enpassant
    fen_components.push_back("-");
    fen_components.push_back("-");
    
    // halfmove and fullmove
    fen_components.push_back(to_string(0));
    fen_components.push_back(to_string(0));

    // returns string joined by spaces
    return utility::our_string::join(fen_components, " ");
}


Piece GameBoard::get_piece_type_on_bitboard(ull bitboard) {
    if (get_pieces<Piece::PAWN>() & bitboard) {
        return Piece::PAWN;
    }
    else if (get_pieces<Piece::KNIGHT>() & bitboard) {
        return Piece::KNIGHT;
    }
    else if (get_pieces<Piece::ROOK>() & bitboard) {
        return Piece::ROOK;
    }
    else if (get_pieces<Piece::QUEEN>() & bitboard) {
        return Piece::QUEEN;
    }
    else if (get_pieces<Piece::KING>() & bitboard) {
        return Piece::KING;
    }
    return Piece::NONE;
}

Color GameBoard::get_color_on_bitboard(ull bitboard) {
    if (get_pieces(Color::WHITE) & bitboard) {
        return Color::WHITE;
    }
    return Color::BLACK;
}
} // end namespace ShumiChess
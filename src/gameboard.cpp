

#include <math.h>
#include <vector>

//#define NDEBUG         // Define (uncomment) this to disable asserts
#undef NDEBUG
#include <assert.h>

#include "gameboard.hpp"
#include "utility.hpp"

using namespace std;

namespace ShumiChess {

    GameBoard::GameBoard() : 
 
    // IN this constructer, the bitboards are the input
    #include "gameboardSetup.hpp"

    {

        // !TODO doesn't belong here i don't think.
        //ShumiChess::initialize_zobrist();
        //set_zobrist();
 
        bool no_pieces_on_same_square = are_bit_boards_valid();
        assert(no_pieces_on_same_square);


    }
//
// IN this constructer, the FEN is the input
// Makes a "bitboard" Gameboard based on a FEN.
//
GameBoard::GameBoard(const std::string& fen_notation) {
    const std::vector<std::string> fen_components = utility::our_string::split(fen_notation);
    
    // Make sure FEN components are valid
    assert(fen_components.size() == 6);
    assert(fen_components[1].size() == 1);
    assert(fen_components[2].size() <= 4);
    assert(fen_components[3].size() <= 2);

    int square_counter = 64;
    for (const char token : fen_components[0]) {

        if (token != '/') {    // In FEN notation these are just seperaters.
            --square_counter;
        }

        if (token >= 49 && token <= 56) {
            // These tokens represent the digits "1" to "8"
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
        default:        // Note: skipping a default is illegal in some states.
            // std::cout << "Unexpected castling rights token: " << token << std::endl;
            // assert(0);  // Note: this fires in some test, is it ok? What is the default case?
            break;
        }
    }

    if (fen_components[3] != "-") { 
        this->en_passant = utility::representation::acn_to_bitboard_conversion(fen_components[3]);
    }

    // halfmove is used to apply the "fifty-move draw" rule in chess
    this->halfmove = std::stoi(fen_components[4]);  

    // fullmove is Used only for display purposes.
    this->fullmove = std::stoi(fen_components[5]);



    bool no_pieces_on_same_square = are_bit_boards_valid();
    assert(no_pieces_on_same_square);


    this->turn = fen_components[1] == "w" ? ShumiChess::WHITE : ShumiChess::BLACK;
    //ShumiChess::initialize_zobrist();
    //set_zobrist();
}



void GameBoard::set_zobrist() {
    // cout << "GameBoard::setting zobrist..." << endl;
    for (int color_int = 0; color_int < 2; color_int++) {
        Color color = static_cast<Color>(color_int);
        for (int j = 0; j < 6; j++) {
            Piece piece_type = static_cast<Piece>(j);
            ull bitboard = get_pieces(color, piece_type);
            while (bitboard) {
                int square = utility::bit::lsb_and_pop_to_square(bitboard);
                zobrist_key ^= zobrist_piece_square[piece_type + color * 6][square];
            }
        }
    }

    // if (st->epSquare != SQ_NONE)
    //     st->key ^= Zobrist::enpassant[file_of(st->epSquare)];

    if (turn == Color::BLACK) {
        zobrist_key ^= zobrist_side;
    }

    // cout << "zobrist key starts at: " << zobrist_key << endl;
    // st->key ^= Zobrist::castling[st->castlingRights];
}

//
// fields for fen are:
// piece placement, current colors turn, castling avaliablity, enpassant, halfmove number (fifty move rule), total moves 
const string GameBoard::to_fen() {
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
    fen_components.push_back(utility::our_string::join(piece_positions, "/"));

    // current turn
    string color_rep = "w";
    if (turn == Color::BLACK) {
        color_rep = "b";
    }
    fen_components.push_back(color_rep);

    // TODO: castling
    // NOTE: What do you mean, TODO. here and below, What's to do? Is this an unfinished project?
    // I think the TODOs are outdated.
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
    return utility::our_string::join(fen_components, " ");
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

// Is there only zero or one pieces on eacxh square?
bool GameBoard::are_bit_boards_valid() const {
    ull occupied = 0ULL;

    // Make an array of the bit maps.
    const ull bbs[] = {
        black_pawns,  white_pawns,
        black_rooks,  white_rooks,
        black_knights,white_knights,
        black_bishops,white_bishops,
        black_queens, white_queens,
        black_king,   white_king
    };

    for (ull bb : bbs) {
        if (bb & occupied) return false;  // overlap: two+ pieces on a square
        occupied |= bb;
    }
    return true; // no overlaps found
}

bool GameBoard::king_coords(Color c, double& centerness) const {
    double row; 
    double col;
    ull bb = (c == Color::WHITE) ? white_king : black_king;
    if (!bb) return false;

    ull tmp = bb; // don’t mutate the real bitboard
    int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63

    int row_idx = s / 8;            // 0..7
    int col_idx = 7 - (s % 8);      // 0..7 (a..h)

    row = static_cast<double>(row_idx) - 3.5;  // -3.5 .. 3.5
    col = static_cast<double>(col_idx) - 3.5;  // -3.5 .. 3.5


    centerness = ((fabs(row)/3.5) + (fabs(col)/3.5)) / 2.0; 
    // Gets smaller closer to center. (0 at dead center, 1.0 on furthest corners)

    return true;
}


bool GameBoard::rook_connectiveness(Color c, double& connectiveness) const
{
    using ull = unsigned long long;

    const ull rooks = (c == Color::WHITE) ? white_rooks : black_rooks;
    const ull my_pieces =
        (c == Color::WHITE)
        ? (white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king)
        : (black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king);

    const int back_rank = (c == Color::WHITE) ? 0 : 7;          // rank 1 (white) or rank 8 (black)
    const ull back_mask = 0xFFULL << (back_rank * 8);
    ull back_rooks = rooks & back_mask;                         // only rooks on the back rank

    // Need at least two rooks on the back rank
    if (back_rooks == 0 || (back_rooks & (back_rooks - 1)) == 0) {
        connectiveness = 0.0;
        return false;
    }

    // Copy the mask; we will *extract* two rook squares from this temporary.
    // lsb_and_pop_to_square(tmp) returns the index (0..63) of the least-significant 1 bit
    // and clears that bit in 'tmp'. We use a temp so the real board state is untouched.
    ull tmp = back_rooks;
    int s1 = utility::bit::lsb_and_pop_to_square(tmp);          // first rook square (by lowest bit)
    int s2 = utility::bit::lsb_and_pop_to_square(tmp);          // second rook square

    // Convert to files (0=a .. 7=h)
    int f1 = s1 % 8;
    int f2 = s2 % 8;

    // Ensure f1 <= f2 so f1 is the left rook and f2 is the right rook on the back rank.
    // (This just normalizes order; it does not change the position.)
    if (f1 > f2) std::swap(f1, f2);

    int files_between = f2 - f1 - 1;                            // squares strictly between
    if (files_between <= 0) {                                   // adjacent rooks ⇒ fully connected
        connectiveness = 1.0;
        return true;
    }

    // Count OWN pieces strictly between the two rooks on the back rank
    int base = back_rank * 8;
    int own_between = 0;
    for (int f = f1 + 1; f <= f2 - 1; ++f) {
        ull bit = 1ULL << (base + f);
        if (my_pieces & bit) ++own_between;
    }

    connectiveness = 1.0 - (static_cast<double>(own_between) / files_between);
    if (connectiveness < 0.0) connectiveness = 0.0;             // clamp
    return true;
}



} // end namespace ShumiChess
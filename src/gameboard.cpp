

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

// Return false if king does not exist. But in any case, returns the correct connectiveness.
// connectiveness gets smaller closer to center. (0 at dead center, 1.0 on furthest corners)
bool GameBoard::king_anti_centerness(Color c, double& centerness) const {
    double row; 
    double col;
    ull bb = (c == Color::WHITE) ? white_king : black_king;
    if (!bb) return false;

    // Finds the index (0–63) of the least-significant 1-bit in bitboard, and returns the index.
    ull tmp = bb; // don’t mutate (pop) the real bitboard
    int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63

    int row_idx = s / 8;            // 0..7
    int col_idx = 7 - (s % 8);      // 0..7 (a..h)

    row = static_cast<double>(row_idx) - 3.5;  // -3.5 .. 3.5
    col = static_cast<double>(col_idx) - 3.5;  // -3.5 .. 3.5


    centerness = ((fabs(row)/3.5) + (fabs(col)/3.5)) / 2.0; 
    // Gets smaller closer to center. (0 at dead center, 1.0 on furthest corners)

    return true;
}




bool GameBoard::knights_centerness(Color c, double& centerness) const
{
    ull bb = (c == Color::WHITE) ? white_knights : black_knights;
    if (!bb) return false;

    double sum = 0.0;
    int count = 0;

    ull tmp = bb; // don’t mutate the real bitboard
    while (tmp)
    {
        // Finds the index (0–63) of the least-significant 1-bit in bitboard, and returns the index.
        int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63

        int row_idx = s / 8;           // 0..7
        int col_idx = 7 - (s % 8);     // 0..7 (a..h)

        double row = static_cast<double>(row_idx) - 3.5;  // -3.5 .. 3.5
        double col = static_cast<double>(col_idx) - 3.5;  // -3.5 .. 3.5

        // 0 at corners, 1.0 at center (inverted from the king version)
        double away   = ((fabs(row)/3.5) + (fabs(col)/3.5)) / 2.0; // 0 center, 1 corner
        double toward = 1.0 - away;                                // 1 center, 0 corner

        sum += toward;
        ++count;
    }

    centerness = sum / static_cast<double>(count);
    return true;
}

// return false if two rooks dont exist. But in any case, returns the correct connectiveness.
bool GameBoard::rook_connectiveness(Color c, double& connectiveness) const
{
    using ull = unsigned long long;

    const ull rooks = (c == Color::WHITE) ? white_rooks : black_rooks;

    // Need at least two rooks of this color
    if ((rooks == 0) || ((rooks & (rooks - 1)) == 0)) {
        connectiveness = 0.0;
        return false;
    }

    // Occupancy of all pieces (both sides)
    const ull occupancy =
        white_pawns | white_knights | white_bishops | white_rooks | white_queens | white_king |
        black_pawns | black_knights | black_bishops | black_rooks | black_queens | black_king;

    // Collect squares of this side's rooks
    int sqs[8]; // promotion could make more than 2, but 8 is plenty
    int n = 0;
    ull tmp = rooks;
    while (tmp) {
        sqs[n++] = utility::bit::lsb_and_pop_to_square(tmp); // 0..63
    }

    auto clear_between_rank = [&](int a, int b) -> bool {
        const int ra = a / 8;               // same rank as b
        int fa = a % 8, fb = b % 8;
        if (fa > fb) std::swap(fa, fb);
        for (int f = fa + 1; f < fb; ++f) {
            const int sq = ra * 8 + f;
            if (occupancy & (1ULL << sq)) return false;
        }
        return true;
    };

    auto clear_between_file = [&](int a, int b) -> bool {
        const int fa = a % 8;               // same file as b
        int ra = a / 8, rb = b / 8;
        if (ra > rb) std::swap(ra, rb);
        for (int r = ra + 1; r < rb; ++r) {
            const int sq = r * 8 + fa;
            if (occupancy & (1ULL << sq)) return false;
        }
        return true;
    };

    // Check all rook pairs: same rank OR same file, with empty squares between
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const int s1 = sqs[i], s2 = sqs[j];
            if ((s1 / 8 == s2 / 8) && clear_between_rank(s1, s2)) {
                connectiveness = 1.0;
                return true;
            }
            if ((s1 % 8 == s2 % 8) && clear_between_file(s1, s2)) {
                connectiveness = 1.0;
                return true;
            }
        }
    }

    connectiveness = 0.0;
    return false;
}

// Count isolated doubled/tripled pawns for side c. DOES not flag regular (single) isolanis
// For each file with k>=2 pawns and no friendly pawns on adjacent files,
// add (k-1). Triples add 2, etc.
int GameBoard::count_isolated_doubled_pawns(Color c) const
{
    using ull = unsigned long long;

    const ull P = (c == Color::WHITE) ? white_pawns : black_pawns;
    if (!P) return 0;

    int file_count[8] = {0};   // file index: 0..7 per your s%8 convention
    unsigned files_present = 0;

    ull tmp = P;
    while (tmp) {
        const int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63
        const int f = s % 8;                                    // file index
        ++file_count[f];
        files_present |= (1u << f);
    }

    int isolations = 0;
    for (int f = 0; f < 8; ++f) {
        const int k = file_count[f];
        if (k < 2) continue;  // not doubled/tripled

        const bool left  = (f < 7) && (files_present & (1u << (f + 1)));
        const bool right = (f > 0) && (files_present & (1u << (f - 1)));

        if (!left && !right)
            isolations += (k - 1);  // count the "extras" on that isolated file
    }

    return isolations;
}





} // end namespace ShumiChess
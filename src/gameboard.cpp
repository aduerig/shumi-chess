

#include <math.h>
#include <vector>

#undef NDEBUG
//#define NDEBUG         // Define (uncomment) this to disable asserts
#include <assert.h>

#include "gameboard.hpp"
#include "move_tables.hpp"
#include "utility.hpp"

using namespace std;






namespace ShumiChess {

    GameBoard::GameBoard() : 
 
    // IN this constructer, the bitboards are the input
    #include "gameboardSetup.hpp"

    {

        // !TODO doesn't belong here i don't think. I think it does.
        ShumiChess::initialize_zobrist();
        set_zobrist();
 
        // No multiple pieces on the same square.
        bool no_pieces_on_same_square = are_bit_boards_valid();
        assert(no_pieces_on_same_square);


    }
//
// IN this constructer, the FEN is the input
// Makes a "bitboard" Gameboard based on a FEN.
//
GameBoard::GameBoard(const std::string& fen_notation) {
    const std::vector<std::string> fen_components = utility::our_string::split(fen_notation);
    
    //cout << "\x1b[33mrestart from fen " << fen_notation << "\x1b[0m" << endl;

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
            this->black_castle_rights |= 1;
            break;
        case 'q':
            this->black_castle_rights |= 2;
            break;
        case 'K':
            this->white_castle_rights |= 1;
            break;
        case 'Q':
            this->white_castle_rights |= 2; 
            break;
        default:
            // std::cout << "Unexpected castling rights token: " << token << std::endl;
            // assert(0);  // Note: this fires in some test, is it ok? What is the default case?
            break;
        }
    }

    if (fen_components[3] != "-") { 
        this->en_passant_rights = utility::representation::acn_to_bitboard_conversion(fen_components[3]);
    }

    // halfmove is used to apply the "fifty-move draw" rule in chess
    this->halfmove = std::stoi(fen_components[4]);  

    // fullmove is Used only for display purposes.
    this->fullmove = std::stoi(fen_components[5]);


    
    bool no_pieces_on_same_square = are_bit_boards_valid();
    assert(no_pieces_on_same_square);


    this->turn = fen_components[1] == "w" ? ShumiChess::WHITE : ShumiChess::BLACK;

    ShumiChess::initialize_zobrist();
    set_zobrist();
}



void GameBoard::set_zobrist() {
    zobrist_key = 0;
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

const string GameBoard::to_fen(bool bFullFEN) {

    //cout << "\x1b[34mto_fen!\x1b[0m" << endl;
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

    // castling
    string castlestuff;
    if (0b00000001 & white_castle_rights) {
        castlestuff += 'K';
    }
    if (0b00000010 & white_castle_rights) {
        castlestuff += 'Q';
    }
    if (0b00000001 & black_castle_rights) {
        castlestuff += 'k';
    }
    if (0b00000010 & black_castle_rights) {
        castlestuff += 'q';
    }
    if (castlestuff.empty()) {
        castlestuff = "-";
    }
    fen_components.push_back(castlestuff);

    // TODO: enpassant
    string enpassant_info = "-";
    if (en_passant_rights != 0) {
        enpassant_info = utility::representation::bitboard_to_acn_conversion(en_passant_rights);
    }
    fen_components.push_back(enpassant_info);

    if (bFullFEN) {
    
        // TODO: halfmove number (fifty move rule)
        fen_components.push_back(to_string(halfmove));

        // TODO: total moves
        fen_components.push_back(to_string(fullmove));

    }

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
    } else {
        return Color::BLACK;
    }
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
//
// Two things (bools) about castling. 
//    1. Can yu castle or not?  (do you have the priviledge)  
//    2. Whether YOU have castled or not. The latter IS NOT stored in a FEN by the way. It has to be a bollean 
//       maintained by the push/pop.
//
int GameBoard::get_castle_status_for_color(Color color1) const {
    bool b_can_castle;
    bool b_has_castled;
    if (color1 == ShumiChess::WHITE) {
        b_can_castle = (white_castle_rights != 0); 
        b_has_castled = bCastledWhite;
    } else {
        b_can_castle = (black_castle_rights != 0); 
        b_has_castled = bCastledBlack;
    }

    int icode = (int)(b_has_castled)*2.0 + (int)(b_can_castle);
    return icode;
}


//
// Returns centipawns. Always positive. 
int GameBoard::get_material_for_color(Color color, int& cp_pawns_only_temp) {

 
    //int cp_score_mat_temp_pawns;
    ull pieces_bitboard;
    int cp_board_score;
    int nPieces;


    // Pawns first
    cp_pawns_only_temp = 0;
    // Get bitboard of all pieces on board of this type and color
    pieces_bitboard = get_pieces(color,  Piece::PAWN);
    // Adds for the piece value multiplied by how many of that piece there is (using centipawns)
    cp_board_score = centipawn_score_of( Piece::PAWN);
    nPieces = bits_in(pieces_bitboard);
    cp_pawns_only_temp += (int)(((double)nPieces * (double)cp_board_score));
    // This return must always be positive.
    assert (cp_pawns_only_temp>=0);


    // Add up the scores for each piece
    int cp_score_mat_temp = 0;                    // no pawns in this one
    for (Piece piece_type = Piece::ROOK;
        piece_type <= Piece::QUEEN;
        piece_type = static_cast<Piece>(static_cast<int>(piece_type) + 1))     // increment
    {    
        
        // Get bitboard of all pieces on board of this type and color
        pieces_bitboard = get_pieces(color, piece_type);
        // Adds for the piece value multiplied by how many of that piece there is (using centipawns)
        cp_board_score = centipawn_score_of(piece_type);
        nPieces = bits_in(pieces_bitboard);
        cp_score_mat_temp += (int)(((double)nPieces * (double)cp_board_score));
        // This return must always be positive.
        assert (cp_score_mat_temp>=0);

    }

    cp_score_mat_temp += cp_pawns_only_temp;


    return cp_score_mat_temp;
}



int GameBoard::bits_in(ull bitboard) const {
    auto bs = bitset<64>(bitboard);
    return (int) bs.count();
}

// “lerp” stands for Linear intERPolation.
// Linear interpolation: t=0 → a, t=1 → b
inline double lerp(double a, double b, double t) { return a + (b - a) * t; }

//
// connectiveness gets smaller closer to center. (0 at dead center, 1.0 on furthest corners)
void GameBoard::king_castle_happiness(Color c, int& centerness) const {
   
    centerness = (double)(get_castle_status_for_color(c));
    assert(centerness>=0);

    return;
}

int GameBoard::queen_still_home(Color color)
{
    // h1 = 0 indexing:
    // d1 = 4
    // d8 = 60
    const int sq_d1 = 4;
    const int sq_d8 = 60;

    if (color == Color::WHITE)
    {
        // Is there still a white queen on d1?
        ull mask = (1ULL << sq_d1);

        // Return 1 if queen hasn't moved (still on d1),
        // 0 if it has moved off d1.
        return (white_queens & mask) ? 1 : 0;
    }
    else // BLACK
    {
        // Is there still a black queen on d8?
        ull mask = (1ULL << sq_d8);

        return (black_queens & mask) ? 1 : 0;
    }
}


// Stupid bishop blocking pawn
int GameBoard::bishop_pawn_pattern(Color color)
{
    // h1 = 0 indexing:
    // d2 = 12
    // d3 = 20
    // d6 = 44
    // d7 = 52
    const int sq_d2 = 12;
    const int sq_d3 = 20;
    const int sq_d6 = 44;
    const int sq_d7 = 52;

    if (color == Color::WHITE)
    {
        // White: bishop on d3, pawn on d2
        ull bishop_mask = (1ULL << sq_d3);
        ull pawn_mask   = (1ULL << sq_d2);

        if ( (white_bishops & bishop_mask) &&
             (white_pawns   & pawn_mask) )
        {
            return 1;
        }
    }
    else // color == BLACK
    {
        // Black: bishop on d6, pawn on d7
        ull bishop_mask = (1ULL << sq_d6);
        ull pawn_mask   = (1ULL << sq_d7);

        if ( (black_bishops & bishop_mask) &&
             (black_pawns   & pawn_mask) )
        {
            return 1;
        }
    }

    return 0;
}


int GameBoard::pawns_attacking_square(Color c, int sq)
{
    ull bit = (1ULL << sq);
    const ull FILE_A = col_masks[Col::COL_A];
    const ull FILE_H = col_masks[Col::COL_H];

    ull origins;
    if (c == Color::WHITE) {
        // white pawn origins that attack sq: from (sq-7) and (sq-9)
        origins = ((bit & ~FILE_A) >> 7) | ((bit & ~FILE_H) >> 9);
    } else {
        // black pawn origins that attack sq: from (sq+7) and (sq+9)
        origins = ((bit & ~FILE_H) << 7) | ((bit & ~FILE_A) << 9);
    }

    ull pawns = get_pieces_template<Piece::PAWN>(c);
    return bits_in(origins & pawns);
}



int GameBoard::pawns_attacking_center_squares(Color c) {
    
    constexpr int CENTER_W = 2;   // weight for e4,d4,e5,d5
    constexpr int ADV_W    = 1;   // weight for e6,d6 (White) or e3,d3 (Black)

    int sum = 0;

    sum += CENTER_W * (  pawns_attacking_square(c, square_e4)
                    + pawns_attacking_square(c, square_d4)
                    + pawns_attacking_square(c, square_e5)
                    + pawns_attacking_square(c, square_d5));

    if (c == Color::WHITE)
        sum += ADV_W * (  pawns_attacking_square(c, square_e6)
                        + pawns_attacking_square(c, square_d6));
    else
        sum += ADV_W * (  pawns_attacking_square(c, square_e3)
                        + pawns_attacking_square(c, square_d3));

    return sum;

}


int GameBoard::knights_attacking_square(Color c, int sq)
{
    ull targets = tables::movegen::knight_attack_table[sq];
    ull knights = get_pieces_template<Piece::KNIGHT>(c);
    return bits_in(targets & knights);  // count the 1-bits
}

int GameBoard::knights_attacking_center_squares(Color c)
{
    int square_e4 = 27;
    int square_d4 = 28;
    int square_e5 = 35;
    int square_d5 = 36;
    int itemp = 0;
    itemp += knights_attacking_square(c, square_e4);
    itemp += knights_attacking_square(c, square_d4);
    itemp += knights_attacking_square(c, square_e5);
    itemp += knights_attacking_square(c, square_d5);
    return itemp;
}

//
// One if rooks connected. 0 if not.
// return false if two rooks dont exist. But in any case, returns the correct connectiveness.
// Note sure what happens with three or more rooks.
bool GameBoard::rook_connectiveness(Color c, int& connectiveness) const
{
    const ull rooks = (c == Color::WHITE) ? white_rooks : black_rooks;

    // Need at least two rooks of this color
    if ((rooks == 0) || ((rooks & (rooks - 1)) == 0)) {
        connectiveness = 0;
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
                connectiveness = 1;
                return true;
            }
            if ((s1 % 8 == s2 % 8) && clear_between_file(s1, s2)) {
                connectiveness = 1;
                return true;
            }
        }
    }

    connectiveness = 0;
    return false;
}

//
// Return 2 if any friendly rook is on an OPEN file (no pawns on that file).
// Return 1 if any friendly rook is on a SEMI-OPEN file (no friendly pawns on that file, but at least one enemy pawn).
// Return 0 otherwise.
int GameBoard::rook_file_status(Color c) const {

    const ull rooks     = (c == Color::WHITE) ? white_rooks : black_rooks;
    const ull own_pawns = (c == Color::WHITE) ? white_pawns  : black_pawns;
    const ull opp_pawns = (c == Color::WHITE) ? black_pawns  : white_pawns;
    const ull all_pawns = own_pawns | opp_pawns;

    if (!rooks) return 0;

    int score = 0;  // +2 per rook on open file, +1 per rook on semi-open

    ull tmp = rooks;
    while (tmp) {
        int s  = utility::bit::lsb_and_pop_to_square(tmp); // 0..63
        int f  = s % 8;            // 0=H ... 7=A in h1=0 layout
        int fi = 7 - f;            // map to col_masks index: 0=A ... 7=H
        ull file_mask = col_masks[fi];

        if ( (all_pawns & file_mask) == 0ULL ) {
            score += 2;                                    // open file
        } else if ( (own_pawns & file_mask) == 0ULL &&
                    (opp_pawns & file_mask) != 0ULL ) {
            score += 1;                                    // semi-open
        }
    }
    return score;  // 0..4 (two rooks)
}



int GameBoard::rook_7th_rankness(Color c) const   /* now counts R+Q; +1 each on enemy 7th */
{
    const ull rooks  = (c == Color::WHITE) ? white_rooks  : black_rooks;
    const ull queens = (c == Color::WHITE) ? white_queens : black_queens;
    ull rq = rooks | queens;
    if (!rq) return 0;

    const int target_rank = (c == Color::WHITE) ? 6 : 1; // enemy 7th rank: White→6, Black→1

    int score = 0; // +1 per rook or queen on enemy 7th (max 2 if both R+Q there)
    while (rq) {
        int s = utility::bit::lsb_and_pop_to_square(rq); // 0..63
        if ((s / 8) == target_rank) ++score;
    }
    return score;
}
//
// returns true only if "insufficient material
// NOTE: known errors here: this logic declares the follwing positions drawn, when they are not:
//      two bishops on one side
//      
//     
bool GameBoard::insufficient_material_simple()
{
    // 1) No pawns anywhere
    ull pawns = white_pawns | black_pawns;
    if (pawns) return false;

    // 2) No queens or rooks anywhere
    ull majors = (white_rooks | black_rooks | white_queens | black_queens);
    if (majors) return false;

    // 3) Count total minor pieces (knights + bishops), both colors
    auto popcount = [](ull x) {
        int c = 0;
        while (x) { x &= (x - 1); ++c; }   // Kernighan’s bit count
        return c;
    };

    int n_knights = popcount(white_knights | black_knights);
    int n_bishops = popcount(white_bishops | black_bishops);
    int minors = n_knights + n_bishops;

    // Your rule: if total minors ≤ 2, it's insufficient
    return (minors <= 2);
}


//
// counts 1 for each isolated pawn, 2 for a isolated doubled pawn, 3 for tripled isolated pawn.
// One count for each instance.
//
int GameBoard::count_isolated_pawns(Color c) const {
    const ull P = (c == Color::WHITE) ? white_pawns : black_pawns;
    if (!P) return 0;

    int file_count[8] = {0};
    unsigned files_present = 0;

    ull tmp = P;
    while (tmp) {
        int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63
        int f = s % 8;                                    // 0..7
        ++file_count[f];
        files_present |= (1u << f);
    }

    int total = 0;
    for (int f = 0; f < 8; ++f) {
        int k = file_count[f];            // pawns on this file
        if (k == 0) continue;

        bool left  = (f < 7) && (files_present & (1u << (f + 1)));
        bool right = (f > 0) && (files_present & (1u << (f - 1)));

        if (!left && !right) {
            // isolated file: single→1, double→2, triple→3, etc.
            total += k;
        }
    }
    return total;
}

//
// Rewards king near the center.
int GameBoard::king_center_weight(Color color) {
    ull kbb = (color == Color::WHITE) ? white_king : black_king;
    assert(kbb != 0ULL);
    int sq = utility::bit::lsb_and_pop_to_square(kbb);  // 0..63 (local copy, ok)

    int r = sq / 8;              // 0..7
    int f = sq % 8;              // 0..7
    int dr = std::min(std::abs(r - 3), std::abs(r - 4));
    int df = std::min(std::abs(f - 3), std::abs(f - 4));
    int ring = std::max(dr, df); // 0..3

    static const int W[4] = {4, 3, 2, 1};  // 2.0, 1.5, 1.0, 0.5 times 2
    if (ring < 0) ring = 0;
    if (ring > 3) ring = 3;
    return W[ring];
}

// Passed pawns bonus in centipawns:
//  - 10 cp on 3rd/4th rank (from that side's perspective)
//  - 20 cp on 5th/6th rank
//  - 30 cp on 7th rank
int GameBoard::count_passed_pawns(Color c) {

    const ull my_pawns  = get_pieces(c, Piece::PAWN);
    const ull his_pawns = get_pieces(utility::representation::opposite_color(c), Piece::PAWN);
    if (!my_pawns) return 0;

    int bonus = 0;
    ull tmp = my_pawns;

    while (tmp) {
        const int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63
        const int f = s % 8;            // h1=0 → 0=H ... 7=A
        const int r = s / 8;            // rank index 0..7 (White's view)

        // Map to A..H index used by col_masks
        const int fi = 7 - f;

        // Same + adjacent files
        ull files_mask = col_masks[fi];
        if (fi > 0) files_mask |= col_masks[fi - 1];
        if (fi < 7) files_mask |= col_masks[fi + 1];

        // Ranks ahead toward promotion
        ull ranks_ahead;
        if (c == Color::WHITE) {
            const int start_bit = (r + 1) * 8;
            ranks_ahead = (start_bit >= 64) ? 0ULL : (~0ULL << start_bit);
        } else {
            const int end_bit = r * 8;
            ranks_ahead = (end_bit <= 0) ? 0ULL : ((1ULL << end_bit) - 1);
        }

        // Passed?
        const ull blockers = his_pawns & files_mask & ranks_ahead;
        if (blockers == 0ULL) {
            // advancement from that side's perspective (2..6 typical for passers)
            const int adv = (c == Color::WHITE) ? r : (7 - r);
            if      (adv == 6) bonus += 30;      // 7th rank
            else if (adv >= 4) bonus += 20;      // 5th/6th
            else if (adv == 3) bonus += 15;      // 4th
            
            else if (adv >= 1) bonus += 10;      // 2cnd/3rd
            // (adv < 2 → no bonus; tweak if you want a small 2nd-rank bonus)
        }
    }
    return bonus;
}


 double GameBoard::openingness_of(int avg_cp) {
    if (avg_cp <= 3000) return 0.0;   // fully "not opening"
    if (avg_cp >= 4000) return 1.0;   // fully opening
    return ( (double)(avg_cp - 3000) ) / 1000.0;  // linear between
}


// Fills an array of up to 9 squares around the king (including the king square).
// Returns how many valid squares were written.
// king_near_squares_out[i] are square indices 0..63.
int GameBoard::get_king_near_squares(Color defender_color, int king_near_squares_out[9]) const
{
    int count = 0;

    // find king square for defender_color
    ull kbb = (defender_color == Color::WHITE) ? white_king : black_king;
    assert(kbb != 0ULL);
    ull tmp = kbb;
    int king_sq = utility::bit::lsb_and_pop_to_square(tmp);  // 0..63

    int king_row = king_sq / 8;
    int king_col = king_sq % 8;

    // collect king square and all neighbors in a 3x3 box
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; dc++) {
            int r = king_row + dr;
            int c = king_col + dc;
            if (r < 0 || r > 7 || c < 0 || c > 7)
                continue;
            king_near_squares_out[count++] = r * 8 + c;
        }
    }

    return count;
}


int GameBoard::kings_in_opposition(Color color)
{
    assert(white_king && black_king);

    ull wk_temp = white_king;
    ull bk_temp = black_king;

    int w_sq = utility::bit::lsb_and_pop_to_square(wk_temp);
    int b_sq = utility::bit::lsb_and_pop_to_square(bk_temp);

    int wr = w_sq / 8, wc = w_sq % 8;
    int br = b_sq / 8, bc = b_sq % 8;

    int dr = std::abs(wr - br);
    int dc = std::abs(wc - bc);

    bool in_opposition =
        ((dr == 0 && dc == 2) ||
         (dc == 0 && dr == 2) ||
         (dr == 2 && dc == 2));

    if (!in_opposition)
        return 0;

    // If it's White's move, Black holds opposition.
    if (color == Color::WHITE)
        return -1;  // reward Black
    else
        return +1;  // reward White
}




//
// Chebyshev distance  (D(x,y)=\max (|x_{1}-x_{2}|,|y_{1}-y_{2}|)\)
// It is also known as "chessboard distance".
// A cheap distance with integers, that avoids sqrt()
//
int GameBoard::get_Chebyshev_distance(int x1, int y1, int x2, int y2) {

    int dx = x1 > x2 ? x1 - x2 : x2 - x1;
    int dy = y1 > y2 ? y1 - y2 : y2 - y1;
    int iDist = (dx > dy) ? dx : dy;
    return iDist;
}


double GameBoard::get_board_distance(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2; if (dx < 0) dx = -dx;
    int dy = y1 - y2; if (dy < 0) dy = -dy;
    assert(dx>=0);
    assert(dy>=0);
    assert(dx<=7);
    assert(dx<=7);
    int d2 = dx*dx + dy*dy;   // 0..98

    struct Pt { int d2; double v; };
    static const Pt table[] = {
        {  0, 0.00 },
        {  1, 1.00 },
        {  2, 1.41 },
        {  4, 2.00 },
        {  5, 2.24 },
        {  8, 2.83 },
        { 10, 3.16 },
        { 13, 3.61 },
        { 17, 4.12 },
        { 20, 4.47 },
        { 25, 5.00 },
        { 29, 5.39 },
        { 34, 5.83 },
        { 40, 6.32 },
        { 45, 6.71 },
        { 52, 7.21 },
        { 58, 7.62 },
        { 65, 8.06 },
        { 73, 8.54 },
        { 82, 9.06 },
        { 90, 9.49 },
        { 98, 9.90 }  // max we care about
    };

    // exact or below first
    if (d2 <= table[0].d2)
        return table[0].v;

    // find segment
    const int N = sizeof(table)/sizeof(table[0]);
    for (int i = 1; i < N; ++i) {
        if (d2 <= table[i].d2) {
            int d2_lo   = table[i-1].d2;
            int d2_hi   = table[i].d2;
            double v_lo = table[i-1].v;
            double v_hi = table[i].v;

            // linear interpolation
            double t = (d2_hi == d2_lo)
                     ? 0.0
                     : (double)(d2 - d2_lo) / (double)(d2_hi - d2_lo);
            return v_lo + t * (v_hi - v_lo);
        }
    }

    // if somehow above max, just clamp
    return table[N-1].v;
}

// double GameBoard::get_board_distance(int x1, int y1, int x2, int y2)
// {
//     int dx = x1 - x2; if (dx < 0) dx = -dx;
//     int dy = y1 - y2; if (dy < 0) dy = -dy;
//     int d2 = dx*dx + dy*dy;   // 0..98

//     if (d2 <= 0)  return 0.00;
//     if (d2 <= 1)  return 1.00;
//     if (d2 <= 2)  return 1.41;  // sqrt(2)
//     if (d2 <= 4)  return 2.00;
//     if (d2 <= 5)  return 2.24;  // sqrt(5)
//     if (d2 <= 8)  return 2.83;  // sqrt(8)
//     if (d2 <= 10) return 3.16;  // sqrt(10)
//     if (d2 <= 13) return 3.61;  // sqrt(13)
//     if (d2 <= 17) return 4.12;  // sqrt(17)
//     if (d2 <= 20) return 4.47;  // sqrt(20)
//     if (d2 <= 25) return 5.00;
//     if (d2 <= 29) return 5.39;  // sqrt(29)
//     if (d2 <= 34) return 5.83;  // sqrt(34)
//     if (d2 <= 40) return 6.32;  // sqrt(40)
//     if (d2 <= 45) return 6.71;  // sqrt(45)
//     if (d2 <= 52) return 7.21;  // sqrt(52)
//     if (d2 <= 58) return 7.62;  // sqrt(58)
//     if (d2 <= 65) return 8.06;  // sqrt(65)
//     if (d2 <= 73) return 8.54;  // sqrt(73)
//     if (d2 <= 82) return 9.06;  // sqrt(82)
//     if (d2 <= 90) return 9.49;  // sqrt(90)
//     return 9.90;                // sqrt(98) ≈ 9.90
// }



//
// Returns "distance" between squares:
//    0 - same square  
//    2 - if opposition (but not diagonal)
//    
//   14 - if full distance of the board
//
double GameBoard::distance_between_squares(int enemyKingSq, int frienKingSq) {
    int iFakeDist;
    double dfakeDist;

    // Get coordinates of the square numbers
    int xEnemy = enemyKingSq % 8;
    int yEnemy = enemyKingSq / 8;
    int xFrien = frienKingSq % 8;
    int yFrien = frienKingSq / 8;
   
    // Method 1 (it stinks)
    //double dfakeDist = (double)(abs(xEnemy - xFrien) +  abs(yEnemy - yFrien));
    //double dfakeDist = (double)iFakeDist;

    // Method 2 Chebyshev. Better but
    // iFakeDist = get_Chebyshev_distance(xEnemy, yEnemy, xFrien, yFrien);
    // double dfakeDist = (double)iFakeDist;

    // Method 3. Best ? (table driven)
    dfakeDist = get_board_distance(xEnemy, yEnemy, xFrien, yFrien);
    
    //dfakeDist = (double)xFrien;     // debug only

    assert (dfakeDist >= 0.0);
    //assert (dfakeDist <= 14.0);
   
    return dfakeDist;
}


bool GameBoard::bIsOnlyKing(Color attacker_color) {
    ull enemyPieces;
    if (attacker_color == ShumiChess:: BLACK) {
        enemyPieces = (black_knights | black_bishops | black_pawns | black_rooks | black_queens);
    } else {
        enemyPieces = (white_knights | white_bishops | white_pawns | white_rooks | white_queens);  
    }
    return (enemyPieces == 0); // enemy has king only
}




// Returns 0 to 7, if lone king of enemy
double GameBoard::king_near_other_king(Color attacker_color) {
    double dBonus = 0;

    int enemyKingSq;
    int frienKingSq;
    ull enemyPieces;
    ull tmpEnemy;
    ull tmpFrien;

    //if ( (white_king == 0ULL) || (black_king == 0ULL) ) return 0;
    assert(white_king != 0ULL);
    assert(black_king != 0ULL);


    if (attacker_color == ShumiChess:: WHITE) {
        enemyPieces = (black_knights | black_bishops | black_pawns | black_rooks | black_queens);
        tmpEnemy = black_king;         // don't mutate the bitboards
        tmpFrien = white_king;         // don't mutate the bitboards

    } else {
        enemyPieces = (white_knights | white_bishops | white_pawns | white_rooks | white_queens);
        tmpEnemy = white_king;         // don't mutate the bitboards
        tmpFrien = black_king;         // don't mutate the bitboards
    }

    enemyKingSq = utility::bit::lsb_and_pop_to_square(tmpEnemy); // 0..63
    assert(enemyKingSq <= 63);
    frienKingSq = utility::bit::lsb_and_pop_to_square(tmpFrien); // 0..63
    assert(frienKingSq >= 0);


    // Bring friendly king near enemy king (reward small distances to other king)
    if (enemyPieces == 0) { // enemy has king only

        // 7 (furthest), to 1 (adjacent), to 0 (identical)
        double dFakeDist = distance_between_squares(enemyKingSq, frienKingSq);
        assert (dFakeDist >= 2.0);      // Kings cant touch
        assert (dFakeDist <= 7.0);      // Board is only so big

        // Bonus higher if friendly king closer to enemy king
        assert (dFakeDist >= 0.0); 
        dBonus = dFakeDist;    //dFakeDist*dFakeDist / 2.0;

        // Bonus debugs
        //dBonus = (double)frienKingSq;               // enemy king is attracted to a8
        //dBonus = 63.0 - (double)(frienKingSq);    // enemy king is attracted to h1

        return dBonus;

    }

    return dBonus;
}




int GameBoard::sliders_and_knights_attacking_square(Color attacker_color, int sq)
{
    // occupancy of all pieces on board
    ull occ =
        white_pawns   | white_knights | white_bishops | white_rooks |
        white_queens  | white_king    |
        black_pawns   | black_knights | black_bishops | black_rooks |
        black_queens  | black_king;

    // -----------------------
    // Knights
    // -----------------------
    ull knight_attackers =
        tables::movegen::knight_attack_table[sq] &
        get_pieces_template<Piece::KNIGHT>(attacker_color);

    // -----------------------
    // Bishops / Queens on diagonals
    // -----------------------
    ull diag_attackers = 0ULL;
    {
        ull bishops = get_pieces_template<Piece::BISHOP>(attacker_color);
        ull queens  = get_pieces_template<Piece::QUEEN >(attacker_color);

        int r0 = sq / 8;
        int c0 = sq % 8;

        // NE (+1,+1)
        {
            int r = r0;
            int c = c0;
            while (true) {
                r += 1;
                c += 1;
                if (r > 7 || c > 7) break;
                int s2 = r * 8 + c;
                ull bb = 1ULL << s2;
                if (occ & bb) {
                    if ( (bb & bishops) || (bb & queens) ) {
                        diag_attackers |= bb;
                    }
                    break;
                }
            }
        }

        // NW (+1,-1)
        {
            int r = r0;
            int c = c0;
            while (true) {
                r += 1;
                c -= 1;
                if (r > 7 || c < 0) break;
                int s2 = r * 8 + c;
                ull bb = 1ULL << s2;
                if (occ & bb) {
                    if ( (bb & bishops) || (bb & queens) ) {
                        diag_attackers |= bb;
                    }
                    break;
                }
            }
        }

        // SE (-1,+1)
        {
            int r = r0;
            int c = c0;
            while (true) {
                r -= 1;
                c += 1;
                if (r < 0 || c > 7) break;
                int s2 = r * 8 + c;
                ull bb = 1ULL << s2;
                if (occ & bb) {
                    if ( (bb & bishops) || (bb & queens) ) {
                        diag_attackers |= bb;
                    }
                    break;
                }
            }
        }

        // SW (-1,-1)
        {
            int r = r0;
            int c = c0;
            while (true) {
                r -= 1;
                c -= 1;
                if (r < 0 || c < 0) break;
                int s2 = r * 8 + c;
                ull bb = 1ULL << s2;
                if (occ & bb) {
                    if ( (bb & bishops) || (bb & queens) ) {
                        diag_attackers |= bb;
                    }
                    break;
                }
            }
        }
    }

    // -----------------------
    // Rooks / Queens on ranks/files
    // -----------------------
    ull ortho_attackers = 0ULL;
    {
        ull rooks  = get_pieces_template<Piece::ROOK >(attacker_color);
        ull queens = get_pieces_template<Piece::QUEEN>(attacker_color);

        int r0 = sq / 8;
        int c0 = sq % 8;

        // North (+1,0)
        {
            int r = r0;
            int c = c0;
            while (true) {
                r += 1;
                if (r > 7) break;
                int s2 = r * 8 + c;
                ull bb = 1ULL << s2;
                if (occ & bb) {
                    if ( (bb & rooks) || (bb & queens) ) {
                        ortho_attackers |= bb;
                    }
                    break;
                }
            }
        }

        // South (-1,0)
        {
            int r = r0;
            int c = c0;
            while (true) {
                r -= 1;
                if (r < 0) break;
                int s2 = r * 8 + c;
                ull bb = 1ULL << s2;
                if (occ & bb) {
                    if ( (bb & rooks) || (bb & queens) ) {
                        ortho_attackers |= bb;
                    }
                    break;
                }
            }
        }

        // East (0,+1)
        {
            int r = r0;
            int c = c0;
            while (true) {
                c += 1;
                if (c > 7) break;
                int s2 = r * 8 + c;
                ull bb = 1ULL << s2;
                if (occ & bb) {
                    if ( (bb & rooks) || (bb & queens) ) {
                        ortho_attackers |= bb;
                    }
                    break;
                }
            }
        }

        // West (0,-1)
        {
            int r = r0;
            int c = c0;
            while (true) {
                c -= 1;
                if (c < 0) break;
                int s2 = r * 8 + c;
                ull bb = 1ULL << s2;
                if (occ & bb) {
                    if ( (bb & rooks) || (bb & queens) ) {
                        ortho_attackers |= bb;
                    }
                    break;
                }
            }
        }
    }

    // Combine everyone
    ull any_attackers = knight_attackers | diag_attackers | ortho_attackers;

    // Return number of attackers
    return bits_in(any_attackers);
}


int GameBoard::attackers_on_enemy_king_near(Color attacker_color)
{
    // enemy (the one whose king we are surrounding)
    Color defender_color =
        (attacker_color == Color::WHITE) ? Color::BLACK : Color::WHITE;

    // grab up to 9 squares around defender's king
    int king_near_squares[9];
    int count = get_king_near_squares(defender_color, king_near_squares);

    int total = 0;

    for (int i = 0; i < count; ++i) {
        int sq = king_near_squares[i];
        total += sliders_and_knights_attacking_square(attacker_color, sq);
    }

    return total;
}



bool GameBoard::is_king_in_check_new(Color color)
{
    // --- 1. find king square ---
    ull king_bb = (color == Color::WHITE) ? white_king : black_king;
    if (!king_bb) return false;  // should never happen
    int king_sq = utility::bit::lsb_and_pop_to_square(king_bb);

    // --- 2. occupancy of all pieces ---
    const ull occ =
        white_pawns | white_knights | white_bishops | white_rooks |
        white_queens | white_king |
        black_pawns | black_knights | black_bishops | black_rooks |
        black_queens | black_king;

    const Color enemy = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;

    // --- 3. pawn attacks ---
    const ull FILE_A = col_masks[Col::COL_A];
    const ull FILE_H = col_masks[Col::COL_H];
    ull bit = (1ULL << king_sq);
    ull pawn_attackers;
    if (color == Color::WHITE)
    {
        // enemy (black) pawns that attack downwards (south)
        pawn_attackers = (((bit & ~FILE_H) << 7) | ((bit & ~FILE_A) << 9))
                       & get_pieces_template<Piece::PAWN>(enemy);
    }
    else
    {
        // enemy (white) pawns that attack upwards (north)
        pawn_attackers = (((bit & ~FILE_A) >> 7) | ((bit & ~FILE_H) >> 9))
                       & get_pieces_template<Piece::PAWN>(enemy);
    }

    if (pawn_attackers) return true;

    // --- 4. knight attacks ---
    ull knight_attackers =
        tables::movegen::knight_attack_table[king_sq] &
        get_pieces_template<Piece::KNIGHT>(enemy);
    if (knight_attackers) return true;

    // --- 5. king adjacency (opposing king) ---
    ull king_attackers =
        tables::movegen::king_attack_table[king_sq] &
        get_pieces_template<Piece::KING>(enemy);
    if (king_attackers) return true;

    // --- 6. bishop/queen diagonals ---
    {
        ull bishops = get_pieces_template<Piece::BISHOP>(enemy);
        ull queens  = get_pieces_template<Piece::QUEEN >(enemy);

        int r0 = king_sq / 8;
        int c0 = king_sq % 8;

        // 4 diagonal directions
        const int dirs[4][2] = { {+1,+1}, {+1,-1}, {-1,+1}, {-1,-1} };
        for (auto& d : dirs)
        {
            int r = r0, c = c0;
            while (true)
            {
                r += d[0]; c += d[1];
                if (r < 0 || r > 7 || c < 0 || c > 7) break;
                int sq = r * 8 + c;
                ull bb = 1ULL << sq;
                if (occ & bb)
                {
                    if ((bb & bishops) || (bb & queens))
                        return true;
                    break;
                }
            }
        }
    }

    // --- 7. rook/queen orthogonals ---
    {
        ull rooks  = get_pieces_template<Piece::ROOK >(enemy);
        ull queens = get_pieces_template<Piece::QUEEN>(enemy);

        int r0 = king_sq / 8;
        int c0 = king_sq % 8;

        const int dirs[4][2] = { {+1,0}, {-1,0}, {0,+1}, {0,-1} };
        for (auto& d : dirs)
        {
            int r = r0, c = c0;
            while (true)
            {
                r += d[0]; c += d[1];
                if (r < 0 || r > 7 || c < 0 || c > 7) break;
                int sq = r * 8 + c;
                ull bb = 1ULL << sq;
                if (occ & bb)
                {
                    if ((bb & rooks) || (bb & queens))
                        return true;
                    break;
                }
            }
        }
    }

    // --- 8. if no attackers found ---
    return false;
}


///////////////////////////////////////////////////////////////////////////////////////////

static bool kings_adjacent(const std::string &a, const std::string &b)
{
    int f1 = a[0] - 'a';
    int r1 = a[1] - '1';
    int f2 = b[0] - 'a';
    int r2 = b[1] - '1';
    return std::abs(f1 - f2) <= 1 && std::abs(r1 - r2) <= 1;
}

static bool queen_attacks(const std::string &q, const std::string &k)
{
    int f1 = q[0] - 'a';
    int r1 = q[1] - '1';
    int f2 = k[0] - 'a';
    int r2 = k[1] - '1';
    return (f1 == f2) || (r1 == r2) || (std::abs(f1 - f2) == std::abs(r1 - r2));
}

////////////////////////////////////////////////////////////////
std::string GameBoard::random_kqk_fen(bool doQueen)
{
    // make all 64 squares in h1=0 order: h1,g1,...,a1, h2,...,a8
    std::vector<std::string> squares;
    for (int r = 1; r <= 8; ++r)
    {
        for (int fi = 0; fi < 8; ++fi)
        {
            char f = char('h' - fi);   // h,g,f,e,d,c,b,a
            std::string sq;
            sq += f;
            sq += char('0' + r);
            squares.push_back(sq);
        }
    }

retry_all:
    // 1) pick white king
    std::string wk = squares[std::rand() % 64];

    // 2) pick black king not adjacent
    std::vector<std::string> bk_choices;
    for (const auto &sq : squares)
    {
        if (sq != wk && !kings_adjacent(wk, sq))
        {
            bk_choices.push_back(sq);
        }
    }
    if (bk_choices.empty())
        goto retry_all;
    std::string bk = bk_choices[std::rand() % bk_choices.size()];

    // helper: rook attacks (orthogonal only)
    auto rook_attacks = [](const std::string &r, const std::string &k) -> bool
    {
        int f1 = r[0] - 'a';
        int r1 = r[1] - '1';
        int f2 = k[0] - 'a';
        int r2 = k[1] - '1';
        return (f1 == f2) || (r1 == r2);
    };

    // 3) pick black Q or R that does NOT give check to WK
    std::vector<std::string> bX_choices;
    for (const auto &sq : squares)
    {
        if (sq == wk || sq == bk)
            continue;

        bool attacks =
            doQueen
            ? queen_attacks(sq, wk)
            : rook_attacks(sq, wk);

        if (!attacks)
            bX_choices.push_back(sq);
    }
    if (bX_choices.empty())
        goto retry_all;
    std::string bX = bX_choices[std::rand() % bX_choices.size()];

    // 4) build board [row 8 → row 1], still standard FEN layout
    char board[8][8] = { 0 };
    auto place = [&](char piece, const std::string &sq)
    {
        int f = sq[0] - 'a';     // 0..7
        int r = sq[1] - '1';     // 0..7
        board[7 - r][f] = piece; // FEN row order
    };
    place('K', wk);
    place('k', bk);
    place(doQueen ? 'q' : 'r', bX);

    // 5) make FEN ranks
    std::string fen;
    for (int row = 0; row < 8; ++row)
    {
        int empty = 0;
        for (int col = 0; col < 8; ++col)
        {
            char c = board[row][col];
            if (c == 0)
            {
                ++empty;
            }
            else
            {
                if (empty)
                {
                    fen += char('0' + empty);
                    empty = 0;
                }
                fen += c;
            }
        }
        if (empty)
            fen += char('0' + empty);
        if (row != 7)
            fen += '/';
    }

    fen += " w - - 0 1";
    return fen;
}



} // end namespace ShumiChess
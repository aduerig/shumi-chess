

#include <math.h>
#include <vector>

//#define NDEBUG         // Define (uncomment) this to disable asserts
#undef NDEBUG
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
        default:        // Note: skipping a default is illegal in some states.
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

    // TODO: castling
    // NOTE: What do you mean, TODO. here and below, What's to do? Is this an unfinished project?
    // I think the TODOs are outdated. The below code works great, and makes great fens.
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
int GameBoard::get_material_for_color(Color color1) {

    int cp_score_pieces_only_temp = 0;

    // Add up the scores for each piece
    for (Piece piece_type = Piece::PAWN;
        piece_type <= Piece::QUEEN;
        piece_type = static_cast<Piece>(static_cast<int>(piece_type) + 1))
    {    
        
        // Get bitboard of all pieces on board of this type and color
        ull pieces_bitboard = get_pieces(color1, piece_type);

        // Adds for the piece value multiplied by how many of that piece there is (using centipawns)
        int cp_board_score = centipawn_score_of(piece_type);
        int nPieces = bits_in(pieces_bitboard);
        cp_score_pieces_only_temp += (int)(((double)nPieces * (double)cp_board_score));

        // This return must always be positive.
        assert (cp_score_pieces_only_temp>=0);

    }

    return cp_score_pieces_only_temp;
}


// Total of 4000 centipawns for each side.
int GameBoard::centipawn_score_of(ShumiChess::Piece p) const {
    switch (p) {
        case ShumiChess::Piece::PAWN:   return 100;
        case ShumiChess::Piece::KNIGHT: return 320;
        case ShumiChess::Piece::BISHOP: return 330;
        case ShumiChess::Piece::ROOK:   return 500;
        case ShumiChess::Piece::QUEEN:  return 900;
        case ShumiChess::Piece::KING:   return 0;   // king is infinite in theory; keep 0 for material sums
        default:                        {assert(0);return 0;}
    }
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
   

        // Gets smaller closer to center. (0 at dead center, 1.0 on furthest corners)
       // centerness = (double)(7 - king_danger_opening[row_idx][col_idx]);    // 1 is minimum danger, 7 is maximum danger
    
       // assert (fabs(centerness == centerness2) < 0.001);

    return;
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


int GameBoard::pawns_attacking_center_squares(Color c)
{
    return  pawns_attacking_square(c, square_e4)
          + pawns_attacking_square(c, square_d4)
          + pawns_attacking_square(c, square_e5)
          + pawns_attacking_square(c, square_d5);
}


int GameBoard::knights_attacking_square(Color c, int sq)
{
    ull targets = tables::movegen::knight_attack_table[sq];
    ull knights = get_pieces_template<Piece::KNIGHT>(c);
    return bits_in(targets & knights);  // count the 1-bits
}

int GameBoard::knights_attacking_center_squares(Color for_color)
{
    int square_e4 = 27;
    int square_d4 = 28;
    int square_e5 = 35;
    int square_d5 = 36;
    int itemp = 0;
    itemp += knights_attacking_square(for_color, square_e4);
    itemp += knights_attacking_square(for_color, square_d4);
    itemp += knights_attacking_square(for_color, square_e5);
    itemp += knights_attacking_square(for_color, square_d5);
    return itemp;
}

//
// One if rooks connected. 0 if not.
// return false if two rooks dont exist. But in any case, returns the correct connectiveness.
// Note sure what happens with three or more rooks.
bool GameBoard::rook_connectiveness(Color c, int& connectiveness) const
{
    using ull = unsigned long long;

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
int GameBoard::rook_file_status(Color c) const
{
    using ull = unsigned long long;

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
    using ull = unsigned long long;

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
// counts 1 for each isolated pawn, 2 for a isolated doubled pawn, 3 for tripled isolated pawn.
// One count for each instance.
//
int GameBoard::count_isolated_pawns(Color c) const
{
    using ull = unsigned long long;

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



// Passed pawns bonus in centipawns:
//  - 10 cp on 3rd/4th rank (from that side's perspective)
//  - 20 cp on 5th/6th rank
//  - 30 cp on 7th rank
int GameBoard::count_passed_pawns(Color c)
{

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



} // end namespace ShumiChess
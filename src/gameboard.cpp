

#include <math.h>
#include <vector>

#include <random>
#include <chrono>  


#include "gameboard.hpp"
//
// Note: Why this needs to be here is a complete mystery. But if #include <assert.h> is 
// done before #include "gameboard.hpp", no asserts come out!
#undef NDEBUG
//#define NDEBUG         // Define (uncomment) this to disable asserts
#include <assert.h>
#undef NDEBUG


#include "move_tables.hpp"
#include "utility.hpp"
#include "endgameTables.hpp"


using namespace std;



namespace ShumiChess {

    GameBoard::GameBoard() : 
 
    // IN this constructer, the bitboards are the input
    #include "gameboardSetup.hpp"

    {
        // Seed randomization, for gameboard. (using microseconds since ?)
        using namespace std::chrono;
        auto now = high_resolution_clock::now().time_since_epoch();
        auto us  = duration_cast<microseconds>(now).count();
        rng.seed(static_cast<unsigned>(us));


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


    // Seed randomization, for gameboard. (using microseconds since ?)
    using namespace std::chrono;
    auto now = high_resolution_clock::now().time_since_epoch();
    auto us  = duration_cast<microseconds>(now).count();
    rng.seed(static_cast<unsigned>(us));

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

const endgameTablePos GameBoard::to_egt() {
    endgameTablePos returnVar;
    return returnVar;
}


const string GameBoard::to_fen(bool bFullFEN) {

    //cout << "\x1b[34mto_fen!\x1b[0m" << endl;
    vector<string> fen_components;

    // the board
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
// Returns in cp.
// Delevers bonus points based on castling.
//  Two things (bools) about castling. 
//      1. Can you castle or not?  (do you have the priviledge)  
//      2. Whether YOU have castled or not. The latter IS NOT stored in a FEN by the way. It has to be a bollean 
//          maintained by the push/pop.
//  Results (bonus points):
//    2 castled
//    1 
//
int GameBoard::get_castle_bonus_cp_for_color(Color color1, int phase) const {
    int i_can_castle = 0;
    bool b_has_castled = false;

    if (color1 == ShumiChess::WHITE) {
        i_can_castle += (white_castle_rights & king_side_castle)  ? 1 : 0;
        i_can_castle += (white_castle_rights & queen_side_castle) ? 1 : 0;
    } else {
        i_can_castle += (black_castle_rights & king_side_castle)  ? 1 : 0;
        i_can_castle += (black_castle_rights & queen_side_castle) ? 1 : 0;
    }
    b_has_castled = bHasCastled_fake(color1);

    // has_castled bonus must be > castled privledge or it will never castle.
    int icode = (b_has_castled ? 150 : 0) + i_can_castle * 35;

    int final_code;

    if      (phase == GamePhase::OPENING) final_code = icode;
    else if (phase == GamePhase::MIDDLE_EARLY) final_code = icode/2;
    else final_code = 0;

    return final_code;  // "centipawns"
}

//
// “Returns true if king is on home rank and positioned near an edge, and there are no friendly 
// rooks between the king and that edge.”
//
bool GameBoard::bHasCastled_fake(Color color1) const {
    // 2) Build an occupancy bitboard (all pieces)
    ull occupied = 0;
        //    white_knights | white_bishops | white_queens  | white_king    |
        //    black_knights | black_bishops | black_queens  | black_king;

    ull rooks_bb = (color1 == ShumiChess::WHITE) ? white_rooks : black_rooks;
    occupied |= rooks_bb;

    // 3) Get this side's king square
    ull king_bb = (color1 == ShumiChess::WHITE) ? white_king : black_king;
    if (!king_bb) return false;  // safety

    int king_sq = utility::bit::bitboard_to_lowest_square(king_bb);
    int file    = king_sq % 8;   // 0 = h, 1 = g, 2 = f, 3 = e, 4 = d, 5 = c, 6 = b, 7 = a
    int rank    = king_sq / 8;   // 0 = rank 1, 7 = rank 8

    int homeRank = (color1 == ShumiChess::WHITE) ? 0 : 7;

    // King must be on home rank and on h/g/c/b/a file: file 0,1,2,5,6,7
    if (!(rank == homeRank && (file <= 1 || file >= 5))) return false;

    bool blocked = false;

    if (file <= 1) {
        // Kingside: scan toward h-file (file 0) (all files must be empty of friendly rooks)
        for (int f = file - 1; f >= 0; --f) {
            int sq = rank * 8 + f;
            if (occupied & (1ULL << sq)) {
                blocked = true;
                break;
            }
        }
    } else {        // file >= 5 
        // Queenside: scan toward a-file (file 7) (all files must be empty friendly rooks)
        for (int f = file + 1; f <= 7; ++f) {
            int sq = rank * 8 + f;
            if (occupied & (1ULL << sq)) {
                blocked = true;
                break;
            }
        }
    }

    return !blocked;
}

// bool GameBoard::bHasCastled(Color color1) const {
//     bool b_has_castled = false;
//     if (color1 == ShumiChess::WHITE) {
//         b_has_castled = bCastledWhite;
//     } else {
//         b_has_castled = bCastledBlack;
//     }
//     return b_has_castled;
// }

//
// Returns centipawns. Always positive. 
int GameBoard::get_material_for_color(Color color, int& cp_pawns_only_temp) {

 
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

//
int GameBoard::bits_in(ull bitboard) const {
    auto bs = bitset<64>(bitboard);
    return (int) bs.count();
}
//
// “lerp” stands for Linear intERPolation.
// Linear interpolation: t=0 → a, t=1 → b
inline double lerp(double a, double b, double t) { return a + (b - a) * t; }


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
    ull bitBoard = (1ULL << sq);
    const ull FILE_A = col_masks[Col::COL_A];
    const ull FILE_H = col_masks[Col::COL_H];

    ull origins;
    if (c == Color::WHITE) {
        // white pawn origins that attack sq: from (sq-7) and (sq-9)
        origins = ((bitBoard & ~FILE_A) >> 7) | ((bitBoard & ~FILE_H) >> 9);
    } else {
        // black pawn origins that attack sq: from (sq+7) and (sq+9)
        origins = ((bitBoard & ~FILE_H) << 7) | ((bitBoard & ~FILE_A) << 9);
    }

    ull pawns = get_pieces_template<Piece::PAWN>(c);
    return bits_in(origins & pawns);
}


// Returns in cp.
int GameBoard::pawns_attacking_center_squares(Color c) {
    
    constexpr int CENTER_W = 40;   // weight for e4,d4,e5,d5
    constexpr int ADV_W    = 30;   // weight for e6,d6 (White) or e3,d3 (Black)

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
    connectiveness = 0;
    const ull rooks = (c == Color::WHITE) ? white_rooks : black_rooks;

    // Need at least two rooks of this color
    if ((rooks == 0) || ((rooks & (rooks - 1)) == 0)) {
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

    ull tmp = rooks;    // dont mutate the bitboards
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


//
// Returns in cp.
// Rooks or queens on 7th or 8th ranks
int GameBoard::rook_7th_rankness(Color c) const   /* now counts R+Q; +1 each on enemy 7th */
{
    const ull rooks  = (c == Color::WHITE) ? white_rooks  : black_rooks;
    const ull queens = (c == Color::WHITE) ? white_queens : black_queens;

    ull bigPieces = rooks | queens;    // Dont mutate the bitboards
    if (!bigPieces) return 0;

    const int seventh_rank = (c == Color::WHITE) ? 6 : 1;
    const int eight_rank = (c == Color::WHITE) ? 7 : 0;

    int score = 0;
    while (bigPieces) {
        int s = utility::bit::lsb_and_pop_to_square(bigPieces); // 0..63
        int rnk = (s / 8);
        if (rnk == seventh_rank) score+=20;
        if (rnk == eight_rank) score+=10;
    }
    return score;
}
//
// returns true only if "insufficient material
// NOTE: known errors here: this logic declares the following positions drawn, when they are not:
//      two knights and pawn .vs. king. 
//      
bool GameBoard::insufficient_material_simple() {
    // 1) No pawns anywhere
    ull pawns = white_pawns | black_pawns;
    if (pawns) return false;

    // 2) No queens or rooks anywhere
    ull majors = (white_rooks | black_rooks | white_queens | black_queens);
    if (majors) return false;

    // 3) Count total minor pieces (knights + bishops), both colors
    int n_knightsW = bits_in(white_knights);
    int n_knightsB = bits_in(black_knights); 

    int n_bishopsW = bits_in(white_bishops);
    int n_bishopsB = bits_in(black_bishops); 

    int n_piecesW = n_knightsW + n_bishopsW;
    int n_piecesB = n_knightsB + n_bishopsB;
    if ( (n_piecesW <= 1) && (n_piecesB <= 1) ) return true;    // no pieces
    if ( (n_piecesW <= 1) && (n_knightsB == 2) ) return true;   // 2 knights
    if ( (n_piecesB <= 1) && (n_knightsW == 2) ) return true;   // 2 knights

    return false;

}


bool GameBoard::isReversableMove(const Move& m)
{
    // "Reversible" here means: does NOT reset the repetition window.
    // Irreversible moves (reset window): pawn moves, captures (incl. en-passant), promotions.

    if (m.piece_type == Piece::PAWN) return false;

    if (m.capture != Piece::NONE) return false;

    if (m.is_en_passent_capture) return false;  // safety: should imply capture, but keep explicit

    if (m.promotion != Piece::NONE) return false;

    return true;
}


//
// Isolated pawns. Returns cp.
// counts 1 for each isolated pawn, 2 for a isolated doubled pawn, 3 for tripled isolated pawn.
// One count for each instance. Rook pawns can be isolated too.
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
    //
    // loop over all files
    for (int file = 0; file < 8; ++file) {
        int k = file_count[file];              // number of pawns on this file
        if (k == 0) continue;

        bool left  = (file < 7) && (files_present & (1u << (file + 1)));
        bool right = (file > 0) && (files_present & (1u << (file - 1)));

        if (!left && !right) {
            // isolated file: single→1, double→2, triple→3, etc.
            total += k;
        }
    }
    return total;
}

//
// Returns cp. 3 for each doubled pawn (4 for rook pawns), 6 for each tripled pawn, 12 for each quadrupled pawn
//   soo worst case quarluped rook pawns is 16 cp.
int GameBoard::count_doubled_pawns(Color c) const {
    const ull P = (c == Color::WHITE) ? white_pawns : black_pawns;
    if (!P) return 0;

    int file_count[8] = {0};
    ull tmp = P;  // dont mutate bitboards
    while (tmp) {
        int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63
        int f = s % 8;                                    // 0..7 (0=H ... 7=A)
        ++file_count[f];
    }

    int total = 0;
    for (int f = 0; f < 8; ++f) {
        int k = file_count[f];
        if (k >= 2) {
            int weight = (f == 0 || f == 7) ? 10 : 8;  // rook files penalized more
            total += k * weight;
        }
    }
    return total;
}


// // Identifies if enemy king only, and friendy side has a lone queen or a lone rook
// // Note: kill this function! Engine should have full range, even if in simple endgame.
// bool GameBoard::IsSimpleEndGame(Color for_color)
// {
//     bool bReturn = false;
//     ull enemyPieces, myPieces, myQueens, myRooks;

//     if (for_color == ShumiChess:: WHITE) {
//         enemyPieces = (black_knights | black_bishops | black_pawns | black_rooks | black_queens);
//         myPieces = (white_knights | white_bishops | white_pawns);
//         myQueens = white_queens;
//         myRooks = white_rooks;
//     } else {
//         enemyPieces = (white_knights | white_bishops | white_pawns | white_rooks | white_queens);
//         myPieces = (black_knights | black_bishops | black_pawns);
//         myQueens = black_queens;
//         myRooks = black_rooks;
//     }

//     if ( ( enemyPieces == 0ULL) && (myPieces == 0ULL) ) {
//         int nQueens = bits_in(myQueens);
//         int nRooks = bits_in(myRooks);
//         if ( (nQueens == 1) && (nRooks == 0) ) {         
//            bReturn = true;
//         }
//         else if ( (nQueens == 0) && (nRooks == 1) ) {         
//            bReturn = true;
//         }
//     }

//     return bReturn;
// }


//
// Returns smaller values near the center. 0 for the inner ring (center) 3 for outer ring (edge squares)
int GameBoard::king_edge_weight(Color color)
{
    ull kbb = (color == Color::WHITE) ? white_king : black_king;   // dont mutate the bitboards
    assert(kbb != 0ULL);
    return piece_edge_weight(kbb);
}

// 0 = center, 3 = edge
int GameBoard::piece_edge_weight(ull kbb) {   
    int sq = utility::bit::lsb_and_pop_to_square(kbb);  // 0..63

    int r = sq / 8;              // 0..7
    int f = sq % 8;              // 0..7
    int dr = std::min(std::abs(r - 3), std::abs(r - 4));
    int df = std::min(std::abs(f - 3), std::abs(f - 4));
    int ring = std::max(dr, df); // 0..3

    if (ring < 0) ring = 0;
    if (ring > 3) ring = 3;

    return ring;  // 0 = center, 3 = edge
}


// Piece occuptation squares scaled for closness to center.
int GameBoard::center_closeness_bonus(Color c) {
    const int BASE_CP = 24;  // tune strength (centipawns)
    int bonus = 0;

    ull nBB = 0ULL, bBB = 0ULL, rBB = 0ULL, qBB = 0ULL;

    if (c == Color::WHITE) {
        nBB = white_knights;
        bBB = white_bishops;
        rBB = white_rooks;
        qBB = white_queens;
    } else {
        nBB = black_knights;
        bBB = black_bishops;
        rBB = black_rooks;
        qBB = black_queens;
    }

    // Knights (divide by 3)
    ull tmp = nBB;
    while (tmp) {
        int sq = utility::bit::lsb_and_pop_to_square(tmp);
        ull one = 1ULL << sq;
        int ring = piece_edge_weight(one);    // 0=center .. 3=edge
        int centerness = 3 - ring;            // 3=center .. 0=edge
        if (centerness < 0) centerness = 0;
        bonus += (BASE_CP * centerness) / 3;
    }

    // Bishops (divide by 3)
    tmp = bBB;
    while (tmp) {
        int sq = utility::bit::lsb_and_pop_to_square(tmp);
        ull one = 1ULL << sq;
        int ring = piece_edge_weight(one);
        int centerness = 3 - ring;
        if (centerness < 0) centerness = 0;
        bonus += (BASE_CP * centerness) / 3;
    }

    // Rooks (divide by 5)
    tmp = rBB;
    while (tmp) {
        int sq = utility::bit::lsb_and_pop_to_square(tmp);
        ull one = 1ULL << sq;
        int ring = piece_edge_weight(one);
        int centerness = 3 - ring;
        if (centerness < 0) centerness = 0;
        bonus += (BASE_CP * centerness) / 5;
    }

    // Queens (divide by 9)
    tmp = qBB;
    while (tmp) {
        int sq = utility::bit::lsb_and_pop_to_square(tmp);
        ull one = 1ULL << sq;
        int ring = piece_edge_weight(one);
        int centerness = 3 - ring;
        if (centerness < 0) centerness = 0;
        bonus += (BASE_CP * centerness) / 9;
    }

    return bonus;
}



// if (Features_mask & _FEATURE_EVAL_TEST1) {
//
// Passed pawns bonus in centipawns: (from that side's perspective)
//  - 10 cp on 2cnd/3rd rank
//  - 15 cp on 4th
//  - 20 cp on 5th
//  - 25 co on 6th rank
//  - 30 cp on 7th rank
// count_passed_pawns():
// A "passed pawn" has no enemy pawns on its own file or adjacent files on any rank ahead of it (toward promotion).
// A "protected passed pawn" is a passed pawn that is defended by one of our pawns:
// i.e., a friendly pawn sits on a square that attacks this pawn's square (diagonally from behind).
int GameBoard::count_passed_pawns(Color c, ull& passed_pawns) {

    passed_pawns = 0ULL;

    int n_passed_pawns = 0;
    const ull my_pawns  = get_pieces(c, Piece::PAWN);
    const ull his_pawns = get_pieces(utility::representation::opposite_color(c), Piece::PAWN);
    if (!my_pawns) return 0;   // I have no pawns

    int bonus = 0;
    ull tmp = my_pawns;     // Dont mutate the bitboards

    while (tmp) {
        const int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63

        // Get file and rank coordinates
        const int f = s % 8;            // h1=0 → 0=H ... 7=A
        const int r = s / 8;            // rank index 0..7 (White's view)

        // Map to A..H index used by col_masks
        const int fi = 7 - f;

        // files_mask <- same file, and both adjacent files
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

            // mark this pawn in this h1=0 bitboard
            passed_pawns |= (1ULL << s);

            n_passed_pawns++;

            // advancement from "c"" side's perspective (2..6 typical for passers)
            const int adv = (c == Color::WHITE) ? r : (7 - r);
            assert(adv != 0);  // pawns can never be on 1st rank!
            assert(adv != 7);  // pawns can never be on 8th rank (they promote)

            int base = 0;
            if      (adv == 6) base = 30;      // 7th rank
            else if (adv == 5) base = 25;      // 6th
            else if (adv == 4) base = 20;      // 5th
            else if (adv == 3) base = 15;      // 4th
            else if (adv >= 1) base = 10;      // 2cnd/3rd

            bonus += base;

            // Is it protected?
            ull protect_mask = 0ULL;

            if (c == Color::WHITE) {
                if (r > 0) {
                    if (f < 7) protect_mask |= (1ULL << (s - 7));  // defender on (f+1, r-1)
                    if (f > 0) protect_mask |= (1ULL << (s - 9));  // defender on (f-1, r-1)
                }
            } else {
                if (r < 7) {
                    if (f < 7) protect_mask |= (1ULL << (s + 9));  // defender on (f+1, r+1)
                    if (f > 0) protect_mask |= (1ULL << (s + 7));  // defender on (f-1, r+1)
                }
            }


            // see if its protected (potentially at least)
            bool bProtected = my_pawns & protect_mask;
            if (bProtected) bonus+=30;
         
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
    ull tmp = kbb;  // dont mutate the bitboards
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
//   10 - if full distance of the board
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

    assert (dfakeDist >=  0.0);
    assert (dfakeDist <= 10.0);
   
    return dfakeDist;
}


// Color has king only, or king with a single minor piece.
bool GameBoard::bIsOnlyKing(Color attacker_color) {
    ull enemyBigPieces;
    ull enemySmallPieces;
    if (attacker_color == ShumiChess:: BLACK) {
        enemyBigPieces = (black_pawns | black_rooks | black_queens);
        enemySmallPieces = (black_knights | black_bishops);
    } else {
        enemyBigPieces = (white_pawns | white_rooks | white_queens);  
        enemySmallPieces = (white_knights | white_bishops);
    }
    if (enemyBigPieces) return false;
    if (bits_in(enemySmallPieces) > 1) return false;
    return true;
}

// Enemy has king only
bool GameBoard::bNoPawns() {
    return ( (white_pawns | black_pawns) == 0);
}

bool GameBoard::is_king_highest_piece() {
    if (white_queens || black_queens) return false;
    if (white_rooks || black_rooks) return false;
    return true;

}

// Punishment for knights on edge of board, knight in corners even worse (twice as bad)
int GameBoard::is_knight_on_edge(Color color) {
    int pointsOff=0;
    ull knghts; // dont mutate the bitboard
    if (color == ShumiChess:: WHITE) {
        knghts = (white_knights);
    } else {
        knghts = (black_knights);
    }
    if (!knghts) return 0;
    while (knghts) {
        int s  = utility::bit::lsb_and_pop_to_square(knghts); // 0..63  
        const int f = s % 8;            // h1=0 → 0=H ... 7=A
        const int r = s / 8;            // rank index 0..7 (White's view)
        if ((f==0) || (f==7)) pointsOff++;
        if ((r==0) || (r==7)) pointsOff++;
    }
    return pointsOff;
}

// Returns 2 to 7,. Zero if in opposition, 7 if in opposite corners.
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
    if (enemyPieces == 0) { // enemy has lone king only

        // 7 (furthest), to 1 (adjacent), to 0 (identical)
        double dFakeDist = distance_between_squares(enemyKingSq, frienKingSq);
        assert (dFakeDist >=  2.0);      // Kings cant touch
        assert (dFakeDist <= 10.0);      // Board is only so big

        // Bonus higher if friendly king closer to enemy king
        dBonus = dFakeDist;    //dFakeDist*dFakeDist / 2.0;

        // Bonus debugs
        //dBonus = (double)frienKingSq;               // enemy king is attracted to a8
        //dBonus = 63.0 - (double)(frienKingSq);    // enemy king is attracted to h1

        return dBonus;

    }

    return dBonus;
}


int GameBoard::king_sq_of(Color color) {
    int frienKingSq;
    ull tmpFrien;
    if (color == ShumiChess:: WHITE) {
        tmpFrien = white_king;         // don't mutate the bitboards
    } else {
        tmpFrien = black_king;         // don't mutate the bitboards
    }
    frienKingSq = utility::bit::lsb_and_pop_to_square(tmpFrien); // 0..63
    assert(frienKingSq >= 0);
    assert(frienKingSq <= 63);

    return frienKingSq;
}


// Returns 1 to 10. 1 if close to the passed sq. 10 if as far as possible from the passed sq.
double GameBoard::king_near_sq(Color attacker_color, ull sq) {
    double dBonus = 0;

    int enemyKingSq;
    int frienKingSq;
    ull enemyPieces;
    ull tmpFrien;

    //if ( (white_king == 0ULL) || (black_king == 0ULL) ) return 0;
    assert(white_king != 0ULL);
    assert(black_king != 0ULL);


    if (attacker_color == ShumiChess:: WHITE) {
        enemyPieces = (black_knights | black_bishops | black_pawns | black_rooks | black_queens);
        tmpFrien = white_king;         // don't mutate the bitboards
    } else {
        enemyPieces = (white_knights | white_bishops | white_pawns | white_rooks | white_queens);
        tmpFrien = black_king;         // don't mutate the bitboards
    }

    enemyKingSq = sq;
    assert(enemyKingSq <= 63);
    frienKingSq = utility::bit::lsb_and_pop_to_square(tmpFrien); // 0..63
    assert(frienKingSq >= 0);


    // Bring friendly king near enemy king (reward small distances to other king)
    if (enemyPieces == 0) { // enemy has king only

        // 7 (furthest), to 1 (adjacent), to 0 (identical)
        double dFakeDist = distance_between_squares(enemyKingSq, frienKingSq);
        assert (dFakeDist >=  1.0);
        assert (dFakeDist <= 10.0);      // Board is only so big

        // Bonus higher if friendly king closer to enemy king
        dBonus = dFakeDist;

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

// Attackers are NOT kings or pawns.
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

        total += pawns_attacking_square(attacker_color,  sq);

    }

    return total;
}

// Counts sliders+knights attacking the enemy's passed pawns.
// passed_white_pwns / passed_black_pwns are bitboards of all passed pawns.
int GameBoard::attackers_on_enemy_passed_pawns(Color attacker_color,
                                               ull passed_white_pwns,
                                               ull passed_black_pwns)
{
    // enemy (the one who owns the passed pawns we are attacking)
    Color defender_color =
        (attacker_color == Color::WHITE) ? Color::BLACK : Color::WHITE;

    // pick the enemy passed pawns bitboard
    ull enemy_passed =
        (defender_color == Color::WHITE) ? passed_white_pwns
                                         : passed_black_pwns;

    if (!enemy_passed) return 0;

    int total = 0;
    ull tmp = enemy_passed;   // dont mutate input

    while (tmp) {

        int sq = utility::bit::lsb_and_pop_to_square(tmp);  // 0..63
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

// Returns an int in [0, RAND_MAX], just like std::rand().
int GameBoard::rand_new()
{
    using namespace std;
    using namespace std::chrono;

    // Distribution that mimics rand(): 0 .. RAND_MAX
    static uniform_int_distribution<int> dist(0, RAND_MAX);

    // Generate and return a random int
    return dist(rng);
}

// White gets lone king. Black gets queen or rook with his king.
///////////////////////////////////////////////////////////////
std::string GameBoard::random_kqk_fen(bool doQueen)
{

    int itrys = 0;
    // make all 64 squares in h1=0 order: h1,g1,...,a1, h2,...,a8
    std::vector<std::string> all_squares;
    for (int r = 1; r <= 8; ++r) {
        for (int fi = 0; fi < 8; ++fi) {
            char f = char('h' - fi);   // h,g,f,e,d,c,b,a
            std::string sq;
            sq += f;
            sq += char('0' + r);
            all_squares.push_back(sq);
        }
    }

retry_all:

    ++itrys;
    assert (itrys < 5);

    // 1) pick random white king position ("choices" here are any square)
    size_t idx = (size_t)rand_new() % 64;
    if (idx == 0) goto retry_all;       // NOTE: how does this happen?
    //assert (idx > 0);
    std::string wk = all_squares[idx];

    // 2) remove squares adjacent to the white king from "choices"
    std::vector<std::string> bk_choices;
    for (const auto &sq : all_squares) {
        if (sq != wk && !kings_adjacent(wk, sq)) {
            bk_choices.push_back(sq);
        }
    }
    if (bk_choices.empty()) goto retry_all;  // no possible position for black king, try again

    // 3) Pick random black king position
    idx = (size_t)rand_new() % bk_choices.size();
    if (idx == 0) goto retry_all;       // NOTE: how does this happen?
    //assert(0);
    std::string bk = bk_choices[idx];

    // helper: rook attacks (orthogonal only)
    auto rook_attacks = [](const std::string &r, const std::string &k) -> bool
    {
        int f1 = r[0] - 'a';
        int r1 = r[1] - '1';
        int f2 = k[0] - 'a';
        int r2 = k[1] - '1';
        return (f1 == f2) || (r1 == r2);
    };

    // 4) remove squares attacking the white king (do NOT give check to WK) from "choices"
    std::vector<std::string> bX_choices;      // Choices for the black heavy piece
    for (const auto &sq : all_squares)
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

    // 5) Pick random black Q/R position
    idx = (size_t)rand_new() % bX_choices.size();
    if (idx == 0) goto retry_all;       // NOTE: how does this happen?
    //assert (idx > 0);
    std::string bX = bX_choices[idx];  

    // 6) build board [row 8 → row 1], still standard FEN layout
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

    static int n = 0;
    ++n;
    if ((n % 1000) == 0) printf("made %d fens\n", n);
    printf("K/Q/R fen=%s\n", fen.c_str());


    return fen;
}

///////////////////////////////////////////////////////////////////////////////////
//
// Returns 0 if no piece on sq. IF piece on sq (not attacked) it returns the 
//    value of the piece in centipawns. If piece on sq attacked, it "adds up" the attackers.  
//    returns positive if the square is controller (attacked) by the passed side, negative otherwise.
//    Returns in centipawns. 
//
// Force asserts ON in this function, even in "release" builds.


int GameBoard::SEE_for_capture(Color side, const Move &mv, FILE* fpDebug)
{
    // mv.from and mv.to are BITBOARDS (ull) with exactly one bit set.
    ull from_bb = mv.from;
    ull to_bb   = mv.to;

    if (from_bb == 0ULL || to_bb == 0ULL) {   
        assert(0);      // NULL bitboards, should never happen.
        return 0;
    }

    // Convert bitboards to 0..63 square indices (h1 = 0) using your helper
    ull tmp = from_bb;
    int from_sq = utility::bit::lsb_and_pop_to_square(tmp);
    tmp = to_bb;
    int to_sq   = utility::bit::lsb_and_pop_to_square(tmp);

    // There must be an enemy victim on 'to_sq' for a normal capture
    Piece victim = get_piece_type_on_bitboard(to_bb);

    if (victim == Piece::NONE) {
        //assert(0);
        return 0;  // ignore en passant etc. for SEE for now
    }

    Color victim_color = get_color_on_bitboard(to_bb);
    if (victim_color == side) {   
        assert(0);     // Caller should prevent this. Cant take your own piece!
        return 0;
    }

    // Identify the moving piece on 'from_sq'
    Piece mover = get_piece_type_on_bitboard(from_bb);
    if (mover == Piece::NONE) {
        assert(0);      // Caller should prevent this. Cant take nothing!
        return 0;
    }

    auto piece_cp = [&](Piece p) -> int
    {
        return centipawn_score_of(p);
    };

    // Get local mutable copies of all piece bitboards and occupancy
    ull wp = white_pawns,   wn = white_knights, wb = white_bishops,
        wr = white_rooks,   wq = white_queens,  wk = white_king;
    ull bp = black_pawns,   bn = black_knights, bb = black_bishops,
        br = black_rooks,   bq = black_queens,  bk = black_king;

    ull occ =
        wp | wn | wb | wr | wq | wk |
        bp | bn | bb | br | bq | bk;

    const ull FILE_A = col_masks[Col::COL_A];
    const ull FILE_H = col_masks[Col::COL_H];

    // Helper: piece type of a given bit for given color (using local boards)
    auto piece_type_of = [&](Color c, ull bb1) -> Piece
    {
        if (c == Color::WHITE)
        {
            if (bb1 & wp) return Piece::PAWN;
            if (bb1 & wn) return Piece::KNIGHT;
            if (bb1 & wb) return Piece::BISHOP;
            if (bb1 & wr) return Piece::ROOK;
            if (bb1 & wq) return Piece::QUEEN;
            if (bb1 & wk) return Piece::KING;
        }
        else
        {
            if (bb1 & bp) return Piece::PAWN;
            if (bb1 & bn) return Piece::KNIGHT;
            if (bb1 & bb) return Piece::BISHOP;
            if (bb1 & br) return Piece::ROOK;
            if (bb1 & bq) return Piece::QUEEN;
            if (bb1 & bk) return Piece::KING;
        }
        return Piece::NONE;
    };

    // Helper: attackers on 'to_sq' for color c, using local boards & occ
    auto attackers_on_initial = [&](Color c, ull occ_now) -> ull
    {
        ull atk = 0ULL;
        ull bit = (1ULL << to_sq);

        // Pawns (origins that attack to_sq)
        ull pawns = (c == Color::WHITE) ? wp : bp;
        ull origins;
        if (c == Color::WHITE)
        {
            origins = ((bit & ~FILE_A) >> 7) | ((bit & ~FILE_H) >> 9);
        } else {
            origins = ((bit & ~FILE_H) << 7) | ((bit & ~FILE_A) << 9);
        }
        atk |= (origins & pawns);

        // Knights
        ull knights = (c == Color::WHITE) ? wn : bn;
        atk |= tables::movegen::knight_attack_table[to_sq] & knights;

        // Kings
        ull kings = (c == Color::WHITE) ? wk : bk;
        atk |= tables::movegen::king_attack_table[to_sq] & kings;

        // Bishops / Queens on diagonals
        ull bishops = (c == Color::WHITE) ? wb : bb;
        ull queens  = (c == Color::WHITE) ? wq : bq;

        int r0 = to_sq / 8;
        int c0 = to_sq % 8;

        const int diag_dirs[4][2] = { {+1,+1}, {+1,-1}, {-1,+1}, {-1,-1} };
        for (int k = 0; k < 4; ++k) {
            int r = r0;
            int f = c0;
            while (true) {
                r += diag_dirs[k][0];
                f += diag_dirs[k][1];
                if (r < 0 || r > 7 || f < 0 || f > 7) break;
                int s2  = r * 8 + f;
                ull bb2 = 1ULL << s2;
                if (occ_now & bb2)
                {
                    if ((bb2 & bishops) || (bb2 & queens))
                        atk |= bb2;
                    break;
                }
            }
        }

        // Rooks / Queens on ranks/files
        ull rooks = (c == Color::WHITE) ? wr : br;
        const int ortho_dirs[4][2] = { {+1,0}, {-1,0}, {0,+1}, {0,-1} };
        for (int k = 0; k < 4; ++k) {
            int r = r0;
            int f = c0;
            while (true) {
                r += ortho_dirs[k][0];
                f += ortho_dirs[k][1];
                if (r < 0 || r > 7 || f < 0 || f > 7) break;
                int s2  = r * 8 + f;
                ull bb2 = 1ULL << s2;
                if (occ_now & bb2)
                {
                    if ((bb2 & rooks) || (bb2 & queens))
                        atk |= bb2;
                    break;
                }
            }
        }

        return atk;
    };

    // Optional debug: dump initial attackers
    if (fpDebug)
    {
        ull att_w0 = attackers_on_initial(Color::WHITE, occ);
        ull att_b0 = attackers_on_initial(Color::BLACK, occ);

        fprintf(fpDebug,
                "debug (before forced capture): side=%s from_bb=0x%016llx to_bb=0x%016llx "
                "from_sq=%d to_sq=%d victim=%d mover=%d\n",
                (side == Color::WHITE ? "WHITE" : "BLACK"),
                (unsigned long long)from_bb,
                (unsigned long long)to_bb,
                from_sq, to_sq,
                (int)victim, (int)mover);

        auto dump_attackers = [&](const char* label, ull mask)
        {
            fprintf(fpDebug, "    %s attackers on sq %d:", label, to_sq);
            if (!mask)
            {
                fprintf(fpDebug, " [none]\n");
                return;
            }
            ull tmp2 = mask;
            while (tmp2)
            {
                int s = utility::bit::lsb_and_pop_to_square(tmp2);
                fprintf(fpDebug, " %d", s);
            }
            fprintf(fpDebug, "\n");
        };

        dump_attackers("white", att_w0);
        dump_attackers("black", att_b0);
    }

    // === Apply the FORCED first capture mv by 'side' ===

    int balance = 0;
    balance += piece_cp(victim);   // side captures victim

    ull to_mask = (1ULL << to_sq);

    // Remove victim from its color's bitboard
    if (victim_color == Color::WHITE) {
        if (victim == Piece::PAWN)   wp &= ~to_mask;
        else if (victim == Piece::KNIGHT) wn &= ~to_mask;
        else if (victim == Piece::BISHOP) wb &= ~to_mask;
        else if (victim == Piece::ROOK)   wr &= ~to_mask;
        else if (victim == Piece::QUEEN)  wq &= ~to_mask;
        else if (victim == Piece::KING)   wk &= ~to_mask;
    } else {
        if (victim == Piece::PAWN)   bp &= ~to_mask;
        else if (victim == Piece::KNIGHT) bn &= ~to_mask;
        else if (victim == Piece::BISHOP) bb &= ~to_mask;
        else if (victim == Piece::ROOK)   br &= ~to_mask;
        else if (victim == Piece::QUEEN)  bq &= ~to_mask;
        else if (victim == Piece::KING)   bk &= ~to_mask;
    }

    // Remove mover from its original square
    if (side == Color::WHITE) {
        if (mover == Piece::PAWN)   wp &= ~from_bb;
        else if (mover == Piece::KNIGHT) wn &= ~from_bb;
        else if (mover == Piece::BISHOP) wb &= ~from_bb;
        else if (mover == Piece::ROOK)   wr &= ~from_bb;
        else if (mover == Piece::QUEEN)  wq &= ~from_bb;
        else if (mover == Piece::KING)   wk &= ~from_bb;
    } else {
        if (mover == Piece::PAWN)   bp &= ~from_bb;
        else if (mover == Piece::KNIGHT) bn &= ~from_bb;
        else if (mover == Piece::BISHOP) bb &= ~from_bb;
        else if (mover == Piece::ROOK)   br &= ~from_bb;
        else if (mover == Piece::QUEEN)  bq &= ~from_bb;
        else if (mover == Piece::KING)   bk &= ~from_bb;
    }

    // Place mover on to_sq
    if (side == Color::WHITE) {
        if (mover == Piece::PAWN)   wp |= to_mask;
        else if (mover == Piece::KNIGHT) wn |= to_mask;
        else if (mover == Piece::BISHOP) wb |= to_mask;
        else if (mover == Piece::ROOK)   wr |= to_mask;
        else if (mover == Piece::QUEEN)  wq |= to_mask;
        else if (mover == Piece::KING)   wk |= to_mask;
    } else {
        if (mover == Piece::PAWN)   bp |= to_mask;
        else if (mover == Piece::KNIGHT) bn |= to_mask;
        else if (mover == Piece::BISHOP) bb |= to_mask;
        else if (mover == Piece::ROOK)   br |= to_mask;
        else if (mover == Piece::QUEEN)  bq |= to_mask;
        else if (mover == Piece::KING)   bk |= to_mask;
    }

    // Update occupancy
    occ &= ~from_bb;   // clear original square of mover
    occ &= ~to_mask;   // clear victim
    occ |=  to_mask;   // mover now on to_sq

    // After the first forced capture, the target is now the mover on to_sq
    Piece target_piece  = mover;
    Color target_color  = side;
    Color root_side     = side;
    Color next_stm      = (side == Color::WHITE) ? Color::BLACK : Color::WHITE;

    // === Recursive SEE on this square after the first forced capture ===
    auto see_rec = [&](auto &&self,
                       Color stm,
                       Piece target_piece_local, Color target_color_local,
                       ull occ_local,
                       ull wp_local, ull wn_local, ull wb_local,
                       ull wr_local, ull wq_local, ull wk_local,
                       ull bp_local, ull bn_local, ull bb_local,
                       ull br_local, ull bq_local, ull bk_local,
                       int balance_local) -> int
    {
        int best = balance_local;

        // Local attackers_on using local boards & occ (same pattern as above)
        auto attackers_on_state = [&](Color c) -> ull
        {
            ull atk = 0ULL;
            ull bit = (1ULL << to_sq);

            ull pawns = (c == Color::WHITE) ? wp_local : bp_local;
            ull origins;
            if (c == Color::WHITE) {
                origins = ((bit & ~FILE_A) >> 7) | ((bit & ~FILE_H) >> 9);
            }
            else {
                origins = ((bit & ~FILE_H) << 7) | ((bit & ~FILE_A) << 9);
            }
            atk |= (origins & pawns);

            ull knights = (c == Color::WHITE) ? wn_local : bn_local;
            atk |= tables::movegen::knight_attack_table[to_sq] & knights;

            ull kings = (c == Color::WHITE) ? wk_local : bk_local;
            atk |= tables::movegen::king_attack_table[to_sq] & kings;

            ull bishops = (c == Color::WHITE) ? wb_local : bb_local;
            ull queens  = (c == Color::WHITE) ? wq_local : bq_local;

            int r0 = to_sq / 8;
            int c0 = to_sq % 8;

            const int diag_dirs[4][2] = { {+1,+1}, {+1,-1}, {-1,+1}, {-1,-1} };
            for (int k = 0; k < 4; ++k) {
                int r = r0;
                int f = c0;
                while (true)
                {
                    r += diag_dirs[k][0];
                    f += diag_dirs[k][1];
                    if (r < 0 || r > 7 || f < 0 || f > 7) break;
                    int s2  = r * 8 + f;
                    ull bb2 = 1ULL << s2;
                    if (occ_local & bb2) {
                        if ((bb2 & bishops) || (bb2 & queens))
                            atk |= bb2;
                        break;
                    }
                }
            }

            ull rooks = (c == Color::WHITE) ? wr_local : br_local;
            const int ortho_dirs[4][2] = { {+1,0}, {-1,0}, {0,+1}, {0,-1} };
            for (int k = 0; k < 4; ++k) {
                int r = r0;
                int f = c0;
                while (true) {
                    r += ortho_dirs[k][0];
                    f += ortho_dirs[k][1];
                    if (r < 0 || r > 7 || f < 0 || f > 7) break;
                    int s2  = r * 8 + f;
                    ull bb2 = 1ULL << s2;
                    if (occ_local & bb2)
                    {
                        if ((bb2 & rooks) || (bb2 & queens))
                            atk |= bb2;
                        break;
                    }
                }
            }

            return atk;
        };

        // Get all attackers for stm on to_sq
        ull atk_side = attackers_on_state(stm);
        if (!atk_side)
            return best;  // no more captures; side chooses to stop

        // Try every possible attacker
        ull tmp_atk = atk_side;
        while (tmp_atk) {

            ull attacker_bb = tmp_atk & (~tmp_atk + 1ULL);  // extract LS1B
            tmp_atk &= ~attacker_bb;

            // Copy local state
            ull occ2  = occ_local;
            ull wp2   = wp_local, wn2 = wn_local, wb2 = wb_local;
            ull wr2   = wr_local, wq2 = wq_local, wk2 = wk_local;
            ull bp2   = bp_local, bn2 = bn_local, bb2 = bb_local;
            ull br2   = br_local, bq2 = bq_local, bk2 = bk_local;

            // Material change from root_side's point of view:
            int val = piece_cp(target_piece_local);
            int new_balance = balance_local;
            if (stm == root_side)
                new_balance += val;
            else
                new_balance -= val;

            // Apply capture: remove target from its bitboard, move attacker to to_sq
            ull to_mask_local = (1ULL << to_sq);

            if (target_color_local == Color::WHITE) {
                if (target_piece_local == Piece::PAWN)   wp2 &= ~to_mask_local;
                else if (target_piece_local == Piece::KNIGHT) wn2 &= ~to_mask_local;
                else if (target_piece_local == Piece::BISHOP) wb2 &= ~to_mask_local;
                else if (target_piece_local == Piece::ROOK)   wr2 &= ~to_mask_local;
                else if (target_piece_local == Piece::QUEEN)  wq2 &= ~to_mask_local;
                else if (target_piece_local == Piece::KING)   wk2 &= ~to_mask_local;
            } else {
                if (target_piece_local == Piece::PAWN)   bp2 &= ~to_mask_local;
                else if (target_piece_local == Piece::KNIGHT) bn2 &= ~to_mask_local;
                else if (target_piece_local == Piece::BISHOP) bb2 &= ~to_mask_local;
                else if (target_piece_local == Piece::ROOK)   br2 &= ~to_mask_local;
                else if (target_piece_local == Piece::QUEEN)  bq2 &= ~to_mask_local;
                else if (target_piece_local == Piece::KING)   bk2 &= ~to_mask_local;
            }

            // Figure out what piece is attacking (before we remove it)
            Piece attacker_piece = piece_type_of(stm, attacker_bb);

            // Remove attacker from its original square
            if (stm == Color::WHITE) {
                if (attacker_bb & wp2) wp2 &= ~attacker_bb;
                else if (attacker_bb & wn2) wn2 &= ~attacker_bb;
                else if (attacker_bb & wb2) wb2 &= ~attacker_bb;
                else if (attacker_bb & wr2) wr2 &= ~attacker_bb;
                else if (attacker_bb & wq2) wq2 &= ~attacker_bb;
                else if (attacker_bb & wk2) wk2 &= ~attacker_bb;
            } else {
                if (attacker_bb & bp2) bp2 &= ~attacker_bb;
                else if (attacker_bb & bn2) bn2 &= ~attacker_bb;
                else if (attacker_bb & bb2) bb2 &= ~attacker_bb;
                else if (attacker_bb & br2) br2 &= ~attacker_bb;
                else if (attacker_bb & bq2) bq2 &= ~attacker_bb;
                else if (attacker_bb & bk2) bk2 &= ~attacker_bb;
            }

            // Place attacker on to_sq
            if (stm == Color::WHITE) {
                if (attacker_piece == Piece::PAWN)   wp2 |= to_mask_local;
                else if (attacker_piece == Piece::KNIGHT) wn2 |= to_mask_local;
                else if (attacker_piece == Piece::BISHOP) wb2 |= to_mask_local;
                else if (attacker_piece == Piece::ROOK)   wr2 |= to_mask_local;
                else if (attacker_piece == Piece::QUEEN)  wq2 |= to_mask_local;
                else if (attacker_piece == Piece::KING)   wk2 |= to_mask_local;
            } else {
                if (attacker_piece == Piece::PAWN)   bp2 |= to_mask_local;
                else if (attacker_piece == Piece::KNIGHT) bn2 |= to_mask_local;
                else if (attacker_piece == Piece::BISHOP) bb2 |= to_mask_local;
                else if (attacker_piece == Piece::ROOK)   br2 |= to_mask_local;
                else if (attacker_piece == Piece::QUEEN)  bq2 |= to_mask_local;
                else if (attacker_piece == Piece::KING)   bk2 |= to_mask_local;
            }

            // Update occupancy
            occ2 &= ~attacker_bb;
            occ2 &= ~to_mask_local;
            occ2 |=  to_mask_local;

            // New target is this attacker on to_sq
            Piece new_target_piece = attacker_piece;
            Color new_target_color = stm;

            Color next = (stm == Color::WHITE ? Color::BLACK : Color::WHITE);

            int child = self(self,
                             next,
                             new_target_piece, new_target_color,
                             occ2,
                             wp2, wn2, wb2, wr2, wq2, wk2,
                             bp2, bn2, bb2, br2, bq2, bk2,
                             new_balance);

            if (stm == root_side) {
                if (child > best) best = child;
            }
            else {
                if (child < best) best = child;
            }
        }

        return best;
    };

    int result = see_rec(see_rec,
                         next_stm,
                         target_piece, target_color,
                         occ,
                         wp, wn, wb, wr, wq, wk,
                         bp, bn, bb, br, bq, bk,
                         balance);

    if (fpDebug) {
        fprintf(fpDebug,
                "debug final: move(from_sq=%d,to_sq=%d) side=%s SEE=%d\n",
                from_sq, to_sq,
                (side == Color::WHITE ? "WHITE" : "BLACK"),
                result);
    }

    return result;
}



int GameBoard::SEE_for_capture_new(Color side, const Move &mv, FILE* fpDebug)
{
    // mv.from and mv.to are BITBOARDS (ull) with exactly one bit set.
    ull from_bb = mv.from;
    ull to_bb   = mv.to;

    if (from_bb == 0ULL || to_bb == 0ULL) {
        assert(0);      // NULL bitboards, should never happen.
        return 0;
    }

    // Convert bitboards to 0..63 square indices (h1 = 0) using your helper
    ull tmp = from_bb;
    int from_sq = utility::bit::lsb_and_pop_to_square(tmp);
    tmp = to_bb;
    int to_sq   = utility::bit::lsb_and_pop_to_square(tmp);

    // There must be an enemy victim on 'to_sq' for a normal capture
    Piece victim = get_piece_type_on_bitboard(to_bb);
    if (victim == Piece::NONE) {
        // ignore en passant etc. for SEE for now
        return 0;
    }

    Color victim_color = get_color_on_bitboard(to_bb);
    if (victim_color == side) {
        assert(0);     // Caller should prevent this. Can't take your own piece!
        return 0;
    }

    // Identify the moving piece on 'from_sq'
    Piece mover = get_piece_type_on_bitboard(from_bb);
    if (mover == Piece::NONE) {
        assert(0);      // Caller should prevent this. Can't take nothing!
        return 0;
    }

    auto piece_cp = [&](Piece p) -> int {
        return centipawn_score_of(p);
    };

    // Local mutable copies of all piece bitboards and occupancy
    ull wp = white_pawns,   wn = white_knights, wb = white_bishops,
        wr = white_rooks,   wq = white_queens,  wk = white_king;
    ull bp = black_pawns,   bn = black_knights, bb = black_bishops,
        br = black_rooks,   bq = black_queens,  bk = black_king;

    ull occ =
        wp | wn | wb | wr | wq | wk |
        bp | bn | bb | br | bq | bk;

    const ull FILE_A = col_masks[Col::COL_A];
    const ull FILE_H = col_masks[Col::COL_H];

    // Helper: current piece type of a bitboard (using local boards)
    auto piece_type_of = [&](Color c, ull bb1) -> Piece {
        if (c == Color::WHITE) {
            if (bb1 & wp) return Piece::PAWN;
            if (bb1 & wn) return Piece::KNIGHT;
            if (bb1 & wb) return Piece::BISHOP;
            if (bb1 & wr) return Piece::ROOK;
            if (bb1 & wq) return Piece::QUEEN;
            if (bb1 & wk) return Piece::KING;
        } else {
            if (bb1 & bp) return Piece::PAWN;
            if (bb1 & bn) return Piece::KNIGHT;
            if (bb1 & bb) return Piece::BISHOP;
            if (bb1 & br) return Piece::ROOK;
            if (bb1 & bq) return Piece::QUEEN;
            if (bb1 & bk) return Piece::KING;
        }
        return Piece::NONE;
    };

    // Helper: all attackers on 'to_sq' for color c using current local boards & occ
    auto attackers_on = [&](Color c) -> ull {
        ull atk = 0ULL;
        ull bit = (1ULL << to_sq);

        // Pawns (origins that attack to_sq)
        ull pawns = (c == Color::WHITE) ? wp : bp;
        ull origins;
        if (c == Color::WHITE) {
            origins = ((bit & ~FILE_A) >> 7) | ((bit & ~FILE_H) >> 9);
        } else {
            origins = ((bit & ~FILE_H) << 7) | ((bit & ~FILE_A) << 9);
        }
        atk |= (origins & pawns);

        // Knights
        ull knights = (c == Color::WHITE) ? wn : bn;
        atk |= tables::movegen::knight_attack_table[to_sq] & knights;

        // Kings
        ull kings = (c == Color::WHITE) ? wk : bk;
        atk |= tables::movegen::king_attack_table[to_sq] & kings;

        // Bishops / Queens on diagonals
        ull bishops = (c == Color::WHITE) ? wb : bb;
        ull queens  = (c == Color::WHITE) ? wq : bq;

        int r0 = to_sq / 8;
        int f0 = to_sq % 8;

        const int diag_dirs[4][2] = { {+1,+1}, {+1,-1}, {-1,+1}, {-1,-1} };
        for (int k = 0; k < 4; ++k) {
            int r = r0;
            int f = f0;
            while (true) {
                r += diag_dirs[k][0];
                f += diag_dirs[k][1];
                if (r < 0 || r > 7 || f < 0 || f > 7) break;
                int s2  = r * 8 + f;
                ull bb2 = 1ULL << s2;
                if (occ & bb2) {
                    if ((bb2 & bishops) || (bb2 & queens))
                        atk |= bb2;
                    break;
                }
            }
        }

        // Rooks / Queens on files/ranks
        ull rooks = (c == Color::WHITE) ? wr : br;
        const int ortho_dirs[4][2] = { {+1,0}, {-1,0}, {0,+1}, {0,-1} };
        for (int k = 0; k < 4; ++k) {
            int r = r0;
            int f = f0;
            while (true) {
                r += ortho_dirs[k][0];
                f += ortho_dirs[k][1];
                if (r < 0 || r > 7 || f < 0 || f > 7) break;
                int s2  = r * 8 + f;
                ull bb2 = 1ULL << s2;
                if (occ & bb2) {
                    if ((bb2 & rooks) || (bb2 & queens))
                        atk |= bb2;
                    break;
                }
            }
        }

        return atk;
    };

    // === Apply the FORCED first capture mv by 'side' ===

    int balance = 0;
    balance += piece_cp(victim);   // side captures victim

    ull to_mask = (1ULL << to_sq);

    // Remove victim from its color's bitboard
    if (victim_color == Color::WHITE) {
        if (victim == Piece::PAWN)      wp &= ~to_mask;
        else if (victim == Piece::KNIGHT) wn &= ~to_mask;
        else if (victim == Piece::BISHOP) wb &= ~to_mask;
        else if (victim == Piece::ROOK)   wr &= ~to_mask;
        else if (victim == Piece::QUEEN)  wq &= ~to_mask;
        else if (victim == Piece::KING)   wk &= ~to_mask;
    } else {
        if (victim == Piece::PAWN)      bp &= ~to_mask;
        else if (victim == Piece::KNIGHT) bn &= ~to_mask;
        else if (victim == Piece::BISHOP) bb &= ~to_mask;
        else if (victim == Piece::ROOK)   br &= ~to_mask;
        else if (victim == Piece::QUEEN)  bq &= ~to_mask;
        else if (victim == Piece::KING)   bk &= ~to_mask;
    }

    // Remove mover from its original square
    if (side == Color::WHITE) {
        if (mover == Piece::PAWN)      wp &= ~from_bb;
        else if (mover == Piece::KNIGHT) wn &= ~from_bb;
        else if (mover == Piece::BISHOP) wb &= ~from_bb;
        else if (mover == Piece::ROOK)   wr &= ~from_bb;
        else if (mover == Piece::QUEEN)  wq &= ~from_bb;
        else if (mover == Piece::KING)   wk &= ~from_bb;
    } else {
        if (mover == Piece::PAWN)      bp &= ~from_bb;
        else if (mover == Piece::KNIGHT) bn &= ~from_bb;
        else if (mover == Piece::BISHOP) bb &= ~from_bb;
        else if (mover == Piece::ROOK)   br &= ~from_bb;
        else if (mover == Piece::QUEEN)  bq &= ~from_bb;
        else if (mover == Piece::KING)   bk &= ~from_bb;
    }

    // Place mover on to_sq
    if (side == Color::WHITE) {
        if (mover == Piece::PAWN)      wp |= to_mask;
        else if (mover == Piece::KNIGHT) wn |= to_mask;
        else if (mover == Piece::BISHOP) wb |= to_mask;
        else if (mover == Piece::ROOK)   wr |= to_mask;
        else if (mover == Piece::QUEEN)  wq |= to_mask;
        else if (mover == Piece::KING)   wk |= to_mask;
    } else {
        if (mover == Piece::PAWN)      bp |= to_mask;
        else if (mover == Piece::KNIGHT) bn |= to_mask;
        else if (mover == Piece::BISHOP) bb |= to_mask;
        else if (mover == Piece::ROOK)   br |= to_mask;
        else if (mover == Piece::QUEEN)  bq |= to_mask;
        else if (mover == Piece::KING)   bk |= to_mask;
    }

    // Update occupancy
    occ &= ~from_bb;   // clear original square of mover
    occ &= ~to_mask;   // clear victim
    occ |=  to_mask;   // mover now on to_sq

    // After the first forced capture, the target is now the mover on to_sq
    Piece current_target_piece  = mover;
    Color current_target_color  = side;
    Color root_side             = side;
    Color stm                   = (side == Color::WHITE) ? Color::BLACK : Color::WHITE;

    // === Stockfish-style SEE: build gain[] along a single exchange line ===

    int gain[32];
    int depth = 0;

    // gain[0]: material after the initial capture by 'side'
    gain[0] = balance;

    // Simulate further captures on to_sq
    while (true) {
        ull atk_side = attackers_on(stm);
        if (!atk_side)
            break;

        // Pick least valuable attacker for stm on this square
        ull attackers = atk_side;
        ull attacker_bb = 0ULL;
        Piece attacker_piece = Piece::NONE;

        // Pawn
        ull pawns = (stm == Color::WHITE) ? wp : bp;
        ull mask = attackers & pawns;
        if (mask) {
            attacker_bb = mask & (~mask + 1ULL);
            attacker_piece = Piece::PAWN;
        } else {
            // Knight
            ull knights = (stm == Color::WHITE) ? wn : bn;
            mask = attackers & knights;
            if (mask) {
                attacker_bb = mask & (~mask + 1ULL);
                attacker_piece = Piece::KNIGHT;
            } else {
                // Bishop
                ull bishops = (stm == Color::WHITE) ? wb : bb;
                mask = attackers & bishops;
                if (mask) {
                    attacker_bb = mask & (~mask + 1ULL);
                    attacker_piece = Piece::BISHOP;
                } else {
                    // Rook
                    ull rooks = (stm == Color::WHITE) ? wr : br;
                    mask = attackers & rooks;
                    if (mask) {
                        attacker_bb = mask & (~mask + 1ULL);
                        attacker_piece = Piece::ROOK;
                    } else {
                        // Queen
                        ull queens = (stm == Color::WHITE) ? wq : bq;
                        mask = attackers & queens;
                        if (mask) {
                            attacker_bb = mask & (~mask + 1ULL);
                            attacker_piece = Piece::QUEEN;
                        } else {
                            // King (last resort)
                            ull kings = (stm == Color::WHITE) ? wk : bk;
                            mask = attackers & kings;
                            if (mask) {
                                attacker_bb = mask & (~mask + 1ULL);
                                attacker_piece = Piece::KING;
                            }
                        }
                    }
                }
            }
        }

        if (!attacker_bb || attacker_piece == Piece::NONE)
            break;  // should not happen, but be safe

        // Material change from root_side's point of view:
        int val_target = piece_cp(current_target_piece);
        int prev_balance = gain[depth];
        int new_balance;
        if (stm == root_side)
            new_balance = prev_balance + val_target;
        else
            new_balance = prev_balance - val_target;

        if (depth + 1 < (int)(sizeof(gain) / sizeof(gain[0])))
            gain[++depth] = new_balance;
        else
            break; // safety guard: shouldn't ever hit

        // Apply capture on the board: attacker takes current_target_piece on to_sq

        // Remove the target from its bitboard
        if (current_target_color == Color::WHITE) {
            if (current_target_piece == Piece::PAWN)      wp &= ~to_mask;
            else if (current_target_piece == Piece::KNIGHT) wn &= ~to_mask;
            else if (current_target_piece == Piece::BISHOP) wb &= ~to_mask;
            else if (current_target_piece == Piece::ROOK)   wr &= ~to_mask;
            else if (current_target_piece == Piece::QUEEN)  wq &= ~to_mask;
            else if (current_target_piece == Piece::KING)   wk &= ~to_mask;
        } else {
            if (current_target_piece == Piece::PAWN)      bp &= ~to_mask;
            else if (current_target_piece == Piece::KNIGHT) bn &= ~to_mask;
            else if (current_target_piece == Piece::BISHOP) bb &= ~to_mask;
            else if (current_target_piece == Piece::ROOK)   br &= ~to_mask;
            else if (current_target_piece == Piece::QUEEN)  bq &= ~to_mask;
            else if (current_target_piece == Piece::KING)   bk &= ~to_mask;
        }

        // Remove attacker from its original square and place it on to_sq
        if (stm == Color::WHITE) {
            if (attacker_piece == Piece::PAWN)      wp &= ~attacker_bb;
            else if (attacker_piece == Piece::KNIGHT) wn &= ~attacker_bb;
            else if (attacker_piece == Piece::BISHOP) wb &= ~attacker_bb;
            else if (attacker_piece == Piece::ROOK)   wr &= ~attacker_bb;
            else if (attacker_piece == Piece::QUEEN)  wq &= ~attacker_bb;
            else if (attacker_piece == Piece::KING)   wk &= ~attacker_bb;

            if (attacker_piece == Piece::PAWN)      wp |= to_mask;
            else if (attacker_piece == Piece::KNIGHT) wn |= to_mask;
            else if (attacker_piece == Piece::BISHOP) wb |= to_mask;
            else if (attacker_piece == Piece::ROOK)   wr |= to_mask;
            else if (attacker_piece == Piece::QUEEN)  wq |= to_mask;
            else if (attacker_piece == Piece::KING)   wk |= to_mask;
        } else {
            if (attacker_piece == Piece::PAWN)      bp &= ~attacker_bb;
            else if (attacker_piece == Piece::KNIGHT) bn &= ~attacker_bb;
            else if (attacker_piece == Piece::BISHOP) bb &= ~attacker_bb;
            else if (attacker_piece == Piece::ROOK)   br &= ~attacker_bb;
            else if (attacker_piece == Piece::QUEEN)  bq &= ~attacker_bb;
            else if (attacker_piece == Piece::KING)   bk &= ~attacker_bb;

            if (attacker_piece == Piece::PAWN)      bp |= to_mask;
            else if (attacker_piece == Piece::KNIGHT) bn |= to_mask;
            else if (attacker_piece == Piece::BISHOP) bb |= to_mask;
            else if (attacker_piece == Piece::ROOK)   br |= to_mask;
            else if (attacker_piece == Piece::QUEEN)  bq |= to_mask;
            else if (attacker_piece == Piece::KING)   bk |= to_mask;
        }

        // Update occupancy
        occ &= ~attacker_bb;
        occ &= ~to_mask;
        occ |=  to_mask;

        // New target on to_sq is now the attacker we just moved
        current_target_piece  = attacker_piece;
        current_target_color  = stm;

        // Next side to move
        stm = (stm == Color::WHITE) ? Color::BLACK : Color::WHITE;
    }

    // === Backward propagation: each side can choose to stop or continue ===
    // Gain[] is from root_side's point of view, alternating turns implicitly.
    while (depth > 0) {
        int g_prev = gain[depth - 1];
        int g_next = gain[depth];
        // Each side chooses whether to continue or not:
        // gain[i-1] = max(-gain[i-1], gain[i]);
        int cont_if = -g_prev;
        gain[depth - 1] = (cont_if > g_next) ? cont_if : g_next;
        --depth;
    }

    int result = gain[0];

    if (fpDebug) {
        fprintf(fpDebug,
                "debug SEE_new final: move(from_sq=%d,to_sq=%d) side=%s SEE=%d\n",
                from_sq, to_sq,
                (side == Color::WHITE ? "WHITE" : "BLACK"),
                result);
    }

    return result;
}


} // end namespace ShumiChess
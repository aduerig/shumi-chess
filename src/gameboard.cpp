
#include <math.h>
#include <vector>

#include <random>
#include <chrono>  


#include "gameboard.hpp"
#include "weights.hpp"
//
#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
#undef NDEBUG
#endif
#include <assert.h>


extern bool global_debug_flag;     // NOTE: make me go away


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


// Piece GameBoard::get_piece_type_on_bitboard(ull bitboard) {
//     vector<Piece> all_piece_types = { Piece::PAWN, Piece::ROOK, Piece::KNIGHT, Piece::BISHOP, Piece::QUEEN, Piece::KING };
//     for (auto piece_type : all_piece_types) {
//         if (get_pieces(piece_type) & bitboard) {
//             return piece_type;
//         }
//     }
//     return Piece::NONE;
// }
// Faster: assumes bitboard is a bitboard mask (can be 1-bit or multi-bit).
// Returns the first matching Piece type found.


Color GameBoard::get_color_on_bitboard(ull bitboard) {
    if (get_pieces(Color::WHITE) & bitboard) {
        return Color::WHITE;
    } else {
        return Color::BLACK;
    }
}

// Is there only zero or one pieces on each square?
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
//
int GameBoard::get_castled_bonus_cp(Color color1, int phase) const {
    //
    // notice: "Has_castled" must be larger thn "can_castle" or the king will move stupidly too early.

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

    // has_castled bonus must be > castled priviledge or it will never castle.
    int icode = (b_has_castled ? Weights::HAS_CASTLED_WGHT : 0) + i_can_castle * Weights::CAN_CASTLE_WGHT;

    int final_cp;

    // But its not so imporant later in the game...
    if      (phase == GamePhase::OPENING) final_cp = icode;
    else if (phase == GamePhase::MIDDLE_EARLY) final_cp = icode/2;
    else final_cp = 0;



    return final_cp;  // "centipawns"
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
// The total of material for that color. Uses centipawn_score_of(). Returns centipawns. Always positive. 
int GameBoard::get_material_for_color(Color color, int& cp_pawns_only_temp) {
    ull pieces_bitboard;
    int cp_board_score;

    const ull itemp   = get_pieces_template<Piece::PAWN> (color);

    int cp_score_mat_temp = 0;                    // no pawns in this one
    cp_pawns_only_temp = bits_in(get_pieces(color, Piece::PAWN))   * centipawn_score_of(Piece::PAWN);

    cp_score_mat_temp += cp_pawns_only_temp;
    
    cp_score_mat_temp += bits_in(get_pieces(color, Piece::KNIGHT)) * centipawn_score_of(Piece::KNIGHT);
    cp_score_mat_temp += bits_in(get_pieces(color, Piece::BISHOP)) * centipawn_score_of(Piece::BISHOP);
    cp_score_mat_temp += bits_in(get_pieces(color, Piece::ROOK))   * centipawn_score_of(Piece::ROOK);
    cp_score_mat_temp += bits_in(get_pieces(color, Piece::QUEEN))  * centipawn_score_of(Piece::QUEEN);

    // // Add up the scores for each piece
    // int cp_score_mat_temp = 0;                    // no pawns in this one
    // for (Piece piece_type = Piece::ROOK;
    //     piece_type <= Piece::QUEEN;
    //     piece_type = static_cast<Piece>(static_cast<int>(piece_type) + 1))     // increment
    // {    
    //     // Get bitboard of all pieces on board of this type and color
    //     pieces_bitboard = get_pieces(color, piece_type);
    //     // Adds for the piece value multiplied by how many of that piece there is (using centipawns)
    //     cp_board_score = centipawn_score_of(piece_type);
    //     int nPieces = bits_in(pieces_bitboard);
    //     //cp_score_mat_temp += (int)(((double)nPieces * (double)cp_board_score));
    //     cp_score_mat_temp += nPieces * cp_board_score;
    //     // This return must always be positive.
    //     assert (cp_score_mat_temp>=0);
    // }
    // cp_score_mat_temp += cp_pawns_only_temp;

    if (global_debug_flag) {
        
        const int nP = bits_in(get_pieces(color, Piece::PAWN));
        const int nN = bits_in(get_pieces(color, Piece::KNIGHT));
        const int nB = bits_in(get_pieces(color, Piece::BISHOP));
        const int nR = bits_in(get_pieces(color, Piece::ROOK));
        const int nQ = bits_in(get_pieces(color, Piece::QUEEN));
        const int nK = bits_in(get_pieces(color, Piece::KING));

        printf("MATDBG color=%d  PAWN=%d KNIGHT=%d BISHOP=%d ROOK=%d QUEEN=%d KING=%d\n",
               (int)color, nP, nN, nB, nR, nQ, nK);

        // Optional extra: show the centipawn contributions too
        const int cpP = nP * centipawn_score_of(Piece::PAWN);
        const int cpN = nN * centipawn_score_of(Piece::KNIGHT);
        const int cpB = nB * centipawn_score_of(Piece::BISHOP);
        const int cpR = nR * centipawn_score_of(Piece::ROOK);
        const int cpQ = nQ * centipawn_score_of(Piece::QUEEN);

        printf("MATDBG cp: P=%d N=%d B=%d R=%d Q=%d  total(noK)=%d\n",
               cpP, cpN, cpB, cpR, cpQ, (cpP + cpN + cpB + cpR + cpQ));
    }

    return cp_score_mat_temp;
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


int GameBoard::pawns_attacking_center_squares_cp(Color c)
{
    // Offensive .vs. defensive center squares. Like Offensive center the best.
    int sum = 0;

    if (c == Color::WHITE) {

        // Offensive center (near enemy)
        sum += Weights::PAWN_ON_CTR_OFF_WGHT * pawns_attacking_square(c, square_e5);
        sum += Weights::PAWN_ON_CTR_OFF_WGHT * pawns_attacking_square(c, square_d5);

        // Defensive center (near friendly king)
        sum += Weights::PAWN_ON_CTR_DEF_WGHT * pawns_attacking_square(c, square_e4);
        sum += Weights::PAWN_ON_CTR_DEF_WGHT * pawns_attacking_square(c, square_d4);

        // Advanced center
        sum += Weights::PAWN_ON_ADV_CTR_WGHT * pawns_attacking_square(c, square_e6);
        sum += Weights::PAWN_ON_ADV_CTR_WGHT * pawns_attacking_square(c, square_d6);

        // Advanced flank center
        sum += Weights::PAWN_ON_ADV_FLK_WGHT * pawns_attacking_square(c, square_c5);
        sum += Weights::PAWN_ON_ADV_FLK_WGHT * pawns_attacking_square(c, square_f5);

    } else {

        // Offensive center (near enemy)
        sum += Weights::PAWN_ON_CTR_OFF_WGHT * pawns_attacking_square(c, square_e4);
        sum += Weights::PAWN_ON_CTR_OFF_WGHT * pawns_attacking_square(c, square_d4);

        // Defensive center (near friendly king)
        sum += Weights::PAWN_ON_CTR_DEF_WGHT * pawns_attacking_square(c, square_e5);
        sum += Weights::PAWN_ON_CTR_DEF_WGHT * pawns_attacking_square(c, square_d5);

        // Advanced center
        sum += Weights::PAWN_ON_ADV_CTR_WGHT * pawns_attacking_square(c, square_e3);
        sum += Weights::PAWN_ON_ADV_CTR_WGHT * pawns_attacking_square(c, square_d3);

        // Advanced flank center
        sum += Weights::PAWN_ON_ADV_FLK_WGHT * pawns_attacking_square(c, square_c4);
        sum += Weights::PAWN_ON_ADV_FLK_WGHT * pawns_attacking_square(c, square_f4);
    }

    return sum;
}





int GameBoard::knights_attacking_square(Color c, int sq)
{
    ull targets = tables::movegen::knight_attack_table[sq];
    ull knights = get_pieces_template<Piece::KNIGHT>(c);
    return bits_in(targets & knights);  // count the 1-bits
}

int GameBoard::knights_attacking_center_squares_cp(Color c)
{
    int itemp = 0;
    itemp += knights_attacking_square(c, square_e4);
    itemp += knights_attacking_square(c, square_d4);
    itemp += knights_attacking_square(c, square_e5);
    itemp += knights_attacking_square(c, square_d5);

    return (itemp * Weights::KNIGHT_ON_CTR_WGHT);
}


//
// This sees through all material
int GameBoard::bishops_attacking_square(Color c, int sq)
{
    const int tf = sq % 8;   // 0..7 (h1=0 => 0=H ... 7=A) 
    const int tr = sq / 8;   // 0..7

    const int diag_sum  = tf + tr;   // NE-SW diagonal id
    const int diag_diff = tf - tr;   // NW-SE diagonal id

    ull bishops = get_pieces_template<Piece::BISHOP>(c);
    int count = 0;

    while (bishops)
    {
        const int s = utility::bit::lsb_and_pop_to_square(bishops);
        const int f = s % 8;
        const int r = s / 8;
        if ((f + r) == diag_sum || (f - r) == diag_diff) count++;
    }

    return count;
}

int GameBoard::bishops_attacking_center_squares_cp(Color c)
{
    int itemp = 0;

    itemp += bishops_attacking_square(c, square_e4);
    itemp += bishops_attacking_square(c, square_d4);
    itemp += bishops_attacking_square(c, square_e5);
    itemp += bishops_attacking_square(c, square_d5);

    return (itemp*Weights::BISHOP_ON_CTR_WGHT);
}





    
// Return true if all squares strictly between a and b on the same rank are empty.
static inline bool clear_between_rank(ull occupancy, int a, int b)
{
    const int ra = a / 8;
    int fa = a % 8, fb = b % 8;
    if (fa > fb) std::swap(fa, fb);

    for (int f = fa + 1; f < fb; ++f) {
        const int sq = ra * 8 + f;
        if (occupancy & (1ULL << sq)) return false;
    }
    return true;
}

// Return true if all squares strictly between a and b on the same file are empty.
static inline bool clear_between_file(ull occupancy, int a, int b)
{
    const int fa = a % 8;
    int ra = a / 8, rb = b / 8;
    if (ra > rb) std::swap(ra, rb);

    for (int r = ra + 1; r < rb; ++r) {
        const int sq = r * 8 + fa;
        if (occupancy & (1ULL << sq)) return false;
    }
    return true;
}


//  2 or more bishops helps out
int GameBoard::two_bishops_cp(Color c) const {

    //int bishops = bits_in(get_pieces_template<Piece::BISHOP>(c));
    
    ull friendlyBishops = (c == ShumiChess::WHITE) ? white_bishops : black_bishops;
    int bishops = bits_in(friendlyBishops);

    if (bishops >= 2) return Weights::TWO_BISHOPS_WGHT;   // in centipawns  
    return 0;
}

// Stupid bishop blocking pawn (king bishop blocking queen pawn)
int GameBoard::bishop_pawn_pattern_cp(Color color) {

    if (color == Color::WHITE) {
        // White: bishop on d3, pawn on d2  OR  bishop on e2, pawn on e3
        ull bishop_mask = (1ULL << square_d3);
        ull pawn_mask   = (1ULL << square_d2);

        if ( (white_bishops & bishop_mask) &&
             (white_pawns   & pawn_mask) ) {
            return Weights::BISHOP_PATTERN_WGHT;
        }

        bishop_mask = (1ULL << square_e2);
        pawn_mask   = (1ULL << square_e3);

        if ( (white_bishops & bishop_mask) &&
             (white_pawns   & pawn_mask) ) {
            return Weights::BISHOP_PATTERN_WGHT;
        }
    } else {    // color == BLACK
        // Black: bishop on d6, pawn on d7  OR  bishop on e7, pawn on e6
        ull bishop_mask = (1ULL << square_d6);
        ull pawn_mask   = (1ULL << square_d7);

        if ( (black_bishops & bishop_mask) &&
             (black_pawns   & pawn_mask) ) {
            return Weights::BISHOP_PATTERN_WGHT;
        }

        bishop_mask = (1ULL << square_e7);
        pawn_mask   = (1ULL << square_e6);

        if ( (black_bishops & bishop_mask) &&
             (black_pawns   & pawn_mask) ) {
            return Weights::BISHOP_PATTERN_WGHT;
        }
    }
    return 0;
}



//
// Returns ROOK_CONNECTED_WGHT if any connected rook pair exists
int GameBoard::rook_connectiveness_cp(Color c) const {

    const ull rooks = (c == Color::WHITE) ? white_rooks : black_rooks;

    // Return 0 if the color has fewer than two rooks (0 or 1 rooks).
    if ((rooks == 0) || ((rooks & (rooks - 1)) == 0)) {
        return 0;
    }

    // Occupancy of all pieces (both sides)
    const ull occupancy = get_pieces();
     
    // Collect squares where the friendly side's rooks sit.
    int sqs[8]; // promotion could make more than 2 rooks, but hopefully 8 is plenty? Note: better way to allocate this.
    
    int n = 0;
    ull tmp = rooks;
    while (tmp) {
        assert(n < 8);
        sqs[n] = utility::bit::lsb_and_pop_to_square(tmp); // 0..63
        n++;
    }

    // Check all rook pairs: same rank OR same file, with empty squares between
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            const int s1 = sqs[i], s2 = sqs[j];
            if ((s1 / 8 == s2 / 8) && clear_between_rank(occupancy, s1, s2)) {
                return (Weights::ROOK_CONNECTED_WGHT);
            }
            if ((s1 % 8 == s2 % 8) && clear_between_file(occupancy, s1, s2)) {
                return (Weights::ROOK_CONNECTED_WGHT);
            }
        }
    }
    return 0;
}


int GameBoard::rooks_file_status_cp(Color c, const PawnFileInfo& pawnInfo)
{
    const ull rooks = get_pieces_template<Piece::ROOK>(c);
    if (!rooks) return 0;

    const Color enemy = utility::representation::opposite_color(c);
    const ull enemy_king = get_pieces_template<Piece::KING>(enemy);

    int score_cp = 0;

    for (int file = 0; file < 8; ++file) {

        // Count rooks on this file (col_masks indexed A..H, your file is 0=H..7=A)
        const ull file_mask = col_masks[7 - file];
        const int nRooksOnFile = bits_in(rooks & file_mask);
        if (!nRooksOnFile) continue;

        const bool ownPawnOnFile = (pawnInfo.p[friendlyP].file_count[file] != 0);
        const bool oppPawnOnFile = (pawnInfo.p[enemyP].file_count[file] != 0);

        int file_mult = 0;
        if (!ownPawnOnFile && !oppPawnOnFile)       file_mult = 2;   // open
        else if (!ownPawnOnFile &&  oppPawnOnFile)  file_mult = 1;   // semi-open
        else continue; // not open/semi-open

        // Base open/semi-open file bonus
        score_cp += nRooksOnFile * file_mult * Weights::ROOK_ON_OPEN_FILE;

        // Extra bonus if enemy king is on this file (even if blocked)
        if (enemy_king & file_mask) {
            score_cp += nRooksOnFile * file_mult * Weights::KING_ON_FILE_WGHT;
        }
    }

    return score_cp;
}


//
// Returns in cp.
// Rooks or queens on 7th or 8th ranks
int GameBoard::rook_7th_rankness_cp(Color c)   /* now counts R+Q; +1 each on enemy 7th */
{
    const ull rooks  = get_pieces_template<Piece::ROOK>(c);
    const ull queens = get_pieces_template<Piece::QUEEN>(c);

    ull bigPieces = rooks | queens;    // Dont mutate the bitboards
    if (!bigPieces) return 0;

    const int seventh_rank = (c == Color::WHITE) ? 6 : 1;
    const int eight_rank = (c == Color::WHITE) ? 7 : 0;

    int score_cp = 0;
    while (bigPieces) {
        int s = utility::bit::lsb_and_pop_to_square(bigPieces); // 0..63
        int rnk = (s / 8);
        if (rnk == seventh_rank) score_cp += Weights::MAJOR_ON_RANK7_WGHT;
        if (rnk == eight_rank) score_cp += Weights::MAJOR_ON_RANK8_WGHT;
    }
    return score_cp;
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

std::string GameBoard::sqToString2(int f, int r) const
{
    char file = 'h' - f;   // 0=H ... 7=A
    char rank = '1' + r;   // 0=1 ... 7=8
    return std::string{file, rank};
}

std::string GameBoard::sqToString(int sq) const
{
    return sqToString2(sq % 8, sq / 8);
}



//
// This summarizes where our pawns are by file, without mutating any bitboards.
//
// Inputs:
//   c               Which side (WHITE or BLACK) to summarize.
//
// Outputs:
//   file_count[8]   Number of friendly pawns on each file (0..7 in h1=0 indexing:
//                   f = s % 8 => 0=H ... 7=A).
//   file_bb[8]      Bitboard of friendly pawns restricted to each file.
//                   (file_bb[f] contains all pawns whose square has file index f.)
//   files_present   Bitmask of which files contain >= 1 friendly pawn.
//                   Bit f is set iff file_count[f] > 0.
//
// Return value:
//   true  if the side has at least one pawn
//   false if the side has no pawns (caller can skip pawn-structure work).
//
bool GameBoard::build_pawn_file_summary(Color c, PInfo& pinfo) {

    const ull Pawns = get_pieces_template<Piece::PAWN>(c);
    if (!Pawns) return false;     

    // Initialize structure elements
    for (int i = 0; i < 8; ++i) { pinfo.file_count[i] = 0; pinfo.file_bb[i] = 0ULL; pinfo.advancedSq[i] = -1; }
    pinfo.files_present = 0;
  
    ull tmp = Pawns;
    while (tmp) {
        int s = utility::bit::lsb_and_pop_to_square(tmp);
        int f = s % 8;
        int r = (s / 8);

        ++pinfo.file_count[f];

        pinfo.file_bb[f] |= (1ULL << s);
        pinfo.files_present |= (1u << f);
        
        if (pinfo.advancedSq[f] == -1) {
            pinfo.advancedSq[f] = s;
        } else {
            int prev_r = pinfo.advancedSq[f] / 8;
            bool isMoreAdv = (c == Color::WHITE) ? (r > prev_r) : (r < prev_r);

            if (isMoreAdv) pinfo.advancedSq[f] = s;
        }
    }
    return true;
}

// Returns true if there if there are any "file_pieces" on the same file,
// strictly ahead of `sq` toward promotion for color `c`. Does not include `sq` itself.
// bool GameBoard::any_piece_ahead_on_file(Color c, int sq, ull pieces) const
// {
//     const int f = sq % 8;   // 0..7 (h1=0 => 0=H ... 7=A)
//     const int r = sq / 8;   // 0..7 (White's view)

//     // Same-file mask (using your existing col_masks[] which is indexed A..H)
//     const int fi = 7 - f;
//     const ull file_mask = col_masks[fi];

//     // Ranks ahead toward promotion (strictly ahead)
//     ull ranks_ahead;
//     if (c == Color::WHITE) {
//         const int start_bit = (r + 1) * 8;
//         ranks_ahead = (start_bit >= 64) ? 0ULL : (~0ULL << start_bit);
//     } else {
//         const int end_bit = r * 8;
//         ranks_ahead = (end_bit <= 0) ? 0ULL : ((1ULL << end_bit) - 1ULL);
//     }

//     const ull mask = file_mask & ranks_ahead;
//     return (pieces & mask) != 0ULL;
// }
// Returns true if there is any bit set in `file_pieces` strictly ahead of sq toward promotion.
bool GameBoard::any_piece_ahead_on_file(Color c, int sq, ull file_pieces) const
{
    const int r = sq / 8;

    ull ranks_ahead;
    if (c == Color::WHITE) {
        const int start_bit = (r + 1) * 8;
        ranks_ahead = (start_bit >= 64) ? 0ULL : (~0ULL << start_bit);
    } else {
        const int end_bit = r * 8;
        ranks_ahead = (end_bit <= 0) ? 0ULL : ((1ULL << end_bit) - 1ULL);
    }

    return (file_pieces & ranks_ahead) != 0ULL;
}



//
// Isolated pawns.
//      One count for each instance. Rook pawns can be isolated too.
//      1.5 times as bad for isolated pawns on open files
//
int GameBoard::count_isolated_pawns_cp(Color c, const PawnFileInfo& pawnInfo) const {

    int isolated_cp = 0;
    for (int file = 0; file < 8; ++file) {
        int k = pawnInfo.p[friendlyP].file_count[file];
        if (k == 0) continue;       // no pawns on this file

        bool left  = (file < 7) && (pawnInfo.p[friendlyP].files_present & (1u << (file + 1)));
        bool right = (file > 0) && (pawnInfo.p[friendlyP].files_present & (1u << (file - 1)));

        if (!left && !right) {
            // Friendly pawn is isolated
            int this_cp;

            assert(Weights::ISOLANI_ROOK_WGHT == wghts.GetWeight(ISOLANI_ROOK));
            if ((file==0)||(file==7)) {
                this_cp = (k*Weights::ISOLANI_ROOK_WGHT);   // single->1, doubled->2, tripled->3, etc. on this file
            } else {
                this_cp = (k*Weights::ISOLANI_WGHT);        // single->1, doubled->2, tripled->3, etc. on this file
            }

            if (get_major_pieces(c)) {
                bool is_blocked = any_piece_ahead_on_file(c,
                                                        pawnInfo.p[friendlyP].advancedSq[file],
                                                        pawnInfo.p[enemyP].file_bb[file]);
                if (!is_blocked) this_cp *= (3/2);
            }

            isolated_cp += this_cp;
        }
    }

    return isolated_cp;
}



int GameBoard::count_knights_on_holes_cp(Color c, ull holes_bb) {

    // Friendly knights (h1=0 bitboard)
    ull knights = get_pieces_template<Piece::KNIGHT>(c);
    if (!knights || !holes_bb) return 0;

    // Knights sitting on holes
    ull on_holes = knights & holes_bb;

    // Count them
    int n = bits_in(on_holes);

    return (n * Weights::KNIGHT_HOLE_WGHT);
}


// Finds pawn-structure "holes":
// For each friendly pawn, mark the square directly in front of it as a hole if no friendly pawn can
// attack that square (i.e., there is no friendly pawn on an adjacent file that is on the same rank
// or behind this pawn from our perspective).
//
// Outputs:
//   holes_bb: bitboard (h1=0) of all hole squares found.
// Returns:
//   count of holes (one per pawn that creates such a hole).
int GameBoard::count_pawn_holes_cp(Color c,
                               const PawnFileInfo& pawnInfo,
                               ull& holes_bb) const {
    int holes = 0;
    holes_bb = 0ULL;

    // Friendly pawns (you can replace this with a stored all-pawns-bb later)
    ull Pawns = 0ULL;
    for (int file = 0; file < 8; ++file) Pawns |= pawnInfo.p[friendlyP].file_bb[file];
    if (!Pawns) return 0;

    // Quick reject for “no adjacent files anywhere” checks
    const unsigned files_present = pawnInfo.p[friendlyP].files_present;

    ull tmp = Pawns;

    while (tmp) {
        int s = utility::bit::lsb_and_pop_to_square(tmp);
        int f = s % 8;
        int r = s / 8;

        // Square directly in front of the pawn (ignore pawns already on 7th/2nd edge appropriately)
        int front_sq;
        if (c == Color::WHITE) {
            if (r == 7) continue;            // shouldn't happen for a pawn, but safe
            front_sq = s + 8;
        } else {
            if (r == 0) continue;            // shouldn't happen for a pawn, but safe
            front_sq = s - 8;
        }

        // For the hole test, we need to know:
        // Is there any friendly pawn on an adjacent file that is on the same rank or behind this pawn
        // (from this side's perspective)?
        //
        // Use a "behind_or_same" mask (same as your backward routine).
        ull behind_or_same_mask;
        if (c == Color::WHITE) {
            int end = (r + 1) * 8;                              // ranks 0..r inclusive
            behind_or_same_mask = (end >= 64) ? ~0ULL : ((1ULL << end) - 1ULL);
        } else {
            int start = r * 8;                                   // ranks r..7 inclusive
            behind_or_same_mask = (start <= 0) ? ~0ULL : (~((1ULL << start) - 1ULL));
        }

        // Check adjacent files existence (bitmask) then actual support (bitboard)
        bool can_be_attacked = false;

        if (f > 0 && (files_present & (1u << (f - 1)))) {
            if (pawnInfo.p[friendlyP].file_bb[f - 1] & behind_or_same_mask) can_be_attacked = true;
        }
        if (!can_be_attacked && f < 7 && (files_present & (1u << (f + 1)))) {
            if (pawnInfo.p[friendlyP].file_bb[f + 1] & behind_or_same_mask) can_be_attacked = true;
        }

        if (!can_be_attacked) {

            // I am a hole
            // if (global_debug_flag) {
            //     cout << "hole from pawn " << sqToString(s)
            //         << " -> " << sqToString(front_sq) << "\n";
            // }

            holes_bb |= (1ULL << front_sq);
            holes++;
        }
    }

    return (holes*Weights::PAWN_HOLE_WGHT);
}


int GameBoard::count_doubled_pawns_cp(Color c, const PawnFileInfo& pawnInfo) const {

    int total = 0;

    for (int file = 0; file < 8; ++file) {

        int k = pawnInfo.p[friendlyP].file_count[file];
        if (k < 2) continue;

        int weight = (file == 0 || file == 7) ? Weights::DOUBLED_ROOK_WGHT : Weights::DOUBLED_WGHT;

        // base: penalize only extra pawns
        int extras = (k - 1);
        int this_cp = extras * weight;

        // "open file" for pawn structure: no enemy pawns on this file
        if (pawnInfo.p[enemyP].file_count[file] == 0) {
            this_cp += extras * Weights::DOUBLED_OPEN_FILE_WGHT;
        }

        total += this_cp;
    }

    return total;
}




//
// Passed pawns bonus in centipawns: (from that side's perspective)
//  - 10 cp on 2cnd/3rd rank
//  - 15 cp on 4th
//  - 20 cp on 5th
//  - 25 co on 6th rank
//  - 30 cp on 7th rank
//
// A "passed pawn" has no enemy pawns on its own file or adjacent files on any rank ahead of it (toward promotion).
// A "protected passed pawn" is a passed pawn that is defended by one of our pawns:
// i.e., a friendly pawn sits on a square that attacks this pawn's square (diagonally from behind).
// int GameBoard::count_passed_pawns_cp(Color c, ull& passed_pawns) {
//
// Passed pawns bonus in centipawns.
// A passed pawn has no enemy pawns on its file or adjacent files on any rank ahead (toward promotion).
// Also awards extra bonus if the passed pawn is protected by a friendly pawn.
//
int GameBoard::count_passed_pawns_cp(Color c,
                                    const PawnFileInfo& pawnInfo,
                                    ull& passed_pawns) const {

    passed_pawns = 0ULL;

    // Friendly pawns (already summarized)
    ull my_pawns = 0ULL;
    for (int f = 0; f < 8; ++f) my_pawns |= pawnInfo.p[friendlyP].file_bb[f];
    if (!my_pawns) return 0;            // I have no pawns

    int bonus_cp = 0;
    ull tmp = my_pawns;     // dont mutate the bitboards

    while (tmp) {
        int s = utility::bit::lsb_and_pop_to_square(tmp); // 0..63
        int f = s % 8;                                    // 0..7 (0=H ... 7=A)
        int r = s / 8;                                    // 0..7 (White's view)

        // Enemy pawns on same file and adjacent files
        ull enemy_files = pawnInfo.p[enemyP].file_bb[f];
        if (f > 0) enemy_files |= pawnInfo.p[enemyP].file_bb[f - 1];
        if (f < 7) enemy_files |= pawnInfo.p[enemyP].file_bb[f + 1];

        // Ranks ahead toward promotion (strictly ahead)
        ull ranks_ahead;
        if (c == Color::WHITE) {
            int start_bit = (r + 1) * 8;
            ranks_ahead = (start_bit >= 64) ? 0ULL : (~0ULL << start_bit);
        } else {
            int end_bit = r * 8;
            ranks_ahead = (end_bit <= 0) ? 0ULL : ((1ULL << end_bit) - 1ULL);
        }

        // Passed?
        if ((enemy_files & ranks_ahead) == 0ULL) {

            passed_pawns |= (1ULL << s);

            // Rank, from the color's point of view (example:pawns start at adv=1, queen at adv=7)
            int adv = (c == Color::WHITE) ? r : (7 - r);

            // advancement from the friendly side's perspective
            int base;
            // if      (adv == 6) base = 200;   // 7th rank    (2 pawns)
            // else if (adv == 5) base = 151;   // 6th         (1.5 pawns)
            // else if (adv == 4) base = 82;    // 5th
            // else if (adv == 3) base = 45;    // 4th
            // else if (adv >= 1) base = 25;    // 2nd/3rd
            // else assert(0);           // Cant have pawns on 1st or last rank (adv=0 or adv=7)
            assert ((adv>0) && (adv<7));  // Cant have pawns on 1st or 8th rank (adv=0 or adv=7)
            base = Weights::PASSED_PAWN_SLOPE_WGHT*adv*adv + Weights::PASSED_PAWN_YINRCPT_WGHT;


            // Protected passed pawn?
            // A friendly pawn protects this pawn if it sits on a square that attacks (s).
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

            if ((my_pawns & protect_mask) != 0ULL) {
                // This passed pawn is protected
                base += 100;            // a whole pawn
               // base = base*4/3;    
            }

            bonus_cp += base;       // base value for passed pawn

        }       // END pawn is passed
    }       // END loop over all friendly pawns

    return bonus_cp;
}





//
// Returns smaller values near the center. 0 for the inner ring (center) 3 for outer ring (edge squares)
int GameBoard::king_edgeness_cp(Color color)
{
    int edgeCode;

    //ull kbb = (color == Color::WHITE) ? white_king : black_king;   // dont mutate the bitboards
    ull kbb = get_pieces_template<Piece::KING>(color);   // dont mutate the bitboard

    assert(kbb != 0ULL);
    edgeCode = piece_edgeness(kbb);    // 0 = center, 3 = edge

    return (edgeCode*Weights::KING_EDGE_WGHT);
}

// 0 = center, 3 = edge
int GameBoard::piece_edgeness(ull kbb) {   
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

//
// Returns true if the given color's queen is on one of the 4 center squares:
// d4, e4, d5, e5  (i.e., (3,3), (4,3), (3,4), (4,4) in file/rank 0..7).
//
// NOTE: If there is no queen (bitboard == 0), returns false.
// NOTE: If there are multiple queens (promotion), returns true if ANY queen is on a center square.
//
int GameBoard::queenOnCenterSquare_cp(Color c)
{
    ull q;
    // If you have a get_pieces_template<QUEEN>(c) that you trust, use it.
    //q = get_pieces_template<Piece::QUEEN>(c);
    q = (c == ShumiChess::WHITE) ? white_queens : black_queens;
    if (q == 0ULL) return 0;

    int itemp = 0;

    // "Occupation" not attack: is the queen actually sitting on the square?
    itemp += ((q & (1ULL << square_e4)) != 0ULL);
    itemp += ((q & (1ULL << square_d4)) != 0ULL);
    itemp += ((q & (1ULL << square_e5)) != 0ULL);
    itemp += ((q & (1ULL << square_d5)) != 0ULL);

    // At most 1 (assumming one queen)
    return itemp * Weights::QUEEN_OUT_EARLY_WGHT;
}

// Penalize moving the f-pawn in the opening.
// This is a small "king-safety / loosened diagonals" heuristic: f2/f7 is a sensitive pawn.
// Returns 0 if the f-pawn is still on its starting square; otherwise returns a small negative score.
int GameBoard::moved_f_pawn_early_cp(Color c) const
{
    // If you prefer, swap this for your get_pieces_template<Piece::PAWN>(c)
    ull p = (c == ShumiChess::WHITE) ? white_pawns : black_pawns;
    if (p == 0ULL) return 0;

    // Starting-square indices MUST match your engine's indexing.
    // If you already have these as constants, use them directly.

    const int startSq = (c == ShumiChess::WHITE) ? square_f2 : square_f7;

    int itemp = 0;

    // If the pawn is NOT on its starting square, it has moved (or been captured).
    // (We are intentionally penalizing both: moving it or losing it early are both usually bad in the opening.)
    itemp += ((p & (1ULL << startSq)) == 0ULL);

    return itemp * Weights::F_PAWN_MOVED_EARLY_WGHT;
}




// Used only in CRAZY_IVAN
// Piece occuptation squares scaled for closeness to center.
int GameBoard::center_closeness_bonus(Color c) {

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
        int ring = piece_edgeness(one);    // 0=center .. 3=edge
        int centerness = 3 - ring;            // 3=center .. 0=edge
        if (centerness < 0) centerness = 0;
        bonus += (Weights::CENTER_OCCUPY_PIECES_WGHT * centerness) / 3;
    }

    // Bishops (divide by 3)
    tmp = bBB;
    while (tmp) {
        int sq = utility::bit::lsb_and_pop_to_square(tmp);
        ull one = 1ULL << sq;
        int ring = piece_edgeness(one);
        int centerness = 3 - ring;
        if (centerness < 0) centerness = 0;
        bonus += (Weights::CENTER_OCCUPY_PIECES_WGHT * centerness) / 3;
    }

    // Rooks (divide by 5)
    tmp = rBB;
    while (tmp) {
        int sq = utility::bit::lsb_and_pop_to_square(tmp);
        ull one = 1ULL << sq;
        int ring = piece_edgeness(one);
        int centerness = 3 - ring;
        if (centerness < 0) centerness = 0;
        bonus += (Weights::CENTER_OCCUPY_PIECES_WGHT * centerness) / 5;
    }

    // Queens (divide by 9)
    tmp = qBB;
    while (tmp) {
        int sq = utility::bit::lsb_and_pop_to_square(tmp);
        ull one = 1ULL << sq;
        int ring = piece_edgeness(one);
        int centerness = 3 - ring;
        if (centerness < 0) centerness = 0;
        bonus += (Weights::CENTER_OCCUPY_PIECES_WGHT * centerness) / 9;
    }

    return bonus;
}


// Fills an array of up to 9 squares around the king (including the king square).
// Returns how many valid squares were written.
// king_near_squares_out[i] are square indices 0..63.
int GameBoard::get_king_near_squares(Color defender_color, int king_near_squares_out[9])
{
    int count = 0;

    // find king square for defender_color
    //ull kbb = (defender_color == Color::WHITE) ? white_king : black_king;
    ull kbb = get_pieces_template<Piece::KING>(defender_color);


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
// Manhattan distance  (D(x,y)=|x1-x2| + |y1-y2|)
// Varys from 0 to 14 inclusive.
// Also known as "taxicab distance". A cheap distance with integers, that avoids sqrt()
//
int GameBoard::get_Manhattan_distance(int x1, int y1, int x2, int y2) {

    int dx = x1 > x2 ? x1 - x2 : x2 - x1;
    int dy = y1 > y2 ? y1 - y2 : y2 - y1;
    int iDist = dx + dy;
    return iDist;
}
//
// Chebyshev distance  (D(x,y)=\max (|x_{1}-x_{2}|,|y_{1}-y_{2}|)\)
// Varys from 0 to 7 inclusive.
// It is also known as "chessboard distance".  A cheap distance with integers, that avoids sqrt()
//
//
int GameBoard::get_Chebyshev_distance(int x1, int y1, int x2, int y2) {

    int dx = x1 > x2 ? x1 - x2 : x2 - x1;
    int dy = y1 > y2 ? y1 - y2 : y2 - y1;
    int iDist = (dx > dy) ? dx : dy;
    return iDist;
}
//
// Most accurate, uses a table. Still to slow.
// Goes from 0.0 to 9.899 inclusive
//
double GameBoard::get_board_distance(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2; if (dx < 0) dx = -dx;
    int dy = y1 - y2; if (dy < 0) dy = -dy;
    assert(dx>=0);
    assert(dy>=0);
    assert(dx<=7);
    assert(dy<=7);
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
    assert (N>=1);
    return table[N-1].v;
}

//
// Fast Euclidean distance using a lookup table.
// Returns approx 100*sqrt(dx*dx + dy*dy) in [0..990].  No doubles.  No loops.
//
// int GameBoard::get_board_distance_100(int x1, int y1, int x2, int y2) const
// {
//     int dx = x1 - x2; if (dx < 0) dx = -dx;
//     int dy = y1 - y2; if (dy < 0) dy = -dy;

//     const int d2 = dx*dx + dy*dy;   // 0..98
//     assert(d2 >= 0 && d2 <= 98);

//     // round(100*sqrt(d2)) for d2=0..98
//     static const unsigned short dist100[99] = {
//           0, 100, 141, 173, 200, 224, 245, 265, 283, 300,
//         316, 332, 346, 361, 374, 387, 400, 412, 424, 436,
//         447, 458, 469, 479, 490, 500, 510, 520, 529, 539,
//         548, 557, 566, 574, 583, 592, 600, 608, 616, 624,
//         632, 640, 648, 656, 663, 671, 678, 686, 693, 700,
//         707, 714, 721, 728, 735, 742, 748, 755, 762, 768,
//         775, 781, 787, 794, 800, 806, 812, 819, 825, 831,
//         837, 843, 849, 854, 860, 866, 872, 877, 883, 889,
//         894, 900, 906, 911, 917, 922, 927, 933, 938, 943,
//         949, 954, 959, 964, 970, 975, 980, 985, 990
//     };

//     return (int)dist100[d2];
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

    // Method 2 Chebyshev. This coorasponds to the number of king moves to the square on an empty board,
    //#define MAX_DIST 14
    // iFakeDist = get_Chebyshev_distance(xEnemy, yEnemy, xFrien, yFrien);
    // dfakeDist = (double)iFakeDist;

    // Method 3. Most accurate distance (table driven)
    //#define MAX_DIST 10
    dfakeDist = get_board_distance(xEnemy, yEnemy, xFrien, yFrien);
    
    //dfakeDist = (double)xFrien;     // debug only

    assert (dfakeDist >=  0.0);
    assert (dfakeDist <= MAX_DIST);
   
    return dfakeDist;
}


// Color has king only, or king with a 1/2 minor pieces.
// So discounting pawns, basically it means no more than "4 points" in material.
bool GameBoard::hasNoMajorPieces(Color attacker_color) {
    ull enemyBigPieces;
    ull enemySmallPieces;
    if (attacker_color == ShumiChess:: BLACK) {
        enemyBigPieces = (black_rooks | black_queens);
        enemySmallPieces = (black_knights | black_bishops);
    } else {
        enemyBigPieces = (white_rooks | white_queens);  
        enemySmallPieces = (white_knights | white_bishops);
    }
    if (enemyBigPieces) return false;
    if (bits_in(enemySmallPieces) > 2) return false;
    return true;
}


bool GameBoard::is_king_highest_piece() {
    if (white_queens || black_queens) return false;
    if (white_rooks || black_rooks) return false;
    return true;

}


// Corners are twice as bad as the edges.
int GameBoard::is_knight_on_edge_cp(Color color) {

    int pointsOff=0;
    ull knghts; // go ahead mutate me, I'm local
    knghts = get_pieces_template<Piece::KNIGHT>(color);
    if (!knghts) return 0;
    
    while (knghts) {
        int s  = utility::bit::lsb_and_pop_to_square(knghts); // 0..63  
        const int f = s % 8;            // h1=0 → 0=H ... 7=A
        const int r = s / 8;            // rank index 0..7 (White's view)
        if ((f==0) || (f==7)) pointsOff += Weights::KNIGHT_ON_EDGE_WGHT;      // I am on a or h file
        if ((r==0) || (r==7)) pointsOff += Weights::KNIGHT_ON_EDGE_WGHT;      // I am on eight or first rank
    }
    return pointsOff;
}


// -----------------------------------------------------------------------------
// Opening development bonus.
// "Development" here means ONLY: minor pieces (knights + bishops) moved off
// their original squares.  (No queen/king/rooks: those lead to silly moves.)
// This is intentionally a small, background nudge and is applied ONLY in the opening.
// -----------------------------------------------------------------------------
int GameBoard::development_opening_cp(Color c) {
    // Very conservative "opening" gate:
    // If queens are gone, or lots of pawns are gone, don't apply this.
    const ull wq = get_pieces(Color::WHITE, Piece::QUEEN);
    const ull bq = get_pieces(Color::BLACK, Piece::QUEEN);
    if (!wq || !bq) return 0;

    // const ull wp = get_pieces(Color::WHITE, Piece::PAWN);
    // const ull bp = get_pieces(Color::BLACK, Piece::PAWN);
    // const int pawnCount = bits_in(wp) + bits_in(bp);
    // if (pawnCount < 12) return 0;   // opening likely over

    // Starting squares for minors
    ull start_knights = 0;
    ull start_bishops = 0;

    if (c == Color::WHITE) {
        start_knights = (1ULL << square_b1) | (1ULL << square_g1);
        start_bishops = (1ULL << square_c1) | (1ULL << square_f1);
    } else {
        start_knights = (1ULL << square_b8) | (1ULL << square_g8);
        start_bishops = (1ULL << square_c8) | (1ULL << square_f8);
    }

    const ull my_knights = get_pieces(c, Piece::KNIGHT);
    const ull my_bishops = get_pieces(c, Piece::BISHOP);

    // Count how many minors are NOT on their original squares.
    // (If a piece was captured, it simply won't be counted as "developed".)
    const int knights_on_start = bits_in(my_knights & start_knights);
    const int bishops_on_start = bits_in(my_bishops & start_bishops);

    const int knights_total = bits_in(my_knights);
    const int bishops_total = bits_in(my_bishops);

    const int developed = (knights_total - knights_on_start) + (bishops_total - bishops_on_start);
    if (developed <= 0) return 0;

    return developed * Weights::DEVELOPMENT_OPENING;
}





double GameBoard::kings_close_toegather_cp(Color attacker_color) {

    // Returns 2 to MAX_DIST inclusive. 2 if in opposition, MAX_DIST if in opposite corners.
    double dkk = kings_far_apart(attacker_color);

    double dFarness = MAX_DIST - dkk;   // 8 if in opposition, 0, if in opposite corners
    assert (dFarness>=0.0);
    return (int)(dFarness * Weights::KINGS_CLOSE_TOGETHER_WGHT);
    
}


// Returns 2 to MAX_DIST inclusive. 2 if in opposition, MAX_DIST if in opposite corners.
double GameBoard::kings_far_apart(Color attacker_color) {
  
    double dBonus = 0.0;

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
        assert (dFakeDist >=  2.0);      // Kings cant touch each other
        assert (dFakeDist <= MAX_DIST);

        // Bonus higher if friendly king closer to enemy king
        dBonus = dFakeDist;

        // Bonus debugs
        //dBonus = (double)frienKingSq;               // enemy king is attracted to a8
        //dBonus = 63.0 - (double)(frienKingSq);    // enemy king is attracted to h1

        return dBonus;

    }

    return dBonus;
}


//
// Minimum Manhattan distance from the friendly king to the 2×2 center box
// center squares: (3,3),(4,3),(3,4),(4,4)  (i.e., d4,e4,d5,e5 in normal coords)
//
// Returns 0..6 on an 8×8 board.
//
int GameBoard::king_center_manhattan_dist(Color c)
{
    int iReturn;
    assert(white_king != 0ULL);
    assert(black_king != 0ULL);

    ull kbb = (c == ShumiChess::WHITE) ? white_king : black_king;

    // king square 0..63 (non-mutating if you have a non-pop lsb routine; otherwise copy is fine)
    int ks = utility::bit::lsb_and_pop_to_square(kbb);

    int f = ks % 8;
    int r = ks / 8;

    int dx1 = f - 3; if (dx1 < 0) dx1 = -dx1;
    int dx2 = f - 4; if (dx2 < 0) dx2 = -dx2;
    int dy1 = r - 3; if (dy1 < 0) dy1 = -dy1;
    int dy2 = r - 4; if (dy2 < 0) dy2 = -dy2;

    int dx = (dx1 < dx2) ? dx1 : dx2;
    int dy = (dy1 < dy2) ? dy1 : dy2;

    iReturn = dx + dy;
    assert(iReturn <= 6);

    return iReturn;
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

// Attackers are NOT kings. Otherwise everybody else.
int GameBoard::attackers_on_enemy_king_near_cp(Color attacker_color)
{

    // enemy (the one whose king we are surrounding)
    Color defender_color = (attacker_color == Color::WHITE) ? Color::BLACK : Color::WHITE;

    // grab up to 9 squares around defender's king
    int king_near_squares[9];
    int count = get_king_near_squares(defender_color, king_near_squares);

    int total = 0;

    for (int i = 0; i < count; ++i) {

        int sq = king_near_squares[i];
        // sliders are queens, bishops, and rooks
        total += sliders_and_knights_attacking_square(attacker_color, sq);

        total += pawns_attacking_square(attacker_color,  sq);

    }

    return (total*Weights::ATTACKERS_ON_KING_WGHT);
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

static bool is_king_move_away(const std::string &a, const std::string &b)
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

///////////////////////////////////////////////////////////////
//
// Returns a FEN. White has a lone king. Black has king + (queen or rook). No other pieces or pawns.
// Randomly chooses the 3 piece squares, with these constraints:
//
//   1) Kings are not adjacent (kings do not attack each other).
//   2) The black heavy piece is not giving check to the white king
//      (note: attack test may be conservative if king-blocking isn't modeled).
//   3) Side to move is always White (" w - - 0 1").
//
// Note: This does NOT does not try to ensure the position is reachable from the initial position.
//
///////////////////////////////////////////////////////////////

std::string GameBoard::random_kqk_fen(bool doQueen) {

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
        if (sq != wk && !is_king_move_away(wk, sq)) {
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
        // Rule: kings cant be on the same square.
        if (sq == wk || sq == bk) continue;

        // New rule: White (lone king) must NOT be able to capture the black major piece immediately.
        if (is_king_move_away(wk, sq)) continue;

        bool attacks = doQueen ? queen_attacks(sq, wk) : rook_attacks(sq, wk);

        if (!attacks)
            bX_choices.push_back(sq);
    }
    if (bX_choices.empty()) goto retry_all;

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

    // 7) make FEN ranks
    std::string fen;
    for (int row = 0; row < 8; ++row) {
        int empty = 0;
        for (int col = 0; col < 8; ++col) {
            char c = board[row][col];
            if (c == 0) {
                ++empty;
            } else {
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

    // Indicates White to move, no castle rights, and first move, 
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
int GameBoard::SEE_for_capture(Color side, const Move &mv, FILE* fpDebug)
{
    // from and to are BITBOARDS (ull) with exactly one bit set.
    ull from_bb = mv.from;
    ull to_bb   = mv.to;
    assert (bits_in(from_bb) == 1);
    assert (bits_in(to_bb) == 1);

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

    // Identify the victim on 'to_sq'
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
        if (c == Color::WHITE){
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
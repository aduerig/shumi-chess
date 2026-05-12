
#include <math.h>
#include <vector>

#include <random>
#include <chrono>  


#include "gameboard.hpp"
#include "weights.hpp"
#if defined(_MSC_VER)
#include <intrin.h>   // _BitScanForward64 / _BitScanReverse64
#endif

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
 
    // In this constructer, the bitboards are the input
    //#include "gameboardSetup.hpp"
      //
    // Initial game setup
    //
    // Comment on bitboards, as used here. You got a "h1=0" (right-to-left) file mapping.
    // More verbosely: your bitboards are indexed so that
    // bit 0 corresponds to h1, and within a rank the file index runs h1 to a1 as the square number increases,
    // while the rank index is standard: 0 is rank 1,  rank 8.
    // In chess-programming lingo, you can think of it as: files are mirrored relative to the common "A1 = 0" layout. 
    // So your rank math was fine; only the file needed the file = 7 - (sq & 7) mirror.
    // 
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Initial positions of the pieces (standard setup)
    black_pawns  (0b00000000'11111111'00000000'00000000'00000000'00000000'00000000'00000000),
    white_pawns  (0b00000000'00000000'00000000'00000000'00000000'00000000'11111111'00000000),
    black_rooks  (0b10000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_rooks  (0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000001),
    black_knights(0b01000010'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_knights(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'01000010),
    black_bishops(0b00100100'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_bishops(0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00100100),
    black_queens (0b00010000'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_queens (0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00010000),
    black_king   (0b00001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000),
    white_king   (0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001000),   
    turn(WHITE),
    black_castle_rights(CASTLE_EITHER),
    white_castle_rights(CASTLE_EITHER),
    en_passant_landing_bb(1),               // The square where the capturing pawn would land in an en-passant capture
    halfmove(0),
    fullmove(1) 

    {

        initGameBoard();                // Code common to both constructers

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
            this->black_castle_rights |= CASTLE_KING;
            break;
        case 'q':
            this->black_castle_rights |= CASTLE_QUEEN;
            break;
        case 'K':
            this->white_castle_rights |= CASTLE_KING;
            break;
        case 'Q':
            this->white_castle_rights |= CASTLE_QUEEN; 
            break;
        default:
            // std::cout << "Unexpected castling rights token: " << token << std::endl;
            // assert(0);  // Note: this fires in some test, is it ok? What is the default case?
            break;
        }
    }

    // "this" here is a GameBoard.
    if (fen_components[3] != "-") { 
        this->en_passant_landing_bb = utility::representation::acn_to_bitboard_conversion(fen_components[3]);
    }

    // halfmove is used to apply the "fifty-move draw" rule in chess
    this->halfmove = std::stoi(fen_components[4]);  

    // fullmove is Used only for display purposes.
    this->fullmove = std::stoi(fen_components[5]);

    this->turn = fen_components[1] == "w" ? ShumiChess::WHITE : ShumiChess::BLACK;

    initGameBoard();                // Code common to both constructers

}

 void GameBoard::initGameBoard(void)   // Code common to both constructers 
 {

    // No multiple pieces on the same square.
    bool no_pieces_on_same_square = are_bit_boards_valid();
    assert(no_pieces_on_same_square);

    // Seed randomization, for gameboard. (using microseconds since ?)
    using namespace std::chrono;
    auto now = high_resolution_clock::now().time_since_epoch();
    auto us  = duration_cast<microseconds>(now).count();
    rng.seed(static_cast<unsigned>(us));

    // !TODO doesn't belong here i don't think. I think it does.
    ShumiChess::initialize_zobrist();
    set_zobrist();
    
    // Fills out the "chessboard" like view of the board
    bitboards_to_pieces_on_square();

    init_castle_touch_tables();

    wghts.multiply_weights(VOLUME_CONTROL);


    compute_bits_in();

}

void GameBoard::set_zobrist() {
    zobrist_key = 0;
    pawn_zobrist_key = 0;

    for (int color_int = 0; color_int < 2; color_int++) {
        Color color = static_cast<Color>(color_int);

        for (int j = 0; j < 6; j++) {
            Piece piece_type = static_cast<Piece>(j);
            ull bitboard = get_pieces(color, piece_type);

            while (bitboard) {
                Square square = utility::bit::lsb_and_pop_to_square(bitboard);
                int zob_index = piece_type + color * 6;

                zobrist_key ^= zobrist_piece_square[zob_index][square];

                if (piece_type == Piece::PAWN) {
                    pawn_zobrist_key ^= zobrist_piece_square[zob_index][square];
                }
            }
        }
    }

    if (turn == Color::BLACK) {
        zobrist_key ^= zobrist_side;
    }
}
//
// fields for fen are:
// piece placement, current colors turn, castling avaliablity, enpassant, halfmove number (fifty move rule), total moves 
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
    if (CASTLE_KING & white_castle_rights) {
        castlestuff += 'K';
    }
    if (CASTLE_QUEEN & white_castle_rights) {
        castlestuff += 'Q';
    }
    if (CASTLE_KING & black_castle_rights) {
        castlestuff += 'k';
    }
    if (CASTLE_QUEEN & black_castle_rights) {
        castlestuff += 'q';
    }
    if (castlestuff.empty()) {
        castlestuff = "-";
    }
    fen_components.push_back(castlestuff);

    // TODO: enpassant
    string enpassant_info = "-";
    if (en_passant_landing_bb != 0) {
        enpassant_info = utility::representation::bitboard_to_acn_conversion(en_passant_landing_bb);
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

Color GameBoard::get_color_on_bitboard(ull bitboard) {
    if (get_pieces(Color::WHITE) & bitboard) {
        return Color::WHITE;
    } else {
        return Color::BLACK;
    }
}

//
// Translate the bitboards to a "chessboard" like "pieces_on_square"
void GameBoard::bitboards_to_pieces_on_square(void)
{
    for (Square sq = 0; sq < 64; ++sq) {
        pieces_on_square[sq] = Piece::NONE;
    }

    ull bb;

    bb = white_pawns;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::PAWN;
    }

    bb = black_pawns;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::PAWN;
    }

    bb = white_rooks;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::ROOK;
    }

    bb = black_rooks;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::ROOK;
    }

    bb = white_knights;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::KNIGHT;
    }

    bb = black_knights;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::KNIGHT;
    }

    bb = white_bishops;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::BISHOP;
    }

    bb = black_bishops;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::BISHOP;
    }

    bb = white_queens;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::QUEEN;
    }

    bb = black_queens;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::QUEEN;
    }

    bb = white_king;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::KING;
    }

    bb = black_king;
    while (bb) {
        Square sq = utility::bit::lsb_and_pop_to_square(bb);
        pieces_on_square[sq] = Piece::KING;
    }
}
//
// Update pieces_on_square[] for a move being pushed onto the board.
// Assumes pieces_on_square[] currently reflects the position BEFORE the move.
void GameBoard::push_move_to_pieces_on_square(const Move& move)
{
    assert(move.fromSQ != NO_SQUARE);
    assert(move.toSQ   != NO_SQUARE);

    // Sanity: from-square should contain the moving piece before the move.
    assert(pieces_on_square[move.fromSQ] == move.piece_type);

    // Remove moving piece from source square.
    pieces_on_square[move.fromSQ] = Piece::NONE;

    // En passant removes the pawn behind the destination square.
    if (move.is_en_passent_capture) {
        Square capSQ;
        if (move.color == Color::WHITE) {
            capSQ = move.toSQ - 8;
        } else {
            capSQ = move.toSQ + 8;
        }

        assert(move.capture == Piece::PAWN);
        assert(pieces_on_square[capSQ] == Piece::PAWN);
        pieces_on_square[capSQ] = Piece::NONE;
    }

    // Castling also moves a rook.
    if (move.is_castle_move) {
        assert(move.piece_type == Piece::KING);
        assert(move.capture == Piece::NONE);

        if (move.toSQ == move.fromSQ - 2) {
            // King-side castle in h1=0 system.
            Square rook_from = move.fromSQ - 3;
            Square rook_to   = move.fromSQ - 1;

            assert(pieces_on_square[rook_from] == Piece::ROOK);
            pieces_on_square[rook_from] = Piece::NONE;
            pieces_on_square[rook_to]   = Piece::ROOK;
        } else if (move.toSQ == move.fromSQ + 2) {
            // Queen-side castle in h1=0 system.
            Square rook_from = move.fromSQ + 4;
            Square rook_to   = move.fromSQ + 1;

            assert(pieces_on_square[rook_from] == Piece::ROOK);
            pieces_on_square[rook_from] = Piece::NONE;
            pieces_on_square[rook_to]   = Piece::ROOK;
        } else {
            assert(0);
        }
    }

    // Place the moving piece (or promoted piece) on destination.
    if (move.promotion != Piece::NONE) {
        assert(move.piece_type == Piece::PAWN);
        pieces_on_square[move.toSQ] = move.promotion;
    } else {
        pieces_on_square[move.toSQ] = move.piece_type;
    }
}

// Restore pieces_on_square[] for a move being popped from the board.
// Assumes pieces_on_square[] currently reflects the position AFTER the move.
void GameBoard::pop_move_to_pieces_on_square(const Move& move)
{
    assert(move.fromSQ != NO_SQUARE);
    assert(move.toSQ   != NO_SQUARE);

    // Undo castling rook move first.
    if (move.is_castle_move) {
        assert(move.piece_type == Piece::KING);
        assert(move.capture == Piece::NONE);

        if (move.toSQ == move.fromSQ - 2) {
            // King-side castle in h1=0 system.
            Square rook_from = move.fromSQ - 3;
            Square rook_to   = move.fromSQ - 1;

            assert(pieces_on_square[rook_to] == Piece::ROOK);
            pieces_on_square[rook_to]   = Piece::NONE;
            pieces_on_square[rook_from] = Piece::ROOK;
        } else if (move.toSQ == move.fromSQ + 2) {
            // Queen-side castle in h1=0 system.
            Square rook_from = move.fromSQ + 4;
            Square rook_to   = move.fromSQ + 1;

            assert(pieces_on_square[rook_to] == Piece::ROOK);
            pieces_on_square[rook_to]   = Piece::NONE;
            pieces_on_square[rook_from] = Piece::ROOK;
        } else {
            assert(0);
        }
    }

    // Undo destination square.
    if (move.is_en_passent_capture) {
        Square capSQ;
        if (move.color == Color::WHITE) {
            capSQ = move.toSQ - 8;
        } else {
            capSQ = move.toSQ + 8;
        }

        // Destination square becomes empty again.
        pieces_on_square[move.toSQ] = Piece::NONE;

        // Restore captured pawn behind destination.
        assert(move.capture == Piece::PAWN);
        pieces_on_square[capSQ] = Piece::PAWN;
    } else if (move.capture != Piece::NONE) {
        // Restore captured piece on destination square.
        pieces_on_square[move.toSQ] = move.capture;
    } else {
        // Quiet move / castle / non-capturing promotion.
        pieces_on_square[move.toSQ] = Piece::NONE;
    }

    // Restore moving piece to source square.
    pieces_on_square[move.fromSQ] = move.piece_type;
}


void GameBoard::init_castle_touch_tables()
{
    for (int sq = 0; sq < 64; sq++) {
        ull bb = (1ULL << sq);

        white_castle_touch[sq] = CASTLE_EITHER;
        if (bb & W_KSIDE_MASK) white_castle_touch[sq] &= CASTLE_KING;
        if (bb & W_QSIDE_MASK) white_castle_touch[sq] &= CASTLE_QUEEN;

        black_castle_touch[sq] = CASTLE_EITHER;
        if (bb & B_KSIDE_MASK) black_castle_touch[sq] &= CASTLE_KING;
        if (bb & B_QSIDE_MASK) black_castle_touch[sq] &= CASTLE_QUEEN;
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
// “lerp” stands for Linear intERPolation.
// Linear interpolation: t=0 → a, t=1 → b
inline double lerp(double a, double b, double t) { return a + (b - a) * t; }


int GameBoard::queen_still_home(Color color)
{

    if (color == Color::WHITE)
    {
        // Is there still a white queen on d1?
        ull mask = (1ULL << square_d1);

        // Return 1 if queen hasn't moved (still on d1),
        // 0 if it has moved off d1.
        return (white_queens & mask) ? 1 : 0;
    }
    else // BLACK
    {
        // Is there still a black queen on d8?
        ull mask = (1ULL << square_d8);

        return (black_queens & mask) ? 1 : 0;
    }
}












    
// Return true if all squares strictly between a and b on the same rank are empty.
static inline bool clear_between_rank(ull occupancy, int a, int b)
{
    const int ra = a >> 3;
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
    int ra = a >> 3, rb = b >> 3;
    if (ra > rb) std::swap(ra, rb);

    for (int r = ra + 1; r < rb; ++r) {
        const int sq = r * 8 + fa;
        if (occupancy & (1ULL << sq)) return false;
    }
    return true;
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
    int n_knightsW = Bits_In[ShumiChess::WHITE][ShumiChess::KNIGHT];
    int n_knightsB = Bits_In[ShumiChess::BLACK][ShumiChess::KNIGHT]; 

    int n_bishopsW = Bits_In[ShumiChess::WHITE][ShumiChess::BISHOP];
    int n_bishopsB = Bits_In[ShumiChess::BLACK][ShumiChess::BISHOP]; 

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

std::string GameBoard::sqToString(int f, int r) const
{
    char file = 'h' - f;   // 0=H ... 7=A
    char rank = '1' + r;   // 0=1 ... 7=8
    return std::string{file, rank};
}


// ---------- build_pawn_file_summary_t ----------
//
// This summarizes where our pawns are by file, without mutating any bitboards.
//
// Inputs:
//   c               Which side (WHITE or BLACK) to summarize.
//
// Outputs:
//   file_count[8]   Number of friendly pawns on each file (0..7 in h1=0 indexing:
//                   f = s % 8 => 0=H ... 7=A).
//   files_present   Bitmask of which files contain >= 1 friendly pawn.
//                   Bit f is set iff file_count[f] > 0.
//   advancedSq[8]   the square index of the side’s most advanced pawn on that file (or NO_SQUARE)
//
// Return value:
//   true  if the side has at least one pawn
//   false if the side has no pawns (caller can skip pawn-structure work).
//
template<Color c> bool GameBoard::build_pawn_file_summary_t(PInfo& pinfo)
{
    for (int i = 0; i < 8; ++i) {
        pinfo.file_count[i] = 0;
        pinfo.advancedSq[i] = NO_SQUARE;
        pinfo.rearSq[i] = NO_SQUARE;
    }

    pinfo.files_present = 0;
    pinfo.guard_files_23 = 0;

    const ull Pawns = get_pieces_template<Piece::PAWN, c>();
    if (!Pawns) return false;

    ull goodRanksMask;
    if constexpr (c == Color::WHITE) goodRanksMask = row_masks[ROW_2] | row_masks[ROW_3];
    else goodRanksMask = row_masks[ROW_7] | row_masks[ROW_6];

    for (int f = 0; f < 8; ++f) {
        ull bb = (Pawns & col_masksHA[f]);

        if (bb & goodRanksMask) {
            pinfo.guard_files_23 |= (uint8_t)(1u << f);
        }

        pinfo.file_count[f] = bits_in(bb);

        if (!bb) continue;

        pinfo.files_present |= (1u << f);

        if constexpr (c == Color::WHITE) {
            pinfo.advancedSq[f] = utility::bit::bitboard_to_highest_square_fast(bb);
            pinfo.rearSq[f]     = utility::bit::bitboard_to_lowest_square_fast(bb);
        } else {
            pinfo.advancedSq[f] = utility::bit::bitboard_to_lowest_square_fast(bb);
            pinfo.rearSq[f]     = utility::bit::bitboard_to_highest_square_fast(bb);
        }
    }

    return true;
}



void GameBoard::dump_pinfo_mismatch(const PInfo& a, const PInfo& b)
{
    if (a.files_present != b.files_present) {
        printf("PInfo mismatch: files_present a=0x%X b=0x%X\n", a.files_present, b.files_present);
        //return;
    }

    for (int i = 0; i < 8; ++i) {
        if (a.file_count[i] != b.file_count[i]) {
            printf("PInfo mismatch: file_count[%d] a=%d b=%d\n", i, a.file_count[i], b.file_count[i]);
            //return;
        }
        if (a.advancedSq[i] != b.advancedSq[i]) {
            printf("PInfo mismatch: advancedSq[%d] a=%d b=%d\n", i, a.advancedSq[i], b.advancedSq[i]);
            //return;
        }
        if (a.rearSq[i] != b.rearSq[i]) {
            printf("PInfo mismatch: rearSq[%d] a=%d b=%d\n", i, a.rearSq[i], b.rearSq[i]);
            //return;
        }
    }

    //printf("PInfo mismatch: (unexpected) no field differed\n");
}


void GameBoard::build_pawn_summaries(PawnFileInfo& pawnFileInfo)
{
    build_pawn_file_summary_t<Color::WHITE>(pawnFileInfo.p[Color::WHITE]);
    build_pawn_file_summary_t<Color::BLACK>(pawnFileInfo.p[Color::BLACK]);
}

//
//  for incremental construction of pawn summaries.
template<Color c>
void GameBoard::refresh_pawn_summary_file_t(PInfo& pinfo, int file)
{
    assert(file >= 0);
    assert(file < 8);

    const ull pawns = get_pieces_template<Piece::PAWN, c>();
    const ull bb = pawns & col_masksHA[file];

    pinfo.file_count[file] = bits_in(bb);

    if (bb) {
        pinfo.files_present |= (1u << file);

        if constexpr (c == Color::WHITE) {
            pinfo.advancedSq[file] = utility::bit::bitboard_to_highest_square_fast(bb);
            pinfo.rearSq[file]     = utility::bit::bitboard_to_lowest_square_fast(bb);
        } else {
            pinfo.advancedSq[file] = utility::bit::bitboard_to_lowest_square_fast(bb);
            pinfo.rearSq[file]     = utility::bit::bitboard_to_highest_square_fast(bb);
        }
    } else {
        pinfo.files_present &= ~(1u << file);
        pinfo.advancedSq[file] = NO_SQUARE;
        pinfo.rearSq[file] = NO_SQUARE;
    }
}

template<Color c> 
void GameBoard::refresh_pawn_summary_files_t(PInfo& pinfo, const bool touched[8])
{
    for (int file = 0; file < 8; file++) {
        if (touched[file]) {
            refresh_pawn_summary_file_t<c>(pinfo, file);
        }
    }
}


void GameBoard::refresh_pawn_summaries_after_move(const Move& move,
                                                  PInfo& whitePInfo,
                                                  PInfo& blackPInfo)
{
    bool white_touched[8] = { false,false,false,false,false,false,false,false };
    bool black_touched[8] = { false,false,false,false,false,false,false,false };

    const int from_file = (move.fromSQ & 7);
    const int to_file   = (move.toSQ & 7);

    // ------------------------------------------------------------
    // Moving pawn affects its own pawn summary
    // ------------------------------------------------------------
    if (move.piece_type == Piece::PAWN) {
        if (move.color == Color::WHITE) {
            white_touched[from_file] = true;
            white_touched[to_file] = true;
        } else {
            black_touched[from_file] = true;
            black_touched[to_file] = true;
        }
    }

    // ------------------------------------------------------------
    // Captured pawn affects enemy pawn summary
    // ------------------------------------------------------------
    if (move.capture == Piece::PAWN) {
        int captured_file;

        if (move.is_en_passent_capture) {
            if (move.color == Color::WHITE) {
                captured_file = ((move.toSQ - 8) & 7);
                black_touched[captured_file] = true;
            } else {
                captured_file = ((move.toSQ + 8) & 7);
                white_touched[captured_file] = true;
            }
        } else {
            captured_file = to_file;

            if (move.color == Color::WHITE) {
                black_touched[captured_file] = true;
            } else {
                white_touched[captured_file] = true;
            }
        }
    }

    // Recompute only touched files from the CURRENT board state.
    refresh_pawn_summary_files_t<Color::WHITE>(whitePInfo, white_touched);
    refresh_pawn_summary_files_t<Color::BLACK>(blackPInfo, black_touched);
}


static void print_bb64(ull bb)
{
    // Prints rank 8 down to 1, and files A..H in your h1=0 mapping
    // (pure debug visualization; you can change formatting later).
    for (int r = 7; r >= 0; --r) {
        for (int f = 7; f >= 0; --f) {   // f=7 is A, f=0 is H
            int sq = r * 8 + f;
            ull bit = (1ULL << sq);
            std::printf("%c ", (bb & bit) ? '1' : '.');
        }
        std::printf("\n");
    }
}

// Note: debug only. 
void GameBoard::validate_row_col_masks_h1_0()
{
    //printf("\nder\n");
    // 1) Each row mask must match (sq/8 == r)
    for (int r = 0; r < 8; ++r) {
        ull expect = 0ULL;
        for (int f = 0; f < 8; ++f) {
            int sq = r * 8 + f;
            expect |= (1ULL << sq);
        }

        if (row_masks[r] != expect) {
            std::printf("ROW MASK MISMATCH r=%d\n", r);
            std::printf("expected:\n"); print_bb64(expect);
            std::printf("actual:\n");   print_bb64(row_masks[r]);
            assert(0);          // To force an exit
        }
    }

    // 2) Each file mask must match (sq%8 == f)
    for (int f = 0; f < 8; ++f) {
        ull expect = 0ULL;
        for (int r = 0; r < 8; ++r) {
            int sq = r * 8 + f;
            expect |= (1ULL << sq);
        }

        if (col_masksHA[f] != expect) {
            std::printf("FILE MASK MISMATCH f=%d (this f is sq%%8)\n", f);
            std::printf("expected:\n"); print_bb64(expect);
            std::printf("actual:\n");   print_bb64(col_masksHA[f]);
            assert(0);          // To force an exit
        }
    }

    // 3) Every square must be in exactly one row and exactly one file
    for (int sq = 0; sq < 64; ++sq) {
        ull bit = (1ULL << sq);

        int rowHits = 0;
        int colHits = 0;

        for (int r = 0; r < 8; ++r) if (row_masks[r] & bit) ++rowHits;
        for (int f = 0; f < 8; ++f) if (col_masksHA[f] & bit) ++colHits;

        if (rowHits != 1 || colHits != 1) {
            std::printf("SQUARE MEMBERSHIP BAD sq=%d rowHits=%d colHits=%d\n", sq, rowHits, colHits);
            assert(0);          // To force an exit
        }
    }

    // 4) Intersection identity: row[r] & file[f] must be exactly bit(r*8+f)
    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            int sq = r * 8 + f;
            ull expect = (1ULL << sq);
            ull got = row_masks[r] & col_masksHA[f];

            if (got != expect) {
                std::printf("INTERSECTION BAD r=%d f=%d (expect sq=%d)\n", r, f, sq);
                std::printf("expected one-bit:\n"); print_bb64(expect);
                std::printf("got:\n");             print_bb64(got);
                assert(0);          // To force an exit
            }
        }
    }

    //std::printf("validate_row_col_masks_h1_0(): OK\n");
}


// 0 = center, 3 = edge
int GameBoard::piece_edgeness(ull kbb) {   

    assert(kbb>0);
    Square sq = utility::bit::lsb_and_pop_to_square(kbb);  // 0..63

    int r = sq >> 3;              // 0..7
    int f = sq & 7;              // 0..7
    int dr = std::min(std::abs(r - 3), std::abs(r - 4));
    int df = std::min(std::abs(f - 3), std::abs(f - 4));
    int ring = std::max(dr, df); // 0..3

    if (ring < 0) ring = 0;
    if (ring > 3) ring = 3;

    return ring;  // 0 = center, 3 = edge
}

//
// Used only in CRAZY_IVAN. CRAZY_IVAN is designed for maximum speed, and doesnt care about anything else.
// Some sort of "texture" must be provided though, or it moves the same piece back and forth. 
//
int GameBoard::center_closeness_bonus(Color c)
{
    int bonus = 0;

    //constexpr ull pieces = get_pieces_template(c);
    //ull pieces = get_pieces();
    ull pieces =
        (c==Color::WHITE)
        ? (white_knights | white_bishops | white_rooks | white_queens)
        : (black_knights | black_bishops | black_rooks | black_queens);

    while (pieces) {
        Square sq = utility::bit::lsb_and_pop_to_square(pieces);
        bonus += GameBoard::CENTER_SCORE[sq];
    }

    return bonus *wghts.GetWeight(CENTER_OCCUPY_PIECES);
}




int GameBoard::kings_in_opposition(Color color)
{
    assert(white_king && black_king);          // Must be kings on board

    ull wk_temp = white_king;
    ull bk_temp = black_king;

    Square w_sq = utility::bit::lsb_and_pop_to_square(wk_temp);
    Square b_sq = utility::bit::lsb_and_pop_to_square(bk_temp);

    int wr = w_sq >> 3, wc = w_sq & 7;
    int br = b_sq >> 3, bc = b_sq & 7;

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
    int xEnemy = enemyKingSq & 7;
    int yEnemy = enemyKingSq >> 3;
    int xFrien = frienKingSq & 7;
    int yFrien = frienKingSq >> 3;
   
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


bool GameBoard::is_king_highest_piece() {
    if (white_queens || black_queens) return false;
    if (white_rooks || black_rooks) return false;
    return true;

}








//
// Minimum Manhattan distance from the friendly king to the 2×2 center box
// center squares: (3,3),(4,3),(3,4),(4,4)  (i.e., d4,e4,d5,e5 in normal coords)
// Range: 0..6 (max occurs from corner squares like a1/h1/a8/h8).
template<Color c> int GameBoard::king_center_manhattan_dist_t()
{
    int iReturn;
    assert(white_king != 0ULL);
    assert(black_king != 0ULL);

    ull kbb = get_pieces_template<Piece::KING,c>();
    Square ks = utility::bit::bitboard_to_lowest_square_fast(kbb);

    int f = ks & 7;
    int r = ks >> 3;

    int dx = (f < 3) ? (3 - f) : (f > 4 ? f - 4 : 0);
    int dy = (r < 3) ? (3 - r) : (r > 4 ? r - 4 : 0);

    iReturn = dx + dy;
    assert(iReturn <= 6);

    return iReturn;
}

// Counts sliders+knights attacking the enemy's passed pawns.
// passed_white_pwns / passed_black_pwns are bitboards of all passed pawns.
int GameBoard::attackers_on_enemy_passed_pawns(Color attacker_color,
                                               ull passed_white_pwns,
                                               ull passed_black_pwns)
{
    // enemy (the one who owns the passed pawns we are attacking)
    Color defender_color = utility::representation::opposite_color(attacker_color);

    // pick the enemy passed pawns bitboard
    ull enemy_passed =
        (defender_color == Color::WHITE) ? passed_white_pwns
                                         : passed_black_pwns;

    if (!enemy_passed) return 0;

    int total = 0;
    ull tmp = enemy_passed;   // dont mutate input

    while (tmp) {

        Square sq = utility::bit::lsb_and_pop_to_square(tmp);  // 0..63

        int nAttackers = (attacker_color == Color::WHITE)
            ? sliders_and_knights_attacking_square2_t<Color::WHITE>(sq)
            : sliders_and_knights_attacking_square2_t<Color::BLACK>(sq);
        total += nAttackers;
    }

    return total;
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
// Pawns in normal positions, all other pieces on back rank. Same number of pieces as regular setup.
// Position symetric for white and black. Each side gets one bishop of each color. KIng must
// lie between the two rooks.
std::string GameBoard::random_960_FEN_strict()
{
    // Files 0..7 correspond to a..h in the FEN string.
    // On rank 1, a1 is dark. So even files (0,2,4,6) are dark; odd files (1,3,5,7) are light.
    std::vector<int> remaining;
    remaining.reserve(8);
    for (int i = 0; i < 8; ++i) {
        remaining.push_back(i);
    }

    char rank1[8];
    for (int i = 0; i < 8; ++i) rank1[i] = '?';

    // --- 1) Bishops on opposite colors ---
    int darkSquares[4]  = { 0, 2, 4, 6 };
    int lightSquares[4] = { 1, 3, 5, 7 };

    int b1_file = darkSquares[rand_new() % 4];
    int b2_file = lightSquares[rand_new() % 4];

    rank1[b1_file] = 'B';
    rank1[b2_file] = 'B';

    // Remove those files from remaining
    for (size_t i = 0; i < remaining.size(); ) {
        if (remaining[i] == b1_file || remaining[i] == b2_file) {
            remaining.erase(remaining.begin() + (int)i);
        } else {
            ++i;
        }
    }

    // --- 2) Queen on a remaining square ---
    {
        int idx = rand_new() % (int)remaining.size();
        int q_file = remaining[idx];
        rank1[q_file] = 'Q';
        remaining.erase(remaining.begin() + idx);
    }

    // --- 3) Two knights on remaining squares ---
    for (int k = 0; k < 2; ++k) {
        int idx = rand_new() % (int)remaining.size();
        int n_file = remaining[idx];
        rank1[n_file] = 'N';
        remaining.erase(remaining.begin() + idx);
    }

    // --- 4) Remaining three squares: R, K, R with king between rooks ---
    // Sort the remaining files and place R-K-R in that order.
    std::sort(remaining.begin(), remaining.end()); // size should be 3
    {
        int r1_file = remaining[0];
        int k_file  = remaining[1];
        int r2_file = remaining[2];

        rank1[r1_file] = 'R';
        rank1[k_file]  = 'K';
        rank1[r2_file] = 'R';
    }

    // Build rank8 as lowercase mirror of rank1
    char rank8[8];
    for (int i = 0; i < 8; ++i) {
        char c = rank1[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        rank8[i] = c;
    }

    // Convert rank arrays to strings
    std::string sRank8(rank8, rank8 + 8);
    std::string sRank7("pppppppp");
    std::string sRank6("8");
    std::string sRank5("8");
    std::string sRank4("8");
    std::string sRank3("8");
    std::string sRank2("PPPPPPPP");
    std::string sRank1(rank1, rank1 + 8);

    // FEN: ranks 8..1, side to move, castling, ep, halfmove, fullmove.
    // For now: no castling rights ("-") so your current orthodox-castling code won’t try.
    std::string fen =
        sRank8 + "/" + sRank7 + "/" + sRank6 + "/" + sRank5 + "/" +
        sRank4 + "/" + sRank3 + "/" + sRank2 + "/" + sRank1 +
        " w - - 0 1";

    return fen;
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

    // const ull frm = mv.from;
    // const ull to  = mv.to;
    const ull from_bb = utility::bit::square_to_bitboard(mv.fromSQ);
    const ull to_bb = utility::bit::square_to_bitboard(mv.toSQ);
    // assert(frm == from_bb);
    // assert(to == to_bb);

    assert (bits_in(from_bb) == 1);
    assert (bits_in(to_bb) == 1);
    
    if (from_bb == 0ULL || to_bb == 0ULL) {   
        assert(0);      // NULL bitboards in the Move, should never happen.
        return 0;
    }

    // Convert bitboards to 0..63 square indices (h1=0)
    ull tmp = from_bb;
    Square from_sq = utility::bit::lsb_and_pop_to_square(tmp);
    tmp = to_bb;
    Square to_sq   = utility::bit::lsb_and_pop_to_square(tmp);


    // There must be an enemy victim on 'to_sq' for a normal capture
    Piece victim = get_piece_type_on_bitboard(to_bb);

    if (victim == Piece::NONE) {
        //assert(0);
        return 0;  // ignore en passant etc. for SEE for now. Note: what is this?
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

    // Get local mutable copies of all piece bitboards
    ull wp = white_pawns,   wn = white_knights, wb = white_bishops,
        wr = white_rooks,   wq = white_queens,  wk = white_king;

    ull bp = black_pawns,   bn = black_knights, bb = black_bishops,
        br = black_rooks,   bq = black_queens,  bk = black_king;

    ull occ = get_pieces();

    const ull FILE_A = col_masksHA[ColHA::COL_A];
    const ull FILE_H = col_masksHA[ColHA::COL_H];

 

    // Optional debug: dump initial attackers
    if (fpDebug)
    {
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

            int c0 = to_sq & 7;
            int r0 = to_sq >> 3;

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


        ull att_w0 = attackers_on_initial(Color::WHITE, occ);
        ull att_b0 = attackers_on_initial(Color::BLACK, occ);

        fprintf(fpDebug,
                "debug (before forced capture): side=%s from_bb=0x%016llx to_bb=0x%016llx "
                "from_sq=%d to_sq=%d victim=%d mover=%d\n",
                utility::representation::color_to_string(side).c_str(),
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
                Square s = utility::bit::lsb_and_pop_to_square(tmp2);
                fprintf(fpDebug, " %d", s);
            }
            fprintf(fpDebug, "\n");
        };

        dump_attackers("white", att_w0);
        dump_attackers("black", att_b0);

    }   // END debug

    // === Apply the FORCED first capture mv by 'side' ===

    int balance = 0;
    balance += centipawn_score_of(victim);   // side captures victim

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
    Color next_clr      = (side == Color::WHITE) ? Color::BLACK : Color::WHITE;

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

            int r0 = to_sq >> 3;
            int c0 = to_sq & 7;

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
            int val = centipawn_score_of(target_piece_local);
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
            //Piece attacker_piece2 = piece_type_of(stm, attacker_bb);
            Piece attacker_piece = get_piece_type_on_bitboard(stm, attacker_bb);
            //assert(attacker_piece == attacker_piece2);

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
                         next_clr,
                         target_piece, target_color,
                         occ,
                         wp, wn, wb, wr, wq, wk,
                         bp, bn, bb, br, bq, bk,
                         balance);

    if (fpDebug) {
        fprintf(fpDebug,
                "debug final: move(from_sq=%d,to_sq=%d) side=%s SEE=%d\n",
                from_sq, to_sq,
                utility::representation::color_to_string(side).c_str(),
                result);
    }

    return result;
}


// -----------------------------------------------------------------------------
// Static Exchange Evaluation (SEE) for a *single* capture move.
//
// Given a capture move `mv`
// this routine estimates the net material gain/loss in centipawns assuming
// optimal recaptures on the destination square.
// Positive value returned means the capture is materially profitable for the side to move.
// Zero value means the capture is materially neutral after exchanges.
//   1) It identifies the moving piece (mover) on `from` and the captured piece
//      (victim) on `to`. If there is no victim (e.g. en-passant), it returns 0.
//   2) It "plays" the forced first capture on local copies of all piece
//      bitboards + occupancy: removes the victim, moves the mover to `to_sq`,
//      and updates `occ`.
//   3) It then calls SEE_recursive() for alternating recaptures on `to_sq`,
//      treating the mover (now on `to_sq`) as the current target.
//   4) Returns the resulting net score from the perspective of `clr` (the side
//      making the initial capture), in centipawns. 
//
// Notes / assumptions:
// - SEE is purely material-based and local to the target square; it does not
//   consider check, pins, discovered tactics beyond what SEE_recursive models.
// -----------------------------------------------------------------------------
int GameBoard::SEE_for_capture_new(Color clr, const Move &mv, FILE* fpDebug)
{
    // from and to are BITBOARDS (ull) with exactly one bit set.
    // ull frm = mv.from;
    // ull to  = mv.to;
    const ull from_bb = utility::bit::square_to_bitboard(mv.fromSQ);
    const ull to_bb = utility::bit::square_to_bitboard(mv.toSQ);
    // assert(frm == from_bb);
    // assert(to == to_bb);

    assert (bits_in(from_bb) == 1);
    assert (bits_in(to_bb) == 1);

    if (from_bb == 0ULL || to_bb == 0ULL) {   
        assert(0);      // NULL bitboards in the Move, should never happen.
        return 0;
    }

    // Convert bitboards to 0..63 square indices (h1 = 0) using your helper
    ull tmp = from_bb;
    Square from_sq = utility::bit::lsb_and_pop_to_square(tmp);
    tmp = to_bb;
    Square to_sq   = utility::bit::lsb_and_pop_to_square(tmp);


    // There must be an enemy victim on 'to_sq' for a normal capture
    Piece victim = get_piece_type_on_bitboard(to_bb);

    if (victim == Piece::NONE) {
        //assert(0);
        return 0;  // ignore en passant etc. for SEE, for now
    }

    // Identify the victim on 'to_sq'
    Color victim_color = get_color_on_bitboard(to_bb);
    if (victim_color == clr) {   
        assert(0);     // Caller should prevent this. Cant take your own piece!
        return 0;
    }

    // Identify the moving piece on 'from_sq'
    Piece mover = get_piece_type_on_bitboard(from_bb);
    if (mover == Piece::NONE) {
        assert(0);      // Caller should prevent this. Cant take nothing!
        return 0;
    }

    // Get local mutable copies of all piece bitboards
    ull wp = white_pawns,   wn = white_knights, wb = white_bishops,
        wr = white_rooks,   wq = white_queens,  wk = white_king;

    ull bp = black_pawns,   bn = black_knights, bb = black_bishops,
        br = black_rooks,   bq = black_queens,  bk = black_king;

    ull occ = get_pieces();

    const ull FILE_A = col_masksHA[ColHA::COL_A];
    const ull FILE_H = col_masksHA[ColHA::COL_H];

 
    // === Apply the FORCED first capture mv by 'color' ===

    int balance = 0;
    balance += centipawn_score_of(victim);   // color captures victim

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
    if (clr == Color::WHITE) {
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
    if (clr == Color::WHITE) {
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
    Color target_color  = clr;
    Color root_side     = clr;
    Color next_clr      = (clr == Color::WHITE) ? Color::BLACK : Color::WHITE;

    SEEBoards b0;
    b0.wp = wp; b0.wn = wn; b0.wb = wb; b0.wr = wr; b0.wq = wq; b0.wk = wk;
    b0.bp = bp; b0.bn = bn; b0.bb = bb; b0.br = br; b0.bq = bq; b0.bk = bk;

    int result = SEE_recursive(next_clr,
                        root_side,
                        to_sq,
                        target_piece,
                        target_color,
                        occ,
                        b0,             // all the bitboards
                        balance);

    if (fpDebug) {
        fprintf(fpDebug,
                "debug final: move(from_sq=%d,to_sq=%d) clt=%s SEE=%d\n",
                from_sq, to_sq,
                utility::representation::color_to_string(clr).c_str(),
                result);
    }

    return result;
}

//
// Returns a bitboard of all pieces of color `c` that currently attack the
// square `sq`, using the *local SEE board state* instead of the real board.
//
// This function is used during Static Exchange Evaluation (SEE).  It does not
// read the GameBoard’s live bitboards; instead it uses the supplied SEEBoards
// snapshot `b` plus the supplied occupancy `occ_now`, which represent a
// hypothetical capture sequence in progress.
//
// Attacks detected:
//   - pawn capture sources
//   - knight attacks (table lookup)
//   - king attacks (table lookup)
//   - sliding attacks from bishops/queens (diagonals)
//   - sliding attacks from rooks/queens (ranks/files)
//
// Important properties:
//   • `sq` is a 0..63 index (h1=0 convention)
//   • `occ_now` must match the board state represented in `b`
//   • Only the first blocker in each ray is considered (correct for SEE)
//   • Returned bitboard may contain multiple attackers
//
// This routine is intentionally side-effect free and fast, since it is called
// repeatedly during SEE recursion.
ull GameBoard::SEE_attackers_on_square_local(Color c,
                                             int sq,
                                             ull occ_now,
                                             const SEEBoards& b) const
{
    ull atk = 0ULL;
    const ull bit = (1ULL << sq);

    const ull FILE_A = col_masksHA[ColHA::COL_A];
    const ull FILE_H = col_masksHA[ColHA::COL_H];

    // -----------------------
    // Pawns (origins that attack sq)
    // -----------------------
    const ull pawns = (c == Color::WHITE) ? b.wp : b.bp;

    ull origins = 0ULL;
    if (c == Color::WHITE) {
        origins = ((bit & ~FILE_A) >> 7) | ((bit & ~FILE_H) >> 9);
    } else {
        origins = ((bit & ~FILE_H) << 7) | ((bit & ~FILE_A) << 9);
    }
    atk |= (origins & pawns);

    // -----------------------
    // Knights
    // -----------------------
    const ull knights = (c == Color::WHITE) ? b.wn : b.bn;
    atk |= (tables::movegen::knight_attack_table[sq] & knights);

    // -----------------------
    // Kings
    // -----------------------
    const ull kings = (c == Color::WHITE) ? b.wk : b.bk;
    atk |= (tables::movegen::king_attack_table[sq] & kings);

    // -----------------------
    // Sliders via your ray-attack helpers
    // -----------------------
    const ull all_pieces_but_self = occ_now & ~bit;

    // Diagonals: bishops/queens

    const ull bishops = (c == Color::WHITE) ? b.wb : b.bb;
    const ull queens  = (c == Color::WHITE) ? b.wq : b.bq;

    const ull diag_attacks = get_diagonal_attacks(all_pieces_but_self, sq);
    atk |= (diag_attacks & (bishops | queens));

    // Straights: rooks/queens

    const ull rooks  = (c == Color::WHITE) ? b.wr : b.br;

    const ull straight_attacks = get_straight_attacks(all_pieces_but_self, sq);
    atk |= (straight_attacks & (rooks | queens));

    return atk;
}


int GameBoard::SEE_recursive(Color stm,
                       Color root_side,
                       int to_sq,
                       Piece target_piece,
                       Color target_color,
                       ull occ_local,
                       const SEEBoards& b_local,
                       int balance_local)
{
    int best = balance_local;

    // attackers for stm on to_sq
    ull atk_side = SEE_attackers_on_square_local(stm, to_sq, occ_local, b_local);
    if (!atk_side) return best;     // No more attackers


    ull tmp_atk = atk_side;
    while (tmp_atk) {

        ull attacker_bb = tmp_atk & (~tmp_atk + 1ULL);   // LS1B
        tmp_atk &= ~attacker_bb;

        // Copy state
        SEEBoards b2 = b_local;
        ull occ2 = occ_local;

        // Material swing is the value of the current target on to_sq
        int val = centipawn_score_of(target_piece);
        int new_balance = balance_local;

        if (stm == root_side) new_balance += val;
        else                  new_balance -= val;

        ull to_mask_local = (1ULL << to_sq);

        // Remove the captured target from its bitboard
        if (target_color == Color::WHITE) {
            if      (target_piece == Piece::PAWN)   b2.wp &= ~to_mask_local;
            else if (target_piece == Piece::KNIGHT) b2.wn &= ~to_mask_local;
            else if (target_piece == Piece::BISHOP) b2.wb &= ~to_mask_local;
            else if (target_piece == Piece::ROOK)   b2.wr &= ~to_mask_local;
            else if (target_piece == Piece::QUEEN)  b2.wq &= ~to_mask_local;
            else if (target_piece == Piece::KING)   b2.wk &= ~to_mask_local;
        } else {
            if      (target_piece == Piece::PAWN)   b2.bp &= ~to_mask_local;
            else if (target_piece == Piece::KNIGHT) b2.bn &= ~to_mask_local;
            else if (target_piece == Piece::BISHOP) b2.bb &= ~to_mask_local;
            else if (target_piece == Piece::ROOK)   b2.br &= ~to_mask_local;
            else if (target_piece == Piece::QUEEN)  b2.bq &= ~to_mask_local;
            else if (target_piece == Piece::KING)   b2.bk &= ~to_mask_local;
        }

        // Identify attacker piece (you already have this overload in your code)
        Piece attacker_piece = get_piece_type_on_bitboard(stm, attacker_bb);

        // Remove attacker from its origin square
        if (stm == Color::WHITE) {
            if      (attacker_bb & b2.wp) b2.wp &= ~attacker_bb;
            else if (attacker_bb & b2.wn) b2.wn &= ~attacker_bb;
            else if (attacker_bb & b2.wb) b2.wb &= ~attacker_bb;
            else if (attacker_bb & b2.wr) b2.wr &= ~attacker_bb;
            else if (attacker_bb & b2.wq) b2.wq &= ~attacker_bb;
            else if (attacker_bb & b2.wk) b2.wk &= ~attacker_bb;
        } else {
            if      (attacker_bb & b2.bp) b2.bp &= ~attacker_bb;
            else if (attacker_bb & b2.bn) b2.bn &= ~attacker_bb;
            else if (attacker_bb & b2.bb) b2.bb &= ~attacker_bb;
            else if (attacker_bb & b2.br) b2.br &= ~attacker_bb;
            else if (attacker_bb & b2.bq) b2.bq &= ~attacker_bb;
            else if (attacker_bb & b2.bk) b2.bk &= ~attacker_bb;
        }

        // Place attacker onto to_sq
        if (stm == Color::WHITE) {
            if      (attacker_piece == Piece::PAWN)   b2.wp |= to_mask_local;
            else if (attacker_piece == Piece::KNIGHT) b2.wn |= to_mask_local;
            else if (attacker_piece == Piece::BISHOP) b2.wb |= to_mask_local;
            else if (attacker_piece == Piece::ROOK)   b2.wr |= to_mask_local;
            else if (attacker_piece == Piece::QUEEN)  b2.wq |= to_mask_local;
            else if (attacker_piece == Piece::KING)   b2.wk |= to_mask_local;
        } else {
            if      (attacker_piece == Piece::PAWN)   b2.bp |= to_mask_local;
            else if (attacker_piece == Piece::KNIGHT) b2.bn |= to_mask_local;
            else if (attacker_piece == Piece::BISHOP) b2.bb |= to_mask_local;
            else if (attacker_piece == Piece::ROOK)   b2.br |= to_mask_local;
            else if (attacker_piece == Piece::QUEEN)  b2.bq |= to_mask_local;
            else if (attacker_piece == Piece::KING)   b2.bk |= to_mask_local;
        }

        // Update occupancy
        occ2 &= ~attacker_bb;
        occ2 &= ~to_mask_local;
        occ2 |=  to_mask_local;

        // Next ply: the new target is the attacker now sitting on to_sq
        Piece new_target_piece = attacker_piece;
        Color new_target_color = stm;

        //Color next = (stm == Color::WHITE) ? Color::BLACK : Color::WHITE;
        Color next = utility::representation::opposite_color(stm);

        int child = SEE_recursive(next,
                            root_side,
                            to_sq,
                            new_target_piece,
                            new_target_color,
                            occ2,
                            b2,
                            new_balance);

        if (stm == root_side)
        {
            if (child > best) best = child;
        }
        else
        {
            if (child < best) best = child;
        }
    }

    return best;
}




// ============================================================================
// Template implementations for Color-parameterized evaluation helpers
// ============================================================================

// ---------- bHasCastled_fake_t ----------
template<Color c>
bool GameBoard::bHasCastled_fake_t(int k_rank, int k_file) const {
 
    // king must be on back ranks to be castled.
    constexpr int homeRank   = (c == Color::WHITE) ? ROW_1 : ROW_8;
    //constexpr int homeRankp1 = (c == Color::WHITE) ? ROW_2 : ROW_7;
    //if ( (k_rank != homeRank) && (k_rank != homeRankp1) ) return false;
    if (k_rank != homeRank) return false;

    // If king still in "center", then you are not castled.
    if ((k_file > COL_G) && (k_file < COL_C)) return false;  // king cannot be on f,e,d files 



    ull occupied = get_pieces_template<Piece::ROOK, c>();

    // Cannot lock the rook in on edge file.
    // Scan from the king toward the nearest edge along its rank.
    // If a rook is found between the king and that edge, mark as blocked.
    // This means the rook would be trapped behind the king on that side.
    bool blocked = false;
    if (k_file <= COL_G) {      // K side castle
        for (int f = k_file - 1; f >= 0; --f) {
            int sq = k_rank * 8 + f;
            if (occupied & (1ULL << sq)) {
                blocked = true;
                break;
            }
        }
    } else {                    // Q side castle
        for (int f = k_file + 1; f <= 7; ++f) {
            int sq = k_rank * 8 + f;
            if (occupied & (1ULL << sq)) {
                blocked = true;
                break;
            }
        }
    }


    // if (global_debug_flag) {
    //     printf ("%d %d bHasCastled_fake_t %d %d\n", c, blocked, k_rank, k_file);
    // }


    return !blocked;
}

// ---------- get_castled_bonus_cp_t ----------
template<Color c>
int GameBoard::get_castled_bonus_cp_t(int phase, const PInfo& PInfoIn) const {

    int i_can_castleNumer = 0;
    int i_can_castleDenom = 0;

    bool b_has_castled = false;


    ull king_bb = get_pieces_template<Piece::KING, c>();
    if (!king_bb) {
        assert(0);      // has to be a king!
        return 0;
    }
    const int king_sq = utility::bit::bitboard_to_lowest_square_fast(king_bb);
    const int k_file  = king_sq & 7;
    const int k_rank  = king_sq >> 3;


    // Has castled //////////////////////////////////////////////

    b_has_castled = bHasCastled_fake_t<c>(k_rank, k_file);

    int cpWght = wghts.GetWeight(HAS_CASTLED);

    if (b_has_castled) {
        // weight for how good the fortress is.

        constexpr int homeRank   = (c == Color::WHITE) ? ROW_1 : ROW_8;
        constexpr int homeRankp1 = (c == Color::WHITE) ? ROW_2 : ROW_7;

        // If king is not on home rank or one step off it, do not count guard pawns.
        if ((k_rank != homeRank) && (k_rank != homeRankp1)) {
            return 0;
        }

        // Take guard files into account
        //int nGuardPawns2 = count_guard_pawn_files_23_t<c>(PInfoIn, k_file);
        int nGuardPawns = count_guard_pawn_files_23_new_t<c>(PInfoIn, k_file);
        //assert(nGuardPawns==nGuardPawns2);

        if (nGuardPawns==3) cpWght = cpWght;
        else if (nGuardPawns==2) cpWght = cpWght * 2 / 3;
        else if (nGuardPawns==1) cpWght = cpWght * 1 / 3;
        else if (nGuardPawns==0) cpWght = 0;
        else assert(0);
    }

    // Can castle //////////////////////////////////////////////

    i_can_castleNumer = 0;
    i_can_castleDenom = 1;
    if constexpr (c == Color::WHITE) {
        i_can_castleNumer += (white_castle_rights & CASTLE_KING)  ? 1 : 0;
        i_can_castleNumer += (white_castle_rights & CASTLE_QUEEN) ? 1 : 0;
    } else {
        i_can_castleNumer += (black_castle_rights & CASTLE_KING)  ? 1 : 0;
        i_can_castleNumer += (black_castle_rights & CASTLE_QUEEN) ? 1 : 0;
    }

    // Make "1" (can castle one side)"3/2"
    if (i_can_castleNumer==1) {
        i_can_castleNumer=3;
        i_can_castleDenom=2;
    }

    int cpWghtB = wghts.GetWeight(CAN_CASTLE);



    int icode = (b_has_castled ? cpWght : 0) + (cpWghtB*i_can_castleNumer)/i_can_castleDenom;

    int final_cp;

    if      (phase == GamePhase::OPENING) final_cp = icode;
    else if (phase == GamePhase::MIDDLE_EARLY) final_cp = (2*icode)/3;
    else if (phase == GamePhase::MIDDLE) final_cp = icode/2;
    else final_cp = 0;

    return final_cp;
}




// Returns how many of the 3 guard files for the king's side
// contain at least one friendly pawn on relative rank 2, 3.
//
// White:
//   queenside king  -> files a,b,c
//   kingside king   -> files f,g,h
// Black:
//   same file logic, but relative ranks are mirrored.
//
// This does NOT check whether castling actually occurred.
// It only checks whether a plausible 3-file pawn shelter exists
// near the king's current side.
//
// Returns:
//   0..3   number of guard files that still have at least one pawn
//          on relative rank 2/3 for this side.

// Returns how many of the 3 guard files for the king's side
// contain at least one friendly pawn on relative rank 2 or 3.
//
// White:
//   queenside king -> files a,b,c
//   kingside king  -> files f,g,h
//
// Black:
//   same file logic, but relative ranks are mirrored.
//
// This does NOT check whether castling actually occurred.
// It only checks whether a plausible 3-file pawn shelter exists
// near the king's current side.
//
// Returns:
//   0..3   number of guard files that still have at least one pawn
//          on relative rank 2/3 for this side.

template<Color c> int GameBoard::count_guard_pawn_files_23_new_t(const PInfo& PInfoIn, int k_file) const
{
    int file0;
    int file1;
    int file2;

    if (k_file >= COL_C) {
        // King on a/b/c side -> guard files a,b,c
        file0 = COL_A;
        file1 = COL_B;
        file2 = COL_C;
    }
    else if (k_file <= COL_G) {
        // King on f/g/h side -> guard files f,g,h
        file0 = COL_F;
        file1 = COL_G;
        file2 = COL_H;
    }
    else {
        return 0;
    }

    int nGuardFiles = 0;

    if (PInfoIn.guard_files_23 & (1u << file0)) ++nGuardFiles;
    if (PInfoIn.guard_files_23 & (1u << file1)) ++nGuardFiles;
    if (PInfoIn.guard_files_23 & (1u << file2)) ++nGuardFiles;

    return nGuardFiles;
}

template<Color c>
int GameBoard::get_material_for_color_t(int& cp_pawns_only) {
    int cp_score_mat_temp = 0;
    cp_pawns_only = bits_in(get_pieces_template<Piece::PAWN, c>()) * centipawn_score_of(Piece::PAWN);

    cp_score_mat_temp += cp_pawns_only;
    cp_score_mat_temp += bits_in(get_pieces_template<Piece::KNIGHT, c>()) * centipawn_score_of(Piece::KNIGHT);
    cp_score_mat_temp += bits_in(get_pieces_template<Piece::BISHOP, c>()) * centipawn_score_of(Piece::BISHOP);
    cp_score_mat_temp += bits_in(get_pieces_template<Piece::ROOK, c>())   * centipawn_score_of(Piece::ROOK);
    cp_score_mat_temp += bits_in(get_pieces_template<Piece::QUEEN, c>())  * centipawn_score_of(Piece::QUEEN);

    
    return cp_score_mat_temp;
}

template<Color c>
int GameBoard::get_material_for_color2_t(int& cp_pawns_only) {
    int cp_score_mat_temp = 0;

    cp_pawns_only = Bits_In[c][Piece::PAWN] * centipawn_score_of(Piece::PAWN);

    cp_score_mat_temp += cp_pawns_only;
    cp_score_mat_temp += Bits_In[c][Piece::KNIGHT] * centipawn_score_of(Piece::KNIGHT);
    cp_score_mat_temp += Bits_In[c][Piece::BISHOP] * centipawn_score_of(Piece::BISHOP);
    cp_score_mat_temp += Bits_In[c][Piece::ROOK]   * centipawn_score_of(Piece::ROOK);
    cp_score_mat_temp += Bits_In[c][Piece::QUEEN]  * centipawn_score_of(Piece::QUEEN);

    return cp_score_mat_temp;
}
// Given a single square, returns a count of the pawns attacking that square.
// Note: is en passant considered here?
template<Color c>
int GameBoard::pawns_attacking_square_t(int sq) {
    ull bitBoard = (1ULL << sq);

    const ull FILE_H = col_masksHA[ColHA::COL_H];
    const ull FILE_A = col_masksHA[ColHA::COL_A];

    ull origins;

    if constexpr (c == Color::WHITE) {
        origins = ((bitBoard & ~FILE_A) >> 7) | ((bitBoard & ~FILE_H) >> 9);
    } else {
        origins = ((bitBoard & ~FILE_H) << 7) | ((bitBoard & ~FILE_A) << 9);
    }

    ull pawns = get_pieces_template<Piece::PAWN, c>();
    return bits_in(origins & pawns);
}
//
// Given a multiple squares bitboard, 
//  returns a count of : distinct pawns of color c that attack at least one square in bitBoard.
// Warning: this only works if none of the squares in the bitboard can be attacked by the same pawn.
//      That is, if ypu place a pawn anywhere on the board, it wont attack more than one square in the passed squares.
//      otherwise it will undercount.
template<Color c>
int GameBoard::pawns_attacking_squares_t(ull bitBoard) {

    const ull FILE_H = col_masksHA[ColHA::COL_H];
    const ull FILE_A = col_masksHA[ColHA::COL_A];

    ull origins;

    if constexpr (c == Color::WHITE) {
        origins = ((bitBoard & ~FILE_A) >> 7) | ((bitBoard & ~FILE_H) >> 9);
    } else {
        origins = ((bitBoard & ~FILE_H) << 7) | ((bitBoard & ~FILE_A) << 9);
    }

    ull pawns = get_pieces_template<Piece::PAWN, c>();
    return bits_in(origins & pawns);
}


template<Color c>
int GameBoard::pawns_attacking_center_squares_cp_t()
{
    int sum = 0;

    if constexpr (c == Color::WHITE) {
        sum +=wghts.GetWeight(PAWN_ON_CTR_OFF) * pawns_attacking_square_t<c>(square_e5);
        sum +=wghts.GetWeight(PAWN_ON_CTR_OFF) * pawns_attacking_square_t<c>(square_d5);

        sum +=wghts.GetWeight(PAWN_ON_CTR_DEF) * pawns_attacking_square_t<c>(square_e4);
        sum +=wghts.GetWeight(PAWN_ON_CTR_DEF) * pawns_attacking_square_t<c>(square_d4);

        sum +=wghts.GetWeight(PAWN_ON_ADV_CTR) * pawns_attacking_square_t<c>(square_e6);
        sum +=wghts.GetWeight(PAWN_ON_ADV_CTR) * pawns_attacking_square_t<c>(square_d6);

        sum +=wghts.GetWeight(PAWN_ON_ADV_FLK) * pawns_attacking_square_t<c>(square_c5);
        sum +=wghts.GetWeight(PAWN_ON_ADV_FLK) * pawns_attacking_square_t<c>(square_f5);
    } else {
        sum +=wghts.GetWeight(PAWN_ON_CTR_OFF) * pawns_attacking_square_t<c>(square_e4);
        sum +=wghts.GetWeight(PAWN_ON_CTR_OFF) * pawns_attacking_square_t<c>(square_d4);

        sum +=wghts.GetWeight(PAWN_ON_CTR_DEF) * pawns_attacking_square_t<c>(square_e5);
        sum +=wghts.GetWeight(PAWN_ON_CTR_DEF) * pawns_attacking_square_t<c>(square_d5);

        sum +=wghts.GetWeight(PAWN_ON_ADV_CTR) * pawns_attacking_square_t<c>(square_e3);
        sum +=wghts.GetWeight(PAWN_ON_ADV_CTR) * pawns_attacking_square_t<c>(square_d3);

        sum +=wghts.GetWeight(PAWN_ON_ADV_FLK) * pawns_attacking_square_t<c>(square_c4);
        sum +=wghts.GetWeight(PAWN_ON_ADV_FLK) * pawns_attacking_square_t<c>(square_f4);
    }

    return sum;
}


template<Color c>
int GameBoard::pawns_attacking_center_squares_cp_fast_t()
{
    int sum = 0;

    if constexpr (c == Color::WHITE) {

        sum +=wghts.GetWeight(PAWN_ON_CTR_OFF) * pawns_attacking_squares_t<c>(squares_e5_d5);

        sum +=wghts.GetWeight(PAWN_ON_CTR_DEF) * pawns_attacking_squares_t<c>(squares_e4_d4);

        sum +=wghts.GetWeight(PAWN_ON_ADV_CTR) * pawns_attacking_squares_t<c>(squares_advanced_centerW);

        sum +=wghts.GetWeight(PAWN_ON_ADV_FLK) * pawns_attacking_squares_t<c>(squares_flanking_centerW);

    } else {

        sum +=wghts.GetWeight(PAWN_ON_CTR_OFF) * pawns_attacking_squares_t<c>(squares_e4_d4);

        sum +=wghts.GetWeight(PAWN_ON_CTR_DEF) * pawns_attacking_squares_t<c>(squares_e5_d5);

        sum +=wghts.GetWeight(PAWN_ON_ADV_CTR) * pawns_attacking_squares_t<c>(squares_advanced_centerB);

        sum +=wghts.GetWeight(PAWN_ON_ADV_FLK) * pawns_attacking_squares_t<c>(squares_flanking_centerB);

    }

    return sum;
}



// ---------- knights_attacking_square_t ----------
template<Color c>
int GameBoard::knights_attacking_square_t(int sq)
{
    ull targets = tables::movegen::knight_attack_table[sq];
    ull knights = get_pieces_template<Piece::KNIGHT, c>();
    return bits_in(targets & knights);
}

// ---------- knights_attacking_center_squares_cp_t ----------
template<Color c>
int GameBoard::knights_attacking_center_squares_cp_t()
{
    int itemp = 0;
    itemp += knights_attacking_square_t<c>(square_e4);
    itemp += knights_attacking_square_t<c>(square_d4);
    itemp += knights_attacking_square_t<c>(square_e5);
    itemp += knights_attacking_square_t<c>(square_d5);

    return (itemp * wghts.GetWeight(KNIGHT_ON_CTR));
}

// ---------- bishops_attacking_square_t ----------

// It counts how many bishops of color c are on the same diagonal as square sq.
// Therefore all things blocking these bishops are ignored. Like as if nothing else but the bishop was on the board.
template<Color c>
int GameBoard::bishops_attacking_square_t(int sq)
{
    const int tf = sq & 7;
    const int tr = sq >> 3;

    // This magic diagonal weighter, 
    const int diag_sum  = tf + tr;
    const int diag_diff = tf - tr;

    ull bishops = get_pieces_template<Piece::BISHOP, c>();
    int count = 0;

    while (bishops)
    {
        const Square s = utility::bit::lsb_and_pop_to_square(bishops);
        const int f = s & 7;
        const int r = s >> 3;
        if ((f + r) == diag_sum || (f - r) == diag_diff) count++;
    }

    return count;
}

// ---------- bishops_attacking_center_squares_cp_t ----------
template<Color c>
int GameBoard::bishops_attacking_center_squares_cp_t()
{
    int itemp = 0;

    itemp += bishops_attacking_square_t<c>(square_e4);
    itemp += bishops_attacking_square_t<c>(square_d4);
    itemp += bishops_attacking_square_t<c>(square_e5);
    itemp += bishops_attacking_square_t<c>(square_d5);

    return (itemp*wghts.GetWeight(BISHOP_ON_CTR));
}

// ---------- two bishops (2 bishops)
template<Color c>
int GameBoard::two_bishops_cp_t(int nPhase) const {
    //ull friendlyBishops = get_pieces_template<Piece::BISHOP, c>();
    //int bishops2 = bits_in(friendlyBishops);
    int bishops = Bits_In[c][Piece::BISHOP];
    //assert(bishops==bishops2);

    if (bishops < 2) return 0;

    int final_cp = 0;
    if (nPhase < GamePhase::MIDDLE)  return final_cp;

    int weight = wghts.GetWeight(TWO_BISHOPS);
    if (nPhase == GamePhase::MIDDLE) final_cp = (2*weight)/3;
    if (nPhase > GamePhase::MIDDLE)  final_cp = weight;

    return final_cp;
}

// ---------- bishop_pawn_pattern_cp_t ----------
template<Color c>
int GameBoard::bishop_pawn_pattern_cp_t() {
    if constexpr (c == Color::WHITE) {
        ull bishop_mask = (1ULL << square_d3);
        ull pawn_mask   = (1ULL << square_d2);

        if ( (white_bishops & bishop_mask) &&
             (white_pawns   & pawn_mask) ) {
            return wghts.GetWeight(BISHOP_PATTERN);
        }

        bishop_mask = (1ULL << square_e2);
        pawn_mask   = (1ULL << square_e3);

        if ( (white_bishops & bishop_mask) &&
             (white_pawns   & pawn_mask) ) {
            return wghts.GetWeight(BISHOP_PATTERN);
        }
    } else {
        ull bishop_mask = (1ULL << square_d6);
        ull pawn_mask   = (1ULL << square_d7);

        if ( (black_bishops & bishop_mask) &&
             (black_pawns   & pawn_mask) ) {
            return wghts.GetWeight(BISHOP_PATTERN);
        }

        bishop_mask = (1ULL << square_e7);
        pawn_mask   = (1ULL << square_e6);

        if ( (black_bishops & bishop_mask) &&
             (black_pawns   & pawn_mask) ) {
            return wghts.GetWeight(BISHOP_PATTERN);
        }
    }
    return 0;
}

// ---------- rook_connectiveness_cp_t ----------
// If more than 2 rooks, it ignores the 3rd and 4th rooks etc.
template<Color c>
int GameBoard::rook_connectiveness_cp_t() const
{
    ull rooks = get_pieces_template<Piece::ROOK, c>();

    if ((rooks == 0ULL) || ((rooks & (rooks - 1ULL)) == 0ULL)) {
        return 0;   // No rooks or just one rook
    }

    const ull occupancy = get_pieces();

    const Square s1 = utility::bit::lsb_and_pop_to_square(rooks);
    const Square s2 = utility::bit::lsb_and_pop_to_square(rooks);

    if (((s1 >> 3) == (s2 >> 3)) && clear_between_rank(occupancy, s1, s2)) {
        return wghts.GetWeight(ROOK_CONNECTED);   // Horizontal connection
    }

    if (((s1 & 7) == (s2 & 7)) && clear_between_file(occupancy, s1, s2)) {
        return wghts.GetWeight(ROOK_CONNECTED);   // Vertical connection
    }

    return 0;
}

// ---------- rooks_file_status_cp_t ----------
// Evaluates rook placement by file openness and alignment with the enemy king.
// File classification:
//   - Open file      (no friendly or enemy pawns):        file_mult = 2
//   - Semi-open file (no friendly pawns, enemy pawns):    file_mult = 1
//   - Closed file    (friendly pawn present):             ignored
//
//   - Additional bonus if enemy king is on the same file:
//         KING_ON_FILE * file_mult * (# rooks on file)
//
template<Color c>
int GameBoard::rooks_file_status_cp_t(const PInfo& pawnInfoF, const PInfo& pawnInfoE)
{
    const ull rooks = get_pieces_template<Piece::ROOK, c>();
    if (!rooks) return 0;

    constexpr Color enemy = utility::representation::opposite_color_t<c>;
    const ull enemy_king = get_pieces_template<Piece::KING, enemy>();

    int score_cp = 0;

    for (int file = 0; file < 8; ++file) {
        const ull file_mask = col_masksHA[file];

        const int nRooksOnFile = bits_in(rooks & file_mask);
        if (!nRooksOnFile) continue;

        const bool ownPawnOnFile = (pawnInfoF.file_count[file] != 0);
        const bool oppPawnOnFile = (pawnInfoE.file_count[file] != 0);

        int file_mult = 0;
        if (!ownPawnOnFile && !oppPawnOnFile)       file_mult = 2;
        else if (!ownPawnOnFile &&  oppPawnOnFile)  file_mult = 1;
        else continue;

        score_cp += nRooksOnFile * file_mult * wghts.GetWeight(ROOK_ON_OPEN_FILE);

        if (enemy_king & file_mask) {
            score_cp += nRooksOnFile * file_mult * wghts.GetWeight(KING_ON_FILE);
        }
    }

    return score_cp;
}

// ---------- rook_7th_rankness_cp_t ----------
template<Color c>
int GameBoard::rook_7th_rankness_cp_t()
{
    const ull rooks  = get_pieces_template<Piece::ROOK, c>();
    const ull queens = get_pieces_template<Piece::QUEEN, c>();

    ull bigPieces = rooks | queens;
    if (!bigPieces) return 0;

    constexpr int seventh_rank = (c == Color::WHITE) ? 6 : 1;
    constexpr int eight_rank   = (c == Color::WHITE) ? 7 : 0;

    int n7 = 0;
    int n8 = 0;

    while (bigPieces) {
        Square s = utility::bit::lsb_and_pop_to_square(bigPieces);
        int rnk = s >> 3;
        if (rnk == seventh_rank) n7++;
        if (rnk == eight_rank) n8++;
    }

    int score_cp = 0;
    if (n7 > 0) {
        score_cp += n7 * wghts.GetWeight(MAJOR_ON_RANK7);
        if (n7 >= 2) score_cp += wghts.GetWeight(MAJOR_ON_RANK7);
    }

    if (n8 > 0) {
        score_cp += n8 * wghts.GetWeight(MAJOR_ON_RANK8);
        if (n8 >= 2) score_cp += wghts.GetWeight(MAJOR_ON_RANK8);
    }

    return score_cp;
}


// ---------- any_piece_ahead_on_file_t ----------
template<Color c>
bool GameBoard::any_piece_ahead_on_file_t(int sq, ull file_pieces) const
{
    const int r = sq >> 3;

    ull ranks_ahead;
    if constexpr (c == Color::WHITE) {
        const int start_bit = (r + 1) * 8;
        ranks_ahead = (start_bit >= 64) ? 0ULL : (~0ULL << start_bit);
    } else {
        const int end_bit = r * 8;
        ranks_ahead = (end_bit <= 0) ? 0ULL : ((1ULL << end_bit) - 1ULL);
    }

    return (file_pieces & ranks_ahead) != 0ULL;
}

template<Color c> inline bool GameBoard::enemy_pawn_ahead_on_file_t(int sq, Square enemyAdvancedSq) const
{
    if (enemyAdvancedSq == NO_SQUARE) return false;

    if constexpr (c == Color::WHITE) {
        // enemy pawn closer to promotion square than us
        return enemyAdvancedSq > sq;
    }
    else {
        return enemyAdvancedSq < sq;
    }
}


// ---------- count_isolated_and_doubled_pawns_cp_t ----------
template<Color c>
int GameBoard::count_isolated_and_doubled_pawns_cp_t(const PInfo& pawnInfoF, const PInfo& pawnInfoE) const
{
    int total_cp = 0;

    const unsigned friendly_files_present = pawnInfoF.files_present;
    const bool b_have_majors = get_major_pieces<c>();

    for (int file = 0; file < 8; ++file) {
        const int k = pawnInfoF.file_count[file];
        if (k == 0) continue;

        const bool b_rook_file = (file == 0) || (file == 7);

        // ---------- isolated pawns ----------
        const bool left  = (file < 7) && (friendly_files_present & (1u << (file + 1)));
        const bool right = (file > 0) && (friendly_files_present & (1u << (file - 1)));

        if (!left && !right) {
            int this_cp = k * (b_rook_file ? wghts.GetWeight(ISOLANI_ROOK)
                                           : wghts.GetWeight(ISOLANI));
            if (b_have_majors)
            {
                // only one penalty for open file, even if multiple isolated pawns
                const int sq = pawnInfoF.advancedSq[file];
                if (sq != NO_SQUARE) {
                    // if no enemy pawns ahead on file toward queening square, penalize more
                
                    //const bool is_blocked2 = any_piece_ahead_on_file_t<c>(sq, pawnInfoE.file_bb[file]);
                    const bool is_blocked = enemy_pawn_ahead_on_file_t<c>(sq, pawnInfoE.rearSq[file]);
                    //assert(is_blocked==is_blocked2);

                    if (!is_blocked) this_cp += wghts.GetWeight(ISOLANI_OPEN_FILE);
                }
            }

            total_cp += this_cp;
        }

        // ---------- doubled pawns ----------
        if (k >= 2)
        {
            const int extras = (k - 1);
            int this_cp = extras * (b_rook_file ? wghts.GetWeight(DOUBLED_ROOK)
                                                : wghts.GetWeight(DOUBLED));
            if (pawnInfoE.file_count[file] == 0)
            {
                this_cp += extras * wghts.GetWeight(DOUBLED_OPEN_FILE);
            }
            total_cp += this_cp;
        }
    }

    return total_cp;
}


// ---------- count_pawn_holes_and_passed_pawns_cp_new_t ----------

template<Color c>
void GameBoard::count_pawn_holes_and_passed_pawns_cp_new_t(
        const PInfo& pawnInfoF,
        const PInfo& pawnInfoE,
        ull& holes_bb,
        int& holes_cp,
        ull& passed_pawns,
        int& passed_cp)
{
    holes_bb = 0ULL;
    holes_cp = 0;

    passed_pawns = 0ULL;
    passed_cp = 0;

    ull my_pawns = get_pieces_template<Piece::PAWN, c>();
    if (!my_pawns) return;

    ull all_pawns = get_pieces_template<Piece::PAWN>();

    const unsigned files_present = pawnInfoF.files_present;

    ull tmp = my_pawns;

    while (tmp) {

        Square s = utility::bit::lsb_and_pop_to_square(tmp);

        int f = s & 7;
        int r = s >> 3;

        // ------------------------------------------------------------
        // pawn holes
        // ------------------------------------------------------------

        int hole_sq;

        if constexpr (c == Color::WHITE) {
            if (r == 7) continue;
            hole_sq = s + 8;
        }
        else {
            if (r == 0) continue;
            hole_sq = s - 8;
        }

        if (all_pawns & (1ULL << hole_sq)) continue;

        bool pawn_can_cover = false;

        if (f > 0 && (files_present & (1u << (f - 1)))) {

            int rear = pawnInfoF.rearSq[f - 1];

            if constexpr (c == Color::WHITE) {
                if (rear != NO_SQUARE && ((rear >> 3) <= r)) {
                    pawn_can_cover = true;
                }
            }
            else {
                if (rear != NO_SQUARE && ((rear >> 3) >= r)) {
                    pawn_can_cover = true;
                }
            }
        }

        if (!pawn_can_cover && f < 7 && (files_present & (1u << (f + 1)))) {

            int rear = pawnInfoF.rearSq[f + 1];

            if constexpr (c == Color::WHITE) {
                if (rear != NO_SQUARE && ((rear >> 3) <= r)) {
                    pawn_can_cover = true;
                }
            }
            else {
                if (rear != NO_SQUARE && ((rear >> 3) >= r)) {
                    pawn_can_cover = true;
                }
            }
        }

        if (!pawn_can_cover) {

            holes_bb |= (1ULL << hole_sq);

            int this_cp = wghts.GetWeight(PAWN_HOLE);

            const bool b_have_majors = get_major_pieces<c>();

            if (b_have_majors && pawnInfoE.file_count[f] == 0) {
                this_cp += wghts.GetWeight(PAWN_HOLE_OPEN_FILE);
            }

            holes_cp += this_cp;
        }

        // ------------------------------------------------------------
        // passed pawns (new compact version)
        // ------------------------------------------------------------

        auto enemy_pawn_ahead_by_rank = [r](Square enemy_rear_sq) {
            if (enemy_rear_sq == NO_SQUARE) return false;

            const int enemy_r = enemy_rear_sq >> 3;

            if constexpr (c == Color::WHITE) {
                return enemy_r > r;
            }
            else {
                return enemy_r < r;
            }
        };

        bool enemy_ahead = enemy_pawn_ahead_by_rank(pawnInfoE.rearSq[f]);

        if (!enemy_ahead && f > 0) {
            enemy_ahead = enemy_pawn_ahead_by_rank(pawnInfoE.rearSq[f - 1]);
        }

        if (!enemy_ahead && f < 7) {
            enemy_ahead = enemy_pawn_ahead_by_rank(pawnInfoE.rearSq[f + 1]);
        }

        if (!enemy_ahead) {

            passed_pawns |= (1ULL << s);

            int adv;

            if constexpr (c == Color::WHITE) adv = r;
            else                             adv = (7 - r);

            assert((adv > 0) && (adv < 7));

            int bonus =
                wghts.GetWeight(PASSED_PAWN_SLOPE) * adv * adv
              + wghts.GetWeight(PASSED_PAWN_YINRCPT);

            ull protect_mask = 0ULL;

            if constexpr (c == Color::WHITE) {

                if (r > 0) {
                    if (f < 7) protect_mask |= (1ULL << (s - 7));
                    if (f > 0) protect_mask |= (1ULL << (s - 9));
                }
            }
            else {

                if (r < 7) {
                    if (f < 7) protect_mask |= (1ULL << (s + 9));
                    if (f > 0) protect_mask |= (1ULL << (s + 7));
                }
            }

            if ((my_pawns & protect_mask) != 0ULL) {

                int temp = (bonus * wghts.GetWeight(PASSED_PAWN_CONNECTED));

                bonus = (temp / 3);
            }

            passed_cp += bonus;
        }
    }
}


// ---------- count_knights_on_holes_cp_t ----------
template<Color c>
int GameBoard::count_knights_on_holes_cp_t(ull holes_bb) {
    ull knights = get_pieces_template<Piece::KNIGHT, c>();
    if (!knights || !holes_bb) return 0;

    ull on_holes = knights & holes_bb;
    int n = bits_in(on_holes);

    return (n * wghts.GetWeight(KNIGHT_HOLE));
}


// ---------- king_edgeness_cp_t ----------

// 0 = center, 3 = edge
template<Color c>
int GameBoard::king_edgeness_cp_t()
{
    ull kbb = get_pieces_template<Piece::KING, c>();
    assert(kbb != 0ULL);
    int edgeCode = piece_edgeness(kbb);

    return (edgeCode*wghts.GetWeight(KING_EDGE));
}

// ---------- queenOnCenterSquare_cp_t ----------
template<Color c>
int GameBoard::queenOnCenterSquare_cp_t()
{
    ull q = get_pieces_template<Piece::QUEEN, c>();
    if (q == 0ULL) return 0;

    int itemp = 0;
    itemp += ((q & (1ULL << square_e4)) != 0ULL);
    itemp += ((q & (1ULL << square_d4)) != 0ULL);
    itemp += ((q & (1ULL << square_e5)) != 0ULL);
    itemp += ((q & (1ULL << square_d5)) != 0ULL);

    return itemp * wghts.GetWeight(QUEEN_OUT_EARLY);
}

// ---------- moved_f_pawn_early_cp_t ----------
template<Color c>
int GameBoard::moved_f_pawn_early_cp_t() const
{
    ull p = get_pieces_template<Piece::PAWN, c>();
    if (p == 0ULL) return 0;

    constexpr int startSq = (c == Color::WHITE) ? square_f2 : square_f7;

    int itemp = 0;
    itemp += ((p & (1ULL << startSq)) == 0ULL);

    return itemp * wghts.GetWeight(F_PAWN_MOVED_EARLY);
}

// ---------- counts bishop_blocked_on_both_original_squares_cp_t ----------
template<Color c>
int GameBoard::bishop_blocked_on_both_original_squares_cp_t()
{
    int pointsOff = 0;
    ull bishops = get_pieces_template<Piece::BISHOP, c>();
    if (!bishops) return 0;

    ull pawns = get_pieces_template<Piece::PAWN, c>();
    if (!pawns) return 0;

    if constexpr (c == Color::WHITE) {
        // White king bishop on f1 blocked by pawns on e2 and g2
        if ( (bishops & (1ULL << square_f1)) &&
             (pawns   & (1ULL << square_e2)) &&
             (pawns   & (1ULL << square_g2)) ) {
            pointsOff += wghts.GetWeight(BISHOP_PATTERN);
        }

        // White queen bishop on c1 blocked by pawns on b2 and d2
        if ( (bishops & (1ULL << square_c1)) &&
             (pawns   & (1ULL << square_b2)) &&
             (pawns   & (1ULL << square_d2)) ) {
            pointsOff += wghts.GetWeight(BISHOP_PATTERN);
        }
    } else {
        // Black king bishop on f8 blocked by pawns on e7 and g7
        if ( (bishops & (1ULL << square_f8)) &&
             (pawns   & (1ULL << square_e7)) &&
             (pawns   & (1ULL << square_g7)) ) {
            pointsOff += wghts.GetWeight(BISHOP_PATTERN);
        }

        // Black queen bishop on c8 blocked by pawns on b7 and d7
        if ( (bishops & (1ULL << square_c8)) &&
             (pawns   & (1ULL << square_b7)) &&
             (pawns   & (1ULL << square_d7)) ) {
            pointsOff += wghts.GetWeight(BISHOP_PATTERN);
        }
    }

    return pointsOff;
}


// ---------- get_king_near_squares_t ----------
template<Color c>
int GameBoard::get_king_near_squares_t(int king_near_squares_out[9])
{
    int count = 0;

    ull kbb = get_pieces_template<Piece::KING, c>();
    assert(kbb != 0ULL);    // Must be a king

    ull tmp = kbb;
    Square king_sq = utility::bit::lsb_and_pop_to_square(tmp);

    int king_row = king_sq >> 3;
    int king_col = king_sq & 7;

    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; dc++) {
            int r = king_row + dr;
            int cc = king_col + dc;
            if (r < 0 || r > 7 || cc < 0 || cc > 7)
                continue;
            king_near_squares_out[count++] = r * 8 + cc;
        }
    }

    return count;
}

// ---------- sliders_and_knights_attacking_square2_t ----------
template<Color c>
int GameBoard::sliders_and_knights_attacking_square2_t(int sq)
{
    const ull occ = get_pieces();

    const ull knights = get_pieces_template<Piece::KNIGHT, c>();
    ull attackers = tables::movegen::knight_attack_table[sq] & knights;

    const ull bishops = get_pieces_template<Piece::BISHOP, c>();
    const ull rooks   = get_pieces_template<Piece::ROOK, c>();
    const ull queens  = get_pieces_template<Piece::QUEEN, c>();

    const ull deadly_diags     = bishops | queens;
    const ull deadly_straights = rooks   | queens;

    if (deadly_diags) {
        const ull diag_attacks = get_diagonal_attacks(occ, sq);
        attackers |= (diag_attacks & deadly_diags);
    }

    if (deadly_straights) {
        const ull straight_attacks = get_straight_attacks(occ, sq);
        attackers |= (straight_attacks & deadly_straights);
    }

    return bits_in(attackers);
}

// ---------- attackers_on_enemy_king_near_cp_t ----------
template<Color c>
int GameBoard::attackers_on_enemy_king_near_cp_t()
{
    constexpr Color defender_color = utility::representation::opposite_color_t<c>;

    int king_near_squares[9];
    int count = get_king_near_squares_t<defender_color>(king_near_squares);

    int total = 0;

    for (int i = 0; i < count; ++i) {
        int sq = king_near_squares[i];
        int nAttackers = sliders_and_knights_attacking_square2_t<c>(sq);
        total += nAttackers;
        total += pawns_attacking_square_t<c>(sq);
    }

    return (total*wghts.GetWeight(ATTACKERS_ON_KING));
}

// ---------- kings_far_apart_t ----------
template<Color c>
double GameBoard::kings_far_apart_t() {

    double dBonus = 0.0;

    assert(white_king != 0ULL);
    assert(black_king != 0ULL);

    ull enemyPieces;
    ull tmpEnemy;
    ull tmpFrien;

    if constexpr (c == Color::WHITE) {
        enemyPieces = (black_knights | black_bishops | black_pawns | black_rooks | black_queens);
        tmpEnemy = black_king;
        tmpFrien = white_king;
    } else {
        enemyPieces = (white_knights | white_bishops | white_pawns | white_rooks | white_queens);
        tmpEnemy = white_king;
        tmpFrien = black_king;
    }

    Square enemyKingSq = utility::bit::lsb_and_pop_to_square(tmpEnemy);
    assert(enemyKingSq <= 63);
    Square frienKingSq = utility::bit::lsb_and_pop_to_square(tmpFrien);
    assert(frienKingSq >= 0);

    if (enemyPieces == 0) {
        double dFakeDist = distance_between_squares(enemyKingSq, frienKingSq);
        assert (dFakeDist >=  2.0);
        assert (dFakeDist <= MAX_DIST);
        dBonus = dFakeDist;
        return dBonus;
    }

    return dBonus;
}

// ---------- kings_close_toegather_cp_t ----------
template<Color c>
double GameBoard::kings_close_toegather_cp_t() {
    double dkk = kings_far_apart_t<c>();

    double dFarness = MAX_DIST - dkk;
    assert (dFarness>=0.0);
    return (int)(dFarness * wghts.GetWeight(KINGS_CLOSE_TOGETHER));
}

// ---------- king_centerness_cp_t ----------
template<Color c>
double GameBoard::king_centerness_cp_t() {
    //double dkk = kings_far_apart_t<c>();

    int itemp = king_center_manhattan_dist_t<c>();

    int iFarness = 6 - itemp;
    assert (iFarness>=0.0);
    return (iFarness * wghts.GetWeight(KING_CENTER_LATE));
}


// ---------- hasNoMajorPieces_t ----------
template<Color c>
bool GameBoard::hasNoMajorPieces_t() {
    ull enemyBigPieces;
    ull enemySmallPieces;
    if constexpr (c == Color::BLACK) {
        enemyBigPieces = (black_rooks | black_queens);
        enemySmallPieces = (black_knights | black_bishops);
    } else {
        enemyBigPieces = (white_rooks | white_queens);
        enemySmallPieces = (white_knights | white_bishops);
    }
    if (enemyBigPieces) return false;
    if (bits_in(enemySmallPieces) > 1) return false;
    return true;
}

// ---------- is_knight_on_edge_cp_t ----------
template<Color c>
int GameBoard::is_knight_on_edge_cp_t() {
    int pointsOff=0;
    ull knghts = get_pieces_template<Piece::KNIGHT, c>();
    if (!knghts) return 0;

    while (knghts) {
        Square s  = utility::bit::lsb_and_pop_to_square(knghts);
        const int f = s & 7;
        const int r = s >> 3;
        if ((f==0) || (f==7)) pointsOff += wghts.GetWeight(KNIGHT_ON_EDGE);
        if ((r==0) || (r==7)) pointsOff += wghts.GetWeight(KNIGHT_ON_EDGE);
    }
    return pointsOff;
}

// ---------- development_opening_cp_t ----------
template<Color c>
int GameBoard::development_opening_cp_t() {
    const ull wq = get_pieces_template<Piece::QUEEN, Color::WHITE>();
    const ull bq = get_pieces_template<Piece::QUEEN, Color::BLACK>();

    if (!wq || !bq) return 0;

    ull start_knights = 0;
    ull start_bishops = 0;

    if constexpr (c == Color::WHITE) {
        start_knights = (1ULL << square_b1) | (1ULL << square_g1);
        start_bishops = (1ULL << square_c1) | (1ULL << square_f1);
    } else {
        start_knights = (1ULL << square_b8) | (1ULL << square_g8);
        start_bishops = (1ULL << square_c8) | (1ULL << square_f8);
    }

    const ull my_knights = get_pieces_template<Piece::KNIGHT, c>();
    const ull my_bishops = get_pieces_template<Piece::BISHOP, c>();

    const int knights_on_start = bits_in(my_knights & start_knights);
    const int bishops_on_start = bits_in(my_bishops & start_bishops);

    const int knights_total = bits_in(my_knights);
    const int bishops_total = bits_in(my_bishops);

    const int developed = (knights_total - knights_on_start) + (bishops_total - bishops_on_start);
    if (developed <= 0) return 0;

    return developed * wghts.GetWeight(DEVELOPMENT_OPENING);
}

// ---------- rook_endgame_keep_rooks_when_down_cp_t ----------
// Trading
// In pure rook endings, the side that is down pawns often wants to keep rooks
// on the board, because rook endings are drawish.  So if side c is down one
// pawn or more, give side c a small bonus for the rooks still being present.
// If the rooks get traded, this bonus disappears, which discourages the trade.
//
template<Color c>
int GameBoard::rook_endgame_keep_rooks_when_down_cp_t()
{
    if (white_queens || black_queens) return 0;
    //if (white_knights || black_knights) return 0;
    //if (white_bishops || black_bishops) return 0;

    if (Bits_In[ShumiChess::WHITE][ShumiChess::ROOK] != 1) return 0;
    if (Bits_In[ShumiChess::BLACK][ShumiChess::ROOK] != 1) return 0;

    const int white_pawn_count = Bits_In[ShumiChess::WHITE][ShumiChess::PAWN];
    const int black_pawn_count = Bits_In[ShumiChess::BLACK][ShumiChess::PAWN];

    int pawn_deficit;
    if constexpr (c == Color::WHITE) {
        pawn_deficit = black_pawn_count - white_pawn_count;
    } else {
        pawn_deficit = white_pawn_count - black_pawn_count;
    }

    if (pawn_deficit <= 0) return 0;            // return now if I'm ahead pawns
    return wghts.GetWeight(KEEP_ROOKS_WHEN_DOWN_PAWN);
}

// ---------- opposite_bishops_cp_t ----------

// white is ahead and c == WHITE -> return negative penalty
// white is ahead and c == BLACK -> return 0
// black is ahead and c == BLACK -> return negative penalty
// black is ahead and c == WHITE -> return 0
template<Color c>
int GameBoard::opposite_bishops_cp_t(Score material_balance_abs)
{
    if (white_queens || black_queens) return 0;
    if (material_balance_abs == 0) return 0;

    const bool white_ahead = (material_balance_abs > 0);
    if constexpr (c == Color::WHITE) {
        if (!white_ahead) return 0;
    } else {
        if (white_ahead) return 0;
    }

    const int weight = wghts.GetWeight(OPPOSITE_BISHOPS);
    const Score abs_material_balance = (material_balance_abs < 0) ? -material_balance_abs : material_balance_abs;
    const int lead_cp = (int)(abs_material_balance * 100.0 + 0.5);
    constexpr int knee_cp = 50;
    const int penalty = (weight * lead_cp + ((lead_cp + knee_cp) / 2)) / (lead_cp + knee_cp);
    return -penalty;
}

// ============================================================================
// Explicit template instantiations
// ============================================================================

// bHasCastled_fake_t
template bool GameBoard::bHasCastled_fake_t<Color::WHITE>(int k_rank, int k_file) const;
template bool GameBoard::bHasCastled_fake_t<Color::BLACK>(int k_rank, int k_file) const;

// get_castled_bonus_cp_t
template int GameBoard::get_castled_bonus_cp_t<Color::WHITE>(int,const PInfo& PInfoIn) const;
template int GameBoard::get_castled_bonus_cp_t<Color::BLACK>(int,const PInfo& PInfoIn) const;

template int GameBoard::get_material_for_color_t<Color::WHITE>(int& cp_pawns_only);
template int GameBoard::get_material_for_color_t<Color::BLACK>(int& cp_pawns_only);
template int GameBoard::get_material_for_color2_t<Color::WHITE>(int& cp_pawns_only);
template int GameBoard::get_material_for_color2_t<Color::BLACK>(int& cp_pawns_only);

// pawns_attacking_square_t
template int GameBoard::pawns_attacking_square_t<Color::WHITE>(int);
template int GameBoard::pawns_attacking_square_t<Color::BLACK>(int);
template int GameBoard::pawns_attacking_squares_t<Color::WHITE>(ull bitboard);
template int GameBoard::pawns_attacking_squares_t<Color::BLACK>(ull bitboard);

// pawns_attacking_center_squares_cp_t
template int GameBoard::pawns_attacking_center_squares_cp_t<Color::WHITE>();
template int GameBoard::pawns_attacking_center_squares_cp_t<Color::BLACK>();
template int GameBoard::pawns_attacking_center_squares_cp_fast_t<Color::WHITE>();
template int GameBoard::pawns_attacking_center_squares_cp_fast_t<Color::BLACK>();


// knights_attacking_square_t
template int GameBoard::knights_attacking_square_t<Color::WHITE>(int);
template int GameBoard::knights_attacking_square_t<Color::BLACK>(int);

// knights_attacking_center_squares_cp_t
template int GameBoard::knights_attacking_center_squares_cp_t<Color::WHITE>();
template int GameBoard::knights_attacking_center_squares_cp_t<Color::BLACK>();

// bishops_attacking_square_t
template int GameBoard::bishops_attacking_square_t<Color::WHITE>(int);
template int GameBoard::bishops_attacking_square_t<Color::BLACK>(int);

// bishops_attacking_center_squares_cp_t
template int GameBoard::bishops_attacking_center_squares_cp_t<Color::WHITE>();
template int GameBoard::bishops_attacking_center_squares_cp_t<Color::BLACK>();

// two_bishops_cp_t
template int GameBoard::two_bishops_cp_t<Color::WHITE>(int) const;
template int GameBoard::two_bishops_cp_t<Color::BLACK>(int) const;

// bishop_pawn_pattern_cp_t
template int GameBoard::bishop_pawn_pattern_cp_t<Color::WHITE>();
template int GameBoard::bishop_pawn_pattern_cp_t<Color::BLACK>();

// rook_connectiveness_cp_t
template int GameBoard::rook_connectiveness_cp_t<Color::WHITE>() const;
template int GameBoard::rook_connectiveness_cp_t<Color::BLACK>() const;

// rooks_file_status_cp_t
template int GameBoard::rooks_file_status_cp_t<Color::WHITE>(const PInfo& pawnInfoF, const PInfo& pawnInfoE);
template int GameBoard::rooks_file_status_cp_t<Color::BLACK>(const PInfo& pawnInfoF, const PInfo& pawnInfoE);

// rook_7th_rankness_cp_t
template int GameBoard::rook_7th_rankness_cp_t<Color::WHITE>();
template int GameBoard::rook_7th_rankness_cp_t<Color::BLACK>();

// build_pawn_file_summary_t
template bool GameBoard::build_pawn_file_summary_t<Color::WHITE>(PInfo&);
template bool GameBoard::build_pawn_file_summary_t<Color::BLACK>(PInfo&);

template bool GameBoard::any_piece_ahead_on_file_t<Color::WHITE>(int, ull) const;
template bool GameBoard::any_piece_ahead_on_file_t<Color::BLACK>(int, ull) const;

template int GameBoard::count_knights_on_holes_cp_t<Color::WHITE>(ull);
template int GameBoard::count_knights_on_holes_cp_t<Color::BLACK>(ull);

template int GameBoard::count_isolated_and_doubled_pawns_cp_t<Color::WHITE>(const PInfo& pawnInfoF, const PInfo& pawnInfoE) const;
template int GameBoard::count_isolated_and_doubled_pawns_cp_t<Color::BLACK>(const PInfo& pawnInfoF, const PInfo& pawnInfoE) const;

// template void GameBoard::count_pawn_holes_and_passed_pawns_cp_t<Color::WHITE>(const PInfo& pawnInfoF, const PInfo& pawnInfoE,
//                                                             ull& holes_bb,
//                                                             int& holes_cp,
//                                                             ull& passed_pawns,
//                                                             int& passed_cp);
// template void GameBoard::count_pawn_holes_and_passed_pawns_cp_t<Color::BLACK>(const PInfo& pawnInfoF, const PInfo& pawnInfoE,
//                                                             ull& holes_bb,
//                                                             int& holes_cp,
//                                                             ull& passed_pawns,
//                                                             int& passed_cp);
template void GameBoard::count_pawn_holes_and_passed_pawns_cp_new_t<Color::WHITE>(const PInfo& pawnInfoF, const PInfo& pawnInfoE,
                                                            ull& holes_bb,
                                                            int& holes_cp,
                                                            ull& passed_pawns,
                                                            int& passed_cp);
template void GameBoard::count_pawn_holes_and_passed_pawns_cp_new_t<Color::BLACK>(const PInfo& pawnInfoF, const PInfo& pawnInfoE,
                                                            ull& holes_bb,
                                                            int& holes_cp,
                                                            ull& passed_pawns,
                                                            int& passed_cp);

// king_edgeness_cp_t
template int GameBoard::king_edgeness_cp_t<Color::WHITE>();
template int GameBoard::king_edgeness_cp_t<Color::BLACK>();

// queenOnCenterSquare_cp_t
template int GameBoard::queenOnCenterSquare_cp_t<Color::WHITE>();
template int GameBoard::queenOnCenterSquare_cp_t<Color::BLACK>();

// moved_f_pawn_early_cp_t
template int GameBoard::moved_f_pawn_early_cp_t<Color::WHITE>() const;
template int GameBoard::moved_f_pawn_early_cp_t<Color::BLACK>() const;

// get_king_near_squares_t
template int GameBoard::get_king_near_squares_t<Color::WHITE>(int[9]);
template int GameBoard::get_king_near_squares_t<Color::BLACK>(int[9]);

// sliders_and_knights_attacking_square2_t
template int GameBoard::sliders_and_knights_attacking_square2_t<Color::WHITE>(int);
template int GameBoard::sliders_and_knights_attacking_square2_t<Color::BLACK>(int);

// attackers_on_enemy_king_near_cp_t
template int GameBoard::attackers_on_enemy_king_near_cp_t<Color::WHITE>();
template int GameBoard::attackers_on_enemy_king_near_cp_t<Color::BLACK>();

template int GameBoard::rook_endgame_keep_rooks_when_down_cp_t<Color::WHITE>();
template int GameBoard::rook_endgame_keep_rooks_when_down_cp_t<Color::BLACK>();
template int GameBoard::opposite_bishops_cp_t<Color::WHITE>(Score material_balance_abs);
template int GameBoard::opposite_bishops_cp_t<Color::BLACK>(Score material_balance_abs);


template double GameBoard::kings_far_apart_t<Color::WHITE>();
template double GameBoard::kings_far_apart_t<Color::BLACK>();

template int GameBoard::king_center_manhattan_dist_t<Color::WHITE>();
template int GameBoard::king_center_manhattan_dist_t<Color::BLACK>();

// kings_close_toegather_cp_t
template double GameBoard::kings_close_toegather_cp_t<Color::WHITE>();
template double GameBoard::kings_close_toegather_cp_t<Color::BLACK>();

template double GameBoard::king_centerness_cp_t<Color::WHITE>();
template double GameBoard::king_centerness_cp_t<Color::BLACK>();

// hasNoMajorPieces_t
template bool GameBoard::hasNoMajorPieces_t<Color::WHITE>();
template bool GameBoard::hasNoMajorPieces_t<Color::BLACK>();

// is_knight_on_edge_cp_t
template int GameBoard::is_knight_on_edge_cp_t<Color::WHITE>();
template int GameBoard::is_knight_on_edge_cp_t<Color::BLACK>();

// development_opening_cp_t
template int GameBoard::development_opening_cp_t<Color::WHITE>();
template int GameBoard::development_opening_cp_t<Color::BLACK>();


} // end namespace ShumiChess

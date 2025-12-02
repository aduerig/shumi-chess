#include <functional>


#include "engine.hpp"
#include "utility"
#include <cmath>


// NOTE: How is NDEBUG being defined by someone (CMAKE?). I would rather do it in the make apparutus than in source files.
#undef NDEBUG
//#define NDEBUG         // Define (uncomment) this to disable asserts
#include <assert.h>
//#define NO_CASTLING

//#define _DEBUGGING_TO_FILE
bool debugNow = false;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define THREE_TIME_REP           // I should always be defined


using namespace std;

namespace ShumiChess {
Engine::Engine() {
    reset_engine();
    ShumiChess::initialize_rays();

    using namespace std::chrono;
    auto now = high_resolution_clock::now().time_since_epoch();
    auto us  = duration_cast<microseconds>(now).count();
    rng.seed(static_cast<unsigned>(us));

    //cout << "Created new engine" << us << endl;

}

//TODO what is right way to handle popping past default state here?
// NOTE: what is this function?? Why is it so different than the default ctr?
Engine::Engine(const string& fen_notation) : game_board(fen_notation) {
    move_string.reserve(_MAX_MOVE_PLUS_SCORE_SIZE);
}

void Engine::reset_engine() {
    //std::cout << "\x1b[94m    hello world() I'm reset_engine()! \x1b[0m";
    
    // Initialize storage buffers (they are here to avoid extra allocation during the game)
    move_string.reserve(_MAX_MOVE_PLUS_SCORE_SIZE);
    psuedo_legal_moves.reserve(MAX_MOVES); 
    all_legal_moves.reserve(MAX_MOVES);

    ///////////////////////////////////////////////////////////////////////
    //
    // You can override the gameboard setup with fen positions as in: (enter FEN here) FEN enter now. enter fen. FEN setup. Setup the FEN
    //game_board = GameBoard("r1bq1r2/pppppkbQ/7p/8/3P1p2/1PPB1N2/1P3PPP/2KR3R w - - 2 17");       // bad
    //game_board = GameBoard("r1bq1r2/pppppkbQ/7p/8/3P1p2/1PPB1N2/1P3PPP/2KR3R w - - 2 17");      // repeat 3 times test
    
    //game_board = GameBoard("rnb1kbnr/pppppppp/5q2/8/8/5Q2/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
    //game_board = GameBoard("8/4k3/6K1/8/8/1q6/8/8 w - - 0 1");

    //game_board = GameBoard("rnb1kbnr/pppppppp/5q2/8/8/5Q2/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
    //game_board = GameBoard("3qk3/8/8/8/8/8/5P2/3Q1K2 w KQkq - 0 1");
   // game_board = GameBoard("1r6/4k3/6K1/8/8/8/8/8 w - - 0 1");
    //game_board = GamegBoard("");

    // Or you can pick a random simple FEN. (maybe)
    // vector<Move> v;
    // do {  
    //     string stemp = game_board.random_kqk_fen(false);
    //     game_board = GameBoard(stemp);

    //     v = get_legal_moves(ShumiChess::WHITE);
    // } while (v.size() == 0);


    game_board = GameBoard();

    //////////////////////////////////////////////////////////////////////.


    move_history = stack<Move>();

    halfway_move_state = stack<int>();
    halfway_move_state.push(0);

    en_passant_history = stack<ull>();
    en_passant_history.push(0);

    castle_opportunity_history = stack<uint8_t>();
    castle_opportunity_history.push(0b1111);

    game_board.bCastledWhite = false;  // I dont care which side i castled.
    game_board.bCastledBlack = false;  // I dont care which side i castled.

    g_iMove = 0;       // real moves in whole game

    repetition_table.clear();

}

void Engine::reset_engine(const string& fen) {

    //std::cout << "\x1b[94m    hello world() I'm reset_engine(FEN)! \x1b[0m";

    // Initialize storage buffers (they are here to avoid extra allocation later)
    move_string.reserve(_MAX_MOVE_PLUS_SCORE_SIZE);
    psuedo_legal_moves.reserve(MAX_MOVES); 
    all_legal_moves.reserve(MAX_MOVES);

    game_board = GameBoard(fen);

    move_history = stack<Move>();

    halfway_move_state = stack<int>();
    halfway_move_state.push(0);

    en_passant_history = stack<ull>();
    en_passant_history.push(0);

    castle_opportunity_history = stack<uint8_t>();
    castle_opportunity_history.push(0b1111);

    //std::cout << "\x1b[94m    hello world() I'm reset_engine(FEN)! \x1b[0m";
    game_board.bCastledWhite = false;  // I dont care which side i castled.
    game_board.bCastledBlack = false;  // I dont care which side i castled.

    g_iMove = 0;       // real moves in whole game

    repetition_table.clear();
    
}

// understand why this is ok (vector can be returned even though on stack), move ellusion? 
// https://stackoverflow.com/questions/15704565/efficient-way-to-return-a-stdvector-in-c



vector<Move> Engine::get_legal_moves() {
    Color color = game_board.turn;
    return get_legal_moves(color);
}

vector<Move> Engine::get_legal_moves(Color color) {

    //Color color = game_board.turn;

    // These are kept as class members, rather than local vriables, for speed reasons only.
    // No need for benchmarks, its plain faster.
    psuedo_legal_moves.clear(); 
    all_legal_moves.clear();


    //
    // Get "psuedo_legal" moves. Those that does not put the king in check, and do not 
    // cross the king over a "checked square".

    get_psuedo_legal_moves(color, psuedo_legal_moves);
    
    for (const Move& move : psuedo_legal_moves) {

        bool bKingInCheck = in_check_after_move(color, move);
        
        if (!bKingInCheck) {
            // King is NOT in check after making the move

            // Add this move to the list of legal moves.
            all_legal_moves.emplace_back(move);
        }

    
    }

    return all_legal_moves;
}

// Doesn't factor in check 
 void Engine::get_psuedo_legal_moves(Color color, vector<Move>& all_psuedo_legal_moves) {
    //vector<Move> all_psuedo_legal_moves;
    
    add_knight_moves_to_vector(all_psuedo_legal_moves, color); 
    add_bishop_moves_to_vector(all_psuedo_legal_moves, color); 
    add_pawn_moves_to_vector(all_psuedo_legal_moves, color); 
    add_queen_moves_to_vector(all_psuedo_legal_moves, color); 
    add_king_moves_to_vector(all_psuedo_legal_moves, color); 
    add_rook_moves_to_vector(all_psuedo_legal_moves, color); 

    return;  // all_psuedo_legal_moves;
}

int Engine::get_minor_piece_move_number (const vector <Move> mvs)
{
    int iReturn = 0;
    for (Move m : mvs) {

        if  ( (m.piece_type == KNIGHT) ||  (m.piece_type == BISHOP) ) {
            iReturn++;
        }
    }
    return iReturn;
}

// Does not take into account castling crossing a checked square? Yes, is_square_in_check() does this.
bool Engine::is_king_in_check(const ShumiChess::Color& color) {
    ull friendly_king = this->game_board.get_pieces_template<Piece::KING>(color);

    bool bReturn =  is_square_in_check(color, friendly_king);
     
    return bReturn;
}

//
// Determines if a square is in check.
// All bitboards are asssummed to be "h1=0". 
bool Engine::is_square_in_check(const ShumiChess::Color& color, const ull& square) {

    //Color enemy_color = utility::representation::opposite_color(color);
    Color enemy_color = (Color)(color ^ 1);   // faster than the above

    const ull themQueens   = game_board.get_pieces_template<Piece::QUEEN> (enemy_color);
    const ull themRooks    = game_board.get_pieces_template<Piece::ROOK>  (enemy_color);
    const ull themBishops  = game_board.get_pieces_template<Piece::BISHOP>(enemy_color);
    const ull themKnights  = game_board.get_pieces_template<Piece::KNIGHT>(enemy_color);
    const ull themPawns    = game_board.get_pieces_template<Piece::PAWN>  (enemy_color);
    const ull themKing     = game_board.get_pieces_template<Piece::KING>  (enemy_color);

    const int sq = utility::bit::bitboard_to_lowest_square(square);

    // knights that can reach (capture to) this square
    const ull reachable_knights = tables::movegen::knight_attack_table[sq];
    if (reachable_knights & themKnights) return true;   // Knight attacks this square

    // kings that can reach (capture to) this square
    const ull reachable_kings   = tables::movegen::king_attack_table[sq];
    if (reachable_kings   & themKing)    return true;   // King attacks this square

    // pawns that can reach (capture to) this square
    ull temp;
    if (color == Color::WHITE) {
        temp = (square & ~row_masks[7]) << 8;
    }
    else {
        temp = (square & ~row_masks[0]) >> 8;
    }
    
    // OLD wrong code that causes "Edge and king bug", FEN: 5r1k/1R5p/8/p2P4/1KP1P3/1P6/P7/8 w - a6 0 42
    // ull reachable_pawns = (((temp & ~col_masks[7]) << 1) | 
    //                        ((temp & ~col_masks[0]) >> 1));

    // New code fix?
    ull reachable_pawns = (((temp & ~col_masks[0]) << 1) | 
                           ((temp & ~col_masks[7]) >> 1));                           

    if (reachable_pawns   & themPawns)   return true;    // Pawn attacks this square


    // Note: ? probably don't need knights here cause pins cannot happen with knights, but 
    //   we don't check if king is in check yet
    ull straight_attacks_from_king = get_straight_attacks(square);
    // cout << "diagonal_attacks_from_king: " << square << endl;
    ull diagonal_attacks_from_king = get_diagonal_attacks(square);

    // bishop, rook, queens that can reach (capture to) this square
    ull deadly_diags = themQueens | themBishops;
    ull deadly_straight = themQueens| themRooks;

    if (deadly_straight & straight_attacks_from_king) return true;    // for queen and rook straight attacks
    if (deadly_diags    & diagonal_attacks_from_king) return true;    // for queen and bishop straight attacks

    return false;

}

////////////////////////////////////////////////////////////////////////////////////////////////
// TODO should this check for draws by internally calling get legal moves and caching that and 
//      returning on the actual call?, very slow calling get_legal_moves again
// NOTE: I complely agree this is wastefull. But it is not called in the "main line", the one below is.
//   This one is called only during testing?
GameState Engine::game_over() {
    vector<Move> legal_moves = get_legal_moves();
    return game_over(legal_moves);
}

GameState Engine::game_over(vector<Move>& legal_moves) {
    if (legal_moves.size() == 0) {
        if ( (!game_board.white_king) || (is_square_in_check(Color::WHITE, game_board.white_king)) ) {
            return GameState::BLACKWIN;     // Checkmate
        } else if ( (!game_board.black_king) || (is_square_in_check(Color::BLACK, game_board.black_king)) ) {
            return GameState::WHITEWIN;     // Checkmate
        }
        else {
            //reason_for_draw = "stalemate";
            if (debugNow) cout<<"stalemate" << endl;
            return GameState::DRAW;    //  Draw by Stalemate
        }
    }
    else if (game_board.halfmove >= 50) {     // NOTE: debug only
        //  After fifty  or 50 "ply" or half moves, without a pawn move or capture, its a draw.
        //cout << "Draw by 50-move rule at ply " << game_board.halfmove ;   50 move rule here
        //reason_for_draw = "50 move rule";
        //cout<<"50 move rule" << endl;
        return GameState::DRAW;           // draw by 50 move rule

    } else {
        // Three move repetition draw

        // Insuffecient material   Note: what about two bishops and bishop and knight
        bool isOverThatWay = game_board.insufficient_material_simple();
        if (isOverThatWay) {
            if (debugNow) cout<<"no material" << endl;
            return GameState::DRAW;
        }


        #ifdef THREE_TIME_REP
            auto it = repetition_table.find(game_board.zobrist_key);
            if (it != repetition_table.end()) {
                //assert(0);
                if (it->second >= 3) {
                    // We've seen this exact position (same zobrist) at least twice already
                    // along the current line. That means we are in a repetition loop.
                    //std::cout << "\x1b[31m3-time-rep\x1b[0m" << std::endl;
                    //assert(0);
                    //reason_for_draw = "3 time rep";
                    if (debugNow) cout<<"3-time-rep"<< endl;
                    return GameState::DRAW;
                }
            }
        #endif
    

    }


    return GameState::INPROGRESS;
}


int Engine::get_draw_status() {
    //cout << reason_for_draw << endl;
    //cout << "ouch" << endl;

    int material_centPawns = 0;
    int pawns_only_centPawns = 0;

    Color for_color = Color::WHITE;     // This makes the score absolute

    for (const auto& color1 : array<Color, 2>{Color::WHITE, Color::BLACK}) {

        // Get the centipawn value for this color
        int cp_score_mat_temp = game_board.get_material_for_color(color1, pawns_only_centPawns);
        assert (cp_score_mat_temp>=0);    // no negative value pieces
        if (color1 != for_color) cp_score_mat_temp *= -1;

        material_centPawns += cp_score_mat_temp;

    }

    return (material_centPawns/100);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

// takes a move, but tracks it so pop() can undo
void Engine::pushMove(const Move& move) {

    // Used for castling only
    int rook_from_sq = -1;
    int rook_to_sq = -1;

    assert(move.piece_type != NONE);

    move_history.push(move);

    // Switch color
    this->game_board.turn = utility::representation::opposite_color(move.color);

    // zobrist_key "push" update (side to move)
    game_board.zobrist_key ^= zobrist_side;

    // Update full move "clock" (used for display)
    this->game_board.fullmove += static_cast<int>(move.color == ShumiChess::Color::BLACK); //Fullmove incs on white only
  
    // Push full move "clock"
    this->halfway_move_state.push(this->game_board.halfmove);

    // Update half move status (used only to apply the "fifty-move draw")
    ++this->game_board.halfmove;
    if(move.piece_type == ShumiChess::Piece::PAWN) {
        this->game_board.halfmove = 0;
    }
    
    // Remove the piece from where it was
    ull& moving_piece = access_pieces_of_color(move.piece_type, move.color);
    moving_piece &= ~move.from;

    // Returns the number of trailing zeros in the binary representation of a 64-bit integer.
    int square_from = utility::bit::bitboard_to_lowest_square(move.from);
    int square_to   = utility::bit::bitboard_to_lowest_square(move.to);

      // zobrist_key "push" update (for normal moves, remove piece from from square)
      game_board.zobrist_key ^= zobrist_piece_square_get(move.piece_type + move.color * 6, square_from);

    // Put the piece where it will go.
    if (move.promotion == Piece::NONE) {

        moving_piece |= move.to;

        // zobrist_key "push" update (for promotions, new piece on square)
        game_board.zobrist_key ^= zobrist_piece_square_get(move.piece_type + move.color * 6, square_to);
    }
    else {
        // Promote the piece
        ull& promoted_piece = access_pieces_of_color(move.promotion, move.color);
        promoted_piece |= move.to;

        // zobrist_key "push" update (for promotions, new piece on square)
        game_board.zobrist_key ^= zobrist_piece_square_get(move.promotion + move.color * 6, square_to);
    }

    if (move.capture != Piece::NONE) {
        // The move is a capture
        this->game_board.halfmove = 0;

        if (move.is_en_passent_capture) {
            // Enpassent capture
            
            // Looks at the rank just "forward" of this pawn, on the same file?
            ull target_pawn_bitboard = (move.color == ShumiChess::Color::WHITE ? move.to >> 8 : move.to << 8);

            // Gets the number of leading zeros in the pawn butboard. So this is the first pawn in the list?
            int target_pawn_square = utility::bit::bitboard_to_lowest_square(target_pawn_bitboard);
            access_pieces_of_color(move.capture, utility::representation::opposite_color(move.color)) &= ~target_pawn_bitboard;

            game_board.zobrist_key ^= zobrist_piece_square_get(move.capture + utility::representation::opposite_color(move.color) * 6, target_pawn_square);

        } else {
            // Regular capture

            // remove piece from where it was.
            ull& where_I_was = access_pieces_of_color(move.capture, utility::representation::opposite_color(move.color));
            where_I_was &= ~move.to;
            
            game_board.zobrist_key ^= zobrist_piece_square_get(move.capture + utility::representation::opposite_color(move.color) * 6, square_to);
        }
    } else if (move.is_castle_move) {

        // if pushing a castle turn castling status on in gameboard.
        if (move.color == ShumiChess::Color::WHITE) {
           game_board.bCastledWhite = true;  // I dont care which side i castled.
        } else {
           game_board.bCastledBlack = true;  // I dont care which side i castled.
        }

        ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        //TODO  Figure out the generic 2 if (castle side) solution, not 4 (castle side x color)
        // cout << "PUSHING: Friendly rooks are:";
        // utility::representation::print_bitboard(friendly_rooks);
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       RNBQKBNR
            // Queenside Castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                rook_from_sq = 7;
                rook_to_sq = 4;
                friendly_rooks &= ~(1ULL<<7);
                friendly_rooks |= (1ULL<<4);
            } else {
                rook_from_sq = 63;
                rook_to_sq = 60;               
                friendly_rooks &= ~(1ULL<<63);
                friendly_rooks |= (1ULL<<60);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
             //                rnbqkbnr                                                       RNBQKBNR
            // Kingside castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                rook_from_sq = 0;
                rook_to_sq = 2;
                friendly_rooks &= ~(1ULL<<0);
                friendly_rooks |= (1ULL<<2);
                //assert (this->game_board.white_castle_rights);
            } else {
                rook_from_sq = 56;
                rook_to_sq = 58;                
                friendly_rooks &= ~(1ULL<<56);
                friendly_rooks |= (1ULL<<58);
            }
        } else {        // Something wrong, its not a castle
            assert(0);
        }
    }

    this->en_passant_history.push(this->game_board.en_passant_rights);

    // Zobrist: remove old en passant (if any)
    if (this->game_board.en_passant_rights) {
        int old_ep_sq   = utility::bit::bitboard_to_lowest_square(this->game_board.en_passant_rights);
        int old_ep_file = old_ep_sq & 7;  // file index 0..7
        game_board.zobrist_key ^= zobrist_enpassant[old_ep_file];
    }

    this->game_board.en_passant_rights = move.en_passant_rights;

    // Zobrist: add new en passant (if any)
    if (this->game_board.en_passant_rights) {
        int new_ep_sq   = utility::bit::bitboard_to_lowest_square(this->game_board.en_passant_rights);
        int new_ep_file = new_ep_sq & 7;
        game_board.zobrist_key ^= zobrist_enpassant[new_ep_file];
    }


    
    // Manage castling rights
    uint8_t castle_rights = (this->game_board.black_castle_rights << 2) | this->game_board.white_castle_rights;
    this->castle_opportunity_history.push(castle_rights);
    
    this->game_board.black_castle_rights &= move.black_castle_rights;
    this->game_board.white_castle_rights &= move.white_castle_rights;

    uint8_t castle_new = (this->game_board.black_castle_rights << 2) | this->game_board.white_castle_rights;

    // Zobrist castle rights
    if (castle_new != castle_rights)
    {
        game_board.zobrist_key ^= zobrist_castling[castle_rights];
        game_board.zobrist_key ^= zobrist_castling[castle_new];
    }

    // ---- Zobrist update for the rook hop in castling ----
    // (has to happen after updating the castle rights, just above)
    if (move.is_castle_move) {
        // King squares were already XORed earlier in pushMove(), so we ONLY fix the rook here.
        assert(rook_from_sq >= 0 && rook_to_sq >= 0);
        game_board.zobrist_key ^= zobrist_piece_square_get(ShumiChess::Piece::ROOK + move.color * 6, rook_from_sq);
        game_board.zobrist_key ^= zobrist_piece_square_get(ShumiChess::Piece::ROOK + move.color * 6, rook_to_sq);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////
//
// undos last move   (the opposite of "pushMove()")
// 
void Engine::popMove() {

    int rook_from_sq = -1;
    int rook_to_sq = -1;

    const Move move = this->move_history.top();
    this->move_history.pop();

    // pop enpassent rights off the top of the stack

    // --- undo zobrist for en passant rights ---
    // 1. XOR out the current en passant (the one set by the move we're undoing)
    if (this->game_board.en_passant_rights) {
        int cur_ep_sq   = utility::bit::bitboard_to_lowest_square(this->game_board.en_passant_rights);
        int cur_ep_file = cur_ep_sq & 7;
        game_board.zobrist_key ^= zobrist_enpassant[cur_ep_file];
    }

    // 2. Peek at the previous en passant square from history (do NOT pop yet)
    ull prev_ep_bb = this->en_passant_history.top();
    if (prev_ep_bb) {
        int prev_ep_sq   = utility::bit::bitboard_to_lowest_square(prev_ep_bb);
        int prev_ep_file = prev_ep_sq & 7;
        game_board.zobrist_key ^= zobrist_enpassant[prev_ep_file];
    }

    // 3. Now actually restore en_passant_rights and pop the stack
    this->game_board.en_passant_rights = this->en_passant_history.top();
    this->en_passant_history.pop();

    // Zobrist undo for castling rights
    uint8_t castle_current =
        (this->game_board.black_castle_rights << 2) |
        this->game_board.white_castle_rights;          // rights AFTER the move (what the board has now)
    uint8_t castle_prev = this->castle_opportunity_history.top();  // rights BEFORE the move (what we saved in push)

    if (castle_current != castle_prev)
    {
        game_board.zobrist_key ^= zobrist_castling[castle_current]; // xor OUT current rights
        game_board.zobrist_key ^= zobrist_castling[castle_prev];    // xor IN previous rights
    }

    // pop castle rights off the top of the stack (after merging)
    this->game_board.black_castle_rights = this->castle_opportunity_history.top() >> 2;      // shift 
    this->game_board.white_castle_rights = this->castle_opportunity_history.top() & 0b0011;  // remove black castle bits
    

    // pop castle opportunity history
    this->castle_opportunity_history.pop();

    game_board.zobrist_key ^= zobrist_side;

    this->game_board.turn = move.color;

    // pop move states
    this->game_board.fullmove -= static_cast<int>(move.color == ShumiChess::Color::BLACK);
    this->game_board.halfmove = this->halfway_move_state.top();
    this->halfway_move_state.pop();

    int square_from = utility::bit::bitboard_to_lowest_square(move.from);
    int square_to   = utility::bit::bitboard_to_lowest_square(move.to);

    // pop the "actual move". This removes the piece from its square and puts it back to where it was.
    ull& moving_piece = access_pieces_of_color(move.piece_type, move.color);
    moving_piece &= ~move.to;
    moving_piece |= move.from;

    assert((move.piece_type + move.color * 6) < 12);
    game_board.zobrist_key ^= zobrist_piece_square_get(move.piece_type + move.color * 6, square_from);

    // pop pawn promotions
    if (move.promotion == Piece::NONE) {
        // Not a pawn promotion
        assert((move.piece_type + move.color * 6) < 12);
        game_board.zobrist_key ^= zobrist_piece_square_get(move.piece_type + move.color * 6, square_to);
    }
    else {
        // Is a pawn promotion
        ull& promoted_piece = access_pieces_of_color(move.promotion, move.color);
        promoted_piece &= ~move.to;

        assert((move.piece_type + move.color * 6) < 12);
        game_board.zobrist_key ^= zobrist_piece_square_get(move.promotion + move.color * 6, square_to);
    }

    if (move.capture != Piece::NONE) {

        if (move.is_en_passent_capture) {
            // Looks at the rank just "forward" of this pawn, on the same file?
            ull target_pawn_bitboard = move.color == ShumiChess::Color::WHITE ? move.to >> 8 : move.to << 8;

            int target_pawn_square = utility::bit::bitboard_to_lowest_square(target_pawn_bitboard);
            // NOTE: here the statements are reversed from the else. What gives?
            game_board.zobrist_key ^= zobrist_piece_square_get(move.capture + utility::representation::opposite_color(move.color) * 6, target_pawn_square);
          
            access_pieces_of_color(move.capture, utility::representation::opposite_color(move.color)) |= target_pawn_bitboard;
    
        } else {

            access_pieces_of_color(move.capture, utility::representation::opposite_color(move.color)) |= move.to;
            game_board.zobrist_key ^= zobrist_piece_square_get(move.capture + utility::representation::opposite_color(move.color) * 6, square_to);
        }
    } else if (move.is_castle_move) {
       
        // if popping a castle turn castling status off in gameboard.
        if (move.color == ShumiChess::Color::WHITE) {
           game_board.bCastledWhite = false;  // I dont care which side i castled.
        } else {
           game_board.bCastledBlack = false;  // I dont care which side i castled.
        }

        // get pointer to the rook? Which rook?
        ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        // ! Bet we can make this part of push a func and do something fancy with to and from
        //  at least keep standard with push implimentation.

        // the castles move has not been popped yet
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       rnbqkbnr   
            // Popping a Queenside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                rook_from_sq = 4;
                rook_to_sq = 7;
                friendly_rooks &= ~(1ULL<<4);
                friendly_rooks |= (1ULL<<7);
            } else {
                rook_from_sq = 60;
                rook_to_sq = 63;
                friendly_rooks &= ~(1ULL<<60);
                friendly_rooks |= (1ULL<<63);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
            //                 rnbqkbnr                                                       rnbqkbnr
            // Popping a Kingside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                rook_from_sq = 2;
                rook_to_sq = 0;
                friendly_rooks &= ~(1ULL<<2);    // Remove white king rook from f1
                friendly_rooks |= (1ULL<<0);     // Add white king rook back to a1
            } else {
                rook_from_sq = 58;
                rook_to_sq = 56;
                friendly_rooks &= ~(1ULL<<58);
                friendly_rooks |= (1ULL<<56);
            }

            
        } else {
            // Something wrong, its not a castle
            assert(0);
        }

        // Put the rook back (in the Zobrist)
        game_board.zobrist_key ^= zobrist_piece_square_get(ShumiChess::Piece::ROOK + move.color * 6, rook_from_sq);
        game_board.zobrist_key ^= zobrist_piece_square_get(ShumiChess::Piece::ROOK + move.color * 6, rook_to_sq);

    }


}


// ---------------------------------------------------------------------------
// Fast push: inverse of popMoveFast()
// Same as pushMove(), except no updates to castling, Zobrist, en passant, move clocks, repetition, etc.
// Designed for use to look for check. We really on the rest of the engine to prevent 
// pushMoveFast() from seeing a position that castles into check.
// ---------------------------------------------------------------------------

void Engine::pushMoveFast(const Move& move)
{
    assert(move.piece_type != Piece::NONE);

    // Use the same move_history as regular push
    move_history.push(move);

    // Flip side to move
    game_board.turn = utility::representation::opposite_color(move.color);

    ull from_mask = move.from;
    ull to_mask   = move.to;

    // --- 1. Remove moving piece from origin ---
    ull& src_bb = access_pieces_of_color(move.piece_type, move.color);
    src_bb &= ~from_mask;

    // --- 2. Handle captures ---
    if (move.capture != Piece::NONE) {
        ull& cap_bb = access_pieces_of_color(
            move.capture,
            utility::representation::opposite_color(move.color));

        if (move.is_en_passent_capture) {
            ull behind_mask = (move.color == Color::WHITE)
                                ? (to_mask >> 8)
                                : (to_mask << 8);
            cap_bb &= ~behind_mask;
        } else {
            cap_bb &= ~to_mask;
        }
    }

    // --- 3. Handle promotion or normal placement ---
    if (move.promotion != Piece::NONE) {
        ull& promo_bb = access_pieces_of_color(move.promotion, move.color);
        promo_bb |= to_mask;
    } else {
        src_bb |= to_mask;
    }

    if (move.is_castle_move) {

        // if pushing a castle turn castling status on in gameboard.
        if (move.color == ShumiChess::Color::WHITE) {
           game_board.bCastledWhite = true;  // I dont care which side i castled.
        } else {
           game_board.bCastledBlack = true;  // I dont care which side i castled.
        }

        ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        //
        //TODO  Figure out the generic 2 if (castle side) solution, not 4 (castle side x color)
        // cout << "PUSHING: Friendly rooks are:";
        // utility::representation::print_bitboard(friendly_rooks);
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       RNBQKBNR
            // Queenside Castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<7);
                friendly_rooks |= (1ULL<<4);
            } else {           
                friendly_rooks &= ~(1ULL<<63);
                friendly_rooks |= (1ULL<<60);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
             //                rnbqkbnr                                                       RNBQKBNR
            // Kingside castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<0);
                friendly_rooks |= (1ULL<<2);
                //assert (this->game_board.white_castle_rights);
            } else {
                friendly_rooks &= ~(1ULL<<56);
                friendly_rooks |= (1ULL<<58);
            }
        } else {        // Something wrong, its not a castle
            assert(0);
        }


    }

    // No zobrist, no repetition, no rights, no clocks, etc.
}

// ---------------------------------------------------------------------------
// Fast pop: inverse of pushMoveFast()
//   - Undoes bitboard changes made by the last pushMoveFast()
// Designed for use to look for check. We really depend on the rest of the engine to prevent 
// pushMoveFast() from seeing a position that castles into check.
// ---------------------------------------------------------------------------
void Engine::popMoveFast()
{
    assert(!move_history.empty());
    Move move = move_history.top();
    move_history.pop();

    // Flip side back
    game_board.turn = move.color;

    ull from_mask = move.from;
    ull to_mask   = move.to;

    // --- 1. Undo promotion or normal move ---
    if (move.promotion != Piece::NONE) {
        ull& promo_bb = access_pieces_of_color(move.promotion, move.color);
        promo_bb &= ~to_mask;

        ull& pawn_bb = access_pieces_of_color(Piece::PAWN, move.color);
        pawn_bb |= from_mask;
    } else {
        ull& piece_bb = access_pieces_of_color(move.piece_type, move.color);
        piece_bb &= ~to_mask;
        piece_bb |= from_mask;
    }

    // --- 2. Restore captured piece, if any ---
    if (move.capture != Piece::NONE) {
        ull& cap_bb = access_pieces_of_color(
            move.capture,
            utility::representation::opposite_color(move.color));

        if (move.is_en_passent_capture) {
            ull behind_mask = (move.color == Color::WHITE)
                                ? (to_mask >> 8)
                                : (to_mask << 8);
            cap_bb |= behind_mask;
        }
        else {
            cap_bb |= to_mask;
        }
    }

    if (move.is_castle_move) {

        // if popping a castle turn castling status off in gameboard.
        if (move.color == ShumiChess::Color::WHITE) {
           game_board.bCastledWhite = false;  // I dont care which side i castled.
        } else {
           game_board.bCastledBlack = false;  // I dont care which side i castled.
        }

        //assert(0 && "popMoveFast(): castling should never appear in fast path");
        // get pointer to the rook? Which rook?
        ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        // ! Bet we can make this part of push a func and do something fancy with to and from
        //  at least keep standard with push implimentation.

        // the castles move has not been popped yet
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       rnbqkbnr   
            // Popping a Queenside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<4);
                friendly_rooks |= (1ULL<<7);
            } else {
                friendly_rooks &= ~(1ULL<<60);
                friendly_rooks |= (1ULL<<63);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
            //                 rnbqkbnr                                                       rnbqkbnr
            // Popping a Kingside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<2);    // Remove white king rook from f1
                friendly_rooks |= (1ULL<<0);     // Add white king rook back to a1
            } else {
                friendly_rooks &= ~(1ULL<<58);
                friendly_rooks |= (1ULL<<56);
            }

        } else {
            // Something wrong, its not a castle
            assert(0);
        }

    }

    // No zobrist, no repetition, no rights, no clocks, etc.
}




////////////////////////////////////////////////////////////////////////////////



ull& Engine::access_pieces_of_color(Piece piece, Color color)
{
    switch (piece)
    {
        case Piece::PAWN:
            return (color != 0) ? this->game_board.black_pawns
                                : this->game_board.white_pawns;
        case Piece::ROOK:
            return (color != 0) ? this->game_board.black_rooks
                                : this->game_board.white_rooks;
        case Piece::KNIGHT:
            return (color != 0) ? this->game_board.black_knights
                                : this->game_board.white_knights;
        case Piece::BISHOP:
            return (color != 0) ? this->game_board.black_bishops
                                : this->game_board.white_bishops;
        case Piece::QUEEN:
            return (color != 0) ? this->game_board.black_queens
                                : this->game_board.white_queens;
        case Piece::KING:
            return (color != 0) ? this->game_board.black_king
                                : this->game_board.white_king;
        default:
            std::cout << "Unexpected piece type in access_pieces_of_color: " << piece << std::endl;
            assert(0);
            return this->game_board.white_king;
    }
}




void Engine::add_move_to_vector(vector<Move>& moves, ull single_bitboard_from, ull bitboard_to, Piece piece, Color color
    , bool capture, bool promotion, ull en_passant_rights, bool is_en_passent_capture, bool is_castle) {

    // code to actually pop all the potential squares and add them as moves
    while (bitboard_to) {
        ull single_bitboard_to = utility::bit::lsb_and_pop(bitboard_to);
        Piece piece_captured = Piece::NONE;
        if (capture) {
            if (!is_en_passent_capture) {
                piece_captured = { game_board.get_piece_type_on_bitboard(single_bitboard_to) };
            }
            else {
                piece_captured = Piece::PAWN;
            }
        }

        Move new_move = {};
        new_move.color = color;
        new_move.piece_type = piece;
        new_move.from = single_bitboard_from;
        new_move.to = single_bitboard_to;
        new_move.capture = piece_captured;
        new_move.en_passant_rights = en_passant_rights;
        new_move.is_en_passent_capture = is_en_passent_capture;
        new_move.is_castle_move = is_castle;

        // castling rights
        ull from_or_to = (single_bitboard_from | single_bitboard_to);
        if (from_or_to & 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10001000) {
            new_move.white_castle_rights &= 0b00000001;
        }
        if (from_or_to & 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001001) {
            new_move.white_castle_rights &= 0b00000010;
        }
        if (from_or_to & 0b10001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000) {
            new_move.black_castle_rights &= 0b00000001;
        }
        if (from_or_to & 0b00001001'00000000'00000000'00000000'00000000'00000000'00000000'00000000) {
            new_move.black_castle_rights &= 0b00000010;
        }

        if (!promotion) {
            new_move.promotion = Piece::NONE;
            // Add new move to the list of moves.  
            moves.emplace_back(new_move);
        }
        else {
            // A promotion
            for (auto& promo_piece : promotion_values) {
                Move promo_move = new_move;
                promo_move.promotion = promo_piece;
                // Add new move to the list of moves. 
                moves.emplace_back(promo_move);
            }
        }
    }
}

void Engine::add_pawn_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {    
    // get just pawns of correct color
    ull pawns = game_board.get_pieces_template<Piece::PAWN>(color);

    // grab variables that will be used several times
    ull enemy_starting_rank_mask = row_masks[Row::ROW_8];
    ull pawn_enemy_starting_rank_mask = row_masks[Row::ROW_7];
    ull pawn_starting_rank_mask = row_masks[Row::ROW_2];
    ull pawn_enpassant_rank_mask = row_masks[Row::ROW_3];
    ull far_right_row = col_masks[Col::COL_H];
    ull far_left_row = col_masks[Col::COL_A];

    if (color == Color::BLACK) {
        enemy_starting_rank_mask = row_masks[Row::ROW_1];
        pawn_enemy_starting_rank_mask = row_masks[Row::ROW_2];
        pawn_enpassant_rank_mask = row_masks[Row::ROW_6];
        pawn_starting_rank_mask = row_masks[Row::ROW_7];
        far_right_row = col_masks[Col::COL_A];
        far_left_row = col_masks[Col::COL_H];
    }

    ull all_pieces = game_board.get_pieces();
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));

    while (pawns) {
        // pop and get one pawn bitboard. This gets the pawn, but also removes it from "pawns" but who cares.
        // "pawns" is not an original.
        ull single_pawn = utility::bit::lsb_and_pop(pawns);
        
        // Look to see if square just forward of me is empty.
        ull one_move_forward = utility::bit::bitshift_by_color(single_pawn & ~pawn_enemy_starting_rank_mask, color, 8); 
        ull one_move_forward_not_blocked = one_move_forward & ~all_pieces;
        ull spaces_to_move = one_move_forward_not_blocked;

        add_move_to_vector(all_psuedo_legal_moves, single_pawn, spaces_to_move, Piece::PAWN, color, false, false, 0ULL, false, false);

        // Look for (and add if its there), a move up two ranks
        ull is_doublable = single_pawn & pawn_starting_rank_mask;
        if (is_doublable) {
           
            // Look to see both squares just forward of me is empty. (TODO share more code with single pushes above)
            ull move_forward_one = utility::bit::bitshift_by_color(single_pawn, color, 8);
            ull move_forward_one_blocked = move_forward_one & ~all_pieces;

            ull move_forward_two = utility::bit::bitshift_by_color(move_forward_one_blocked, color, 8);
            ull move_forward_two_blocked = move_forward_two & ~all_pieces;

            add_move_to_vector(all_psuedo_legal_moves, single_pawn, move_forward_two_blocked, Piece::PAWN, color, false, false, move_forward_one_blocked, false, false);
        }

        // Look for (and add if its there), promotions
        ull potential_promotion = utility::bit::bitshift_by_color(single_pawn & pawn_enemy_starting_rank_mask, color, 8); 
        ull promotion_not_blocked = potential_promotion & ~all_pieces;
        ull promo_squares = promotion_not_blocked;
        add_move_to_vector(all_psuedo_legal_moves, single_pawn, promo_squares, Piece::PAWN, color, 
                false, true, 0ULL, false, false);

        // Look for (and add if its there), attacks forward left and forward right, also includes promotions like this
        // "Forward" means away from the pawns' back or "1st" rank.
        ull attack_fleft = utility::bit::bitshift_by_color(single_pawn & ~far_left_row, color, 9);
        ull attack_fright = utility::bit::bitshift_by_color(single_pawn & ~far_right_row, color, 7);
        
        // normal attacks is set to nonzero if enemy pieces are in the pawns "crosshairs"
        ull normal_attacks = attack_fleft & all_enemy_pieces;
        normal_attacks |= attack_fright & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_pawn, normal_attacks, Piece::PAWN, color, 
                     true, (bool) (normal_attacks & enemy_starting_rank_mask), 0ULL, false, false);

        // enpassant attacks
        // TODO improvement here, cause we KNOW that enpassant results in the capture of a pawn, but it adds a lot 
        //      of code here to get the speed upgrade. Works fine as is

        ull enpassant_end_location = (attack_fleft | attack_fright) & game_board.en_passant_rights;
        if (enpassant_end_location) {

            // Returns the number of trailing zeros in the binary representation of a 64-bit integer.
            int origin_pawn_square = utility::bit::bitboard_to_lowest_square(single_pawn);
            int dest_pawn_square = utility::bit::bitboard_to_lowest_square(one_move_forward);
      
            add_move_to_vector(all_psuedo_legal_moves, single_pawn, enpassant_end_location, Piece::PAWN, color, 
                         true, false, 0ULL, true, false);
        }
    }
}

void Engine::add_knight_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull knights = game_board.get_pieces_template<Piece::KNIGHT>(color);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (knights) {
        ull single_knight = utility::bit::lsb_and_pop(knights);
        ull avail_attacks = tables::movegen::knight_attack_table[utility::bit::bitboard_to_lowest_square(single_knight)];
        
        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_knight, enemy_piece_attacks, Piece::KNIGHT, color, true, false, 0ULL, false, false);

        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(all_psuedo_legal_moves, single_knight, non_attack_moves, Piece::KNIGHT, color, false, false, 0ULL, false, false);
    }
}

void Engine::add_rook_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull rooks = game_board.get_pieces_template<Piece::ROOK>(color);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (rooks) {
        ull single_rook = utility::bit::lsb_and_pop(rooks);
        ull avail_attacks = get_straight_attacks(single_rook);

        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_rook, enemy_piece_attacks, Piece::ROOK, color, true, false, 0ULL, false, false);
    
        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(all_psuedo_legal_moves, single_rook, non_attack_moves, Piece::ROOK, color, false, false, 0ULL, false, false);
    }
}

void Engine::add_bishop_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull bishops = game_board.get_pieces_template<Piece::BISHOP>(color);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (bishops) {
        ull single_bishop = utility::bit::lsb_and_pop(bishops);
        ull avail_attacks = get_diagonal_attacks(single_bishop);


        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_bishop, enemy_piece_attacks, Piece::BISHOP, color, true, false, 0ULL, false, false);
    
        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(all_psuedo_legal_moves, single_bishop, non_attack_moves, Piece::BISHOP, color, false, false, 0ULL, false, false);
    }
}

void Engine::add_queen_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull queens = game_board.get_pieces_template<Piece::QUEEN>(color);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (queens) {
        ull single_queen = utility::bit::lsb_and_pop(queens);
        ull avail_attacks = get_diagonal_attacks(single_queen) | get_straight_attacks(single_queen);

        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_queen, enemy_piece_attacks, Piece::QUEEN, color, true, false, 0ULL, false, false);
    
        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(all_psuedo_legal_moves, single_queen, non_attack_moves, Piece::QUEEN, color, false, false, 0ULL, false, false);
    }
}

// assumes 1 king exists per color
void Engine::add_king_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull king = game_board.get_pieces_template<Piece::KING>(color);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));
    ull all_pieces = game_board.get_pieces();
    ull own_pieces = game_board.get_pieces(color);

    ull avail_attacks = tables::movegen::king_attack_table[utility::bit::bitboard_to_lowest_square(king)];

    // captures
    ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
    add_move_to_vector(all_psuedo_legal_moves, king, enemy_piece_attacks, Piece::KING, color, true, false, 0ULL, false, false);

    // non-capture moves
    ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
    add_move_to_vector(all_psuedo_legal_moves, king, non_attack_moves, Piece::KING, color, false, false, 0ULL, false, false);
    

    #ifndef NO_CASTLING

        // castling, NOTE: a Rook must exist in the right place to castle the king.
        ull squares_inbetween;
        ull needed_rook_location;
        ull actual_rooks_location;
        ull king_origin_square;
        
        if (color == Color::WHITE) {
            if (game_board.white_castle_rights & (0b00000001)) {
                //assert(!game_board.bCastledWhite);
                // Move is a White king side castle
                squares_inbetween = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000110;
                if (((squares_inbetween & ~all_pieces) == squares_inbetween) &&
                    !is_square_in_check(color, king) && !is_square_in_check(color, king>>1) && !is_square_in_check(color, king>>2) ) {   
                    // King square, and squares inbetween are empty and not in check
                    needed_rook_location = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001;
                    actual_rooks_location = game_board.get_pieces(Color::WHITE, Piece::ROOK);
                    if (actual_rooks_location & needed_rook_location) {
                        // Rook is on correct square for castling
                        king_origin_square = 1ULL<<1;
                        add_move_to_vector(all_psuedo_legal_moves, king, king_origin_square, Piece::KING, color, false, false, 0ULL, false, true);
                    }
                }
            }
            if (game_board.white_castle_rights & (0b00000010)) {
                // Move is a White queen side castle
                //assert(!game_board.bCastledWhite);
                squares_inbetween = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'01110000;
                if (((squares_inbetween & ~all_pieces) == squares_inbetween) &&
                        !is_square_in_check(color, king) && !is_square_in_check(color, king<<1) && !is_square_in_check(color, king<<2) ) {
                    needed_rook_location = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000000;
                    actual_rooks_location = game_board.get_pieces(Color::WHITE, Piece::ROOK);
                    if (actual_rooks_location & needed_rook_location) {
                        // Rook is on correct square for castling
                        king_origin_square = 1ULL<<5;
                        add_move_to_vector(all_psuedo_legal_moves, king, king_origin_square, Piece::KING, color, false, false, 0ULL, false, true);
                    }
                }
            }
        } else if (color == Color::BLACK) {
            if (game_board.black_castle_rights & (0b00000001)) {
                //assert(!game_board.bCastledBlack);
                // Move is a black king side castle
                squares_inbetween = 0b00000110'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
                if (((squares_inbetween & ~all_pieces) == squares_inbetween) &&
                        !is_square_in_check(color, king) && !is_square_in_check(color, king>>1) && !is_square_in_check(color, king>>2) ) {
                    // King square, and squares inbetween are empty and not in check
                    needed_rook_location = 0b00000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
                    actual_rooks_location = game_board.get_pieces(Color::BLACK, Piece::ROOK);
                    if (actual_rooks_location & needed_rook_location) {
                        // Rook is on correct square for castling
                        king_origin_square = 1ULL<<57;
                        add_move_to_vector(all_psuedo_legal_moves, king, king_origin_square, Piece::KING, color, false, false, 0ULL, false, true);
                    }
                }
            }
            if (game_board.black_castle_rights & (0b00000010)) {
                // Move is a Black queen side castle
                //assert(!game_board.bCastledBlack);
                squares_inbetween = 0b01110000'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
                if (((squares_inbetween & ~all_pieces) == squares_inbetween) &&
                        !is_square_in_check(color, king) && !is_square_in_check(color, king<<1) && !is_square_in_check(color, king<<2) ) {
                    // King square, and squares inbetween are empty and not in check
                    needed_rook_location = 0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
                    actual_rooks_location = game_board.get_pieces(Color::BLACK, Piece::ROOK);
                    if (actual_rooks_location & needed_rook_location) {
                        // Rook is on correct square for castling
                        king_origin_square = 1ULL<<61;
                        add_move_to_vector(all_psuedo_legal_moves, king, king_origin_square, Piece::KING, color, false, false, 0ULL, false, true);
                    }
                }
            }
        } else {
            cout << "Unexpected color in king moves: " << color << endl;
            assert(0);
        }

    #endif

}

//
// engine.cpp
int Engine::bishops_attacking_square(Color c, int sq)
{
    ull bit_target = (1ULL << sq);
    ull rays       = get_diagonal_attacks(bit_target);  // your existing diagonal (bishop) attack generator
    ull bishops    = game_board.get_pieces_template<Piece::BISHOP>(c);
    return game_board.bits_in(rays & bishops);
}

int Engine::bishops_attacking_center_squares(Color c)
{
    int itemp = 0;
    itemp += bishops_attacking_square(c, game_board.square_e4);
    itemp += bishops_attacking_square(c, game_board.square_d4);
    itemp += bishops_attacking_square(c, game_board.square_e5);
    itemp += bishops_attacking_square(c, game_board.square_d5);
    return itemp;
}



// inline void safe_push_back(std::string &s, char c) {
//     if (s.size() < s.capacity()) s.push_back(c); else assert(0);
//}
inline void safe_push_back(std::string &s, char c) {
    s.push_back(c);   // always works
}
//
// Translates a "Move" to SAN (standard algebriac notation).
// Dad's first ShumiChess function.
// Does check, mate, promotion, and disambiguation. 
//   Only thing I added was '~' for forced draws (50 move, or 3-time).
//
void Engine::bitboards_to_algebraic(ShumiChess::Color color_that_moved, const ShumiChess::Move the_move
                            , GameState state 
                            , bool isCheck
                            //, const vector<ShumiChess::Move>* p_legal_moves   // from this position. This is only used for disambigouation
                            , bool bPadTrailing
                            , std::string& MoveText)            // output
{
    char thisChar;
   
    MoveText.clear();        // start fresh (does NOT free capacity)


    // MoveText += '[';
    // MoveText += (isCheck ? '1' : '0'); 
    // MoveText += ']';

    if (the_move.piece_type == Piece::NONE) {
        MoveText += "none";
    } else {

        bool b_is_pawn_move = (the_move.piece_type == Piece::PAWN);

        if (b_is_pawn_move) {
            // For pawn move we give the ".from" file, and omit the "p"
            thisChar = file_from_move(the_move);
            safe_push_back(MoveText,thisChar);
        } else {
            // Add the correct piece character
            thisChar = get_piece_char(the_move.piece_type);
            safe_push_back(MoveText,thisChar);
        }

        // disambiguation ??? Code should live here
        //if (legal_moves && !legal_moves->empty()) {
    

        // Add "capture" character
        bool b_is_capture = (the_move.capture != Piece::NONE);
        if (b_is_capture) safe_push_back(MoveText,'x');


        // Add ".to" information
        if (b_is_pawn_move && !b_is_capture) {
            // Omit the "from" column, for pawn moves that are not captures.
            thisChar = rank_to_move(the_move);
            safe_push_back(MoveText,thisChar);

        } else {
            thisChar = file_to_move(the_move);
            safe_push_back(MoveText,thisChar);

            thisChar = rank_to_move(the_move);
            safe_push_back(MoveText,thisChar);
        }

        // Add promotion information, if needed
        if (the_move.promotion != Piece::NONE)
        {
            safe_push_back(MoveText,'=');  

            assert(the_move.promotion != Piece::PAWN);   // certainly a promotion error. Pawns don't promote to pawns
            thisChar = get_piece_char(the_move.promotion);
            safe_push_back(MoveText,thisChar);

        }

        // Add the check or checkmate symbol if needed.
        if ( (state == GameState::WHITEWIN) || (state == GameState::BLACKWIN) ) {
            safe_push_back(MoveText,'#');
        }
        else if (state == GameState::DRAW) {      
            safe_push_back(MoveText,'~');       // NOT technically SAN, but I need to see 3-time rep and 5- move rule.
        }
        else {
            // // Not a checkmate or draw. See if its a check

            if (isCheck) {
                safe_push_back(MoveText,'+');
            }

            // Add a check character if needed (This is absurdly, ridicululy expensive!)
            // if (in_check_after_move(color_that_moved, the_move)) {
            //     safe_push_back(MoveText,'+');
            // }
        }

        // Note: disambiguation. Fix this. Doesnt work cause the legal_moves passed in are of the wrong color!
        // if (0) {  // p_legal_moves) {

        //     for (const Move& m : *p_legal_moves) {
        //         //int iPieces = bits_in(correct_bit_board);
        //         if ( (m.from == the_move.from) && (m.to == the_move.to) ) continue;
        //         ull mask = (the_move.to & m.to);    //assert (iPieces>0);
        //         if ( (mask != 0ull) && (the_move.piece_type == m.piece_type) ) {
        //             safe_push_back(MoveText,'&';
        //             safe_push_back(MoveText,'(';

        //             char temp[64];
        //             bitboards_to_algebraic(color_that_moved, m, GameState::INPROGRESS
        //                         , NULL                      // NO disambiguation
        //                         , temp);            // output
        //             strcpy(p, temp);           // temp is NULL-terminated
        //             p += strlen(temp);         // now p points at the '\0' we just wrote
        //         }
        //     }
        // }

        //safe_push_back(MoveText,'\n';        // Post face with a carriage return

    }

    //*p = '\0';              // don�t forget to terminate



     if (bPadTrailing) {

        auto it = std::find(MoveText.begin(), MoveText.end(), ' ');
        int nonblank_len = static_cast<int>(it - MoveText.begin());
        int i = std::max(0, 5 - nonblank_len);

        assert (i < _MAX_ALGEBRIAC_SIZE);
        for (int k = 0; k < i; ++k) {
            safe_push_back(MoveText, ' ');
        }  
    }



    return;

}

// Returns character, for the piece (note in algebriac, SAN, caps are always used)
char Engine::get_piece_char(Piece p) {

    switch (p)
    {
        case Piece::PAWN:
            return ' ';
            break;
        case Piece::ROOK:
            return 'R';
            break;
        case Piece::KNIGHT:
            return 'N';
            break;
        case Piece::BISHOP:
            return 'B';
            break;
        case Piece::QUEEN:
            return 'Q';
            break;
        case Piece::KING:
            return 'K';
            break;
        case Piece::NONE:
            return '?';
            break;
        default:
            assert(0);
            break;
    }
    return ' ';
}

// Returns character, for the rank of the "from" square
char Engine::rank_from_move(const Move& m)
{
    int from_sq = utility::bit::bitboard_to_lowest_square(m.from);
    int rank    = from_sq >> 3;             // 0..7 for ranks 1..8
    return '1' + rank;                      // '1'..'8'
}

// Returns character, for the file of the "from" square
char Engine::file_to_move(const Move& m)
{
    int to_sq = utility::bit::bitboard_to_lowest_square(m.to); // 0..63
    int file  = to_sq & 7;     // within-rank index 0..7
    file      = 7 - file;      // mirror cause bit 0 = h1 in your layout
    return 'a' + file;         // 'a'..'h'
}

/// Returns character, for the rank of the "to" square
char Engine::rank_to_move(const Move& m)
{
    int to_sq = utility::bit::bitboard_to_lowest_square(m.to); // 
    int rank  = to_sq / 8;   // 0=rank 1, 1=rank 2, ..., 7=rank 8
    return '1' + rank;       // convert to character '1'..'8'
}

char Engine::file_from_move(const Move& m)
{
    int from_sq = utility::bit::bitboard_to_lowest_square(m.from); // 0..63
    int file  = from_sq & 7;     // within-rank index 0..7
    file      = 7 - file;      // mirror cause bit 0 = h1 in your layout
    return 'a' + file;         // 'a'..'h'
}


// pop!
void Engine::set_random_on_next_move() {
    // user_request_next_move++;
    // if (user_request_next_move > 10) {
    //     user_request_next_move = 7;
    // }
    // cout << "\x1b[34m nextMove->\x1b[0m" << user_request_next_move 
    //      << "\x1b[34m<-nextMove \x1b[0m" << endl;

    //assert(0);  // exploratory
    //debugNow = !debugNow;

    // RANDOMIZING_EQUAL_MOVES
    if (g_iMove==0) {
        i_randomize_next_move = 1;
        cout << "\033[1;31mrandomize_next_move: " << i_randomize_next_move << "\033[0m" << endl;
    }



    //killTheKing(ShumiChess::BLACK);
}


void Engine::killTheKing(Color color) {
    game_board.black_king = 0;
}

void Engine::debug_print_repetition_table() const {
    std::cout << "=== repetition_table dump ===\n";
    for (const auto& entry : repetition_table) {
        uint64_t key   = entry.first;
        int      count = entry.second;
        std::cout << "key 0x" << std::hex << key
                  << "  count " << std::dec << count
                  << "\n";
    }
    std::cout << "=== end dump (" << repetition_table.size() << " entries) ===\n";
}

void Engine::print_bitboard_to_file(ull bb, FILE* fp)
{
    int nChars;

    nChars = fputs("\n", fp);
    if (nChars == EOF) assert(0);

    string stringFrom = utility::representation::bitboard_to_string(bb);
    nChars = fputs(stringFrom.c_str(), fp);
    if (nChars == EOF) assert(0);

    nChars = fputs("\n", fp);
    if (nChars == EOF) assert(0);

}

///////////////////////////////////////////////////////////



void makeMoveScoreList(const Move ) {

}


void Engine::print_moves_and_scores_to_file(const MoveAndScoreList move_and_scores_list, bool b_convert_to_abs_score, FILE* fp)
{
    for (const MoveAndScore& ms : move_and_scores_list) {
        print_move_and_score_to_file(ms, b_convert_to_abs_score, fp);
    }
}

void Engine::print_move_and_score_to_file(const MoveAndScore move_and_score, bool b_convert_to_abs_score, FILE* fp)
{
    const ShumiChess::Move& best_move = move_and_score.first;
    double d_best_move_value = move_and_score.second;   

    move_and_score_to_string(best_move, d_best_move_value, b_convert_to_abs_score);

    int nChars = fputc('\n', fp);
    if (nChars == EOF) assert(0);

    nChars = fputs(move_string.c_str(), fp);
    if (nChars == EOF) assert(0);
}

// Puts best move and absolute score. 
void Engine::move_and_score_to_string(const Move best_move, double d_best_move_value, bool b_convert_to_abs_score)
{
    // Convert relative score to abs score
    if (b_convert_to_abs_score) {
        if (game_board.turn == ShumiChess::BLACK) d_best_move_value = -d_best_move_value;  
    }
    
    if (std::fabs(d_best_move_value) < VERY_SMALL_SCORE) d_best_move_value = 0.0;        // avoid negative zero

    bitboards_to_algebraic(game_board.turn, best_move
                    , (GameState::INPROGRESS)
                    , false
                    , true 
                    , move_string); 

    char buf[32];

    if (d_best_move_value < 0.0) {

        std::snprintf(buf, sizeof(buf), "; %.2f", d_best_move_value);  // 2 digits after decimal
    }
    else {
        std::snprintf(buf, sizeof(buf), "; %+.2f", d_best_move_value);  // 2 digits after decimal
    }

    move_string += buf;

}

//////////////////////////////////////////////////////////////////

bool Engine::has_unquiet_move(const vector<ShumiChess::Move>& moves) {

    bool bReturn = false;
    for (const ShumiChess::Move& mv : moves) {
        if (is_unquiet_move(mv)) return true;
    }
    return bReturn;
}

vector<ShumiChess::Move> Engine::reduce_to_unquiet_moves(const vector<ShumiChess::Move>& moves) {

    vector<ShumiChess::Move> vReturn;
    for (const ShumiChess::Move& mv : moves) {
        if (is_unquiet_move(mv))
        {
            vReturn.push_back(mv); // or: vReturn.emplace_back(mv);
            // Have an object? push_back(obj) / push_back(std::move(obj)).
            // Have constructor args? emplace_back(args...).
        }
    }
    return vReturn;
}

//
// Reduce to tactical (“unquiet”) moves and orders them.
//    Input:   a moves vector
//    Returns: ordered vector of unquiet moves (captures first, promotions second).
//      • Captures: sorted by MVV-LVA (bigger victim, smaller attacker = higher).
//      • Small recapture bonus in sort if mv.to == opponent’s last “to” square.
//      • Non-capture promotions: keep only QUEEN promotions; appended after captures.
// Notes: O(U^2) sort due to linear insertion; U is usually small. (<5)
//
vector<ShumiChess::Move> Engine::reduce_to_unquiet_moves_MVV_LVA(
                const vector<ShumiChess::Move>& moves,       // input
                //const Move& move_last,                       // input
                int qPlys,
                vector<ShumiChess::Move>& vReturn            // output
            )
{

    // recapture bias: if a capture lands on opponent's last-to square, try it earlier
    const bool have_last = !move_history.empty();
    const ull  last_to   = have_last ? move_history.top().to : 0ULL;


    for (const ShumiChess::Move& mv : moves) {
        if (is_unquiet_move(mv)) {
            // its either a capture or a promotion (or both)

            // Very late in analysis! So discard negaive SEE captures.
            int testValue = game_board.SEE_for_capture(game_board.turn, mv, nullptr);
            if (qPlys > MAX_QPLY2) {
                if (testValue < -100) {     // centipawns
                    continue;
                }
            }

            // If it's a capture, insert in descending MVV-LVA order
            if (mv.capture != ShumiChess::Piece::NONE) {

                int key = mvv_lva_key(mv);  // uses your static function (captures only)
                if (have_last && mv.to == last_to) key += 800;  // small recapture bump for opponent's last-to square,
 
                auto it = vReturn.begin();
                for (; it != vReturn.end(); ++it) {
                    // Only compare against other captures; promos-without-capture stay after captures
                    if ((it->capture != ShumiChess::Piece::NONE) && (key > mvv_lva_key(*it))) {
                        break;
                    }
                }
                vReturn.insert(it, mv);
            
            // Its a Promotion (without capture)
            } else {
                assert (mv.promotion != Piece::NONE);

                if (mv.promotion != Piece::QUEEN)    // DO NOT push non queen promotions up in the list
                    continue;
                else
                    vReturn.push_back(mv);
            }
        }
    }
    return vReturn;
}


bool Engine::flip_a_coin(void) {
    std::uniform_int_distribution<int> dist(0,1);
    return dist(rng) == 1;
}

int Engine::rand_int(int min_val, int max_val)
{
    std::uniform_int_distribution<int> dist(min_val, max_val);
    return dist(rng);
}


void Engine::move_into_string(ShumiChess::Move m) {
    bitboards_to_algebraic(game_board.turn, m
                , (GameState::INPROGRESS)
                //, NULL
                , false
                , false
                , move_string);    // Output
}


//
// Prints the move history from oldest → most recent 
// Uses: nPly = -2, isInCheck = false, bFormated = false
void Engine::print_move_history_to_file(FILE* fp) {
    bool bFlipColor = false;
    int ierr = fputs("\n\nhistory:\n", fp);
    assert (ierr!=EOF);

    // copy stack so we don't mutate Engine's history
    std::stack<ShumiChess::Move> tmp = move_history;

    // collect in a vector (top = newest), then reverse to oldest → newest
    std::vector<ShumiChess::Move> seq;
    seq.reserve(tmp.size());
    while (!tmp.empty()) {
        seq.push_back(tmp.top());
        tmp.pop();
    }
    std::reverse(seq.begin(), seq.end());

    // print each move
    for (const ShumiChess::Move& m : seq) {
        bFlipColor = !bFlipColor;
        print_move_to_file(m, -2, (GameState::INPROGRESS), false, false, bFlipColor, fp);
    }
}
//
// Get algebriac (SAN) text form of the last move.
// Tabs over based on ply. Pass in nPly=-2 for no tabs. 
// The formatted version does one move per line. 
// The unformatted version puts them all on one line.
int Engine::print_move_to_file(const ShumiChess::Move m, int nPly, GameState gs
                                    , bool isInCheck, bool bFormated, bool bFlipColor
                                    , FILE* fp
                                ) {
    // NOTE: Here I am assumming the "human" player is white
    Color aColor = ShumiChess::WHITE;  //engine.game_board.turn;

    if (bFlipColor) aColor = utility::representation::opposite_color(aColor);

    bitboards_to_algebraic(aColor, m
                                , gs
                                , isInCheck
                                , false
                                , move_string);

    if (bFormated) { 
        print_move_to_file_from_string(move_string.c_str(), aColor, nPly
                                        , '\n', ',', false
                                        , fp);
    } else {
        print_move_to_file_from_string(move_string.c_str(), aColor, nPly
                                        , ' ', ',', false
                                        , fp); 
    }

    return move_string.length();

}



// Tabs over based on ply. Pass in nPly=-2 for no tabs
// Prints the preCharacter, then the move, then the postCharacter. 
// If b_right_pad is on, Will align, padding with trailing blanks.
void Engine::print_move_to_file_from_string(const char* p_move_text, Color turn, int nPly
                                            , char preCharacter
                                            , char postCharacter
                                            , bool b_right_Pad
                                            , FILE* fp) {
    int ierr = fputc(preCharacter, fp);
    assert (ierr!=EOF);

    
    char szValue[_MAX_ALGEBRIAC_SIZE+8];

    // Indent the whole thing over based on depth level
    int nTabs = nPly+2;
    
    if (nTabs<0) nTabs=0;

    int nSpaces = nTabs*4;
    int nChars = fprintf(fp, "%*s", nSpaces, "");

    // compose "..."+move (for Black) or just move (for White)
    if (turn == utility::representation::opposite_color(ShumiChess::BLACK)) {
        snprintf(szValue, sizeof(szValue), "...%s", p_move_text);
    } else {
        snprintf(szValue, sizeof(szValue), "%s",    p_move_text);
    }

    // print as a single left-justified 8-char field: "...e4   " or "e4     "
    //                                                 12345678
    if (b_right_Pad) fprintf(fp, "%-10.8s", szValue);  // option 1
    else             fprintf(fp, "%.8s", szValue);     // option 2

    ierr = fputc(postCharacter, fp);
    assert (ierr!=EOF);

    int ferr = fflush(fp);
    assert(ferr == 0);
}


#undef NDEBUG



// Debug helper: dump SEE for all capture moves in the current position.
void Engine::debug_SEE_for_all_captures(FILE* fp)
{
    // All legal moves for the current side to move
    std::vector<Move> moves = get_legal_moves();

    fprintf(fp, "ddebug_SEE_for_all_captures: %ld\n", (int)moves.size());

    for (const Move& mv : moves)
    {
        // Convert move to SAN using your existing helper
        std::string san;
        san.reserve(_MAX_ALGEBRIAC_SIZE);
        bitboards_to_algebraic(
            mv.color,
            mv,
            GameState::INPROGRESS,
            false,          // isCheck (we don't care here)
            false,          // bPadTrailing
            san
        );

        // Only look at captures
        if (mv.capture == Piece::NONE)
            continue;

        fprintf(fp, "\n%s  %llu  %llu \n", san.c_str(), mv.to, mv.from);

        // SEE for this *specific* capture from the mover's point of view
        int see_value = game_board.SEE_for_capture(mv.color, mv, fp);
        //assert(see_value>=0);

        // Flag clearly losing captures
        if (see_value < 0)
        {
            int n = fprintf(
                fp,
                "*** NEG SEE ***  %s   SEE=%d\n",
                san.c_str(),
                see_value
            );
            if (n < 0) assert(0);
        }
        else
        {
            int n = fprintf(
                fp,
                "                %s   SEE=%d\n",
                san.c_str(),
                see_value
            );
            if (n < 0) assert(0);
        }
    }

    fflush(fp);
}




} // end namespace ShumiChess
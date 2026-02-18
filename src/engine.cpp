#include <functional>


#include "engine.hpp"
#include "utility"
#include <cmath>

#ifdef SHUMI_FORCE_ASSERTS  // Operated by the -asserts" and "-no-asserts" args to run_gui.py. By default on.
#undef NDEBUG
#endif
#include <assert.h>

//#define SHUMI_ASSERTS

/////////// Debug ////////////////////////////////////////////////////////////////////////////////////

//#define _SUPRESSING_MOVE_HISTORY_RESULTS  // Prevents 3 time rep and the fifty move rule  

//#define DEBUG_NO_CASTLING    // Debug, to disallow castling for everyone

//#define _DEBUGGING_TO_FILE

#ifdef _DEBUGGING_TO_FILE
    extern FILE *fpDebug;
#endif


//bool debugNow = false;


//////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

namespace ShumiChess {

#define PGN_MAX 10000
PGN::PGN() 
{
    text.reserve(PGN_MAX);
};

void PGN::clear()
{
    text.clear();      // sets size to 0, keeps the reserved capacity
}

string PGN::spitout()
{
    text += " *";
    //std::cout << text << std::endl;
    return text;
} 

int PGN::addMe(Move& m, Engine& e)
{
    // Add the move number to the string (PGN needs this)
    if (e.game_board.turn == ShumiChess::WHITE) {
        char sztmp[16];
        sprintf(sztmp, "%i. ", (e.ply_so_far/2+1));
        
        text += sztmp;
    }

    // Puts the move in simple algebriac (SAN)
    // Warning: this function is expensive. (because of disambigouation) Should be called only 
    // for making formal PGN or move files.
    e.move_into_string_full(m);

    // Add the algebriac (disambiguated) to the string
    e.move_string += " ";
    
    text += e.move_string;

    return 0;
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Engine::Engine() {

    reset_engine();
    ShumiChess::initialize_rays();

    // Seed randomization, for engine. (using microseconds since ?)
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


//
//
// By "reset engine" is meant: "new game". 
//
void Engine::reset_engine() {         // New game.
    std::cout << "\x1b[94mNew Game \x1b[0m" << endl;

    // Initialize storage buffers (they are here to avoid extra allocation during the game)
    move_string.reserve(_MAX_MOVE_PLUS_SCORE_SIZE);
    psuedo_legal_moves.reserve(MAX_MOVES); 
    all_legal_moves.reserve(MAX_MOVES);

    ///////////////////////////////////////////////////////////////////////
    //
    // Debug only. You can override the gameboard setup with fen positions as in: (enter FEN here) FEN enter now. enter fen. FEN setup. Setup the FEN
    //game_board = GameBoard("r1bq1r2/pppppkbQ/7p/8/3P1p2/1PPB1N2/1P3PPP/2KR3R w - - 2 17");       // bad
    //game_board = GameBoard("r1bq1r2/pppppkbQ/7p/8/3P1p2/1PPB1N2/1P3PPP/2KR3R w - - 2 17");      // repeat 3 times test
    
    //game_board = GameBoard("rnb1kbnr/pppppppp/5q2/8/8/5Q2/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
    //game_board = GameBoard("8/4k3/6K1/8/8/1q6/8/8 w - - 0 1");

    //game_board = GameBoard("rnb1kbnr/pppppppp/5q2/8/8/5Q2/PPPPPPPP/RNB1KBNR w KQkq - 0 1");
    //game_board = GameBoard("3qk3/8/8/8/8/8/5P2/3Q1K2 w KQkq - 0 1");
    //game_board = GameBoard("1r6/4k3/6K1/8/8/8/8/8 w - - 0 1");
    // 
    // horrible doubled pawn hater bug.
    //game_board = GameBoard("r4rk1/p1Nn2pp/p4q2/2p1p3/2P1P1n1/3BQ3/1P3PK1/R2R4 w - - 2 24");

    // burp2 bug then c5 for black.
    //game_board = GameBoard("rnbq1rk1/ppp2p1p/3ppp2/8/2PP4/2PQ1N2/P3PPPP/R3KB1R b KQ - 1 8");
   
    ////////////////////game_board = GameBoard("rnbq1rk1/pp3p1p/3ppp2/2p5/2PP4/2PQ1N2/P3PPPP/R3KB1R w KQ - 1 8");
    
    // burp2 bug then d5 for black.     (-t500 -d4)
    //game_board = GameBoard("r1bqkb1r/pppppppp/2n2n2/8/6P1/2N2P2/PPPPP2P/R1BQKBNR b KQkq g3 0 3");

    //   then g5 with CRAZY_IVAN
    //game_board = GameBoard("r1bqkbnr/ppppp1pp/2n2p2/8/8/1PN2N2/P1PPPPPP/R1BQKB1R b KQkq - 3 3");

    // Or you can pick a random simple endgame FEN. (maybe)
    // vector<Move> v;
    // int itrys = 0;
    // do {  
    //     string stemp = game_board.random_kqk_fen(false);
    //     game_board = GameBoard(stemp);
    //     v = get_legal_moves(ShumiChess::WHITE);
    //     ++itrys;
    //     assert(itrys < 4);

    // } while (v.size() == 0);

    // "half chess"
    //game_board = GameBoard("3qkbnr/2pppppp/8/8/8/8/2PPPPPP/3QKBNR w KQkq - 0 1");

    game_board = GameBoard();

    //////////////////////////////////////////////////////////////////////.


    move_history = stack<Move>();

    halfway_move_state = stack<int>();
    halfway_move_state.push(0);

    en_passant_history = stack<ull>();
    en_passant_history.push(0);

    castle_opportunity_history = stack<uint8_t>();
    castle_opportunity_history.push(0b1111);

    computer_ply_so_far = 0;       // real moves in whole game
    ply_so_far = 0;     // ply played in game so far
    game_white_time_msec = 0;     // total white thinking time for game 
    game_black_time_msec = 0;     // total black thinking time for game

    // These things are cleared every game.
    gamePGN.clear();

    //repetition_table.clear();
    key_stack.clear();
    boundary_stack.clear();

    reason_for_draw = DRAW_NULL;


}
//
// By "reset engine" is meant: "new game". 
//
void Engine::reset_engine(const string& fen) {      // New game (with fen)

    d_bestScore_at_root = 0.0;

    //std::cout << "\x1b[94m    hello world() I'm reset_engine(FEN)! \x1b[0m";
    std::cout << "\x1b[94mNew Game (from FEN) \x1b[0m" << endl;

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


    computer_ply_so_far = 0;       // real moves in whole game
    ply_so_far = 0;     // ply played in game so far
    game_white_time_msec = 0;     // total white thinking time for game
    game_black_time_msec = 0;     // total black thinking time for game

    // These things are cleared every game.
    gamePGN.clear();

    //repetition_table.clear();
    key_stack.clear();
    boundary_stack.clear();
    
}

// understand why this is ok (vector can be returned even though on stack), move ellusion? 
// https://stackoverflow.com/questions/15704565/efficient-way-to-return-f-stdvector-in-c


vector<Move> Engine::get_legal_moves() {
    Color color = game_board.turn;
    return get_legal_moves(color);
}

bool Engine::in_check_after_move(Color color, const Move& move) {
    // NOTE: is this the most effecient way to do this (push()/pop())?
    #ifdef _DEBUGGING_PUSH_POP_FAST
        std::string temp_fen_before = game_board.to_fen();
    #endif

    pushMoveFast(move);   

    bool bReturn = is_king_in_check2(color);

    popMoveFast();
    
    #ifdef _DEBUGGING_PUSH_POP_FAST
        std::string temp_fen_after = game_board.to_fen();
        if (temp_fen_before != temp_fen_after) {
            std::cout << "\x1b[31m";
            std::cout << "PROBLEM WITH PUSH POP FAST!!!!!" << std::endl;
            std::cout << "FEN before  push/pop: " << temp_fen_before  << std::endl;
            std::cout << "FEN after   push/pop: " << temp_fen_after   << std::endl;
            std::cout << "\x1b[0m";
            assert(0);
        }
    #endif
    return bReturn;
}
//
//   Return true if, AFTER applying `move`, the king of `color` is in check.
//
// Why this exists (algorithm idea):
//   Full pushMoveFast()/popMoveFast() is expensive because it touches move_history,
//   turn, and does a lot of per-piece bookkeeping. But to answer "king attacked?",
//   we only need the position 'as it would look after the move' in the limited ways
//   that affect attacks on the king.
//
// What a move can change that matters for check:
//   (1) Occupancy at `from` (moving piece leaves)  -> can open a discovered attack ray.
//   (2) Occupancy at `to`   (moving piece arrives) -> can block/unblock an attack ray.
//   (3) Captures remove an enemy piece (normal: at `to`; en-passant: behind `to`).
//   (4) Promotion changes which friendly piece type occupies `to`.
//   (5) Castling also moves a rook (so rook occupancy changes too).
//
// Implementation strategy (tiny make/unmake):
//   - Save old values of only the bitboards we will touch.
//   - Apply minimal edits to those bitboards to represent the "after-move" position.
//   - Call is_king_in_check(color) (your optimized king-square test).
//   - Restore the saved bitboards and return the result.
//
// NOTE:
//   This does NOT touch move_history, turn, zobrist, repetition, rights, clocks, etc.
//   It is strictly a fast "after-move, is king attacked?" query.
// ---------------------------------------------------------------------------
bool Engine::in_check_after_move_fast(Color color, const Move& move)
{
    assert(move.piece_type != Piece::NONE);

    // Pointers to the specific bitboards we mutate, plus snapshots for the "pop".
    ull* pSrc   = nullptr;
    ull  srcOld = 0;

    ull* pCap   = nullptr;
    ull  capOld = 0;

    ull* pPromo   = nullptr;
    ull  promoOld = 0;

    ull* pRooks   = nullptr;
    ull  rooksOld = 0;

    const ull from_mask = move.from;
    const ull to_mask   = move.to;

    // --- 1) Moving piece leaves `from` (always) ---
    // This changes occupancy on `from`, which is one of the only squares that can
    // change whether the king is attacked (discovered checks / pins).
    pSrc   = &access_pieces_of_color(move.piece_type, move.color);
    srcOld = *pSrc;
    (*pSrc) &= ~from_mask;      // Clear the from square

    // --- 2) Remove captured enemy piece (if any) ---
    // Captures change occupancy too.
    // En-passant is special: captured pawn is not on `to`; it's one rank behind `to`.
    if (move.capture != Piece::NONE) {
        const Color enemy = utility::representation::opposite_color(move.color);

        pCap   = &access_pieces_of_color(move.capture, enemy);
        capOld = *pCap;

        if (move.is_en_passent_capture) {
            ull behind_mask = (move.color == Color::WHITE) ? (to_mask >> 8) : (to_mask << 8);
            (*pCap) &= ~behind_mask;
        } else {
            (*pCap) &= ~to_mask;
        }
    }

    // --- 3) Occupy `to` with the correct friendly piece ---
    // Normally the moving piece occupies `to`. If promotion, the promoted piece type
    // occupies `to` instead (pawn disappears, promoted piece appears).
    if (move.promotion != Piece::NONE) {
        pPromo   = &access_pieces_of_color(move.promotion, move.color);
        promoOld = *pPromo;
        (*pPromo) |= to_mask;      
    } else {
        (*pSrc) |= to_mask;      // Add the to square 
    }

    // --- 4) Castling also moves a rook ---
    // This mirrors the rook adjustment in pushMoveFast(), but ONLY for the occupancy effect.
    if (move.is_castle_move) {
        pRooks   = &access_pieces_of_color_tp<ShumiChess::Piece::ROOK>(move.color);
        rooksOld = *pRooks;

        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            // Queenside castle
            if (move.color == ShumiChess::Color::WHITE) {
                (*pRooks) &= ~(1ULL << game_board.square_a1);
                (*pRooks) |=  (1ULL << game_board.square_d1);
            } else {
                (*pRooks) &= ~(1ULL << game_board.square_a8);
                (*pRooks) |=  (1ULL << game_board.square_d8);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
            // Kingside castle
            if (move.color == ShumiChess::Color::WHITE) {
                (*pRooks) &= ~(1ULL << game_board.square_h1);
                (*pRooks) |=  (1ULL << game_board.square_f1);
            } else {
                (*pRooks) &= ~(1ULL << game_board.square_h8);
                (*pRooks) |=  (1ULL << game_board.square_f8);
            }
        } else {
            assert(0);
        }
    }

    // After the edits above, the bitboards represent the "after-move" position
    // in every way that can affect attacks on the king. So a plain king-in-check test
    // answers the question.
    const bool bReturn = is_king_in_check2(color);

    // Restore all mutated bitboards (this does the "pop")
    if (pRooks) *pRooks = rooksOld;
    if (pPromo) *pPromo = promoOld;
    if (pCap)   *pCap   = capOld;
    if (pSrc)   *pSrc   = srcOld;

    return bReturn;
}

// NOTE: This is intended ONLY for king moves. Faster than in_check_after_move_fast().
// It answers: "After this KING move, is the king's destination square attacked?"
bool Engine::in_check_after_king_move(Color color, const Move& move)
{
    assert(move.piece_type == Piece::KING);
    assert(move.color == color);


    const Color enemy = utility::representation::opposite_color(color);

    ull occ_BB = game_board.get_pieces();

    const ull fromBB = move.from;
    const ull toBB   = move.to;
    const int toSQ   = utility::bit::bitboard_to_lowest_square_fast(toBB);

    assert(game_board.bits_in(fromBB) == 1);
    assert(game_board.bits_in(toBB)   == 1);


    // Remove king from fromSq
    occ_BB &= ~fromBB;

    // If capture, remove captured piece from occupancy at toSq
    if (move.capture != Piece::NONE) occ_BB &= ~toBB;

    // Place king on toSq
    occ_BB |= toBB;

    // Handle rook occupancy for castling
    if (move.is_castle_move) {
        ull rookFrom_BB = 0ULL;
        ull rookTo_BB   = 0ULL;

        if (color == Color::WHITE) {
            if (toSQ == game_board.square_g1) { rookFrom_BB = (1ULL << game_board.square_h1); rookTo_BB = (1ULL << game_board.square_f1); }
            else if (toSQ == game_board.square_c1) { rookFrom_BB = (1ULL << game_board.square_a1); rookTo_BB = (1ULL << game_board.square_d1); }
            else assert(0);
        } else {
            if (toSQ == game_board.square_g8) { rookFrom_BB = (1ULL << game_board.square_h8); rookTo_BB = (1ULL << game_board.square_f8); }
            else if (toSQ == game_board.square_c8) { rookFrom_BB = (1ULL << game_board.square_a8); rookTo_BB = (1ULL << game_board.square_d8); }
            else assert(0);
        }

        occ_BB &= ~rookFrom_BB;
        occ_BB |=  rookTo_BB;
    }

    // --- Build "enemy piece" masks, adjusted for king capture ---
    ull themKnights = game_board.get_pieces_template<Piece::KNIGHT>(enemy);
    ull themKing    = game_board.get_pieces_template<Piece::KING>(enemy);
    ull themPawns   = game_board.get_pieces_template<Piece::PAWN>(enemy);
    ull themQueens  = game_board.get_pieces_template<Piece::QUEEN>(enemy);
    ull themRooks   = game_board.get_pieces_template<Piece::ROOK>(enemy);
    ull themBishops = game_board.get_pieces_template<Piece::BISHOP>(enemy);

    if (move.capture != Piece::NONE) {
        // captured piece is on toBB (king cannot capture en-passant)
        if (move.capture == Piece::KNIGHT) themKnights &= ~toBB;
        if (move.capture == Piece::KING)   themKing    &= ~toBB;
        if (move.capture == Piece::PAWN)   themPawns   &= ~toBB;
        if (move.capture == Piece::QUEEN)  themQueens  &= ~toBB;
        if (move.capture == Piece::ROOK)   themRooks   &= ~toBB;
        if (move.capture == Piece::BISHOP) themBishops &= ~toBB;
    }

    // Now call a new attack test that uses these locals + occ_BB.
    return is_square_attacked_with_masks(enemy, toBB, toSQ, occ_BB,
                               themKnights, themKing, themPawns,
                               themQueens, themRooks, themBishops);

}




vector<Move> Engine::get_legal_moves(Color color) {

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

// remove asserts (SHUMI_ASSERTS)
std::vector<Move> Engine::get_legal_moves_fast(Color color)
{
    psuedo_legal_moves.clear();
    all_legal_moves.clear();

    const bool in_check_before_move = is_king_in_check2(color);

    PinnedInfo pinnedInfo;
    CheckInfo  checkInfo;

    if (!in_check_before_move) {
        pinnedInfo = compute_pins(color);
    } else {
        checkInfo  = find_checkers_and_blockmask(color);
        pinnedInfo = compute_pins(color);
    }

    get_psuedo_legal_moves(color, psuedo_legal_moves);

    // Helper: Move.toSq (computed from 1-bit bb)
    // (You said you don't keep fromSq/toSq in Move anymore.)
    // We'll just compute toSq/fromSq when needed.

    if (!in_check_before_move) {

        // Not in check BEFORE move is made.
        for (const Move& move : psuedo_legal_moves) {

            bool legal = false;

            if (move.piece_type == Piece::KING) {

                legal = !in_check_after_king_move(color, move);

                #ifdef SHUMI_ASSERTS
                {
                    const bool legal2 = !in_check_after_move_fast(color, move);
                    assert(legal == legal2);
                }
                #endif

            } else {

                // En-passant is special (removes a pawn NOT on move.to), so don't “shortcut” it.
                if (move.is_en_passent_capture) {

                    legal = !in_check_after_move_fast(color, move);

                } else {

                    const int fromSq = utility::bit::bitboard_to_lowest_square_fast(move.from);
                    const int toSq   = utility::bit::bitboard_to_lowest_square_fast(move.to);

                    if (!pinnedInfo.isPinned(fromSq)) {
                        legal = true;
                    } else {
                        legal = pinnedInfo.moveObeysPinLine(fromSq, toSq);
                    }
                }
            }

            if (legal) {
                all_legal_moves.push_back(move);
            }
        }

    } else {

        // In check BEFORE move is made.
        for (const Move& move : psuedo_legal_moves) {

            bool legal = false;

            if (checkInfo.numCheckers >= 2) {

                // Double-check: only king moves can be legal
                if (move.piece_type == Piece::KING) {

                    legal = !in_check_after_king_move(color, move);

                    #ifdef SHUMI_ASSERTS
                    {
                        const bool legal2 = !in_check_after_move_fast(color, move);
                        assert(legal == legal2);
                    }
                    #endif
                }

            } else {

                // Single check
                if (move.piece_type == Piece::KING) {

                    legal = !in_check_after_king_move(color, move);

                    #ifdef SHUMI_ASSERTS
                    {
                        const bool legal2 = !in_check_after_move_fast(color, move);
                        assert(legal == legal2);
                    }
                    #endif

                } else {

                    const int fromSq = utility::bit::bitboard_to_lowest_square_fast(move.from);
                    const int toSq   = utility::bit::bitboard_to_lowest_square_fast(move.to);

                    bool helps = false;

                    if (!move.is_en_passent_capture) {

                        // Normal case: must capture checker OR block check ray by moving toSq.
                        helps = checkInfo.toSquareHelps(toSq);

                    } else {

                        // En-passant: the "captured piece square" is BEHIND move.to.
                        // If that captured pawn is the checker, EP is a valid "capture the checker".
                        // Also allow the normal toSq-help test (harmless).
                        ull epCapturedBB = 0ULL;
                        if (color == Color::WHITE) {
                            epCapturedBB = (move.to >> 8);
                        } else {
                            epCapturedBB = (move.to << 8);
                        }

                        if (checkInfo.toHelpMask & move.to) {
                            helps = true;
                        } else if (checkInfo.captureMask & epCapturedBB) {
                            helps = true;
                        } else {
                            helps = false;
                        }
                    }

                    if (helps) {

                        if (move.is_en_passent_capture) {

                            // EP is tricky: removing the pawn can uncover a rook/bishop check.
                            // So do the full “after move king attacked?” test.
                            legal = !in_check_after_move_fast(color, move);

                        } else {

                            if (!pinnedInfo.isPinned(fromSq)) {
                                legal = true;
                            } else {
                                legal = pinnedInfo.moveObeysPinLine(fromSq, toSq);
                            }
                        }
                    }
                }
            }

            if (legal) {
                all_legal_moves.push_back(move);
            }
        }
    }

    return all_legal_moves;
}



bool Engine::assert_same_moves(const std::vector<Move>& a,
                        const std::vector<Move>& b)
{
    // Sizes must match
    if (a.size() != b.size()) {
        printf("size mismatch\n");
        return false;
    }

    // Every move in A must appear in B
    for (const Move& m1 : a) {
        bool found = false;
        for (const Move& m2 : b) {
            if (m1 == m2) {
                found = true;
                break;
            }
        }
        if (!found)
        {
            printf("mismatch A\n");
            return false;         
        }
    }

    // Every move in B must appear in A
    for (const Move& m2 : b) {
        bool found = false;
        for (const Move& m1 : a) {
            if (m1 == m2) {
                found = true;
                break;
            }
        }
        if (!found)
        {
            printf("mismatch B\n");
            return false;         
        }
    }
    return true;
}


// Put these helpers ABOVE Engine::compute_pins(...) in engine.cpp (file-scope helpers, not lambdas).

static inline int nearest_blocker_sq(ull masked_blockers, bool use_lowest)
{
    if (!masked_blockers) return -1;
    return use_lowest
        ? utility::bit::bitboard_to_lowest_square_fast(masked_blockers)
        : utility::bit::bitboard_to_highest_square_fast(masked_blockers);
}

static inline void process_pin_ray(
    const ShumiChess::Engine* e,
    ShumiChess::Engine::PinnedInfo& info,    // I am the output
    const int kingSq,
    const ull occ,
    const ull myPieces,
    const ull enemyStraight,
    const ull enemyDiag,
    const ull* ray_from_sq,
    const bool use_lowest_for_nearest,
    const bool is_straight
)
{
    // First piece from king along this ray
    ull blockers1 = occ & ray_from_sq[kingSq];
    const int firstSq = nearest_blocker_sq(blockers1, use_lowest_for_nearest);
    if (firstSq < 0) return;

    const ull firstBB = (1ULL << firstSq);

    // Must be my piece to be pinned
    if ((myPieces & firstBB) == 0ULL) return;

    // Next piece beyond that candidate, same direction
    ull blockers2 = occ & ray_from_sq[firstSq];
    const int pinnerSq = nearest_blocker_sq(blockers2, use_lowest_for_nearest);
    if (pinnerSq < 0) return;

    const ull pinnerBB = (1ULL << pinnerSq);

    // Must be an enemy slider that attacks along this line
    if (is_straight) {
        if ((pinnerBB & enemyStraight) == 0ULL) return;
    } else {
        if ((pinnerBB & enemyDiag) == 0ULL) return;
    }

    // It's a real pin.
    info.pinnedMask |= firstBB;

    // Allowed squares: ONLY on the king<->pinner line, including capture of pinner.
    // NOTE: If squares_between_exclusive is not a member, replace e-> with just the function name.
    info.allowedMask[firstSq] = e->squares_between_exclusive(kingSq, pinnerSq) | pinnerBB;
}


// ============================================================================
// Engine function
// ============================================================================
ShumiChess::Engine::PinnedInfo ShumiChess::Engine::compute_pins(Color color)
{
    PinnedInfo info;
    info.clear();

    const Color enemy = utility::representation::opposite_color(color);

    const int kingSq = get_king_square(color);
    const ull kingBB = (1ULL << kingSq);
    (void)kingBB;           // so you don’t get an “unused variable” warning on kingBB ?

    const ull occ      = game_board.get_pieces();
    const ull myPieces = game_board.get_pieces(color);

    const ull themQueens  = game_board.get_pieces_template<Piece::QUEEN>(enemy);
    const ull themRooks   = game_board.get_pieces_template<Piece::ROOK>(enemy);
    const ull themBishops = game_board.get_pieces_template<Piece::BISHOP>(enemy);

    const ull enemyStraight = themQueens | themRooks;
    const ull enemyDiag     = themQueens | themBishops;

    // Straight rays (these magic functions fill in "info")
    process_pin_ray(this, info, kingSq, occ, myPieces, enemyStraight, enemyDiag,
                    north_square_ray.data()
                    , true,  true);
    process_pin_ray(this, info, kingSq, occ, myPieces, enemyStraight, enemyDiag,
                    south_square_ray.data()
                    , false, true);
    process_pin_ray(this, info, kingSq, occ, myPieces, enemyStraight, enemyDiag,
                    west_square_ray.data()
                    ,  true,  true);
    process_pin_ray(this, info, kingSq, occ, myPieces, enemyStraight, enemyDiag,
                    east_square_ray.data()
                    ,  false, true);

    // Diagonal rays (these magic functions fill in "info")
    process_pin_ray(this, info, kingSq, occ, myPieces, enemyStraight, enemyDiag,
                    north_east_square_ray.data()
                    , true,  false);
    process_pin_ray(this, info, kingSq, occ, myPieces, enemyStraight, enemyDiag,
                    north_west_square_ray.data()
                    , true,  false);
    process_pin_ray(this, info, kingSq, occ, myPieces, enemyStraight, enemyDiag,
                    south_east_square_ray.data()
                    , false, false);
    process_pin_ray(this, info, kingSq, occ, myPieces, enemyStraight, enemyDiag,
                    south_west_square_ray.data()
                    , false, false);

    return info;
}



// Doesn't factor in check 
 void Engine::get_psuedo_legal_moves(Color color, vector<Move>& all_psuedo_legal_moves) {
    
    add_knight_moves_to_vector(all_psuedo_legal_moves, color); 
    add_bishop_moves_to_vector(all_psuedo_legal_moves, color); 
    add_pawn_moves_to_vector(all_psuedo_legal_moves, color); 
    add_queen_moves_to_vector(all_psuedo_legal_moves, color); 
    add_king_moves_to_vector(all_psuedo_legal_moves, color); 
    add_rook_moves_to_vector(all_psuedo_legal_moves, color); 

    return;
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

bool Engine::is_king_in_check(const ShumiChess::Color color) {
    ull friendly_king = this->game_board.get_pieces_template<Piece::KING>(color);

    Color enemy_color = utility::representation::opposite_color(color);
    bool bReturn =  is_square_in_check(enemy_color, friendly_king);
     
    return bReturn;
}

bool Engine::is_king_in_check2(const ShumiChess::Color color) {
    ull friendly_king = this->game_board.get_pieces_template<Piece::KING>(color);

    Color enemy_color = utility::representation::opposite_color(color);
    //bool bReturn3 =  is_square_in_check(enemy_color, friendly_king);
    bool bReturn =  is_square_in_check3(enemy_color, friendly_king);
    //assert(bReturn == bReturn3);

    return bReturn;
}


//
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Determines if a square is "in check". All bitboards are assummed to be "h1=0"
// IMPORTANT: It is known that IN ALL CALLERS, square_bb is the bitboard for a single King.
// Inputs: "square_bb" is a bitboard (must be one bit set)

bool Engine::is_square_in_check0(const ShumiChess::Color enemy_color, const ull square_bb) {

    assert (game_board.bits_in(square_bb) == 1);
   
    // This guy does not check for inputs of zero. We are relying on the assert() above.
    const int square = utility::bit::bitboard_to_lowest_square_fast(square_bb);

    // knights that can reach (capture to) this square
    const ull themKnights  = game_board.get_pieces_template<Piece::KNIGHT>(enemy_color);
    const ull reachable_knights = tables::movegen::knight_attack_table[square];
    if (reachable_knights & themKnights) return true;   // Knight attacks this square

    // kings that can reach (capture to) this square
    const ull themKing     = game_board.get_pieces_template<Piece::KING>  (enemy_color);
    const ull reachable_kings   = tables::movegen::king_attack_table[square];
    if (reachable_kings   & themKing)    return true;   // King attacks this square

    // pawns that can reach (capture to) this square
    const ull themPawns    = game_board.get_pieces_template<Piece::PAWN>  (enemy_color);
 
    // Enemy pawn capture sources that could attack target `square_bb`.
    ull square_just_behind_target;  // “the square one rank behind square_bb, from the pawn’s point of view.” (opposite the pawn’s capture direction)

    if (enemy_color == Color::BLACK) {
        square_just_behind_target = (square_bb & ~row_masks[ROW_8]) << 8;
    }
    else {
        square_just_behind_target = (square_bb & ~row_masks[ROW_1]) >> 8;
    }
    //
    // 1. Get the squares attacking the target diagonally, from "left" and "right" side. 
    //      (as if there was a pawn on square_just_behind_target).
    // 2. Mask out the rook file on the opposite side of the board.
    //
    ull FILE_H = col_masksHA[ColHA::COL_H];
    ull FILE_A = col_masksHA[ColHA::COL_A];

    ull towardH_from_target = ((square_just_behind_target & ~FILE_H) >> 1);
    ull towardA_from_target = ((square_just_behind_target & ~FILE_A) << 1);

    // Squares where an enemy pawn could sit and capture onto square_bb (the target).
    ull reachable_pawns = towardA_from_target | towardH_from_target;
  
    if (reachable_pawns   & themPawns)   return true;    // Pawn attacks this square_bb


    // Now look at "sliders" (queens, rooks, bishops)
    const ull themQueens   = game_board.get_pieces_template<Piece::QUEEN> (enemy_color);
    const ull themRooks    = game_board.get_pieces_template<Piece::ROOK>  (enemy_color);
    const ull themBishops  = game_board.get_pieces_template<Piece::BISHOP>(enemy_color);

    ull deadly_diags = themQueens | themBishops;
    ull deadly_straights = themQueens | themRooks;
    if (!(deadly_straights | deadly_diags)) return false;   // No sliders on board

    // Look at both diagonal and straight moves
    ull all_pieces_but_self = game_board.get_pieces() & ~square_bb;
    assert(all_pieces_but_self != 0ULL);       // There must be someone else on the board, right?

    ull straight_attacks_from_passed_sq = 0ULL;
    ull diagonal_attacks_from_passed_sq = 0ULL;

    if (deadly_straights) {
        straight_attacks_from_passed_sq = get_straight_attacks(all_pieces_but_self, square);
    }

    if (deadly_diags) {
        diagonal_attacks_from_passed_sq = get_diagonal_attacks(all_pieces_but_self, square);
    }

    // bishop, rook, queens that can reach (capture to) this square
    if (deadly_straights & straight_attacks_from_passed_sq) return true;    // for queen and rook straight attacks
    if (deadly_diags     & diagonal_attacks_from_passed_sq) return true;    // for queen and bishop straight attacks

    return false;

}


bool Engine::is_square_in_check(const ShumiChess::Color enemy_color, const ull square_bb) {

    assert (game_board.bits_in(square_bb) == 1);
   
    // This guy does not check for inputs of zero. We are relying on the aseert() above.
    const int square = utility::bit::bitboard_to_lowest_square_fast(square_bb);

    // knights that can reach (capture to) this square
    const ull themKnights  = game_board.get_pieces_template<Piece::KNIGHT>(enemy_color);
    const ull reachable_knights = tables::movegen::knight_attack_table[square];
    if (reachable_knights & themKnights) return true;   // Knight attacks this square

    // kings that can reach (capture to) this square
    const ull themKing     = game_board.get_pieces_template<Piece::KING>  (enemy_color);
    const ull reachable_kings   = tables::movegen::king_attack_table[square];
    if (reachable_kings   & themKing)    return true;   // King attacks this square

    // pawns that can reach (capture to) this square
    const ull themPawns    = game_board.get_pieces_template<Piece::PAWN>  (enemy_color);
 
    // Enemy pawn capture sources that could attack target `square_bb`.
    ull square_just_behind_target;  // “the square one rank behind square_bb, from the pawn’s point of view.” (opposite the pawn’s capture direction)

    if (enemy_color == Color::BLACK) {
        square_just_behind_target = (square_bb & ~row_masks[ROW_8]) << 8;
    }
    else {
        square_just_behind_target = (square_bb & ~row_masks[ROW_1]) >> 8;
    }
    //
    // 1. Get the squares attacking the target diagonally, from "left" and "right" side. 
    //      (as if there was a pawn on square_just_behind_target).
    // 2. Mask out the rook file on the opposite side of the board.
    //
    ull FILE_H = col_masksHA[ColHA::COL_H];
    ull FILE_A = col_masksHA[ColHA::COL_A];

    //ull towardH_from_target = ((square_just_behind_target & ~FILE_H) >> 1);
    //ull towardA_from_target = ((square_just_behind_target & ~FILE_A) << 1); 
    ull towardH_from_target = ((square_just_behind_target & ~FILE_H) >> 1);
    ull towardA_from_target = ((square_just_behind_target & ~FILE_A) << 1);

    // Squares where an enemy pawn could sit and capture onto square_bb (the target).
    ull reachable_pawns = towardA_from_target | towardH_from_target;
  
    if (reachable_pawns   & themPawns)   return true;    // Pawn attacks this square_bb


    // Now look at "sliders" (queens, rooks, bishops)
    const ull themQueens   = game_board.get_pieces_template<Piece::QUEEN> (enemy_color);
    const ull themRooks    = game_board.get_pieces_template<Piece::ROOK>  (enemy_color);
    const ull themBishops  = game_board.get_pieces_template<Piece::BISHOP>(enemy_color);

    ull deadly_diags = themQueens | themBishops;
    ull deadly_straights = themQueens | themRooks;
    if (!(deadly_straights | deadly_diags)) return false;   // No sliders on board

    // Look at both diagonal and straight moves
    ull all_pieces_but_self = game_board.get_pieces() & ~square_bb;
    assert(all_pieces_but_self != 0ULL);       // There must be someone else on the board, right?

    ull straight_attacks_from_passed_sq = 0ULL;
    ull diagonal_attacks_from_passed_sq = 0ULL;

    if (deadly_straights) {
        straight_attacks_from_passed_sq = get_straight_attacks(all_pieces_but_self, square);
    }

    if (deadly_diags) {
        diagonal_attacks_from_passed_sq = get_diagonal_attacks(all_pieces_but_self, square);
    }

    // bishop, rook, queens that can reach (capture to) this square
    if (deadly_straights & straight_attacks_from_passed_sq) return true;    // for queen and rook straight attacks
    if (deadly_diags     & diagonal_attacks_from_passed_sq) return true;    // for queen and bishop straight attacks

    return false;

}


bool Engine::is_square_in_check3(Color enemy_color, ull square_bb)
{
    assert(game_board.bits_in(square_bb) == 1);
    int square = utility::bit::bitboard_to_lowest_square_fast(square_bb);

    ull themKnights = game_board.get_pieces_template<Piece::KNIGHT>(enemy_color);
    ull themKing    = game_board.get_pieces_template<Piece::KING>(enemy_color);
    ull themPawns   = game_board.get_pieces_template<Piece::PAWN>(enemy_color);
    ull themQueens  = game_board.get_pieces_template<Piece::QUEEN>(enemy_color);
    ull themRooks   = game_board.get_pieces_template<Piece::ROOK>(enemy_color);
    ull themBishops = game_board.get_pieces_template<Piece::BISHOP>(enemy_color);

    ull occ_BB = game_board.get_pieces();

    return is_square_attacked_with_masks(enemy_color, square_bb, square, occ_BB,
                                         themKnights, themKing, themPawns,
                                         themQueens, themRooks, themBishops);
}

bool Engine::is_square_attacked_with_masks(
    const ShumiChess::Color enemy_color,
    const ull square_bb,     // 1-bit bb of target square
    const int square,        // 0..63, must match square_bb
    const ull occ_BB,        // occupancy bitboard to use (can be "after-move")
    const ull themKnights,
    const ull themKing,
    const ull themPawns,
    const ull themQueens,
    const ull themRooks,
    const ull themBishops
)
{
    assert(game_board.bits_in(square_bb) == 1);
    // Optional consistency check (debug only):
    // assert(square == utility::bit::bitboard_to_lowest_square_fast(square_bb));

    // Knights
    if (tables::movegen::knight_attack_table[square] & themKnights) return true;

    // Kings
    if (tables::movegen::king_attack_table[square] & themKing) return true;

    // Pawns
    ull square_just_behind_target;
    if (enemy_color == Color::BLACK) {
        square_just_behind_target = (square_bb & ~row_masks[ROW_8]) << 8;
    } else {
        square_just_behind_target = (square_bb & ~row_masks[ROW_1]) >> 8;
    }

    ull FILE_H = col_masksHA[ColHA::COL_H];
    ull FILE_A = col_masksHA[ColHA::COL_A];

    ull towardH_from_target = ((square_just_behind_target & ~FILE_H) >> 1);
    ull towardA_from_target = ((square_just_behind_target & ~FILE_A) << 1);

    ull reachable_pawns = towardA_from_target | towardH_from_target;
    if (reachable_pawns & themPawns) return true;

    // Sliders
    ull deadly_diags     = themQueens | themBishops;
    ull deadly_straights = themQueens | themRooks;
    if (!(deadly_diags | deadly_straights)) return false;

    // Use the provided occupancy (not game_board.get_pieces())
    ull all_pieces_but_self = occ_BB & ~square_bb;

    ull straight_attacks_from_sq = 0ULL;
    ull diagonal_attacks_from_sq = 0ULL;

    if (deadly_straights) {
        straight_attacks_from_sq = get_straight_attacks(all_pieces_but_self, square);
        if (deadly_straights & straight_attacks_from_sq) return true;
    }

    if (deadly_diags) {
        diagonal_attacks_from_sq = get_diagonal_attacks(all_pieces_but_self, square);
        if (deadly_diags & diagonal_attacks_from_sq) return true;
    }

    return false;
}


Engine::CheckInfo Engine::find_checkers_and_blockmask(Color color)
{
    CheckInfo info;

    const Color enemy = utility::representation::opposite_color(color);

    const int kingSq = get_king_square(color);
    const ull kingBB = (1ULL << kingSq);

    // Enemy pieces
    const ull themKnights = game_board.get_pieces_template<Piece::KNIGHT>(enemy);
    const ull themPawns   = game_board.get_pieces_template<Piece::PAWN>(enemy);
    const ull themQueens  = game_board.get_pieces_template<Piece::QUEEN>(enemy);
    const ull themRooks   = game_board.get_pieces_template<Piece::ROOK>(enemy);
    const ull themBishops = game_board.get_pieces_template<Piece::BISHOP>(enemy);

    // ------------------------------------------------------------
    // 1) Knight checkers
    // ------------------------------------------------------------
    ull knight_checkers = tables::movegen::knight_attack_table[kingSq] & themKnights;
    while (knight_checkers)
    {
        ull chk = utility::bit::lsb_and_pop(knight_checkers);

        info.numCheckers++;
        if (info.numCheckers == 1) info.checkerBB = chk;
        else info.checkerBB = 0ULL;

        if (info.numCheckers >= 2)
        {
            info.captureMask = 0ULL;
            info.blockMask   = 0ULL;
            info.toHelpMask  = 0ULL;
            info.checkerBB   = 0ULL;
            return info;
        }
    }

    // ------------------------------------------------------------
    // 2) Pawn checkers (same logic as is_square_in_check)
    // ------------------------------------------------------------
    ull square_just_behind_target;
    if (enemy == Color::BLACK) {
        square_just_behind_target = (kingBB & ~row_masks[ROW_8]) << 8;
    } else {
        square_just_behind_target = (kingBB & ~row_masks[ROW_1]) >> 8;
    }

    const ull FILE_H = col_masksHA[ColHA::COL_H];
    const ull FILE_A = col_masksHA[ColHA::COL_A];

    ull towardH_from_target = ((square_just_behind_target & ~FILE_H) >> 1);
    ull towardA_from_target = ((square_just_behind_target & ~FILE_A) << 1);

    ull pawn_sources  = towardA_from_target | towardH_from_target;
    ull pawn_checkers = pawn_sources & themPawns;

    while (pawn_checkers)
    {
        ull chk = utility::bit::lsb_and_pop(pawn_checkers);

        info.numCheckers++;
        if (info.numCheckers == 1) info.checkerBB = chk;
        else info.checkerBB = 0ULL;

        if (info.numCheckers >= 2)
        {
            info.captureMask = 0ULL;
            info.blockMask   = 0ULL;
            info.toHelpMask  = 0ULL;
            info.checkerBB   = 0ULL;
            return info;
        }
    }

    // ------------------------------------------------------------
    // 3) Slider checkers (COUNT ALL OF THEM, not just one)
    // ------------------------------------------------------------
    const ull occ             = game_board.get_pieces();
    const ull occ_without_king = occ & ~kingBB;

    const ull deadly_straights = themQueens | themRooks;
    const ull deadly_diags     = themQueens | themBishops;

    if (deadly_straights) {
        ull straight_attacks  = get_straight_attacks(occ_without_king, kingSq);
        ull straight_checkers = straight_attacks & deadly_straights;

        while (straight_checkers)
        {
            ull chk = utility::bit::lsb_and_pop(straight_checkers);

            info.numCheckers++;
            if (info.numCheckers == 1) info.checkerBB = chk;
            else info.checkerBB = 0ULL;

            if (info.numCheckers >= 2)
            {
                info.captureMask = 0ULL;
                info.blockMask   = 0ULL;
                info.toHelpMask  = 0ULL;
                info.checkerBB   = 0ULL;
                return info;
            }
        }
    }

    if (deadly_diags) {
        ull diag_attacks  = get_diagonal_attacks(occ_without_king, kingSq);
        ull diag_checkers = diag_attacks & deadly_diags;

        while (diag_checkers) {
            ull chk = utility::bit::lsb_and_pop(diag_checkers);

            info.numCheckers++;
            if (info.numCheckers == 1) info.checkerBB = chk;
            else info.checkerBB = 0ULL;

            if (info.numCheckers >= 2) {
                info.captureMask = 0ULL;
                info.blockMask   = 0ULL;
                info.toHelpMask  = 0ULL;
                info.checkerBB   = 0ULL;
                return info;
            }
        }
    }

    // ------------------------------------------------------------
    // 4) Build masks
    // ------------------------------------------------------------
    if (info.numCheckers == 0) {
        info.checkerBB   = 0ULL;
        info.captureMask = 0ULL;
        info.blockMask   = 0ULL;
        info.toHelpMask  = 0ULL;
        return info;
    }

    // If we got here, numCheckers == 1
    info.captureMask = info.checkerBB;

    const int checkerSq = utility::bit::bitboard_to_lowest_square_fast(info.checkerBB);

    // For knight/pawn checks, this should return 0ULL (fine).
    info.blockMask = squares_between_exclusive(kingSq, checkerSq);

    info.toHelpMask = info.captureMask | info.blockMask;
    return info;
}




////////////////////////////////////////////////////////////////////////////////////////////////
// Notes:
//     TODO should this check for draws by internally calling get legal moves and caching that and 
//      returning on the actual call?, very slow calling get_legal_moves again.
// NOTE: I complely agree this is wastefull. But it is not called in the "main line", it is only called as a 
// "python method". The one below this, is the one called in actual play.
//

//
// I am called only from python, when the game is over. I am very waseful. as get_legal_moves() is very 
// expensive
GameState Engine::is_game_over() {
    vector<Move> legal_moves = get_legal_moves();
    return is_game_over(legal_moves);
}

// I am called in every node C++ only). Here speed is not a problem, as we are passed in the legal moves.
GameState Engine::is_game_over(vector<Move>& legal_moves) {
    if (legal_moves.size() == 0) {
        // if no moves, then gave is over. Either somebody wins or its a stalemate
        if ( (!game_board.white_king) || (is_square_in_check0(Color::BLACK, game_board.white_king)) ) {
            return GameState::BLACKWIN;     // Checkmate
        } else if ( (!game_board.black_king) || (is_square_in_check0(Color::WHITE, game_board.black_king)) ) {
            return GameState::WHITEWIN;     // Checkmate
        }
        else {
            reason_for_draw = DRAW_STALEMATE;
            //if (debugNow) cout<<"stalemate" << endl;
            return GameState::DRAW;    //  Draw by Stalemate
        }
    }
    else if (game_board.halfmove >= FIFTY_MOVE_RULE_PLY) {
        //  After fifty  or 50 "ply" or half moves, without a pawn move or capture, its a draw.
        //cout << "Draw by 50-move rule at ply " << game_board.halfmove ;   50 move rule here
        reason_for_draw = DRAW_50MOVERULE;
        //cout<<"50 move rule" << endl;
        return GameState::DRAW;           // draw by 50 move rule

    } else {

        // Insuffecient material.
        bool isOverThatWay = game_board.insufficient_material_simple();
        if (isOverThatWay) {
            reason_for_draw = DRAW_INSUFFMATER;
            //if (debugNow) cout<<"no material" << endl;
            return GameState::DRAW;
        }

        // Three time repetition
        // auto it = repetition_table.find(game_board.zobrist_key);
        // if (it != repetition_table.end()) {
        //     //assert(0);
        //     if (it->second >= THREE_TIME_REP) {
        //         // We've seen this exact position (same zobrist) at least twice already
        //         // along the current line. That means we are in a repetition loop.
        //         //std::cout << "\x1b[31m3-time-rep\x1b[0m" << std::endl;
        //         //assert(0);
        //         reason_for_draw = DRAW_3TIME_REP;
        //         //if (debugNow) cout<<"3-time-rep"<< endl;
        //         return GameState::DRAW;
        //     }
        // }
        int start = boundary_stack.empty() ? 0 : boundary_stack.back();
        int count = 0;
        uint64_t cur = game_board.zobrist_key;
        for (int i = start; i < (int)key_stack.size(); ++i) {
            if (key_stack[i] == cur) count++;
        }
        if (count >= THREE_TIME_REP) {
            // threefold repetition draw
            reason_for_draw = DRAW_3TIME_REP;
            //if (debugNow) cout<<"3-time-rep"<< endl;
            return GameState::DRAW;
        }



    

    }


    return GameState::INPROGRESS;
}


int Engine::get_best_score_at_root() {

    //cout << reason_for_draw << endl;
    //cout << "ouch" << endl;

    int material_centPawns = 0;
    //int pawns_only_centPawns = 0;

    Color for_color = Color::WHITE;     // This makes the score absolute

    material_centPawns = (int)(d_bestScore_at_root*100.0);
    return material_centPawns;


    // for (const auto& color1 : array<Color, 2>{Color::WHITE, Color::BLACK}) {

    //     // Get the centipawn value for this color
    //     int cp_score_mat_temp;

    //     // cp_score_mat_temp = game_board.get_material_for_color(color1, pawns_only_centPawns);
    //     // assert (cp_score_mat_temp>=0);    // no negative value pieces
    //     // if (color1 != for_color) cp_score_mat_temp *= -1;






    //     material_centPawns += cp_score_mat_temp;

    // }

    //return (material_centPawns/100);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

// takes a move, but tracks it so pop() can undo
void Engine::pushMove(const Move& move) {


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
    int square_from = utility::bit::bitboard_to_lowest_square_safe(move.from);
    int square_to   = utility::bit::bitboard_to_lowest_square_safe(move.to);

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
            int target_pawn_square = utility::bit::bitboard_to_lowest_square_safe(target_pawn_bitboard);
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

        // Used for castling only
        int rook_from_sq;
        int rook_to_sq;

        ull& friendly_rooks = access_pieces_of_color_tp<ShumiChess::Piece::ROOK>(move.color);
        //ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        //TODO  Figure out the generic 2 if (castle side) solution, not 4 (castle side x color)
        // cout << "PUSHING: Friendly rooks are:";
        // utility::representation::print_bitboard(friendly_rooks);
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       RNBQKBNR
            // Queenside Castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                rook_from_sq = 7;   // game_board.square_a1
                rook_to_sq = 4;     // game_board.square_d1
                friendly_rooks &= ~(1ULL <<7);
                friendly_rooks |= (1ULL <<4);
            } else {
                rook_from_sq = 63;  // game_board.square_a8
                rook_to_sq = 60;    // game_board.square_d8              
                friendly_rooks &= ~(1ULL <<63);
                friendly_rooks |= (1ULL <<60);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
             //                rnbqkbnr                                                       RNBQKBNR
            // Kingside castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                rook_from_sq = 0;   // game_board.square_h1
                rook_to_sq = 2;     // game_board.square_f1
                friendly_rooks &= ~(1ULL <<0);
                friendly_rooks |= (1ULL <<2);
                //assert (this->game_board.white_castle_rights);
            } else {
                rook_from_sq = 56;  // game_board.square_h8
                rook_to_sq = 58;    // game_board.square_f8               
                friendly_rooks &= ~(1ULL <<56);
                friendly_rooks |= (1ULL <<58);
            }
        } else {        // Something wrong, its not a castle
            assert(0);
        }

        // ---- Zobrist update for the rook hop in castling ----
        // (has to happen after updating the castle rights, just above)
   
        // King squares were already XORed earlier in pushMove(), so we ONLY fix the rook here.
        assert(rook_from_sq >= 0 && rook_to_sq >= 0);
        game_board.zobrist_key ^= zobrist_piece_square_get(ShumiChess::Piece::ROOK + move.color * 6, rook_from_sq);
        game_board.zobrist_key ^= zobrist_piece_square_get(ShumiChess::Piece::ROOK + move.color * 6, rook_to_sq);

        
    }

    this->en_passant_history.push(this->game_board.en_passant_rights);

    // Zobrist: remove old en passant (if any)
    if (this->game_board.en_passant_rights) {
        int old_ep_sq   = utility::bit::bitboard_to_lowest_square_safe(this->game_board.en_passant_rights);
        int old_ep_file = old_ep_sq & 7;  // file index 0..7
        game_board.zobrist_key ^= zobrist_enpassant[old_ep_file];
    }

    this->game_board.en_passant_rights = move.en_passant_rights;

    // Zobrist: add new en passant (if any)
    if (this->game_board.en_passant_rights) {
        int new_ep_sq   = utility::bit::bitboard_to_lowest_square_safe(this->game_board.en_passant_rights);
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

 

}

/////////////////////////////////////////////////////////////////////////////////////////
//
// undos last move   (the opposite of "pushMove()")
// 
void Engine::popMove() {


    const Move move = this->move_history.top();
    this->move_history.pop();

    // pop enpassent rights off the top of the stack

    // --- undo zobrist for en passant rights ---
    // 1. XOR out the current en passant (the one set by the move we're undoing)
    if (this->game_board.en_passant_rights) {
        int cur_ep_sq   = utility::bit::bitboard_to_lowest_square_safe(this->game_board.en_passant_rights);
        int cur_ep_file = cur_ep_sq & 7;
        game_board.zobrist_key ^= zobrist_enpassant[cur_ep_file];
    }

    // 2. Peek at the previous en passant square from history (do NOT pop yet)
    ull prev_ep_bb = this->en_passant_history.top();
    if (prev_ep_bb) {
        int prev_ep_sq   = utility::bit::bitboard_to_lowest_square_safe(prev_ep_bb);
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

    int square_from = utility::bit::bitboard_to_lowest_square_safe(move.from);
    int square_to   = utility::bit::bitboard_to_lowest_square_safe(move.to);

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

            int target_pawn_square = utility::bit::bitboard_to_lowest_square_safe(target_pawn_bitboard);
            
            access_pieces_of_color(move.capture, utility::representation::opposite_color(move.color)) |= target_pawn_bitboard;
            game_board.zobrist_key ^= zobrist_piece_square_get(move.capture + utility::representation::opposite_color(move.color) * 6, target_pawn_square);

        } else {

            access_pieces_of_color(move.capture, utility::representation::opposite_color(move.color)) |= move.to;
            game_board.zobrist_key ^= zobrist_piece_square_get(move.capture + utility::representation::opposite_color(move.color) * 6, square_to);
        }

    } else if (move.is_castle_move) {
       
        int rook_from_sq;
        int rook_to_sq;

        // get pointer to the rook? Which rook?
        ull& friendly_rooks = access_pieces_of_color_tp<ShumiChess::Piece::ROOK>(move.color);
        //ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        // ! Bet we can make this part of push a func and do something fancy with to and from
        //  at least keep standard with push implimentation.

        // the castles move has not been popped yet
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       rnbqkbnr   
            // Popping a Queenside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                rook_from_sq = 4;       // game_board.square_d1
                rook_to_sq = 7;         // game_board.square_a1
                friendly_rooks &= ~(1ULL <<4);
                friendly_rooks |= (1ULL <<7);
            } else {
                rook_from_sq = 60;      // game_board.square_d8
                rook_to_sq = 63;        // game_board.square_a8
                friendly_rooks &= ~(1ULL <<60);
                friendly_rooks |= (1ULL <<63);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
            //                 rnbqkbnr                                                       rnbqkbnr
            // Popping a Kingside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                rook_from_sq = 2;       // game_board.square_f1
                rook_to_sq = 0;         // game_board.square_h1
                friendly_rooks &= ~(1ULL <<2);    // Remove white king rook from f1
                friendly_rooks |= (1ULL <<0);     // Add white king rook back to a1
            } else {
                rook_from_sq = 58;      // game_board.square_f8
                rook_to_sq = 56;        // game_board.square_h8
                friendly_rooks &= ~(1ULL <<58);
                friendly_rooks |= (1ULL <<56);
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

    // (popMoveFast() needs the last move)
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
        ull& cap_bb = access_pieces_of_color(move.capture, game_board.turn);

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

        ull& friendly_rooks = access_pieces_of_color_tp<ShumiChess::Piece::ROOK>(move.color);
        //ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        //
        //TODO  Figure out the generic 2 if (castle side) solution, not 4 (castle side x color)
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       RNBQKBNR
            // Queenside Castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL <<7);
                friendly_rooks |= (1ULL <<4);
            } else {           
                friendly_rooks &= ~(1ULL <<63);
                friendly_rooks |= (1ULL <<60);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
            //                 rnbqkbnr                                                       RNBQKBNR
            // Kingside castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL <<0);
                friendly_rooks |= (1ULL <<2);
                //assert (this->game_board.white_castle_rights);
            } else {
                friendly_rooks &= ~(1ULL <<56);
                friendly_rooks |= (1ULL <<58);
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

        //sull& pawn_bb = access_pieces_of_color(Piece::PAWN, move.color);
        ull& pawn_bb = access_pieces_of_color_tp<ShumiChess::Piece::PAWN>(move.color);
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

        //assert() && "popMoveFast(): castling should never appear in fast path");
        // get pointer to the rook? Which rook?
        ull& friendly_rooks = access_pieces_of_color_tp<ShumiChess::Piece::ROOK>(move.color);
        //ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        // ! Bet we can make this part of push a func and do something fancy with to and from
        //  at least keep standard with pushMoveFast implementation.

        // the castles move has not been popped yet
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       rnbqkbnr   
            // Popping a Queenside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL <<4);
                friendly_rooks |= (1ULL <<7);
            } else {
                friendly_rooks &= ~(1ULL <<60);
                friendly_rooks |= (1ULL <<63);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
            //                 rnbqkbnr                                                       rnbqkbnr
            // Popping a Kingside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL <<2);    // Remove white king rook from f1
                friendly_rooks |= (1ULL <<0);     // Add white king rook back to a1
            } else {
                friendly_rooks &= ~(1ULL <<58);
                friendly_rooks |= (1ULL <<56);
            }

        } else {
            // Something wrong, its not a castle
            assert(0);
        }

    }

    // No zobrist, no repetition, no rights, no clocks, etc.
}


ull& Engine::access_pieces_of_color(Piece piece, Color color) {
    switch (piece)  {
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
// In a header (or in the .cpp but then explicitly instantiated), since it's a template.
template <Piece P> ull& Engine::access_pieces_of_color_tp(Color color)
{
    if constexpr (P == Piece::PAWN) {
        return (color != 0) ? this->game_board.black_pawns
                            : this->game_board.white_pawns;
    } else if constexpr (P == Piece::ROOK) {
        return (color != 0) ? this->game_board.black_rooks
                            : this->game_board.white_rooks;
    } else if constexpr (P == Piece::KNIGHT) {
        return (color != 0) ? this->game_board.black_knights
                            : this->game_board.white_knights;
    } else if constexpr (P == Piece::BISHOP) {
        return (color != 0) ? this->game_board.black_bishops
                            : this->game_board.white_bishops;
    } else if constexpr (P == Piece::QUEEN) {
        return (color != 0) ? this->game_board.black_queens
                            : this->game_board.white_queens;
    } else if constexpr (P == Piece::KING) {
        return (color != 0) ? this->game_board.black_king
                            : this->game_board.white_king;
    } else {
        static_assert(P != P, "Unexpected Piece in access_pieces_of_color_tp");
        return this->game_board.white_king; // unreachable, but keeps compiler happy
    }
}



////////////////////////////////////////////////////////////////////////////////

//
// Fills in a Move data structure based on a single "from" square, and multiple "to" squares.
//
void Engine::add_move_to_vector(vector<Move>& moves, 
                                ull single_bitboard_from, 
                                ull bitboard_to,            // I can be multiple squares in one bitboard. 
                                Piece piece, Color color, bool capture, bool promotion,
                                ull en_passant_rights,
                                bool is_en_passent_capture, bool is_castle) {
    // Castled bitmaps
    constexpr ull W_KSIDE_MASK = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10001000;
    constexpr ull W_QSIDE_MASK = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001001;
    constexpr ull B_KSIDE_MASK = 0b10001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
    constexpr ull B_QSIDE_MASK = 0b00001001'00000000'00000000'00000000'00000000'00000000'00000000'00000000;

    // "from" square better be single bit. (the "to" square may be multiple or single piece)
    assert(game_board.bits_in(single_bitboard_from) == 1);

    // for all "to" squares and add them as moves
    while (bitboard_to) {

        ull single_bitboard_to = utility::bit::lsb_and_pop(bitboard_to);
        assert(game_board.bits_in(single_bitboard_to) == 1);
        
        Piece piece_captured = Piece::NONE;
        if (capture) {
            if (!is_en_passent_capture) {
                piece_captured = game_board.get_piece_type_on_bitboard(single_bitboard_to);
            }
            else {
                piece_captured = Piece::PAWN;
            }
        }

        //
        // Faster than old way: construct the Move directly in the vector (emplace_back()),
        // then fill its fields in-place. This avoids creating a temporary Move and
        // copying it into the vector (emplace_back(new_move)).
        if (!promotion)
        {
            // Get address of next slot in the vector.
            moves.emplace_back();
            Move& new_move = moves.back();

            // Fill it in
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
            if (from_or_to & W_KSIDE_MASK) new_move.white_castle_rights &= 0b00000001;
            if (from_or_to & W_QSIDE_MASK) new_move.white_castle_rights &= 0b00000010;
            if (from_or_to & B_KSIDE_MASK) new_move.black_castle_rights &= 0b00000001;
            if (from_or_to & B_QSIDE_MASK) new_move.black_castle_rights &= 0b00000010;

            new_move.promotion = Piece::NONE;
        }
        else
        {
            // A promotion. Add all possible promotion moves.
            for (const auto promo_piece : promotion_values) {
                // Get address of next slot in the vector.
                moves.emplace_back();
                Move& new_move = moves.back();

                // Fill it in
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
                if (from_or_to & W_KSIDE_MASK) new_move.white_castle_rights &= 0b00000001;
                if (from_or_to & W_QSIDE_MASK) new_move.white_castle_rights &= 0b00000010;
                if (from_or_to & B_KSIDE_MASK) new_move.black_castle_rights &= 0b00000001;
                if (from_or_to & B_QSIDE_MASK) new_move.black_castle_rights &= 0b00000010;

                new_move.promotion = promo_piece;   // only difference from the non-promotion case 
            }
        }



    }       // END loop over all "to" squares of move


}

void Engine::add_pawn_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {    
    // get just pawns of correct color
    ull pawns = game_board.get_pieces_template<Piece::PAWN>(color);

    ull enemy_starting_rank_mask;
    ull pawn_enemy_starting_rank_mask;
    ull pawn_starting_rank_mask;
    ull pawn_enpassant_rank_mask;
    ull far_right_row;
    ull far_left_row;
    //ull far_right_row2;
    //ull far_left_row2;

    // grab variables that will be used several times
    if (color == Color::WHITE) {
        enemy_starting_rank_mask      = row_masks[Row::ROW_8];
        pawn_enemy_starting_rank_mask = row_masks[Row::ROW_7];
        pawn_starting_rank_mask       = row_masks[Row::ROW_2];
        pawn_enpassant_rank_mask      = row_masks[Row::ROW_3];

        far_right_row                 = col_masksHA[ColHA::COL_H];
        far_left_row                  = col_masksHA[ColHA::COL_A];


    } else {
        enemy_starting_rank_mask      = row_masks[Row::ROW_1];
        pawn_enemy_starting_rank_mask = row_masks[Row::ROW_2];
        pawn_starting_rank_mask       = row_masks[Row::ROW_7];
        pawn_enpassant_rank_mask      = row_masks[Row::ROW_6];

        far_right_row                 = col_masksHA[ColHA::COL_A];
        far_left_row                  = col_masksHA[ColHA::COL_H];

    }


    ull all_pieces = game_board.get_pieces();
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));

    while (pawns) {
        // pop and get one pawn bitboard. This gets the pawn, but also removes it from "pawns" but who cares.
        // "pawns" is not an original. (you can mutate it)
        ull single_pawn = utility::bit::lsb_and_pop(pawns);
        
        // Look to see if square just forward of me is empty.
        ull one_move_forward = utility::bit::bitshift_by_color(single_pawn & ~pawn_enemy_starting_rank_mask, color, 8); 
        ull one_move_forward_not_blocked = one_move_forward & ~all_pieces;
        ull spaces_to_move = one_move_forward_not_blocked;

        add_move_to_vector(all_psuedo_legal_moves, single_pawn, spaces_to_move, Piece::PAWN
            , color, false, false
            , 0ULL, false, false);

        // Look for (and add if its there), a move up two ranks
        ull is_doublable = single_pawn & pawn_starting_rank_mask;
        if (is_doublable) {
           
            // Look to see both squares just forward of me is empty. (TODO share more code with single pushes above)
            ull move_forward_one = utility::bit::bitshift_by_color(single_pawn, color, 8);
            ull move_forward_one_blocked = move_forward_one & ~all_pieces;

            ull move_forward_two = utility::bit::bitshift_by_color(move_forward_one_blocked, color, 8);
            ull move_forward_two_blocked = move_forward_two & ~all_pieces;

            add_move_to_vector(all_psuedo_legal_moves, single_pawn, move_forward_two_blocked, Piece::PAWN
                , color, false, false
                , move_forward_one_blocked, false, false);
        }

        // Look for (and add if its there), promotions
        ull potential_promotion = utility::bit::bitshift_by_color(single_pawn & pawn_enemy_starting_rank_mask, color, 8); 
        ull promotion_not_blocked = potential_promotion & ~all_pieces;
        ull promo_squares = promotion_not_blocked;
        add_move_to_vector(all_psuedo_legal_moves, single_pawn, promo_squares, Piece::PAWN
                , color, false, true
                , 0ULL, false, false);

        // Look for (and add if its there), attacks forward left and forward right, also includes promotions like this
        // "Forward" means away from the pawns' back or "1st" rank.
        ull attack_fleft = utility::bit::bitshift_by_color(single_pawn & ~far_left_row, color, 9);
        ull attack_fright = utility::bit::bitshift_by_color(single_pawn & ~far_right_row, color, 7);
        
        // normal attacks is set to nonzero if enemy pieces are in the pawns "crosshairs"
        ull normal_attacks = attack_fleft & all_enemy_pieces;
        normal_attacks |= attack_fright & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_pawn, normal_attacks, Piece::PAWN
                    , color, true, (bool) (normal_attacks & enemy_starting_rank_mask)
                    , 0ULL, false, false);

        // enpassant attacks
        // TODO improvement here, cause we KNOW that enpassant results in the capture of a pawn, but it adds a lot 
        //      of code here to get the speed upgrade. Works fine as is

        ull enpassant_end_location = (attack_fleft | attack_fright) & game_board.en_passant_rights;
        if (enpassant_end_location) {

            // Returns the number of trailing zeros in the binary representation of a 64-bit integer.
            // int origin_pawn_square = utility::bit::bitboard_to_lowest_square(single_pawn);
            // int dest_pawn_square = utility::bit::bitboard_to_lowest_square(one_move_forward);
      
            add_move_to_vector(all_psuedo_legal_moves, single_pawn, enpassant_end_location, Piece::PAWN
                , color, true, false
                , 0ULL, true, false);
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
        add_move_to_vector(all_psuedo_legal_moves, single_knight, enemy_piece_attacks, Piece::KNIGHT
            , color, true, false
            , 0ULL, false, false);

        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(all_psuedo_legal_moves, single_knight, non_attack_moves, Piece::KNIGHT
            , color, false, false
            , 0ULL, false, false);
    }
}

void Engine::add_rook_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull rooks = game_board.get_pieces_template<Piece::ROOK>(color);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (rooks) {

        ull single_rook = utility::bit::lsb_and_pop(rooks);

        // Straight moves 
        ull all_pieces_but_self = game_board.get_pieces() & ~single_rook;
        int square = utility::bit::bitboard_to_lowest_square(single_rook);       
        ull avail_attacks = get_straight_attacks(all_pieces_but_self, square);

        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_rook, enemy_piece_attacks, Piece::ROOK
            , color, true, false
            , 0ULL, false, false);
    
        // all else
        ull non_attack_moves = avail_attacks & (~own_pieces & ~enemy_piece_attacks);
        add_move_to_vector(all_psuedo_legal_moves, single_rook, non_attack_moves, Piece::ROOK
            , color, false, false
            , 0ULL, false, false);
    }
}

void Engine::add_bishop_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull bishops = game_board.get_pieces_template<Piece::BISHOP>(color);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (bishops) {

        ull single_bishop = utility::bit::lsb_and_pop(bishops);

         // Diagonal moves
        ull all_pieces_but_self = game_board.get_pieces() & ~single_bishop;
        assert(all_pieces_but_self != 0);
        int square = utility::bit::bitboard_to_lowest_square(single_bishop);
        ull avail_attacks = get_diagonal_attacks(all_pieces_but_self, square);

        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_bishop, enemy_piece_attacks, Piece::BISHOP
            , color, true, false
            , 0ULL, false, false);
    
        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(all_psuedo_legal_moves, single_bishop, non_attack_moves, Piece::BISHOP
            , color, false, false
            , 0ULL, false, false);
    }
}

void Engine::add_queen_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull queens = game_board.get_pieces_template<Piece::QUEEN>(color);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (queens) {

        ull single_queen = utility::bit::lsb_and_pop(queens);       // To get it to a single bit

        // Diagonal and straight moves
        ull all_pieces_but_self = game_board.get_pieces() & ~single_queen;
        assert(all_pieces_but_self != 0);
        int square = utility::bit::bitboard_to_lowest_square(single_queen);
        ull avail_attacks = get_diagonal_attacks(all_pieces_but_self, square) | get_straight_attacks(all_pieces_but_self, square);

        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_queen, enemy_piece_attacks, Piece::QUEEN
            , color, true, false
            , 0ULL, false, false);
    
        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(all_psuedo_legal_moves, single_queen, non_attack_moves, Piece::QUEEN
            , color, false, false
            , 0ULL, false, false);
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
    add_move_to_vector(all_psuedo_legal_moves, king, enemy_piece_attacks, Piece::KING
        , color, true, false
        , 0ULL, false, false);

    // non-capture moves
    ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
    add_move_to_vector(all_psuedo_legal_moves, king, non_attack_moves, Piece::KING
        , color, false, false
        , 0ULL, false, false);
    

    #ifndef DEBUG_NO_CASTLING

        Color enemy_color = utility::representation::opposite_color(color);

        // castling, NOTE: a Rook must exist in the right place to castle the king.
        ull squares_inbetween;
        ull needed_rook_location;
        ull actual_rooks_location;
        ull king_origin_square;
        bool b_no_inbetween_squares_in_check;
        
        if (color == Color::WHITE) {
            if (game_board.white_castle_rights & (0b00000001)) {    // White kside casling is allowed
                squares_inbetween = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000110;
                if ((squares_inbetween & ~all_pieces) == squares_inbetween) {   
                    // No pieces inbetween the king square and king rook square. 
                    b_no_inbetween_squares_in_check = !is_square_in_check0(enemy_color, king) && !is_square_in_check0(enemy_color, king>>1) && !is_square_in_check0(enemy_color, king>>2);
                    if (b_no_inbetween_squares_in_check) { 
                        // King square, and squares inbetween (3 squares total) are not in check
                        needed_rook_location = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000001;
                        actual_rooks_location = game_board.get_pieces_template<Piece::ROOK, Color::WHITE>();
                        if (actual_rooks_location & needed_rook_location) {
                            // Rook is on correct square for castling
                            king_origin_square = 1ULL <<1;
                            add_move_to_vector(all_psuedo_legal_moves, king, king_origin_square, Piece::KING
                                , color, false, false
                                , 0ULL, false, true);
                        }
                    }
                }
            }
            if (game_board.white_castle_rights & (0b00000010)) {    // White qside casling is allowed
                squares_inbetween = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'01110000;
                if ((squares_inbetween & ~all_pieces) == squares_inbetween) {
                    // No pieces inbetween the king square and queen rook square. 
                    b_no_inbetween_squares_in_check = !is_square_in_check0(enemy_color, king) && !is_square_in_check0(enemy_color, king<<1) && !is_square_in_check0(enemy_color, king<<2);
                    if (b_no_inbetween_squares_in_check) { 
                        // King square, and squares inbetween (3 squares total) are not in check (note that the knight square can be in check to castle qside)
                        needed_rook_location = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000000;
                        actual_rooks_location = game_board.get_pieces_template<Piece::ROOK, Color::WHITE>();
                        if (actual_rooks_location & needed_rook_location) {
                            // Rook is on correct square for castling
                            king_origin_square = 1ULL <<5;
                            add_move_to_vector(all_psuedo_legal_moves, king, king_origin_square, Piece::KING
                                , color, false, false
                                , 0ULL, false, true);
                        }
                    }
                }
            }
        } else if (color == Color::BLACK) {
            if (game_board.black_castle_rights & (0b00000001)) {;        // Black kside casling is allowed
                squares_inbetween = 0b00000110'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
                if ((squares_inbetween & ~all_pieces) == squares_inbetween) {
                    // No pieces inbetween the king square and king rook square
                    b_no_inbetween_squares_in_check = !is_square_in_check0(enemy_color, king) && !is_square_in_check0(enemy_color, king>>1) && !is_square_in_check0(enemy_color, king>>2);
                    if (b_no_inbetween_squares_in_check) { 
                        // King square, and squares inbetween are empty and not in check
                        needed_rook_location = 0b00000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
                        actual_rooks_location = game_board.get_pieces_template<Piece::ROOK, Color::BLACK>();
                        if (actual_rooks_location & needed_rook_location) {
                            // Rook is on correct square for castling
                            king_origin_square = 1ULL <<57;
                            add_move_to_vector(all_psuedo_legal_moves, king, king_origin_square, Piece::KING
                                , color, false, false
                                , 0ULL, false, true);
                        }
                    }
                }
            }
            if (game_board.black_castle_rights & (0b00000010)) {         // Black qside casling is allowed
                squares_inbetween = 0b01110000'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
                if ((squares_inbetween & ~all_pieces) == squares_inbetween) {
                    // No pieces inbetween the king square and king rook square (note that the knight square can be in check to castle qside)
                    b_no_inbetween_squares_in_check = !is_square_in_check0(enemy_color, king) && !is_square_in_check0(enemy_color, king<<1) && !is_square_in_check0(enemy_color, king<<2);
                    if (b_no_inbetween_squares_in_check) { 
                        // King square, and squares inbetween are empty and not in check
                        needed_rook_location = 0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
                        actual_rooks_location = game_board.get_pieces_template<Piece::ROOK, Color::BLACK>();
                        if (actual_rooks_location & needed_rook_location) {
                            // Rook is on correct square for castling
                            king_origin_square = 1ULL <<61;
                            add_move_to_vector(all_psuedo_legal_moves, king, king_origin_square, Piece::KING
                                , color, false, false
                                , 0ULL, false, true);
                        }
                    }
                }
            }
        } else {
            cout << "Unexpected color in king moves: " << color << endl;
            assert(0);
        }

    #endif

}


inline void safe_push_back(std::string &s, char c) {
    s.push_back(c);   // always works
}
//
// Translates a "Move" to SAN (standard algebriac notation).
// Dad's first ShumiChess function.
// Does check, mate, promotion, and disambiguation. 
//   Only thing I added was '~' for forced draws (50 move, or 3-time).
//
void Engine::bitboards_to_algebraic(ShumiChess::Color color_that_moved
                            , const ShumiChess::Move the_move
                            , GameState state 
                            , bool isCheck
                            , bool bPadTrailing
                            , const vector<ShumiChess::Move>* p_legal_moves   // from this position. This is only used for disambigouation
                            , std::string& MoveText) const           // output
{
    char thisChar;
    char aChar;
   
    MoveText.clear();        // start fresh (does NOT free capacity)


    // MoveText += '[';
    // MoveText += (isCheck ? '1' : '0'); 
    // MoveText += ']';

    if (the_move.piece_type == Piece::NONE) {
        MoveText += "none";
    } else {

        bool isCastles=false;
  
        if (the_move.piece_type == Piece::KING) {
            int from_sq = utility::bit::bitboard_to_lowest_square(the_move.from); // 0..63

            if ( (from_sq == game_board.square_e1) || (from_sq == game_board.square_e8) )
            {
                int to_sq = utility::bit::bitboard_to_lowest_square(the_move.to); // 0..63

                //printf("quyyy %ld\n", to_sq);

                if ( (to_sq == game_board.square_g1) || (to_sq == game_board.square_g8) ) {
                     MoveText += "O-O";
                     isCastles=true;
                }
                if ( (to_sq == game_board.square_c1) || (to_sq == game_board.square_c8) ) {
                     MoveText += "O-O-O";
                     isCastles=true;
                }
            }
        }


        if (!isCastles) {

            bool b_is_pawn_move = (the_move.piece_type == Piece::PAWN);

            if (b_is_pawn_move) {
                // For pawn move we give the "from" file, and omit the "p"
                thisChar = file_from_move(the_move);
                safe_push_back(MoveText,thisChar);
            } else {
                // Add the correct piece character
                thisChar = get_piece_char(the_move.piece_type);
                safe_push_back(MoveText,thisChar);
            }

            // disambiguation
            if (!b_is_pawn_move) {      // Dont disambigouate pawn moves.
                if (p_legal_moves) {

                    for (const Move& m : *p_legal_moves) {

                        if (m == the_move) continue;    // skip this move

                        ull mask = (the_move.to & m.to);
                        if ( (mask != 0ull) && (the_move.piece_type == m.piece_type) ) {
                            // Try file first
                            aChar = file_from_move(the_move);
                            if (aChar == file_from_move(m)) {
                                aChar = rank_from_move(the_move);
                            }
                            safe_push_back(MoveText, aChar);
                        }
                    }
                }
            }

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



        }

    }

 
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
char Engine::get_piece_char(Piece p) const {

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
char Engine::rank_from_move(const Move& m) const
{
    int from_sq = utility::bit::bitboard_to_lowest_square(m.from);
    int rank    = from_sq >> 3;             // 0..7 for ranks 1..8
    return '1' + rank;                      // '1'..'8'
}

// Returns character, for the file of the "from" square
char Engine::file_to_move(const Move& m) const
{
    int to_sq = utility::bit::bitboard_to_lowest_square(m.to); // 0..63
    int file  = to_sq & 7;     // within-rank index 0..7
    file      = 7 - file;      // mirror cause bit 0 = h1 in your layout
    return 'a' + file;         // 'a'..'h'
}

/// Returns character, for the rank of the "to" square
char Engine::rank_to_move(const Move& m) const
{
    int to_sq = utility::bit::bitboard_to_lowest_square(m.to); // 
    int rank  = to_sq / 8;   // 0=rank 1, 1=rank 2, ..., 7=rank 8
    return '1' + rank;       // convert to character '1'..'8'
}

char Engine::file_from_move(const Move& m) const
{
    int from_sq = utility::bit::bitboard_to_lowest_square(m.from); // 0..63
    int file  = from_sq & 7;     // within-rank index 0..7
    file      = 7 - file;      // mirror cause bit 0 = h1 in your layout
    return 'a' + file;         // 'a'..'h'
}


// pop!
void Engine::set_random_on_next_move(int randomMoveCount) {
    //
    // user_request_next_move++;
    // if (user_request_next_move > 10) {
    //     user_request_next_move = 7;
    // }
    // cout << "\x1b[34m nextMove->\x1b[0m" << user_request_next_move 
    //      << "\x1b[34m<-nextMove \x1b[0m" << endl;

    //assert(0);  // exploratory
    //debugNow = !debugNow;

    // On first move(ply) of game, we initialize the "number of random plys". It decrements, after  
    // every random move chosen. When it hits zero, no more random plys will be chosen.
    if (computer_ply_so_far==0) {
        i_randomize_next_move = randomMoveCount;
        cout << "\033[1;34m\nrandomize_next_move: " << i_randomize_next_move << "\033[0m" << endl;
    }

    // Is this a way to resign?
    //killTheKing(ShumiChess::BLACK);
}


void Engine::killTheKing(Color color) {
    game_board.black_king = 0;
}

// void Engine::debug_print_repetition_table() const {
//     std::cout << "=== repetition_table dump ===\n";
//     for (const auto& entry : repetition_table) {
//         uint64_t key   = entry.first;
//         int      count = entry.second;
//         std::cout << "key 0x" << std::hex << key
//                   << "  count " << std::dec << count
//                   << "\n";
//     }
//     std::cout << "=== end dump (" << repetition_table.size() << " entries) ===\n";
// }

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






#include <algorithm> // sort
#include <cmath>     // abs

void Engine::print_moves_and_scores_to_file(MoveAndScoreList move_and_scores_list,
                                           bool b_convert_to_abs_score,
                                           bool b_sort_descending,
                                           FILE* fp)
{
    if (b_sort_descending)
    {
        std::sort(move_and_scores_list.begin(), move_and_scores_list.end(),
                  [&](const MoveAndScore& a, const MoveAndScore& b)
                  {
                      double sa = a.second;
                      double sb = b.second;

                      if (b_convert_to_abs_score)
                      {
                          sa = std::abs(sa);
                          sb = std::abs(sb);
                      }

                      return sa > sb; // descending
                  });
    }

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
                    , NULL
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

            // Very late in analysis! So discard negative SEE captures below one pawn.
            int testValue = game_board.SEE_for_capture(game_board.turn, mv, nullptr);
            if (qPlys > MAX_QPLY2) {

                if (testValue > 0) {     // centipawns
                    #ifdef _DEBUGGING_TO_FILE 
                       
                        fprintf(fpDebug,"\nSEE OK: %ld ", testValue);
    
                        print_move_to_file(mv, -2, (GameState::INPROGRESS), false, false, false, fpDebug); 
                        
                        print_move_history_to_file(fpDebug, "SEE hist");
                        fputc('\n', fpDebug);
                    #endif
                }

                if (testValue < -100) {     // centipawns
                    #ifdef _DEBUGGING_TO_FILE 
                       
                        fprintf(fpDebug,"\nSEE ELIM: %ld ", testValue);
    
                        print_move_to_file(mv, -2, (GameState::INPROGRESS), false, false, false, fpDebug); 
                        
                        print_move_history_to_file(fpDebug, "SEE hist");
                        fputc('\n', fpDebug);
                    #endif

                    continue;
                }
            }

            // If it's a capture, insert in descending MVV-LVA order
            if (mv.capture != ShumiChess::Piece::NONE) {

                // Determine sort key: MVV-LVA  Most Valuable Victim, Least Valuable Attacker: prefer taking the 
                // biggest victim with the smallest attacker.
                int key = mvv_lva_key(mv);  // (call me on captures only)

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
       assert(0);
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
                , false
                , false
                , NULL
                , move_string);    // Output
}

string Engine::moves_into_string(const std::vector<Move>& mvs)
{
    std::string sret = "";
    sret.reserve(mvs.size() * 6);   // rough guess: "e2e4 " etc.

    for (size_t i = 0; i < mvs.size(); i++) {

        move_string.clear();     // reuse the same output string each time
        move_into_string(mvs[i]); // fills move_string

        sret += move_string;

        if (i + 1 < mvs.size()) {
            sret += " ";
        }
    }
    sret += '\n';
    return sret;
}


// Warning: this function is expensive. Should be called only for making formal PGN or move files.
void Engine::move_into_string_full(ShumiChess::Move m) {

    // Warning: this function is expensive. Should be called only for making formal PGN or move files.
    vector<ShumiChess::Move> moves = get_legal_moves(game_board.turn);


    bitboards_to_algebraic(game_board.turn, m
                , (GameState::INPROGRESS)
                , false
                , false
                , &moves
                , move_string);    // Output
}


void Engine::print_move_history_to_buffer(char *out, size_t out_size)
{
    if (out_size == 0) return;
    out[0] = '\0';       // fail safely

    FILE *tmp = tmpfile();   // creates an anonymous temporary file
    if (!tmp) return;

    // Write into the temp file using your existing function
    print_move_history_to_file(tmp, "_tO-buffer");

    // Flush and find out how much was written
    fflush(tmp);
    if (fseek(tmp, 0, SEEK_END) != 0)
    {
        fclose(tmp);
        out[0] = '\0';
        return;
    }

    long len = ftell(tmp);
    if (len < 0)
    {
        fclose(tmp);
        out[0] = '\0';
        return;
    }

    // Clamp to buffer size - 1 for the null terminator
    if ((size_t)len >= out_size)
    {
        len = (long)out_size - 1;
    }

    // Rewind and read the data
    rewind(tmp);
    size_t nread = fread(out, 1, (size_t)len, tmp);
    out[nread] = '\0';

    fclose(tmp);
}





//
// Prints the move history from oldest → most recent 
// Uses: nPly = -2, isInCheck = false, bFormated = false
void Engine::print_move_history_to_file(FILE* fp, char* psz) {

    //int ierr = fputs("\nhistory: ", fp);
    int ierr = fprintf(fp, " (%03ld) %s:", (long)move_history.size(), psz);
    assert (ierr!=EOF);

    // copy stack so we don't mutate Engine's history
    std::stack<ShumiChess::Move> tmp = move_history;

    print_move_history_to_file0(fp, tmp);
}


void Engine::print_move_history_to_file0(FILE* fp, std::stack<ShumiChess::Move> tmp) {
    bool bFlipColor = false;

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

    fputc('\n', fp);
}
//
// Get algebriac (SAN) text form of the move
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
                                , NULL
                                , move_string);

    if (bFormated) { 
        print_move_to_file_from_string(move_string.c_str(), aColor, nPly
                                        , "\n", ',', false
                                        , fp);
    } else {
        print_move_to_file_from_string(move_string.c_str(), aColor, nPly
                                        , "", ',', false
                                        , fp); 
    }

    return move_string.length();

}

int Engine::print_move_to_file2(const ShumiChess::Move m, int nPly, GameState gs
                                    , bool isInCheck, bool bFlipColor
                                    , const char* preString
                                    , FILE* fp
                                ) {
    // NOTE: Here I am assumming the "human" player is white
    Color aColor = ShumiChess::WHITE;  //engine.game_board.turn;

    if (bFlipColor) aColor = utility::representation::opposite_color(aColor);

    bitboards_to_algebraic(aColor, m
                                , gs
                                , isInCheck
                                , false
                                , NULL
                                , move_string);

    print_move_to_file_from_string(move_string.c_str(), aColor, nPly
                                        , preString, ',', false
                                        , fp);
 
    return move_string.length();

}




void Engine::print_tabOver(int nPly, FILE* fp)
{
    int nTabs = nPly;
    
    if (nTabs<0) nTabs=0;

    int nSpaces = nTabs*4;
    int nChars = fprintf(fp, "%*s", nSpaces, "");
}

// Tabs over based on ply. Pass in nPly=-2 for no tabs
// Prints the preCharacter, then the move, then the postCharacter. 
// If b_right_pad is on, Will align, padding with trailing blanks.
void Engine::print_move_to_file_from_string(const char* p_move_text, Color turn, int nPly
                                            , const char* preString
                                            , char postCharacter
                                            , bool b_right_Pad
                                            , FILE* fp) {
    int ierr;
    char szValue[_MAX_ALGEBRIAC_SIZE+8];

    // Indent the whole thing over based on depth level
    print_tabOver(nPly, fp);

    // print prestring
    if (preString != "") {
        ierr = fputs(preString, fp);
        assert (ierr!=EOF);
    }

    // compose "..."+move (for Black) or just move (for White)
    if (turn == utility::representation::opposite_color(ShumiChess::BLACK)) {
        snprintf(szValue, sizeof(szValue), "...%s", p_move_text);
    } else {
        snprintf(szValue, sizeof(szValue), "%s",    p_move_text);
    }

    // print move as a single left-justified 8-char field: "...e4   " or "e4     "
    //                                                 12345678
    if (b_right_Pad) fprintf(fp, "%-10.8s", szValue);  // option 1
    else             fprintf(fp, "%.8s", szValue);     // option 2

    // print post character
    ierr = fputc(postCharacter, fp);
    assert (ierr!=EOF);

    int ferr = fflush(fp);
    assert(ferr == 0);
}


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
            NULL,
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
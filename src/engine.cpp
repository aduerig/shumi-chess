#include <functional>

#include "engine.hpp"
#include "utility"


// NOTE: How is NDEBUG being defined by someone (CMAKE?). I would rather do it in the make apparutus than in source files.
//#define NDEBUG         // Define (uncomment) this to disable asserts
#undef NDEBUG
#include <assert.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

namespace ShumiChess {
Engine::Engine() {
    reset_engine();
    ShumiChess::initialize_rays();

    cout << "Created new engine" << endl;
    // cout << utility::colorize(utility::AnsiColor::BRIGHT_BLUE, "south_west_square_ray[25]") << endl;
    // utility::representation::print_bitboard(south_west_square_ray[25]);
    // cout << utility::colorize(utility::AnsiColor::BRIGHT_BLUE, "south_west_square_ray[28]") << endl;
    // utility::representation::print_bitboard(south_west_square_ray[28]);
    // cout << utility::colorize(utility::AnsiColor::BRIGHT_BLUE, "south_west_square_ray[37]") << endl;
    // utility::representation::print_bitboard(south_west_square_ray[37]);
}

//TODO what is right way to handle popping past default state here?
Engine::Engine(const string& fen_notation) : game_board(fen_notation) {
}

void Engine::reset_engine() {

    // You can override the gameboard setup with fen positions as in:
    //game_board = GameBoard("r4rk1/7R/8/8/8/8/8/R3K3 w Q - 2 2");
    game_board = GameBoard();

    // ! is reinitalize these stacks the right way to clear the previous entries?
    move_history = stack<Move>();
    halfway_move_state = stack<int>();
    halfway_move_state.push(0);
    stack<ull> en_passant_history;
    en_passant_history.push(0);
    castle_opportunity_history = stack<uint8_t>();
    castle_opportunity_history.push(0b1111);
}

void Engine::reset_engine(const string& fen) {
    game_board = GameBoard(fen);
    // ! is reinitalize these stacks the right way to clear the previous entries?
    move_history = stack<Move>();
    halfway_move_state = stack<int>();
    halfway_move_state.push(0);
    stack<ull> en_passant_history;
    en_passant_history.push(0);
    castle_opportunity_history = stack<uint8_t>();
    castle_opportunity_history.push(0b1111);
}

// understand why this is ok (vector can be returned even though on stack), move ellusion? 
// https://stackoverflow.com/questions/15704565/efficient-way-to-return-a-stdvector-in-c
vector<Move> Engine::get_legal_moves() {
    vector<Move> all_legal_moves;
    Color color = game_board.turn;
    //
    // Get "psuedo_legal" moves. Those that does not put the king in check, and do not 
    // cross the king over a "checked square".
    vector<Move> psuedo_legal_moves; 
    psuedo_legal_moves = get_psuedo_legal_moves(color);
    //
    // Ensure the output array is big enough (NOTE:could this be done as a constant allocation?)
    // Smart idea, we should calculate a sane max and run it through benchmarks
    all_legal_moves.reserve(psuedo_legal_moves.size());

    for (Move move : psuedo_legal_moves) {

        // NOTE: is this the most effecient way to do this (push()/pop())?
        push(move);        
        
        if (!is_king_in_check(color)) {
            // King is NOT in check after making the move
            all_legal_moves.emplace_back(move);
        }

        pop();

    }
    return all_legal_moves;
}

// Doesn't factor in check
vector<Move> Engine::get_psuedo_legal_moves(Color color) {
    vector<Move> all_psuedo_legal_moves;
    
    add_pawn_moves_to_vector(all_psuedo_legal_moves, color); 
    add_rook_moves_to_vector(all_psuedo_legal_moves, color); 
    add_bishop_moves_to_vector(all_psuedo_legal_moves, color); 
    add_queen_moves_to_vector(all_psuedo_legal_moves, color); 
    add_king_moves_to_vector(all_psuedo_legal_moves, color); 
    add_knight_moves_to_vector(all_psuedo_legal_moves, color); 
    
    return all_psuedo_legal_moves;
}

// Does not take into account castling crossing a checked square? Yes, is_square_in_check() does this.
bool Engine::is_king_in_check(const ShumiChess::Color& color) {
    ull friendly_king = this->game_board.get_pieces_template<Piece::KING>(color);
    return is_square_in_check(color, friendly_king);
}

bool Engine::is_square_in_check(const ShumiChess::Color& color, const ull& square) {
    Color enemy_color = utility::representation::opposite_color(color);

    // ? probably don't need knights here because pins cannot happen with knights, but we don't check if king is in check yet
    ull straight_attacks_from_king = get_straight_attacks(square);
    // cout << "diagonal_attacks_from_king: " << square << endl;
    ull diagonal_attacks_from_king = get_diagonal_attacks(square);
    
    ull deadly_diags = game_board.get_pieces_template<Piece::QUEEN>(enemy_color) | game_board.get_pieces_template<Piece::BISHOP>(enemy_color);
    ull deadly_straight = game_board.get_pieces_template<Piece::QUEEN>(enemy_color) | game_board.get_pieces_template<Piece::ROOK>(enemy_color);

    // pawns
    ull temp;
    if (color == Color::WHITE) {
        temp = (square & ~row_masks[7]) << 8;
    }
    else {
        temp = (square & ~row_masks[0]) >> 8;
    }
    ull reachable_pawns = (((temp & ~col_masks[7]) << 1) | 
                           ((temp & ~col_masks[0]) >> 1));

    // knights
    ull reachable_knights = tables::movegen::knight_attack_table[utility::bit::bitboard_to_lowest_square(square)];

    // kings
    ull reachable_kings = tables::movegen::king_attack_table[utility::bit::bitboard_to_lowest_square(square)];

    return ((deadly_straight & straight_attacks_from_king) ||
            (deadly_diags & diagonal_attacks_from_king) ||
            (reachable_knights & game_board.get_pieces_template<Piece::KNIGHT>(enemy_color)) ||
            (reachable_pawns & game_board.get_pieces_template<Piece::PAWN>(enemy_color)) ||
            (reachable_kings & game_board.get_pieces_template<Piece::KING>(enemy_color)));
}

// TODO should this check for draws by internally calling get legal moves and caching that and 
//      returning on the actual call?, very slow calling get_legal_moves again
GameState Engine::game_over() {
    vector<Move> legal_moves = get_legal_moves();
    return game_over(legal_moves);
}

GameState Engine::game_over(vector<Move>& legal_moves) {
    if (legal_moves.size() == 0) {
        if (is_square_in_check(Color::WHITE, game_board.white_king)) {
            return GameState::BLACKWIN;
        }
        else if (is_square_in_check(Color::BLACK, game_board.black_king)) {
            return GameState::WHITEWIN;
        }
        return GameState::DRAW;
    }
    // TODO check if this is off by one or something
    else if (game_board.halfmove >= 50) {
        //  After fifty "ply" or half moves, without a pawn move or capture, its a draw.
        return GameState::DRAW;
    }
    return GameState::INPROGRESS;
}

// takes a move, but tracks it so pop() can undo
void Engine::push(const Move& move) {

    assert(move.piece_type != NONE);

    move_history.push(move);

    // Switch color
    this->game_board.turn = utility::representation::opposite_color(move.color);

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
    ull& moving_piece = access_piece_of_color(move.piece_type, move.color);
    moving_piece &= ~move.from;

    // Returns the number of trailing zeros in the binary representation of a 64-bit integer.
    int square_from = utility::bit::bitboard_to_lowest_square(move.from);
    int square_to   = utility::bit::bitboard_to_lowest_square(move.to);

    // zobrist_key update (for normal moves)
    game_board.zobrist_key ^= zobrist_piece_square[move.piece_type + move.color * 6][square_from];

    // Put the piece where it will go.
    if (move.promotion == Piece::NONE) {
        moving_piece |= move.to;

        game_board.zobrist_key ^= zobrist_piece_square[move.piece_type + move.color * 6][square_to];
    }
    else {
        // Promote the piece
        ull& promoted_piece = access_piece_of_color(move.promotion, move.color);
        promoted_piece |= move.to;

        game_board.zobrist_key ^= zobrist_piece_square[move.promotion + move.color * 6][square_to];
    }

    if (move.capture != Piece::NONE) {
        // The move is a capture
        this->game_board.halfmove = 0;

        if (move.is_en_passent_capture) {
            // Enpassent capture
            ull target_pawn_bitboard = move.color == ShumiChess::Color::WHITE ? move.to >> 8 : move.to << 8;
            int target_pawn_square = utility::bit::bitboard_to_lowest_square(target_pawn_bitboard);
            access_piece_of_color(move.capture, utility::representation::opposite_color(move.color)) &= ~target_pawn_bitboard;
            game_board.zobrist_key ^= zobrist_piece_square[move.capture + utility::representation::opposite_color(move.color) * 6][target_pawn_square];
        } else {
            // Regular capture
            ull& where_I_was = access_piece_of_color(move.capture, utility::representation::opposite_color(move.color));
            where_I_was &= ~move.to;
            game_board.zobrist_key ^= zobrist_piece_square[move.capture + utility::representation::opposite_color(move.color) * 6][square_to];
        }
    } else if (move.is_castle_move) {

        // !TODO zobrist update for castling

        ull& friendly_rooks = access_piece_of_color(ShumiChess::Piece::ROOK, move.color);
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
            // Kingside castle
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<0);
                friendly_rooks |= (1ULL<<2);
            } else {
                friendly_rooks &= ~(1ULL<<56);
                friendly_rooks |= (1ULL<<58);
            }
        } else {        // Something wrong, its not a castle
            assert(0);
        }
    }

    this->en_passant_history.push(this->game_board.en_passant);
    // !TODO zobrist update for en_passant
    this->game_board.en_passant = move.en_passant;
    
    // Manage castling rights
    uint8_t castle_opp = (this->game_board.black_castle << 2) | this->game_board.white_castle;
    this->castle_opportunity_history.push(castle_opp);
    this->game_board.black_castle &= move.black_castle;
    this->game_board.white_castle &= move.white_castle;
}

// undos last move, errors if no move was made before
// NOTE: Ummm, how does it return an error?
// 
void Engine::pop() {
    const Move move = this->move_history.top();
    this->move_history.pop();

    game_board.zobrist_key ^= zobrist_side;

    this->game_board.turn = move.color;

    // pop move states
    this->game_board.fullmove -= static_cast<int>(move.color == ShumiChess::Color::BLACK);
    this->game_board.halfmove = this->halfway_move_state.top();
    this->halfway_move_state.pop();

    int square_from = utility::bit::bitboard_to_lowest_square(move.from);
    int square_to   = utility::bit::bitboard_to_lowest_square(move.to);

    // pop the "actual move". This removes the piece from its square and puts it back to where it was.
    ull& moving_piece = access_piece_of_color(move.piece_type, move.color);
    moving_piece &= ~move.to;
    moving_piece |= move.from;

    game_board.zobrist_key ^= zobrist_piece_square[move.piece_type + move.color * 6][square_from];

    // pop pawn promotions
    if (move.promotion == Piece::NONE) {
        // Not a pawn promotion
        game_board.zobrist_key ^= zobrist_piece_square[move.piece_type + move.color * 6][square_to];
    }
    else {
        // Is a pawn promotion
        ull& promoted_piece = access_piece_of_color(move.promotion, move.color);
        promoted_piece &= ~move.to;

        game_board.zobrist_key ^= zobrist_piece_square[move.promotion + move.color * 6][square_to];
    }

    if (move.capture != Piece::NONE) {

        if (move.is_en_passent_capture) {
            ull target_pawn_bitboard = move.color == ShumiChess::Color::WHITE ? move.to >> 8 : move.to << 8;
            int target_pawn_square = utility::bit::bitboard_to_lowest_square(target_pawn_bitboard);
            // NOTE: here the statements are reversed from the else. What gives?
            game_board.zobrist_key ^= zobrist_piece_square[move.capture + utility::representation::opposite_color(move.color) * 6][target_pawn_square];
          
            access_piece_of_color(move.capture, utility::representation::opposite_color(move.color)) |= target_pawn_bitboard;
     
        } else {

            access_piece_of_color(move.capture, utility::representation::opposite_color(move.color)) |= move.to;
            game_board.zobrist_key ^= zobrist_piece_square[move.capture + utility::representation::opposite_color(move.color) * 6][square_to];
        }
    } else if (move.is_castle_move) {
        
        // get pointer to the rook? Which rook?
        ull& friendly_rooks = access_piece_of_color(ShumiChess::Piece::ROOK, move.color);
        // ! Bet we can make this part of push a func and do something fancy with to and from
        // TODO at least keep standard with push implimentation.

        // NOTE: the castles move has not been popped yet
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

    // pop enpassent status
    this->game_board.en_passant = this->en_passant_history.top();
    this->en_passant_history.pop();
    
    // pop castle status
    this->game_board.black_castle = this->castle_opportunity_history.top() >> 2;
    this->game_board.white_castle = this->castle_opportunity_history.top() & 0b0011;
    this->castle_opportunity_history.pop();
}

// ull& Engine::access_piece_of_color(Piece piece, Color color) {
//     switch (piece)
//     {
//     case Piece::PAWN:
//         if (color) {return ref(this->game_board.black_pawns);}
//         else {return ref(this->game_board.white_pawns);}
//         break;
//     case Piece::ROOK:
//         if (color) {return ref(this->game_board.black_rooks);}
//         else {return ref(this->game_board.white_rooks);}
//         break;
//     case Piece::KNIGHT:
//         if (color) {return ref(this->game_board.black_knights);}
//         else {return ref(this->game_board.white_knights);}
//         break;
//     case Piece::BISHOP:
//         if (color) {return ref(this->game_board.black_bishops);}
//         else {return ref(this->game_board.white_bishops);}
//         break;
//     case Piece::QUEEN:
//         if (color) {return ref(this->game_board.black_queens);}
//         else {return ref(this->game_board.white_queens);}
//         break;
//     case Piece::KING:
//         if (color) {return ref(this->game_board.black_king);}
//         else {return ref(this->game_board.white_king);}
//         break;
//     default:
//         cout << "Unexpected piece type in access_piece_of_color: " << piece << endl;
//         assert(0);
//         break;
//     }
//     // TODO remove this, i'm just putting it here because it prevents a warning
//     // NOTE: remove it I agree. 
//     return this->game_board.white_king;
// }

ull& Engine::access_piece_of_color(Piece piece, Color color) {
    switch (piece)
    {
        case Piece::PAWN:
            if (color) {return this->game_board.black_pawns;}
            else       {return this->game_board.white_pawns;}
            break;
        case Piece::ROOK:
            if (color) {return this->game_board.black_rooks;}
            else       {return this->game_board.white_rooks;}
            break;
        case Piece::KNIGHT:
            if (color) {return this->game_board.black_knights;}
            else       {return this->game_board.white_knights;}
            break;
        case Piece::BISHOP:
            if (color) {return this->game_board.black_bishops;}
            else       {return this->game_board.white_bishops;}
            break;
        case Piece::QUEEN:
            if (color) {return this->game_board.black_queens;}
            else       {return this->game_board.white_queens;}
            break;
        case Piece::KING:
            if (color) {return this->game_board.black_king;}
            else       {return this->game_board.white_king;}
            break;

        default:
            cout << "Unexpected piece type in access_piece_of_color: " << piece << endl;
            assert(0);
            break;
    }

    // Unreachable, but required to avoid warnings
    return this->game_board.white_king;
}




void Engine::add_move_to_vector(vector<Move>& moves, ull single_bitboard_from, ull bitboard_to, Piece piece, Color color
    , bool capture, bool promotion, ull en_passant, bool is_en_passent_capture, bool is_castle) {

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

        Move new_move;
        new_move.color = color;
        new_move.piece_type = piece;
        new_move.from = single_bitboard_from;
        new_move.to = single_bitboard_to;
        new_move.capture = piece_captured;
        new_move.en_passant = en_passant;
        new_move.is_en_passent_capture = is_en_passent_capture;
        new_move.is_castle_move = is_castle;

        // castling rights
        ull from_or_to = (single_bitboard_from | single_bitboard_to);
        if (from_or_to & 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10001000) {
            new_move.white_castle &= 0b00000001;
        }
        if (from_or_to & 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001001) {
            new_move.white_castle &= 0b00000010;
        }
        if (from_or_to & 0b10001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000) {
            new_move.black_castle &= 0b00000001;
        }
        if (from_or_to & 0b00001001'00000000'00000000'00000000'00000000'00000000'00000000'00000000) {
            new_move.black_castle &= 0b00000010;
        }

        if (!promotion) {
            new_move.promotion = Piece::NONE;
            moves.emplace_back(new_move);
        }
        else {
            for (auto& promo_piece : promotion_values) {
                Move promo_move = new_move;
                promo_move.promotion = promo_piece;
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
        // pop and get one pawn bitboard
        ull single_pawn = utility::bit::lsb_and_pop(pawns);
        
        // single moves forward, don't check for promotions
        // NOTE: What happens during promotion then?
        ull move_forward = utility::bit::bitshift_by_color(single_pawn & ~pawn_enemy_starting_rank_mask, color, 8); 
        ull move_forward_not_blocked = move_forward & ~all_pieces;
        ull spaces_to_move = move_forward_not_blocked;
        add_move_to_vector(all_psuedo_legal_moves, single_pawn, spaces_to_move, Piece::PAWN, color, false, false, 0ULL, false, false);

        // move up two ranks
        ull is_doublable = single_pawn & pawn_starting_rank_mask;
        if (is_doublable) {
            // TODO share more code with single pushes above
            ull move_forward_one = utility::bit::bitshift_by_color(single_pawn, color, 8);
            ull move_forward_one_blocked = move_forward_one & ~all_pieces;
            ull move_forward_two = utility::bit::bitshift_by_color(move_forward_one_blocked, color, 8);
            ull move_forward_two_blocked = move_forward_two & ~all_pieces;
            add_move_to_vector(all_psuedo_legal_moves, single_pawn, move_forward_two_blocked, Piece::PAWN, color, false, false, move_forward_one_blocked, false, false);
        }

        // promotions
        ull potential_promotion = utility::bit::bitshift_by_color(single_pawn & pawn_enemy_starting_rank_mask, color, 8); 
        ull promotion_not_blocked = potential_promotion & ~all_pieces;
        ull promo_squares = promotion_not_blocked;
        add_move_to_vector(all_psuedo_legal_moves, single_pawn, promo_squares, Piece::PAWN, color, 
                false, true, 0ULL, false, false);

        // attacks forward left and forward right, also includes promotions like this
        ull attack_fleft = utility::bit::bitshift_by_color(single_pawn & ~far_left_row, color, 9);
        ull attack_fright = utility::bit::bitshift_by_color(single_pawn & ~far_right_row, color, 7);
        ull normal_attacks = attack_fleft & all_enemy_pieces;
        normal_attacks |= attack_fright & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_pawn, normal_attacks, Piece::PAWN, color, 
                     true, (bool) (normal_attacks & enemy_starting_rank_mask), 0ULL, false, false);

        // NOTE: enpassent is not allowed 
        // enpassant attacks
        // TODO improvement here, because we KNOW that enpassant results in the capture of a pawn, but it adds a lot 
        //      of code here to get the speed upgrade. Words fine as is
        // ull enpassant_end_location = (attack_fleft | attack_fright) & game_board.en_passant;
        // add_move_to_vector(all_psuedo_legal_moves, single_pawn, enpassant_end_location, Piece::PAWN, color, 
        //              true, false, 0ULL, true, false);
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
    
    // castling, NOTE: a Rook must exist in the right place to castle the king.
    ull squares_inbetween;
    ull needed_rook_location;
    ull actual_rooks_location;
    ull king_origin_square;
    
    if (color == Color::WHITE) {
        if (game_board.white_castle & (0b00000001)) {
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
        if (game_board.white_castle & (0b00000010)) {
            // Move is a White queen side castle
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
        if (game_board.black_castle & (0b00000001)) {
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
        if (game_board.black_castle & (0b00000010)) {
            // Move is a Black queen side castle
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
}

// !TODO: https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html, currently implemented with slow method at top
ull Engine::get_diagonal_attacks(ull bitboard) {
    ull all_pieces_but_self = game_board.get_pieces() & ~bitboard;
    ull square = utility::bit::bitboard_to_lowest_square(bitboard);

    // up right
    ull masked_blockers_ne = all_pieces_but_self & north_east_square_ray[square];
    int blocked_square = utility::bit::bitboard_to_lowest_square(masked_blockers_ne);
    ull ne_attacks = ~north_east_square_ray[blocked_square] & north_east_square_ray[square];

    // up left
    ull masked_blockers_nw = all_pieces_but_self & north_west_square_ray[square];
    blocked_square = utility::bit::bitboard_to_lowest_square(masked_blockers_nw);
    ull nw_attacks = ~north_west_square_ray[blocked_square] & north_west_square_ray[square];

    // down right
    ull masked_blockers_se = all_pieces_but_self & south_east_square_ray[square];
    blocked_square = utility::bit::bitboard_to_highest_square(masked_blockers_se);
    ull se_attacks = ~south_east_square_ray[blocked_square] & south_east_square_ray[square];

    // down left
    ull masked_blockers_sw = all_pieces_but_self & south_west_square_ray[square];
    blocked_square = utility::bit::bitboard_to_highest_square(masked_blockers_sw);
    ull sw_attacks = ~south_west_square_ray[blocked_square] & south_west_square_ray[square];

    return ne_attacks | nw_attacks | se_attacks | sw_attacks;
}

ull Engine::get_straight_attacks(ull bitboard) {
    ull all_pieces_but_self = game_board.get_pieces() & ~bitboard;
    ull square = utility::bit::bitboard_to_lowest_square(bitboard);

    // north
    ull masked_blockers_n = all_pieces_but_self & north_square_ray[square];
    int blocked_square = utility::bit::bitboard_to_lowest_square(masked_blockers_n);
    ull n_attacks = ~north_square_ray[blocked_square] & north_square_ray[square];

    // south 
    ull masked_blockers_s = all_pieces_but_self & south_square_ray[square];
    blocked_square = utility::bit::bitboard_to_highest_square(masked_blockers_s);
    ull s_attacks = ~south_square_ray[blocked_square] & south_square_ray[square];

    // left
    ull masked_blockers_w = all_pieces_but_self & west_square_ray[square];
    blocked_square = utility::bit::bitboard_to_lowest_square(masked_blockers_w);
    ull w_attacks = ~west_square_ray[blocked_square] & west_square_ray[square];

    // right
    ull masked_blockers_e = all_pieces_but_self & east_square_ray[square];
    blocked_square = utility::bit::bitboard_to_highest_square(masked_blockers_e);
    ull e_attacks = ~east_square_ray[blocked_square] & east_square_ray[square];
    return n_attacks | s_attacks | w_attacks | e_attacks;
}

} // end namespace ShumiChess
#include <functional>

#include "engine.hpp"
#include "utility"

using namespace std;

namespace ShumiChess {
Engine::Engine() : game_board() {
}

Engine::Engine(const string& fen_notation) : game_board(fen_notation) {
}

void Engine::reset_engine() {
    game_board = GameBoard();
    move_history = stack<Move>();
    halfway_move_history = stack<int>();
    castle_opportunity_history = stack<uint8_t>();
}

// understand why this is ok (vector can be returned even though on stack), move ellusion? 
// https://stackoverflow.com/questions/15704565/efficient-way-to-return-a-stdvector-in-c
vector<Move> Engine::get_legal_moves() {
    vector<Move> all_legal_moves;
    Color color = game_board.turn;
    Color opposite_color = utility::representation::get_opposite_color(color);
    vector<Move> psuedo_legal_moves = get_psuedo_legal_moves(color);
    all_legal_moves.reserve(psuedo_legal_moves.size());

    for (Move move : psuedo_legal_moves) {
        // !
        // TODO once pop is done, uncomment this, and comment out the push_back() below 
        // push(move);
        // ull friendly_king = game_board.get_pieces(color, Piece::KING);
        
        // // ? probably don't need knights here because pins cannot happen with knights, but we don't check if king is in check yet
        // ull straight_attacks_from_king = get_straight_attacks(friendly_king);
        // ull diagonal_attacks_from_king = get_diagonal_attacks(friendly_king);
        
        // ull deadly_straight = game_board.get_pieces(opposite_color, Piece::QUEEN) | game_board.get_pieces(opposite_color, Piece::BISHOP);
        // ull deadly_diags = game_board.get_pieces(opposite_color, Piece::QUEEN) | game_board.get_pieces(opposite_color, Piece::ROOK);
        
        // // is NOT in check after making the move
        // if (!(deadly_straight & straight_attacks_from_king) &&
        //     !(deadly_diags & diagonal_attacks_from_king)) {
        //         all_legal_moves.push_back(move);
        //     }
        // pop();
        // !
     
        all_legal_moves.push_back(move);
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

// takes a move, but tracks it so pop() can undo
void Engine::push(const Move& move) {
    move_history.push(move);

    this->game_board.turn = utility::representation::get_opposite_color(move.color);

    this->game_board.fullmove += static_cast<int>(move.color == ShumiChess::Color::BLACK); //Fullmove incs on white only
    ++this->game_board.halfmove;
    if(move.piece_type == ShumiChess::Piece::PAWN) {
        this->game_board.halfmove = 0;
    }
    
    ull& moving_piece = access_piece_of_color(move.piece_type, move.color);
    moving_piece &= ~move.from;

    if (!move.promotion) {
        moving_piece |= move.to;
    }
    else {
        access_piece_of_color(*move.promotion, move.color) |= move.to;
    }
    if (move.capture) {
        this->game_board.halfmove = 0;
        if (!move.is_en_passent_capture) {
            access_piece_of_color(*move.capture, utility::representation::get_opposite_color(move.color)) &= ~move.to;
        } else {
            ull target_pawn_bitboard = move.color == ShumiChess::Color::WHITE ? move.to >> 8 : move.to << 8;
            access_piece_of_color(*move.capture, utility::representation::get_opposite_color(move.color)) &= ~target_pawn_bitboard;
        }
    } else if (move.is_castle_move) {  
        ull& friendly_rooks = access_piece_of_color(ShumiChess::Piece::ROOK, move.color);
        //TODO  Figure out the generic 2 if (castle side) solution, not 4 (castle side x color)
        std::cout << "castle" << std::endl;
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //Queenside Castle
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<7);
                friendly_rooks |= (1ULL<<4);
            } else {
                friendly_rooks &= ~(1ULL<<63);
                friendly_rooks |= (1ULL<<60);
            }
        } else {
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<0);
                friendly_rooks |= (1ULL<<2);
            } else {
                friendly_rooks &= ~(1ULL<<56);
                friendly_rooks |= (1ULL<<58);
            }
        }
    }
    this->game_board.en_passant = move.en_passent;

    this->halfway_move_history.push(this->game_board.halfmove);
    
    this->game_board.black_castle &= move.black_castle;
    this->game_board.white_castle &= move.white_castle;
    ull castle_opp = this->game_board.black_castle << 2 &&
                     this->game_board.white_castle;
    this->castle_opportunity_history.push(castle_opp);
}

// ? should this check for draws by internally calling get legal moves and caching that and returning on the actual call?
GameState Engine::game_over() {
    if (!game_board.white_king) {
        return GameState::BLACKWIN;
    }
    else if (!game_board.black_king) {
        return GameState::WHITEWIN;
    }
    // TODO check if this is off by one or something
    else if (game_board.halfmove >= 50) {
        return GameState::DRAW;
    }
    return GameState::INPROGRESS;
}

ull& Engine::access_piece_of_color(Piece piece, Color color) {
    switch (piece)
    {
    case Piece::PAWN:
        if (color) {return std::ref(this->game_board.black_pawns);}
        else {return std::ref(this->game_board.white_pawns);}
        break;
    case Piece::ROOK:
        if (color) {return std::ref(this->game_board.black_rooks);}
        else {return std::ref(this->game_board.white_rooks);}
        break;
    case Piece::KNIGHT:
        if (color) {return std::ref(this->game_board.black_knights);}
        else {return std::ref(this->game_board.white_knights);}
        break;
    case Piece::BISHOP:
        if (color) {return std::ref(this->game_board.black_bishops);}
        else {return std::ref(this->game_board.white_bishops);}
        break;
    case Piece::QUEEN:
        if (color) {return std::ref(this->game_board.black_queens);}
        else {return std::ref(this->game_board.white_queens);}
        break;
    case Piece::KING:
        if (color) {return std::ref(this->game_board.black_king);}
        else {return std::ref(this->game_board.white_king);}
        break;
    }
    // TODO remove this, i'm just putting it here because it prevents a warning
    return this->game_board.white_king;
}

// undos last move, errors if no move was made before
// TODO not completed
void Engine::pop() {
    this->move_history.pop();
    const Move move = this->move_history.top();
}

Piece Engine::get_piece_on_bitboard(ull bitboard) {
    vector<Piece> all_piece_types = { Piece::PAWN, Piece::ROOK, Piece::KNIGHT, Piece::BISHOP, Piece::QUEEN, Piece::KING };
    for (auto piece_type : all_piece_types) {
        if (game_board.get_pieces(piece_type) & bitboard) {
            return piece_type;
        }
    }
    assert(false);
    // TODO remove this, i'm just putting it here because it prevents a warning
    return Piece::KING;
}

void Engine::add_move_to_vector(vector<Move>& moves, ull single_bitboard_from, ull bitboard_to, Piece piece, Color color, bool capture, bool promotion, ull en_passent, bool is_en_passent_capture, bool is_castle) {
    // code to actually pop all the potential squares and add them as moves
    while (bitboard_to) {
        ull single_bitboard_to = utility::bit::lsb_and_pop(bitboard_to);
        std::optional<Piece> piece_captured = nullopt;
        if (capture) {
            if (is_en_passent_capture) {
                piece_captured = Piece::PAWN;
            }
            else {
                piece_captured = { get_piece_on_bitboard(single_bitboard_to) };
            }
        }

        Move new_move;
        new_move.color = color;
        new_move.piece_type = piece;
        new_move.from = single_bitboard_from;
        new_move.to = single_bitboard_to;
        new_move.capture = piece_captured;
        new_move.en_passent = en_passent;
        new_move.is_en_passent_capture = is_en_passent_capture;

        if (!promotion) {
            new_move.promotion = std::nullopt;
            moves.push_back(new_move);
        }
        else {
            for (auto& promo_piece : promotion_values) {
                Move promo_move = new_move;
                new_move.promotion = promo_piece;
                moves.push_back(new_move);
            }
        }
    }
}

void Engine::add_pawn_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {    
    // get just pawns of correct color
    ull pawns = game_board.get_pieces(color, Piece::PAWN);

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
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::get_opposite_color(color));

    while (pawns) {
        // pop and get one pawn bitboard
        ull single_pawn = utility::bit::lsb_and_pop(pawns);
        
        // single moves forward, don't check for promotions
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

        // enpassant attacks
        // TODO improvement here, because we KNOW that enpassant results in the capture of a pawn, but it adds a lot of code here to get the speed upgrade. Words fine as is
        ull enpassant_end_location = (attack_fleft | attack_fright) & game_board.en_passant;
        add_move_to_vector(all_psuedo_legal_moves, single_pawn, enpassant_end_location, Piece::PAWN, color, 
                     true, false, 0ULL, true, false);
    }
}

void Engine::add_knight_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull knights = game_board.get_pieces(color, Piece::KNIGHT);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::get_opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (knights) {
        ull single_knight = utility::bit::lsb_and_pop(knights);
        ull avail_attacks = tables::movegen::knight_attack_table[utility::bit::bitboard_to_square(single_knight)];
        
        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(all_psuedo_legal_moves, single_knight, enemy_piece_attacks, Piece::KNIGHT, color, true, false, 0ULL, false, false);

        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(all_psuedo_legal_moves, single_knight, non_attack_moves, Piece::KNIGHT, color, false, false, 0ULL, false, false);
    }
}

void Engine::add_rook_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull rooks = game_board.get_pieces(color, Piece::ROOK);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::get_opposite_color(color));
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
    ull bishops = game_board.get_pieces(color, Piece::BISHOP);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::get_opposite_color(color));
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
    vector<Move> queen_moves;
    ull queens = game_board.get_pieces(color, Piece::QUEEN);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::get_opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (queens) {
        ull single_queen = utility::bit::lsb_and_pop(queens);
        ull avail_attacks = get_diagonal_attacks(single_queen) | get_straight_attacks(single_queen);

        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        add_move_to_vector(queen_moves, single_queen, enemy_piece_attacks, Piece::QUEEN, color, true, false, 0ULL, false, false);
    
        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        add_move_to_vector(queen_moves, single_queen, non_attack_moves, Piece::QUEEN, color, false, false, 0ULL, false, false);
    }
}

// assumes 1 king exists per color
void Engine::add_king_moves_to_vector(vector<Move>& all_psuedo_legal_moves, Color color) {
    ull king = game_board.get_pieces(color, Piece::KING);
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::get_opposite_color(color));
    ull all_pieces = game_board.get_pieces();
    ull own_pieces = game_board.get_pieces(color);

    ull avail_attacks = tables::movegen::king_attack_table[utility::bit::bitboard_to_square(king)];

    // captures
    ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
    add_move_to_vector(all_psuedo_legal_moves, king, enemy_piece_attacks, Piece::KING, color, true, false, 0ULL, false, false);

    // non-capture moves
    ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
    add_move_to_vector(all_psuedo_legal_moves, king, non_attack_moves, Piece::KING, color, false, false, 0ULL, false, false);
    
    // castling
    // TODO worry about check
    // if (color == Color::WHITE) {
    //     if (game_board.white_castle & (0b00000000 << 0) && 
    //             0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00000110 & ~all_pieces) {
    //         add_move_to_vector(all_psuedo_legal_moves, king, non_attack_moves, Piece::KING, color, false, false, 0ULL, false, true);
    //     }
    // }
    // else {

    // }
}

ull Engine::get_diagonal_attacks(ull bitboard) {
    ull all_pieces_but_self = game_board.get_pieces() & ~bitboard;
    ull attacks = 0;

    ull curr = bitboard;
    // up left
    for (int i = 0; i < 7; i++) {
        curr = (curr & ~row_masks[Row::ROW_8] & ~col_masks[Col::COL_A] & ~all_pieces_but_self) << 9;
        attacks |= curr;
    }

    // down left
    curr = bitboard;
    for (int i = 0; i < 7; i++) {
        curr = (curr & ~row_masks[Row::ROW_1] & ~col_masks[Col::COL_A] & ~all_pieces_but_self) >> 7;
        attacks |= curr;
    }

    // up right
    curr = bitboard;
    for (int i = 0; i < 7; i++) {
        curr = (curr & ~row_masks[Row::ROW_8] & ~col_masks[Col::COL_H] & ~all_pieces_but_self) << 7;
        attacks |= curr;
    }

    // down right
    curr = bitboard;
    for (int i = 0; i < 7; i++) {
        curr = (curr & ~row_masks[Row::ROW_1] & ~col_masks[Col::COL_H] & ~all_pieces_but_self) >> 9;
        attacks |= curr;
    }
    return attacks;
}

ull Engine::get_straight_attacks(ull bitboard) {
    ull all_pieces_but_self = game_board.get_pieces() & ~bitboard;
    ull attacks = 0;

    ull curr = bitboard;
    // left
    for (int i = 0; i < 7; i++) {
        curr = (curr & ~col_masks[Col::COL_A] & ~all_pieces_but_self) << 1;
        attacks |= curr;
    }

    // right
    curr = bitboard;
    for (int i = 0; i < 7; i++) {
        curr = (curr & ~col_masks[Col::COL_H] & ~all_pieces_but_self) >> 1;
        attacks |= curr;
    }

    // up
    curr = bitboard;
    for (int i = 0; i < 7; i++) {
        curr = (curr & ~row_masks[Row::ROW_8] & ~all_pieces_but_self) << 8;
        attacks |= curr;
    }

    // down
    curr = bitboard;
    for (int i = 0; i < 7; i++) {
        curr = (curr & ~row_masks[Row::ROW_1] & ~all_pieces_but_self) >> 8;
        attacks |= curr;
    }
    return attacks;
}

} // end namespace ShumiChess
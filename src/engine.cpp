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
}

// understand why this is ok (vector can be returned even though on stack), move ellusion? 
// https://stackoverflow.com/questions/15704565/efficient-way-to-return-a-stdvector-in-c
// TODO ignoring check atm
vector<Move> Engine::get_legal_moves() {
    vector<Move> all_moves;
    Color color = game_board.turn;

    for(Move move : get_pawn_moves(color)) 
        all_moves.push_back(move);
    for(Move move : get_rook_moves(color)) 
        all_moves.push_back(move);
    for(Move move : get_bishop_moves(color)) 
        all_moves.push_back(move);
    for(Move move : get_queen_moves(color)) 
        all_moves.push_back(move);
    for(Move move : get_king_moves(color)) 
        all_moves.push_back(move);
    for(Move move : get_knight_moves(color)) 
        all_moves.push_back(move);

    return all_moves;
}

// takes a move, but tracks it so pop() can undo
void Engine::push(const Move& move) {
    move_history.push(move);

    this->game_board.turn = utility::representation::get_opposite_color(move.color);

    ++this->game_board.fullmove;
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
        access_piece_of_color(*move.capture, utility::representation::get_opposite_color(move.color)) &= ~move.to;
    }
    this->game_board.en_passant = move.en_passent;

    this->halfway_move_history.push(this->game_board.halfmove);
    ull castle_opp = move.black_castle << 2 && 
                     this->game_board.black_castle << 2 &&
                     move.white_castle &&
                     this->game_board.white_castle;
    this->castle_opportunity_history.push(castle_opp);
}

ull& Engine::access_piece_of_color(ShumiChess::Piece piece, ShumiChess::Color color) {
    switch (piece)
    {
    case ShumiChess::Piece::PAWN:
        if (color) {return std::ref(this->game_board.black_pawns);}
        else {return std::ref(this->game_board.white_pawns);}
        break;
    case ShumiChess::Piece::ROOK:
        if (color) {return std::ref(this->game_board.black_rooks);}
        else {return std::ref(this->game_board.white_rooks);}
        break;
    case ShumiChess::Piece::KNIGHT:
        if (color) {return std::ref(this->game_board.black_knights);}
        else {return std::ref(this->game_board.white_knights);}
        break;
    case ShumiChess::Piece::BISHOP:
        if (color) {return std::ref(this->game_board.black_bishops);}
        else {return std::ref(this->game_board.white_bishops);}
        break;
    case ShumiChess::Piece::QUEEN:
        if (color) {return std::ref(this->game_board.black_queens);}
        else {return std::ref(this->game_board.white_queens);}
        break;
    case ShumiChess::Piece::KING:
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

void Engine::add_as_moves(vector<Move>& moves, ull single_bitboard_from, ull bitboard_to, Piece piece, Color color, bool capture, bool promotion, ull en_passent, bool is_en_passent_capture) {
    // code to actually pop all the potential squares and add them as moves
    vector<std::optional<Piece>> promotion_values;
    if (promotion) {
        promotion_values.push_back(std::optional<Piece> {Piece::BISHOP});
        promotion_values.push_back(std::optional<Piece> {Piece::KNIGHT});
        promotion_values.push_back(std::optional<Piece> {Piece::QUEEN});
        promotion_values.push_back(std::optional<Piece> {Piece::ROOK});
    }
    else {
        promotion_values.push_back(std::nullopt);
    }

    while (bitboard_to) {
        ull single_bitboard_to = utility::bit::lsb_and_pop(bitboard_to);
        std::optional<Piece> piece_captured = nullopt;
        if (capture) {
            utility::bit::print_bitboard(single_bitboard_to);
            if (is_en_passent_capture) {
                piece_captured = Piece::PAWN;
            }
            else {
                piece_captured = { get_piece_on_bitboard(single_bitboard_to) };
            }
        }

        for (auto promo_piece : promotion_values) {
            Move new_move;
            new_move.from = single_bitboard_from;
            new_move.to = single_bitboard_to;
            new_move.piece_type = piece;
            new_move.color = color;
            new_move.capture = piece_captured;
            new_move.promotion = promo_piece;
            new_move.en_passent = en_passent;
            moves.push_back(new_move);
        }
    }
}

vector<Move> Engine::get_pawn_moves(Color color) {
    vector<Move> pawn_moves;
    
    // get just pawns of correct color
    ull pawns = game_board.get_pieces(color, Piece::PAWN);

    // grab variables that will be used several times
    ull pawn_enemy_starting_rank_mask = rank_masks[7];
    ull pawn_starting_rank_mask = rank_masks[2];
    ull pawn_enpassant_rank_mask = rank_masks[3];
    ull far_right_row = col_masks['h'];
    ull far_left_row = col_masks['a'];
    if (color == Color::BLACK) {
        pawn_enemy_starting_rank_mask = rank_masks[2];
        pawn_enpassant_rank_mask = rank_masks[6];
        pawn_starting_rank_mask = rank_masks[7];
        far_right_row = col_masks['a'];
        far_left_row = col_masks['h'];
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
        add_as_moves(pawn_moves, single_pawn, spaces_to_move, Piece::PAWN, color, false, false, 0ULL, false);

        // move up two ranks
        ull is_doublable = single_pawn & pawn_starting_rank_mask;
        if (is_doublable) {
            // TODO share more code with single pushes above
            ull move_forward_one = utility::bit::bitshift_by_color(single_pawn, color, 8);
            ull move_forward_one_blocked = move_forward_one & ~all_pieces;
            ull move_forward_two = utility::bit::bitshift_by_color(move_forward_one_blocked, color, 8);
            ull move_forward_two_blocked = move_forward_two & ~all_pieces;
            add_as_moves(pawn_moves, single_pawn, move_forward_two_blocked, Piece::PAWN, color, false, false, move_forward_one_blocked, false);
        }


        // promotions
        ull potential_promotion = utility::bit::bitshift_by_color(single_pawn & pawn_enemy_starting_rank_mask, color, 8); 
        ull promotion_not_blocked = potential_promotion & ~all_pieces;
        ull promo_squares = promotion_not_blocked;
        add_as_moves(pawn_moves, single_pawn, promo_squares, Piece::PAWN, color, 
                     false, true, 0ULL, false);

        // attacks forward left and forward right, also includes promotions like this
        ull attack_fleft = utility::bit::bitshift_by_color(single_pawn & ~far_left_row, color, 9);
        ull attack_fright = utility::bit::bitshift_by_color(single_pawn & ~far_right_row, color, 7);
        ull normal_attacks = attack_fleft & all_enemy_pieces;
        normal_attacks |= attack_fright & all_enemy_pieces;
        add_as_moves(pawn_moves, single_pawn, normal_attacks, Piece::PAWN, color, 
                     true, (bool) normal_attacks & pawn_enemy_starting_rank_mask, 0ULL, false);

        // enpassant attacks
        // TODO improvement here, because we KNOW that enpassant results in the capture of a pawn, but it adds a lot of code here to get the speed upgrade. Words fine as is
        ull enpassant_end_location = (attack_fleft | attack_fright) & game_board.en_passant;
        add_as_moves(pawn_moves, single_pawn, enpassant_end_location, Piece::PAWN, color, 
                     true, false, 0ULL, true);
    }

    return pawn_moves;
}

vector<Move> Engine::get_knight_moves(Color color) {
    vector<Move> knight_moves;
    return knight_moves;
}

vector<Move> Engine::get_rook_moves(Color color) {
    vector<Move> rook_moves;
    return rook_moves;
}

vector<Move> Engine::get_bishop_moves(Color color) {
    vector<Move> bishop_moves;
    return bishop_moves;
}

vector<Move> Engine::get_queen_moves(Color color) {
    vector<Move> queen_moves;
    return queen_moves; }

vector<Move> Engine::get_king_moves(Color color) {
    vector<Move> king_moves;
    return king_moves;
}
} // end namespace ShumiChess
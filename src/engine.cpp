#include <functional>

#include "engine.hpp"
#include "utility"

using namespace std;
using namespace utility;
using namespace utility::representation;
using namespace utility::bit;

namespace ShumiChess {
Engine::Engine() {
    reset_engine();
    ShumiChess::initialize_rays();
    // cout << colorize(AnsiColor::BRIGHT_BLUE, "south_west_square_ray[25]") << endl;
    // print_bitboard(south_west_square_ray[25]);
    // cout << colorize(AnsiColor::BRIGHT_BLUE, "south_west_square_ray[28]") << endl;
    // print_bitboard(south_west_square_ray[28]);
    // cout << colorize(AnsiColor::BRIGHT_BLUE, "south_west_square_ray[37]") << endl;
    // print_bitboard(south_west_square_ray[37]);
}

//TODO what is right way to handle popping past default state here?
Engine::Engine(const string& fen_notation) : game_board(fen_notation) {
}

void Engine::reset_engine() {
    game_board = GameBoard();
    move_history = stack<Move>();
    three_fold_rep_draw = false;
    count_zobrist_states.clear();
}

tuple<Move*, int> Engine::get_legal_moves() {
    Color color = game_board.turn;
    Move* curr_move = moves;
    curr_move = add_pawn_moves_to_vector(curr_move, color); 
    curr_move = add_rook_moves_to_vector(curr_move, color); 
    curr_move = add_queen_moves_to_vector(curr_move, color); 
    curr_move = add_king_moves_to_vector(curr_move, color); 
    curr_move = add_knight_moves_to_vector(curr_move, color);
    // 
    return make_tuple(moves, curr_move - moves);
}

GameState Engine::game_over() {
    tuple<Move*, int> legal_moves_tuple = get_legal_moves();
    return game_over(legal_moves_tuple);
}

GameState Engine::game_over(tuple<Move*, int> legal_moves_tuple) {
    return game_over(get<0>(legal_moves_tuple), get<1>(legal_moves_tuple));
}

GameState Engine::game_over(Move* all_moves, int num_moves) {
    if (num_moves == 0) {
        return GameState::DRAW;
    }
    if (three_fold_rep_draw == true) {
        return GameState::DRAW;
    }
    if (game_board.black_king == 0 && game_board.white_king == 0) {
        return GameState::DRAW;
    }
    if (game_board.black_king == 0) {
        return GameState::WHITEWIN;
    }
    if (game_board.white_king == 0) {
        return GameState::BLACKWIN;
    }
    ull white_win_squares = 0b00000000'00000000'00000000'00000000'10000000'10000000'10000000'11110000;
    if ((game_board.get_pieces_template<Piece::PAWN, Color::WHITE>() & white_win_squares) != 0ULL) {
        return GameState::WHITEWIN;
    }
    ull black_win_squares = 0b00001111'00000001'00000001'00000001'00000000'00000000'00000000'00000000;
    if ((game_board.get_pieces_template<Piece::PAWN, Color::BLACK>() & black_win_squares) != 0) {
        return GameState::BLACKWIN;
    }
    return GameState::INPROGRESS;
}

void Engine::push(const Move& move) {
    // cout << "starting push" << endl;
    move_history.push(move);

    this->game_board.turn = opposite_color(move.color);
    game_board.zobrist_key ^= zobrist_side;
    
    int square_from = bitboard_to_lowest_square(move.from);
    int square_to = bitboard_to_lowest_square(move.to);


    if (move.is_laser) {
        for (int i = 0; i < move.lasered_pieces; i++) {
            ull& to_kill_piece = access_piece_of_color(move.lasered_piece[i], move.lasered_color[i]);
            to_kill_piece &= ~move.lasered_bitboard[i];

            int laser_square = bitboard_to_lowest_square(move.lasered_bitboard[i]);
            game_board.zobrist_key ^= zobrist_piece_square[move.lasered_piece[i] + move.lasered_color[i] * 5][laser_square];
        }
    }
    else {
        ull& moving_piece = access_piece_of_color(move.piece_type, move.color);

        moving_piece |= move.to;
        moving_piece &= ~move.from;

        game_board.zobrist_key ^= zobrist_piece_square[move.piece_type + move.color * 5][square_from];
        game_board.zobrist_key ^= zobrist_piece_square[move.piece_type + move.color * 5][square_to];
    }
    if (move.capture != Piece::NONE) {
        access_piece_of_color(move.capture, opposite_color(move.color)) &= ~move.to;
        game_board.zobrist_key ^= zobrist_piece_square[move.capture + opposite_color(move.color) * 5][square_to];
    }
    count_zobrist_states[game_board.zobrist_key]++;
    if (count_zobrist_states[game_board.zobrist_key] == 3) {
        three_fold_rep_draw = true;
    }
    // cout << "ending push" << endl;
}

// undos last move, errors if no move was made before
void Engine::pop() {
    // cout << "starting pop" << endl;
    count_zobrist_states[game_board.zobrist_key]--;
    three_fold_rep_draw = false;
    const Move move = this->move_history.top();
    this->move_history.pop();

    game_board.zobrist_key ^= zobrist_side;

    this->game_board.turn = move.color;

    int square_from = bitboard_to_lowest_square(move.from);
    int square_to = bitboard_to_lowest_square(move.to);

    if (move.is_laser) {
        for (int i = 0; i < move.lasered_pieces; i++) {
            ull& to_regen_piece = access_piece_of_color(move.lasered_piece[i], move.lasered_color[i]);
            to_regen_piece |= move.lasered_bitboard[i];

            int laser_square = bitboard_to_lowest_square(move.lasered_bitboard[i]);
            game_board.zobrist_key ^= zobrist_piece_square[move.lasered_piece[i] + move.lasered_color[i] * 5][laser_square];
        }
    }
    else {
        ull& moving_piece = access_piece_of_color(move.piece_type, move.color);
        moving_piece &= ~move.to;
        moving_piece |= move.from;

        game_board.zobrist_key ^= zobrist_piece_square[move.piece_type + move.color * 5][square_from];
        game_board.zobrist_key ^= zobrist_piece_square[move.piece_type + move.color * 5][square_to];
    }

    if (move.capture != Piece::NONE) {
        access_piece_of_color(move.capture, opposite_color(move.color)) |= move.to;
        game_board.zobrist_key ^= zobrist_piece_square[move.capture + opposite_color(move.color) * 5][square_to];
    }
    // cout << "ending pop" << endl;
}

ull& Engine::access_piece_of_color(Piece piece, Color color) {
    switch (piece)
    {
    case Piece::PAWN:
        if (color) {return ref(this->game_board.black_pawns);}
        else {return ref(this->game_board.white_pawns);}
        break;
    case Piece::ROOK:
        if (color) {return ref(this->game_board.black_rooks);}
        else {return ref(this->game_board.white_rooks);}
        break;
    case Piece::KNIGHT:
        if (color) {return ref(this->game_board.black_knights);}
        else {return ref(this->game_board.white_knights);}
        break;
    case Piece::QUEEN:
        if (color) {return ref(this->game_board.black_queens);}
        else {return ref(this->game_board.white_queens);}
        break;
    case Piece::KING:
        if (color) {return ref(this->game_board.black_king);}
        else {return ref(this->game_board.white_king);}
        break;
    }
    return this->game_board.white_king; // warning prevention
}

Move* Engine::add_pawn_moves_to_vector(Move* curr_move, Color color) {    
    ull pawns = game_board.get_pieces_template<Piece::PAWN>(color);

    ull all_pieces = game_board.get_pieces();
    ull all_enemy_pieces = game_board.get_pieces(opposite_color(color));

    while (pawns) {
        ull single_pawn = lsb_and_pop(pawns);
        
        // moves diagonols
        ull move_fleft = (single_pawn & ~col_masks[Col::COL_A]) << 9;
        ull move_fright = (single_pawn & ~col_masks[Col::COL_H]) << 7;
        ull move_rleft = (single_pawn & ~col_masks[Col::COL_A]) >> 7;
        ull move_rright = (single_pawn & ~col_masks[Col::COL_H]) >> 9;
        ull non_attacks = (move_fleft | move_fright | move_rleft | move_rright) & ~all_pieces;
        curr_move = add_move_to_vector(curr_move, single_pawn, non_attacks, Piece::PAWN, color, false, false, 0ULL);

        // attacks sides
        ull attack_up = (single_pawn & ~row_masks[Row::ROW_8]) << 8;
        ull attack_down = (single_pawn & ~row_masks[Row::ROW_1]) >> 8;
        ull attack_right = (single_pawn & ~col_masks[Col::COL_H]) >> 1;
        ull attack_left = (single_pawn & ~col_masks[Col::COL_A]) << 1;

        ull attacks = (attack_up | attack_down | attack_right | attack_left) & all_enemy_pieces;
        curr_move = add_move_to_vector(curr_move, single_pawn, attacks, Piece::PAWN, color, true, false, 0ULL);
    }
    return curr_move;
}

Move* Engine::add_knight_moves_to_vector(Move* curr_move, Color color) {
    ull knights = game_board.get_pieces_template<Piece::KNIGHT>(color);
    ull all_enemy_pieces = game_board.get_pieces(opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (knights) {
        ull single_knight = lsb_and_pop(knights);
        ull avail_attacks = tables::movegen::knight_attack_table[bitboard_to_lowest_square(single_knight)];
        
        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        curr_move = add_move_to_vector(curr_move, single_knight, enemy_piece_attacks, Piece::KNIGHT, color, true, false, 0ULL);

        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        curr_move = add_move_to_vector(curr_move, single_knight, non_attack_moves, Piece::KNIGHT, color, false, false, 0ULL);
    }
    return curr_move;
}

Move* Engine::add_rook_moves_to_vector(Move* curr_move, Color color) {
    ull rooks = game_board.get_pieces_template<Piece::ROOK>(color);
    ull all_enemy_pieces = game_board.get_pieces(opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (rooks) {
        ull single_rook = lsb_and_pop(rooks);
        ull avail_attacks = get_straight_attacks(single_rook);

        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        curr_move = add_move_to_vector(curr_move, single_rook, enemy_piece_attacks, Piece::ROOK, color, true, false, 0ULL);

        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        curr_move = add_move_to_vector(curr_move, single_rook, non_attack_moves, Piece::ROOK, color, false, false, 0ULL);
    }
    return curr_move;
}

Move* Engine::add_queen_moves_to_vector(Move* curr_move, Color color) {
    ull queens = game_board.get_pieces_template<Piece::QUEEN>(color);
    ull all_pieces = game_board.get_pieces();
    ull all_enemy_pieces = game_board.get_pieces(opposite_color(color));
    ull own_pieces = game_board.get_pieces(color);

    while (queens) {
        ull single_queen = lsb_and_pop(queens);

        // moves
        ull avail_moves = get_straight_attacks(single_queen) & ~all_pieces;
        curr_move = add_move_to_vector(curr_move, single_queen, avail_moves, Piece::QUEEN, color, false, false, 0ULL);

        // laser
        ull square = bitboard_to_lowest_square(single_queen);
        ull rooks = game_board.get_pieces_template<Piece::ROOK>();
        ull transposed_rooks;

        ull top_row = row_masks[Row::ROW_8];
        ull bottom_row = row_masks[Row::ROW_1];
        ull right_col = col_masks[Col::COL_H];
        ull left_col = col_masks[Col::COL_A];


        // up right
        transposed_rooks = (rooks & ~left_col & ~bottom_row) >> 7;
        ull up_and_right_far = ~rooks & ~single_queen & highest_bitboard(north_east_square_ray[square]);
        ull masked_blockers_ne = (transposed_rooks | up_and_right_far) & north_east_square_ray[square];
        ull ne_attacks = ~single_queen & lowest_bitboard(masked_blockers_ne);
        if (ne_attacks != 0) {
            ull back_ray = ne_attacks | (north_east_square_ray[square] & south_west_square_ray[bitboard_to_highest_square(ne_attacks)]);
            curr_move = add_move_to_vector(curr_move, single_queen, ne_attacks, Piece::QUEEN, color, false, true, back_ray);
        }

        // up left
        transposed_rooks = (rooks & ~left_col & ~top_row) >> 9;
        ull up_and_left_far = ~rooks & ~single_queen & highest_bitboard(north_west_square_ray[square]);
        ull masked_blockers_nw = (transposed_rooks | up_and_left_far) & north_west_square_ray[square];
        ull nw_attacks = ~single_queen & lowest_bitboard(masked_blockers_nw);
        if (nw_attacks != 0) {
            ull back_ray = nw_attacks | (north_west_square_ray[square] & south_east_square_ray[bitboard_to_highest_square(nw_attacks)]);
            curr_move = add_move_to_vector(curr_move, single_queen, nw_attacks, Piece::QUEEN, color, false, true, back_ray);
        }

        // down right
        transposed_rooks = (rooks & ~right_col & ~bottom_row) << 9;
        ull down_and_right_far = ~rooks & ~single_queen & lowest_bitboard(south_east_square_ray[square]);
        ull masked_blockers_se = (transposed_rooks | down_and_right_far) & south_east_square_ray[square];
        ull se_attacks = ~single_queen & highest_bitboard(masked_blockers_se);
        if (se_attacks != 0) {
            ull back_ray = se_attacks | (south_east_square_ray[square] & north_west_square_ray[bitboard_to_lowest_square(se_attacks)]);
            curr_move = add_move_to_vector(curr_move, single_queen, se_attacks, Piece::QUEEN, color, false, true, back_ray);
        }

        // down left
        transposed_rooks = (rooks & ~left_col & ~bottom_row) << 7;
        ull down_and_left_far = ~rooks & ~single_queen & lowest_bitboard(south_west_square_ray[square]);
        ull masked_blockers_sw = (transposed_rooks | down_and_left_far) & south_west_square_ray[square];
        ull sw_attacks = ~single_queen & highest_bitboard(masked_blockers_sw);
        if (sw_attacks != 0) {
            ull back_ray = sw_attacks | (south_west_square_ray[square] & north_east_square_ray[bitboard_to_lowest_square(sw_attacks)]);
            curr_move = add_move_to_vector(curr_move, single_queen, sw_attacks, Piece::QUEEN, color, false, true, back_ray);
        }
    }
    return curr_move;
}

Move* Engine::add_king_moves_to_vector(Move* curr_move, Color color) {
    ull king = game_board.get_pieces_template<Piece::KING>(color);
    ull all_enemy_pieces = game_board.get_pieces(opposite_color(color));
    ull all_pieces = game_board.get_pieces();
    ull own_pieces = game_board.get_pieces(color);

    ull avail_attacks = tables::movegen::king_attack_table[bitboard_to_lowest_square(king)];

    // captures
    ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
    curr_move = add_move_to_vector(curr_move, king, enemy_piece_attacks, Piece::KING, color, true, false, 0ULL);

    // non-capture moves
    ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
    
    curr_move = add_move_to_vector(curr_move, king, non_attack_moves, Piece::KING, color, false, false, 0ULL);
    return curr_move;
}


ull Engine::get_straight_attacks(ull bitboard) {
    ull all_pieces_but_self = game_board.get_pieces() & ~bitboard;
    ull square = bitboard_to_lowest_square(bitboard);

    // north
    ull masked_blockers_n = all_pieces_but_self & north_square_ray[square];
    int blocked_square = bitboard_to_lowest_square(masked_blockers_n);
    ull n_attacks = ~north_square_ray[blocked_square] & north_square_ray[square];

    // south 
    ull masked_blockers_s = all_pieces_but_self & south_square_ray[square];
    blocked_square = bitboard_to_highest_square(masked_blockers_s);
    ull s_attacks = ~south_square_ray[blocked_square] & south_square_ray[square];

    // left
    ull masked_blockers_w = all_pieces_but_self & west_square_ray[square];
    blocked_square = bitboard_to_lowest_square(masked_blockers_w);
    ull w_attacks = ~west_square_ray[blocked_square] & west_square_ray[square];

    // right
    ull masked_blockers_e = all_pieces_but_self & east_square_ray[square];
    blocked_square = bitboard_to_highest_square(masked_blockers_e);
    ull e_attacks = ~east_square_ray[blocked_square] & east_square_ray[square];
    return n_attacks | s_attacks | w_attacks | e_attacks;
}

} // end namespace ShumiChess



// ull Engine::get_diagonal_attacks(ull bitboard) {
//     ull all_pieces_but_self = game_board.get_pieces() & ~bitboard;
//     ull square = bitboard_to_lowest_square(bitboard);

//     // up right
//     ull masked_blockers_ne = all_pieces_but_self & north_east_square_ray[square];
//     int blocked_square = bitboard_to_lowest_square(masked_blockers_ne);
//     ull ne_attacks = ~north_east_square_ray[blocked_square] & north_east_square_ray[square];

//     // up left
//     ull masked_blockers_nw = all_pieces_but_self & north_west_square_ray[square];
//     blocked_square = bitboard_to_lowest_square(masked_blockers_nw);
//     ull nw_attacks = ~north_west_square_ray[blocked_square] & north_west_square_ray[square];

//     // down right
//     ull masked_blockers_se = all_pieces_but_self & south_east_square_ray[square];
//     blocked_square = bitboard_to_highest_square(masked_blockers_se);
//     ull se_attacks = ~south_east_square_ray[blocked_square] & south_east_square_ray[square];

//     // down left
//     ull masked_blockers_sw = all_pieces_but_self & south_west_square_ray[square];
//     blocked_square = bitboard_to_highest_square(masked_blockers_sw);
//     ull sw_attacks = ~south_west_square_ray[blocked_square] & south_west_square_ray[square];

//     return ne_attacks | nw_attacks | se_attacks | sw_attacks;
// }
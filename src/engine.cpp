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


LegalMoves Engine::get_legal_moves() {
    Color color = game_board.turn;
    if (color == Color::WHITE) {
        return get_legal_moves<Color::WHITE>();
    } else {
        return get_legal_moves<Color::BLACK>();
    }
}


template <Color color>
LegalMoves Engine::get_legal_moves() {
    all_pieces_cache = game_board.get_pieces();

    Move* curr_move = start_of_moves;
    curr_move = add_pawn_moves_to_vector<color>(curr_move); 
    curr_move = add_rook_moves_to_vector<color>(curr_move); 
    curr_move = add_queen_moves_to_vector<color>(curr_move); 
    curr_move = add_king_moves_to_vector<color>(curr_move); 
    curr_move = add_knight_moves_to_vector<color>(curr_move);
    return {start_of_moves, (int) (curr_move - start_of_moves)};
}


GameState Engine::game_over() {
    return game_over(get_legal_moves());
}


GameState Engine::game_over(LegalMoves legal_moves) {
    if (legal_moves.num_moves == 0) {
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
    if ((game_board.get_pieces<Piece::PAWN, Color::WHITE>() & white_win_squares) != 0ULL) {
        return GameState::WHITEWIN;
    }
    ull black_win_squares = 0b00001111'00000001'00000001'00000001'00000000'00000000'00000000'00000000;
    if ((game_board.get_pieces<Piece::PAWN, Color::BLACK>() & black_win_squares) != 0) {
        return GameState::BLACKWIN;
    }
    return GameState::INPROGRESS;
}


void Engine::push(const Move& move) {
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
    if (move.is_capture != Piece::NONE) {
        access_piece_of_color(move.is_capture, opposite_color(move.color)) &= ~move.to;
        game_board.zobrist_key ^= zobrist_piece_square[move.is_capture + opposite_color(move.color) * 5][square_to];
    }
    count_zobrist_states[game_board.zobrist_key]++;
    if (count_zobrist_states[game_board.zobrist_key] == 3) {
        three_fold_rep_draw = true;
    }
}


void Engine::pop() {
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

    if (move.is_capture != Piece::NONE) {
        access_piece_of_color(move.is_capture, opposite_color(move.color)) |= move.to;
        game_board.zobrist_key ^= zobrist_piece_square[move.is_capture + opposite_color(move.color) * 5][square_to];
    }
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


template <Color color>
Move* Engine::add_pawn_moves_to_vector(Move* curr_move) {    
    ull pawns = game_board.get_pieces<Piece::PAWN, color>();

    ull all_enemy_pieces = game_board.get_pieces<opposite_color_constexpr(color)>();

    ull not_a_file = ~col_masks[Col::COL_A];
    ull not_h_file = ~col_masks[Col::COL_H];
    ull not_8_row = ~row_masks[Row::ROW_8];
    ull not_1_row = ~row_masks[Row::ROW_1];

    while (pawns) {
        ull single_pawn = lsb_and_pop(pawns);
        
        // moves diagonols
        ull move_fleft = (single_pawn & not_a_file) << 9;
        ull move_fright = (single_pawn & not_h_file) << 7;
        ull move_rleft = (single_pawn & not_a_file) >> 7;
        ull move_rright = (single_pawn & not_h_file) >> 9;
        ull non_attacks = (move_fleft | move_fright | move_rleft | move_rright) & ~all_pieces_cache;
        curr_move = add_move_to_vector<Piece::PAWN, color, false, false>(curr_move, single_pawn, non_attacks, 0ULL);

        // attacks sides
        ull attack_up = (single_pawn & not_8_row) << 8;
        ull attack_down = (single_pawn & not_1_row) >> 8;
        ull attack_right = (single_pawn & not_h_file) >> 1;
        ull attack_left = (single_pawn & not_a_file) << 1;

        ull attacks = (attack_up | attack_down | attack_right | attack_left) & all_enemy_pieces;
        curr_move = add_move_to_vector<Piece::PAWN, color, true, false>(curr_move, single_pawn, attacks, 0ULL);
    }
    return curr_move;
}


template <Color color>
Move* Engine::add_knight_moves_to_vector(Move* curr_move) {
    ull knights = game_board.get_pieces<Piece::KNIGHT, color>();
    ull all_enemy_pieces = game_board.get_pieces<opposite_color_constexpr(color)>();
    ull own_pieces = game_board.get_pieces<color>();

    while (knights) {
        ull single_knight = lsb_and_pop(knights);
        ull avail_attacks = tables::movegen::knight_attack_table[bitboard_to_lowest_square(single_knight)];
        
        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        curr_move = add_move_to_vector<Piece::KNIGHT, color, true, false>(curr_move, single_knight, enemy_piece_attacks, 0ULL);

        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        curr_move = add_move_to_vector<Piece::KNIGHT, color, false, false>(curr_move, single_knight, non_attack_moves, 0ULL);
    }
    return curr_move;
}


template <Color color>
Move* Engine::add_rook_moves_to_vector(Move* curr_move) {
    ull single_rook = game_board.get_pieces<Piece::ROOK, color>();

    if (single_rook) {
        ull all_enemy_pieces = game_board.get_pieces<opposite_color_constexpr(color)>();
        ull own_pieces = game_board.get_pieces<color>();

        ull avail_attacks = get_straight_attacks(single_rook);

        // captures
        ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
        curr_move = add_move_to_vector<Piece::ROOK, color, true, false>(curr_move, single_rook, enemy_piece_attacks, 0ULL);

        // all else
        ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
        curr_move = add_move_to_vector<Piece::ROOK, color, false, false>(curr_move, single_rook, non_attack_moves, 0ULL);
    }
    return curr_move;
}


template <Color color>
Move* Engine::add_queen_moves_to_vector(Move* curr_move) {
    ull single_queen = game_board.get_pieces<Piece::QUEEN, color>();

    if (single_queen) {
        // queen moves
        ull avail_moves = get_straight_attacks(single_queen) & ~all_pieces_cache;
        curr_move = add_move_to_vector<Piece::QUEEN, color, false, false>(curr_move, single_queen, avail_moves, 0ULL);

        // laser rays
        ull rooks = game_board.get_pieces<Piece::ROOK>();

        int queen_square = bitboard_to_lowest_square(single_queen);
        
        ull north_east_rook_cancels = ~rooks;
        ull north_west_rook_cancels = ~rooks;
        ull south_east_rook_cancels = ~rooks;
        ull south_west_rook_cancels = ~rooks;

        while (rooks) {
            ull single_rook = lsb_and_pop(rooks);

            ull single_rook_for_ne = single_rook & north_east_square_ray[queen_square];
            north_east_rook_cancels &= ~north_east_square_ray[bitboard_to_lowest_square(single_rook_for_ne)];

            ull single_rook_for_nw = single_rook & north_west_square_ray[queen_square];
            north_west_rook_cancels &= ~north_west_square_ray[bitboard_to_lowest_square(single_rook_for_nw)];

            ull single_rook_for_se = single_rook & south_east_square_ray[queen_square];
            south_east_rook_cancels &= ~south_east_square_ray[bitboard_to_lowest_square(single_rook_for_se)];

            ull single_rook_for_sw = single_rook & south_west_square_ray[queen_square];
            south_west_rook_cancels &= ~south_west_square_ray[bitboard_to_lowest_square(single_rook_for_sw)];
        }

        // up right
        ull laser_ray = north_east_rook_cancels & north_east_square_ray[queen_square];
        if (laser_ray != 0) {
            curr_move = add_move_to_vector<Piece::QUEEN, color, false, true>(curr_move, single_queen, highest_bitboard(laser_ray), laser_ray);
        }

        // up left
        laser_ray = north_west_rook_cancels & north_west_square_ray[queen_square];
        if (laser_ray != 0) {
            curr_move = add_move_to_vector<Piece::QUEEN, color, false, true>(curr_move, single_queen, highest_bitboard(laser_ray), laser_ray);
        }

        // down right
        laser_ray = south_east_rook_cancels & south_east_square_ray[queen_square];
        if (laser_ray != 0) {
            curr_move = add_move_to_vector<Piece::QUEEN, color, false, true>(curr_move, single_queen, lowest_bitboard(laser_ray), laser_ray);
        }

        // down left
        laser_ray = south_west_rook_cancels & south_west_square_ray[queen_square];
        if (laser_ray != 0) {
            curr_move = add_move_to_vector<Piece::QUEEN, color, false, true>(curr_move, single_queen, lowest_bitboard(laser_ray), laser_ray);
        }
    }
    return curr_move;
}


template <Color color>
Move* Engine::add_king_moves_to_vector(Move* curr_move) {
    ull king = game_board.get_pieces<Piece::KING, color>();
    ull all_enemy_pieces = game_board.get_pieces<opposite_color_constexpr(color)>();
    ull own_pieces = game_board.get_pieces<color>();

    ull avail_attacks = tables::movegen::king_attack_table[bitboard_to_lowest_square(king)];

    // captures
    ull enemy_piece_attacks = avail_attacks & all_enemy_pieces;
    curr_move = add_move_to_vector<Piece::KING, color, true, false>(curr_move, king, enemy_piece_attacks, 0ULL);

    // non-capture moves
    ull non_attack_moves = avail_attacks & ~own_pieces & ~enemy_piece_attacks;
    
    curr_move = add_move_to_vector<Piece::KING, color, false, false>(curr_move, king, non_attack_moves, 0ULL);
    return curr_move;
}

ull Engine::get_straight_attacks(ull bitboard) {
    ull all_pieces_but_self = all_pieces_cache & ~bitboard;
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
#include "engine.hpp"

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
// TODO implement
void Engine::push(Move move) {
}

// undos last move, errors if no move was made before
// TODO implement
void Engine::pop() {
}

vector<Move> Engine::get_pawn_moves(Color color) {
    vector<Move> pawn_moves;
    
    // get just pawns of correct color
    ull pawns = game_board.get_pieces(color, Piece::PAWN);

    // grab variables that will be used several times
    ull pawn_starting_rank_mask = rank_masks[2];
    ull pawn_enpassant_rank_mask = rank_masks[3];
    if (color == Color::BLACK) {
        pawn_enpassant_rank_mask = rank_masks[5];
        pawn_starting_rank_mask = rank_masks[6];
    }
    ull all_pieces = game_board.get_pieces();
    ull all_enemy_pieces = game_board.get_pieces(utility::representation::get_opposite_color(color));

    while (pawns) {
        ull single_pawn = utility::bit::lsb_and_pop(pawns);
        ull spaces_to_move = 0ULL;
        
        // single moves forward
        ull move_forward = utility::bit::bitshift_by_color(single_pawn, color, 8); 
        ull move_forward_blocked = move_forward & ~all_pieces;
        spaces_to_move |= move_forward_blocked;

        // attacks forward left and forward right
        ull attack_fleft = utility::bit::bitshift_by_color(single_pawn, color, 9);
        ull attack_fright = utility::bit::bitshift_by_color(single_pawn, color, 7);
        spaces_to_move |= attack_fleft & all_enemy_pieces;
        spaces_to_move |= attack_fright & all_enemy_pieces;

        // enpassant attacks
        // TODO need to return to this with push() and pop() to see if they coorperate 
        spaces_to_move |= (attack_fleft | attack_fright) & game_board.en_passant;
        
        // move up two ranks
        ull is_doublable = single_pawn & pawn_starting_rank_mask;
        if (is_doublable) {
            ull move_forward_two = utility::bit::bitshift_by_color(single_pawn, color, 8);
            ull move_forward_blocked = move_forward_two & ~all_pieces;
            move_forward_two = utility::bit::bitshift_by_color(move_forward_blocked, color, 8);
            move_forward_blocked = move_forward_two & ~all_pieces;
            spaces_to_move |= move_forward_blocked;
        }

        // code to actually pop all the potential squares and add them as moves
        while (spaces_to_move) {
            ull single_place_to_move = utility::bit::lsb_and_pop(spaces_to_move);
            Move new_move;
            new_move.from = single_pawn;
            new_move.to = single_place_to_move;
            new_move.piece_type = Piece::PAWN;
            new_move.color = color;
            pawn_moves.push_back(new_move);
        }
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
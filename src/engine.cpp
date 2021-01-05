#include <functional>

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
    move_history.push(move);

    this->game_board.turn = ShumiChess::Color::BLACK - move.color;

    ++this->game_board.fullmove;
    ++this->game_board.halfmove;
    if(move.piece_type == ShumiChess::Piece::PAWN) {this->game_board.halfmove = 0;}
    
    ull& moving_piece = access_piece_of_color(move.piece_type, move.color);
    moving_piece &= ~move.from;

    if (!move.promotion) {moving_piece |= move.to;}
    else
    {
        access_piece_of_color(*move.promotion, move.color) |= move.to;
    }
    if (move.capture) {
        this->game_board.halfmove = 0;
        access_piece_of_color(*move.capture, ShumiChess::Color::BLACK - move.color) &= ~move.to;
    }

    //castling
    //en_passant
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
}

// undos last move, errors if no move was made before
// TODO implement
void Engine::pop() {
}

vector<Move> Engine::get_pawn_moves(Color color) {
    vector<Move> pawn_moves;
    
    // single
    ull pawns = game_board.get_pieces(color, Piece::PAWN);

    while (pawns) {
        ull single_pawn = utility::representation::lsb_and_pop(pawns);
        ull spaces_to_move = 0ULL;
        
        // single moves forward
        ull move_forward = utility::representation::bitshift_by_color(single_pawn, color, 8); 
        ull move_forward_blocked = move_forward & (~game_board.get_pieces());
        spaces_to_move |= move_forward_blocked;

        // attacks
        // TODO do later, i don't feel like it right now

        // move up two ranks
        ull starting_rank_mask = rank_masks[2];
        if (color == Color::BLACK) {
            starting_rank_mask = rank_masks[6];
        }
        ull is_doulbable = single_pawn & starting_rank_mask;
        if (is_doulbable) {
            ull move_forward_two = utility::representation::bitshift_by_color(single_pawn, color, 8);
            ull move_forward_blocked = move_forward_two & (~game_board.get_pieces());
            move_forward_two = utility::representation::bitshift_by_color(move_forward_blocked, color, 8);
            move_forward_blocked = move_forward_two & (~game_board.get_pieces());
            spaces_to_move |= move_forward_blocked;
        }

        while (spaces_to_move) {
            ull single_place_to_move = utility::representation::lsb_and_pop(spaces_to_move);
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
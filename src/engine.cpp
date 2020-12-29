#include "engine.hpp"

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
vector<Move> Engine::get_legal_moves()
{
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
void Engine::push(Move move)
{
}

// undos last move, errors if no move was made before
// TODO implement
void Engine::pop()
{
}

vector<Move> Engine::get_pawn_moves(Color color)
{
    vector<Move> pawn_moves;
    // Move new_move;
    // new_move.from = 0;
    // new_move.to = 0;
    // new_move.piece_type = Piece::PAWN;
    // new_move.color = color;
    
    // single
    ull pawns = game_board.get_piece(color, Piece::PAWN);
    
    // double
    // ull rank_masks[2];

    // pawn_moves.push_back(new_move);
    return pawn_moves;
}

vector<Move> Engine::get_knight_moves(Color color)
{
    vector<Move> knight_moves;
    return knight_moves;
}

vector<Move> Engine::get_rook_moves(Color color)
{
    vector<Move> rook_moves;
    return rook_moves;
}

vector<Move> Engine::get_bishop_moves(Color color)
{
    vector<Move> bishop_moves;
    return bishop_moves;
}

vector<Move> Engine::get_queen_moves(Color color)
{
    vector<Move> queen_moves;
    return queen_moves;
}

vector<Move> Engine::get_king_moves(Color color)
{
    vector<Move> king_moves;
    return king_moves;
}
} // end namespace ShumiChess
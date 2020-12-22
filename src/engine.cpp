#include "engine.hpp"

namespace ShumiChess {
    Engine::Engine() : game_board() {
    }

    Engine::Engine(const std::string& fen_notation) : game_board(fen_notation) {
    }

    void Engine::reset_engine() {
        game_board = GameBoard();
    }

    // understand why this is ok (vector can be returned even though on stack), move ellusion? 
    // https://stackoverflow.com/questions/15704565/efficient-way-to-return-a-stdvector-in-c
    std::vector<std::string> Engine::get_legal_moves()
    {
        std::vector<std::string> all_moves;
        Color player_Color = game_board.turn;

        for(std::string move : get_pawn_moves(player_Color)) 
            all_moves.push_back(move);
        for(std::string move : get_rook_moves(player_Color)) 
            all_moves.push_back(move);
        for(std::string move : get_bishop_moves(player_Color)) 
            all_moves.push_back(move);
        for(std::string move : get_queen_moves(player_Color)) 
            all_moves.push_back(move);
        for(std::string move : get_king_moves(player_Color)) 
            all_moves.push_back(move);
        for(std::string move : get_knight_moves(player_Color)) 
            all_moves.push_back(move);

        return all_moves;
    }

    std::vector<std::string> Engine::get_pawn_moves(Color player_Color)
    {
        std::vector<std::string> pawn_moves;
        pawn_moves.push_back("fake_move");
        return pawn_moves;
    }

    std::vector<std::string> Engine::get_knight_moves(Color player_Color)
    {
        std::vector<std::string> knight_moves;
        return knight_moves;
    }

    std::vector<std::string> Engine::get_rook_moves(Color player_Color)
    {
        std::vector<std::string> rook_moves;
        return rook_moves;
    }

    std::vector<std::string> Engine::get_bishop_moves(Color player_Color)
    {
        std::vector<std::string> bishop_moves;
        return bishop_moves;
    }

    std::vector<std::string> Engine::get_queen_moves(Color player_Color)
    {
        std::vector<std::string> queen_moves;
        return queen_moves;
    }

    std::vector<std::string> Engine::get_king_moves(Color player_Color)
    {
        std::vector<std::string> king_moves;
        return king_moves;
    }
}
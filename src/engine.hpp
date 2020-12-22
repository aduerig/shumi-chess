#pragma once

#include <vector>

#include "gameboard.hpp"

namespace ShumiChess {
    class Engine {
        public:
            // Members
            // color lol;
            GameBoard game_board;
        
            // Constructors
            explicit Engine();
            explicit Engine(const std::string&);

            // Member methods
            void reset_engine();

            std::vector<std::string> get_legal_moves();
            std::vector<std::string> get_pawn_moves(color player_color);
            std::vector<std::string> get_knight_moves(color player_color);
            std::vector<std::string> get_bishop_moves(color player_color);
            std::vector<std::string> get_queen_moves(color player_color);
            std::vector<std::string> get_king_moves(color player_color);
            std::vector<std::string> get_rook_moves(color player_color);
    };
}
#pragma once

#include <vector>

#include "gameboard.hpp"

namespace ShumiChess {
    class Engine {
        public:
            // Members
            // Color lol;
            GameBoard game_board;
        
            // Constructors
            //TODO Should the engine be tied to a single boardstate
            //Should engine functions act independently and take board objects?
            explicit Engine();
            explicit Engine(const std::string&);

            // Member methods
            void reset_engine();

            std::vector<std::string> get_legal_moves();
            std::vector<std::string> get_pawn_moves(Color player_Color);
            std::vector<std::string> get_knight_moves(Color player_Color);
            std::vector<std::string> get_bishop_moves(Color player_Color);
            std::vector<std::string> get_queen_moves(Color player_Color);
            std::vector<std::string> get_king_moves(Color player_Color);
            std::vector<std::string> get_rook_moves(Color player_Color);
    };
}
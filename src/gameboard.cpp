#include <assert.h>
#include <math.h>
#include <vector>

#include "gameboard.hpp"
#include "utility.hpp"

namespace ShumiChess {
    GameBoard::GameBoard() : 
        black_pawns(0b0000000011111111000000000000000000000000000000000000000000000000),
        white_pawns(0b0000000000000000000000000000000000000000000000001111111100000000),
        black_rooks(0b1000000100000000000000000000000000000000000000000000000000000000),
        white_rooks(0b0000000000000000000000000000000000000000000000000000000010000001),
        black_knights(0b0100001000000000000000000000000000000000000000000000000000000000),
        white_knights(0b0000000000000000000000000000000000000000000000000000000001000010),
        black_bishops(0b0010010000000000000000000000000000000000000000000000000000000000),
        white_bishops(0b0000000000000000000000000000000000000000000000000000000000100100),
        black_queens(0b0001000000000000000000000000000000000000000000000000000000000000),
        white_queens(0b0000000000000000000000000000000000000000000000000000000000010000),
        black_king(0b0000100000000000000000000000000000000000000000000000000000000000),
        white_king(0b0000000000000000000000000000000000000000000000000000000000001000),
        turn(WHITE) {
    }

    //TODO
    GameBoard::GameBoard(const std::string& fen_notation) {
        const std::vector<std::string> fen_components = utility::string::split(fen_notation);
        
        assert(fen_components.size() == 6);
        assert(fen_components[1].size() == 1);

        int square_counter = 64;
        for (const char token : fen_components[0]) {
            if (token != '/') { --square_counter; }
            if (token >= 49 && token <= 56) {
                //Token is 1-8
                square_counter -= token-49; //Purposely subtract 1 too few as we always sub 1 to start.
            } else if (token == 'p') {
                this->black_pawns |= 1ULL << square_counter;
            } else if (token == 'P') {
                this->white_pawns |= 1ULL << square_counter;
            } else if (token == 'r') {
                this->black_rooks |= 1ULL << square_counter;
            } else if (token == 'R') {
                this->white_rooks |= 1ULL << square_counter;
            } else if (token == 'n') {
                this->black_knights |= 1ULL << square_counter;
            } else if (token == 'N') {
                this->white_knights |= 1ULL << square_counter;
            } else if (token == 'b') {
                this->black_bishops |= 1ULL << square_counter;
            } else if (token == 'B') {
                this->white_bishops |= 1ULL << square_counter;
            } else if (token == 'q') {
                this->black_queens |= 1ULL << square_counter;
            } else if (token == 'Q') {
                this->white_queens |= 1ULL << square_counter;
            } else if (token == 'k') {
                this->black_king |= 1ULL << square_counter;
            } else if (token == 'K') {
                this->white_king |= 1ULL << square_counter;
            }
        }
        assert(square_counter == 0);
        
        this->turn = fen_components[1] == "w" ? ShumiChess::WHITE : ShumiChess::BLACK;
    }
}
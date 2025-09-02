#pragma once

#include <cassert>
#include <iostream>
#include <stack>
#include <vector>

#include "globals.hpp"
#include "gameboard.hpp"
#include "move_tables.hpp"
#include "utility.hpp"

using namespace std;


#define _MAX_ALGEBRIAC_SIZE 16        // longest text possible? -> "exd8=Q#" or "axb8=R+"

namespace ShumiChess {
class Engine {
    public:
        // Members
        GameBoard game_board;

        std::stack<Move> move_history;
        std::stack<int> halfway_move_state;
        std::stack<ull> en_passant_history;
        //TODO This way of tracking is inconsistent with gameboard, think over. Requires splitting and merging oof
        std::stack<uint8_t> castle_opportunity_history; //Bits right to left: white kingside, white queenside, black kingside, black queenside
    
        // Constructors
        //? Should the engine be tied to a single boardstate
        //? Should engine functions act independently and take board objects?
        explicit Engine();
        explicit Engine(const string&);

        // Member methods
        void reset_engine();
        void reset_engine(const string&);

        void push(const Move&);
        void pop();
        GameState game_over();
        GameState game_over(vector<Move>&);

        // Returns direct pointer (reference) to a bit board.
        ull& access_pieces_of_color(Piece, Color);

        // void apply_en_passant_checks(const Move&);
        // void apply_castling_changes(const Move&);

        void add_move_to_vector(vector<Move>&, ull, ull, Piece, Color, bool, bool, ull, bool, bool);

        vector<Move> get_legal_moves();
        vector<Move> get_psuedo_legal_moves(Color);


        //bool in_check_after_move(Color color, const Move move);
        inline bool in_check_after_move(Color color, const Move move) {
            // NOTE: is this the most effecient way to do this (push()/pop())?
            push(move);        
            
            bool bReturn = is_king_in_check(color);
            
            pop();
            return bReturn;
        }


        bool is_king_in_check(const Color&);
        bool is_square_in_check(const Color&, const ull&);

        void add_pawn_moves_to_vector(vector<Move>&, Color);
        void add_knight_moves_to_vector(vector<Move>&, Color);
        void add_bishop_moves_to_vector(vector<Move>&, Color);
        void add_queen_moves_to_vector(vector<Move>&, Color);
        void add_king_moves_to_vector(vector<Move>&, Color);
        void add_rook_moves_to_vector(vector<Move>&, Color);

        // helpers for move generation
        ull get_diagonal_attacks(ull);
        ull get_straight_attacks(ull);

        char sz_move_text[_MAX_ALGEBRIAC_SIZE];     // longest text possible? -> "exd8=Q#" or "axb8=R+"

        void bitboards_to_algebraic(ShumiChess::Color color_that_moved, const ShumiChess::Move move, GameState state 
                                    , const vector<ShumiChess::Move>* p_legal_moves   // from this position
                                    , char* pszMoveText);            // output

        char get_piece_char(Piece p);
        char file_from_move(const Move& m);
        char rank_from_move(const Move& m);

        char file_to_move(const Move& m);
        char rank_to_move(const Move& m);

        void print_bitboard_to_file(ull bb, FILE* fp);
        void print_moves_to_file(const vector<ShumiChess::Move>& moves, FILE* fp);

        int bits_in(ull);
        bool flip_a_coin(void);

    };
} // end namespace ShumiChess
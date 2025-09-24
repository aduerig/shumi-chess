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

        void pushMove(const Move&);
        void popMove();
        
        GameState game_over();
        GameState game_over(vector<Move>&);

        // Returns direct pointer (reference) to a bit board.
        ull& access_pieces_of_color(Piece, Color);

        // void apply_en_passant_checks(const Move&);
        // void apply_castling_changes(const Move&);

        void add_move_to_vector(vector<Move>&, ull, ull, Piece, Color, bool, bool, ull, bool, bool);

        vector<Move> get_legal_moves();
        vector<Move> get_psuedo_legal_moves(Color);

        inline bool in_check_after_move(Color color, const Move move) {
            // NOTE: is this the most effecient way to do this (push()/pop())?
            pushMove(move);        
            
            bool bReturn = is_king_in_check(color);
            
            popMove();
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
        // ull get_diagonal_attacks(ull);
        // ull get_straight_attacks(ull);

        // !TODO: https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html, currently implemented with slow method at top
        inline ull get_diagonal_attacks(ull bitboard) {
            ull all_pieces_but_self = game_board.get_pieces() & ~bitboard;
            ull square = utility::bit::bitboard_to_lowest_square(bitboard);

            // up right
            ull masked_blockers_ne = all_pieces_but_self & north_east_square_ray[square];
            int blocked_square = utility::bit::bitboard_to_lowest_square(masked_blockers_ne);
            ull ne_attacks = ~north_east_square_ray[blocked_square] & north_east_square_ray[square];

            // up left
            ull masked_blockers_nw = all_pieces_but_self & north_west_square_ray[square];
            blocked_square = utility::bit::bitboard_to_lowest_square(masked_blockers_nw);
            ull nw_attacks = ~north_west_square_ray[blocked_square] & north_west_square_ray[square];

            // down right
            ull masked_blockers_se = all_pieces_but_self & south_east_square_ray[square];
            blocked_square = utility::bit::bitboard_to_highest_square(masked_blockers_se);
            ull se_attacks = ~south_east_square_ray[blocked_square] & south_east_square_ray[square];

            // down left
            ull masked_blockers_sw = all_pieces_but_self & south_west_square_ray[square];
            blocked_square = utility::bit::bitboard_to_highest_square(masked_blockers_sw);
            ull sw_attacks = ~south_west_square_ray[blocked_square] & south_west_square_ray[square];

            return ne_attacks | nw_attacks | se_attacks | sw_attacks;
        }

        inline ull get_straight_attacks(ull bitboard) {
            ull all_pieces_but_self = game_board.get_pieces() & ~bitboard;
            ull square = utility::bit::bitboard_to_lowest_square(bitboard);

            // north
            ull masked_blockers_n = all_pieces_but_self & north_square_ray[square];
            int blocked_square = utility::bit::bitboard_to_lowest_square(masked_blockers_n);
            ull n_attacks = ~north_square_ray[blocked_square] & north_square_ray[square];

            // south 
            ull masked_blockers_s = all_pieces_but_self & south_square_ray[square];
            blocked_square = utility::bit::bitboard_to_highest_square(masked_blockers_s);
            ull s_attacks = ~south_square_ray[blocked_square] & south_square_ray[square];

            // left
            ull masked_blockers_w = all_pieces_but_self & west_square_ray[square];
            blocked_square = utility::bit::bitboard_to_lowest_square(masked_blockers_w);
            ull w_attacks = ~west_square_ray[blocked_square] & west_square_ray[square];

            // right
            ull masked_blockers_e = all_pieces_but_self & east_square_ray[square];
            blocked_square = utility::bit::bitboard_to_highest_square(masked_blockers_e);
            ull e_attacks = ~east_square_ray[blocked_square] & east_square_ray[square];
            return n_attacks | s_attacks | w_attacks | e_attacks;
        }


        std::string move_string;             // longest text possible? -> "exd8=Q#" or "axb8=R+"
        Move users_last_move = {};

        void bitboards_to_algebraic(ShumiChess::Color color_that_moved, const ShumiChess::Move move, GameState state 
                                    //, const vector<ShumiChess::Move>* p_legal_moves   // from this position
                                    //, char* pszMoveText);            // output
                                    , std::string& MoveText);           // output

        char get_piece_char(Piece p);
        char file_from_move(const Move& m);
        char rank_from_move(const Move& m);

        char file_to_move(const Move& m);
        char rank_to_move(const Move& m);

        void print_bitboard_to_file(ull bb, FILE* fp);
        void print_moves_to_file(const vector<ShumiChess::Move>& moves, int nTabs, FILE* fp);




        vector<ShumiChess::Move> reduce_to_unquiet_moves(const vector<ShumiChess::Move>& moves);
        vector<ShumiChess::Move> reduce_to_unquiet_moves3(const vector<ShumiChess::Move>& moves);


        int centipawn_score_of(ShumiChess::Piece p);
        int mvv_lva_key(const ShumiChess::Move& m);

        int bits_in(ull);
        bool flip_a_coin(void);

    };
} // end namespace ShumiChess
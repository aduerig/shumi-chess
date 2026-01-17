#pragma once

#include <string>
#include <iostream>
#include <random>

#include "globals.hpp"
#include "endgameTables.hpp"


// Note neither of these include the "depth". So if depth=6, then the actual levels analyzed is (6+MAX_QPLY).
#define MAX_QPLY  7        // Units = plys. Very late in analysis! At this point we just evaluate (stand pat)
#define MAX_QPLY2 4        // Units = plys. Late in analysis! So discard negative SEE captures below one pawn.

namespace ShumiChess {
// TODO think about copy and move constructors.
// ? Will we ever want to copy?

// NOTE: Use us constants everywhere.
constexpr uint8_t king_side_castle  = 0b00000001;
constexpr uint8_t queen_side_castle = 0b00000010;

class GameBoard {
    public:

        // Piece location bitboards
        ull black_pawns {};
        ull white_pawns {};

        ull black_rooks {};
        ull white_rooks {};

        ull black_knights {};
        ull white_knights {};

        ull black_bishops {};
        ull white_bishops {};

        ull black_queens {};
        ull white_queens {};

        ull black_king {};
        ull white_king {};

        // other information about the board state
        Color turn;

        // Castling priviledges. 1<<1 for queenside, 1<<0 for kingside (other bits not used)
        uint8_t black_castle_rights = 0b00000000;
        uint8_t white_castle_rights = 0b00000000;

        // NOTE: why is this a ull? Should it be coded into a unint8_t like castling privilidges?
        //square behind enpassantable pawn after moving
        //0 is impossible state in conventional game
        // ? is this right way to represent this
        // ? do we care about non standard gameboards / moves
        // ? would one consider an extra bit for if we should look at this val
        // ? what about an std::optional https://stackoverflow.com/questions/23523184/overhead-of-stdoptionalt
        ull en_passant_rights {0}; 

        uint64_t zobrist_key {0};

        // move clocks
        uint8_t halfmove;  // Used only to apply the "fifty-move draw" rule in chess
        uint8_t fullmove;  // Used only for display purposes.

        // Constructors
        explicit GameBoard();
        explicit GameBoard(const std::string&);

        // Member methods
        const std::string to_fen(bool bFullFEN=true);

        void set_zobrist();

        // // Castled status. This is different than castle priviledge, this tracks wether it actully happens.
        // bool bCastledWhite;
        // bool bCastledBlack;

        template <Piece p>
        inline ull get_pieces_template() {
            if constexpr (p == Piece::PAWN) { return black_pawns | white_pawns; }
            if constexpr (p == Piece::ROOK) { return black_rooks | white_rooks; }
            if constexpr (p == Piece::KNIGHT) { return black_knights | white_knights; }
            if constexpr (p == Piece::BISHOP) { return black_bishops | white_bishops; }
            if constexpr (p == Piece::QUEEN) { return black_queens | white_queens; }
            if constexpr (p == Piece::KING) { return black_king | white_king; }
        };

        template <Piece p, Color c>  
        inline ull get_pieces_template() {
            if constexpr (p == Piece::PAWN && c == Color::BLACK) { return black_pawns; }
            if constexpr (p == Piece::PAWN && c == Color::WHITE) { return white_pawns; }   
            if constexpr (p == Piece::ROOK && c == Color::BLACK) { return black_rooks; }
            if constexpr (p == Piece::ROOK && c == Color::WHITE) { return white_rooks; }
            if constexpr (p == Piece::KNIGHT && c == Color::BLACK) { return black_knights; }
            if constexpr (p == Piece::KNIGHT && c == Color::WHITE) { return white_knights; }     
            if constexpr (p == Piece::BISHOP && c == Color::BLACK) { return black_bishops; }
            if constexpr (p == Piece::BISHOP && c == Color::WHITE) { return white_bishops; }    
            if constexpr (p == Piece::QUEEN && c == Color::BLACK) { return black_queens; }
            if constexpr (p == Piece::QUEEN && c == Color::WHITE) { return white_queens; } 
            if constexpr (p == Piece::KING && c == Color::BLACK) { return black_king; }
            if constexpr (p == Piece::KING && c == Color::WHITE) { return white_king; }
        }; 

        template <Piece p>
        inline ull get_pieces_template(Color c) {
            if (c == Color::BLACK) { 
                return get_pieces_template<p, Color::BLACK>(); 
            }
            return get_pieces_template<p, Color::WHITE>(); 
        };

        template <Color c>
        inline ull get_pieces_template() {
            if constexpr (c == Color::WHITE) { return white_pawns | white_rooks | white_knights | white_bishops | white_queens | white_king; }   
            else if constexpr (c == Color::BLACK) { return black_pawns | black_rooks | black_knights | black_bishops | black_queens | black_king; }
            else {assert(0);return 0;}
        }

        inline ull get_pieces(Color color) {
            if (color == Color::WHITE) {
                return white_pawns | white_rooks | white_knights | 
                    white_bishops | white_queens | white_king;
            }
            else if (color == Color::BLACK) {
                return black_pawns | black_rooks | black_knights | 
                    black_bishops | black_queens | black_king;
            }
            else {
                std::cout << "Unexpected color in get_pieces 1: " << color << std::endl;
                assert(0);
                return 0;
            }

        }

        inline ull get_pieces(Piece piece_type) {
            if (piece_type == Piece::PAWN) {
                return black_pawns | white_pawns;
            }
            else if (piece_type == Piece::ROOK) {
                return black_rooks | white_rooks;
            }
            else if (piece_type == Piece::KNIGHT) {
                return black_knights | white_knights;
            }
            else if (piece_type == Piece::BISHOP) {
                return black_bishops | white_bishops;
            }
            else if (piece_type == Piece::QUEEN) {
                return black_queens | white_queens;
            }
            else if (piece_type == Piece::KING) {
               return black_king | white_king;
            }
            else {
                std::cout << "Unexpected piece type in get_pieces 2: " << piece_type << std::endl;
                assert(0);
                return 0;
            }
        }

        inline ull get_pieces(Color color, Piece piece_type) {
            return get_pieces(piece_type) & get_pieces(color);
        }

        // Returns a bitboard
        inline ull get_pieces() {
            return white_pawns | white_rooks | white_knights | white_bishops | white_queens | white_king | 
                black_pawns | black_rooks | black_knights | black_bishops | black_queens | black_king;
        }

        // Piece get_piece_type_on_bitboard_using_templates(ull bitboard);
        Piece get_piece_type_on_bitboard(ull);
        Color get_color_on_bitboard(ull);

        bool are_bit_boards_valid() const;
        
        bool insufficient_material_simple();

        int king_edge_weight(Color color);
        int piece_edge_weight(ull pieces);

        int center_closeness_bonus(Color c);

        int pawns_attacking_square(Color c, int sq);
        int pawns_attacking_center_squares(Color c);

        int knights_attacking_square(Color c, int sq);
        int knights_attacking_center_squares(Color for_color);

        int king_castle_happiness(Color c) const;
        // bool knights_centerness(Color c, double& centerness) const;
        // bool bishops_centerness(Color c, double& centerness) const;

        int bishop_pawn_pattern(Color color);       // Stupid bishop blocking pawn
        int queen_still_home(Color color);          // Stupid queen move too early

        bool is_king_in_check_new(Color color);

        bool rook_connectiveness(Color c, int& connectiveness) const;
        int rook_file_status(Color c) const;
        int rook_7th_rankness(Color c) const;
        int count_isolated_pawns(Color c) const;
        int count_passed_pawns(Color c, ull& passedPawns);
        int count_doubled_pawns(Color c) const;

        std::string random_kqk_fen(bool doQueen);

        int get_king_near_squares(Color defender_color, int king_near_squares_out[9]) const;
        int kings_in_opposition(Color defender_color);
        int sliders_and_knights_attacking_square(Color attacker_color, int sq);
        int attackers_on_enemy_king_near(Color attacker_color);
        int attackers_on_enemy_passed_pawns(Color attacker_color,
                                               ull passed_white_pwns,
                                               ull passed_black_pswns);

        double distance_between_squares(int enemySq, int frienSq);
        int get_Chebyshev_distance(int x1, int y1, int x2, int y2);
        double get_board_distance(int x1, int y1, int x2, int y2);

        int king_sq_of(Color color);    
        double king_near_sq(Color attacker_color, ull sq);    
        double king_near_other_king(Color attacker_color);
        int is_knight_on_edge(Color color);
        bool bIsOnlyKing(Color attacker_color);
        bool bNoPawns();
        bool is_king_highest_piece();
        //bool IsSimpleEndGame(Color for_color);
        
        bool isReversableMove(const Move& m);


        std::mt19937 rng;       // 32-bit Mersenne Twister PRNG. For randomness. This is fine. Let it go.
        int rand_new();

        int get_castle_bonus_cp_for_color(Color color1, int phase) const;
        int get_material_for_color(ShumiChess::Color color1, int& cp_pawns_only_temp);
        //bool bHasCastled(Color color1) const;
        bool bHasCastled_fake(Color color1) const;

        // returns 0 if sq has no attackers. 
        int SEE_for_capture(Color side, const Move &mv, FILE* fp);
        int SEE_for_capture_new(Color side, const Move &mv, FILE* fpDebug);

        const endgameTablePos to_egt();

        //int centipawn_score_of(ShumiChess::Piece p) const;
        // Total of 4000 centipawns for each side.
        inline int centipawn_score_of(ShumiChess::Piece p) const {
            switch (p) {
                case ShumiChess::Piece::PAWN:   return 100;
                case ShumiChess::Piece::KNIGHT: return 320;
                case ShumiChess::Piece::BISHOP: return 330;
                case ShumiChess::Piece::ROOK:   return 500;
                case ShumiChess::Piece::QUEEN:  return 900;
                case ShumiChess::Piece::KING:   return 0;   // king is infinite in theory; keep 0 for material sums
                default:                        {assert(0);return 0;}
            }
        }




        int bits_in(ull x) const;
        int bits_in0(ull x) const;

        // Square identities (in h1=0 bit board lingo)
        int square_h1 = 0;
        int square_g1 = 1;
        int square_e1 = 3;
        int square_c1 = 5;
        int square_a1 = 7;
        int square_e4 = 27;
        int square_d4 = 28;
        int square_e5 = 35;
        int square_d5 = 36;
        int square_e6 = 43;  // e4 + 16
        int square_d6 = 44;  // d4 + 16
        int square_e3 = 19;  // e4 - 8
        int square_d3 = 20;  // d4 - 8
        int square_f3 = 18;  // e3 - 1
        int square_g3 = 17;  // e3 - 2
        int square_h3 = 16;  // e3 - 3
        int square_h8 = 56;
        int square_g8 = 57;
        int square_e8 = 59;
        int square_c8 = 61;
        int square_a8 = 63;





        double openingness_of(int avg_cp);

};
} // end namespace ShumiChess

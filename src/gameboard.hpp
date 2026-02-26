#pragma once

#include <string>
#include <iostream>
#include <random>

#include "globals.hpp"
#include "weights.hpp"
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


struct PInfo {
    int file_count[8];          // Count of pawns on this file
    ull file_bb[8];             // Bitboard of pawns on this file
    unsigned files_present;     // Bitmask of which files contain >= 1 pawn
    int advancedSq[8];          // Square index of the side’s most advanced pawn on that file (or -1)
    int rearSq[8];              // Square index of the side’s least advanced pawn on that file (or -1)
};

inline bool operator==(const PInfo& a, const PInfo& b) {
    if (a.files_present != b.files_present) return false;

    for (int i = 0; i < 8; ++i) {
        if (a.file_count[i] != b.file_count[i]) return false;
        if (a.file_bb[i]    != b.file_bb[i])    return false;
        if (a.advancedSq[i] != b.advancedSq[i]) return false;
    }
    return true;
}



struct PawnFileInfo {
    PInfo p[2];   // [0] friendly, [1] enemy
};
#define friendlyP 0 
#define enemyP    1

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

        // NOTE: why is this a ull? Should it be coded into a unint8_t like castling privilidges? NO. see summary.txt
        //square behind enpassantable pawn after moving
        //0 is impossible state in conventional game
        // ? is this right way to represent this
        // ? do we care about non standard gameboards / moves
        // ? would one consider an extra bit for if we should look at this val
        // ? what about an std::optional https://stackoverflow.com/questions/23523184/overhead-of-stdoptionalt
        //
        ull en_passant_rights = 0;  // A 1-bitboard, the square where the capturing pawn would land in an en-passant capture

        uint64_t zobrist_key = 0;

        // move clocks
        uint8_t halfmove;  // Used only to apply the "fifty-move draw" rule in chess
        uint8_t fullmove;  // Used only for display purposes.

        // Constructors
        explicit GameBoard();
        explicit GameBoard(const std::string& fen_notation);

        // Member methods
        const std::string to_fen(bool bFullFEN=true);

        void set_zobrist();

        template <Piece p>
        inline ull get_pieces_template() const {
            if constexpr (p == Piece::PAWN) { return black_pawns | white_pawns; }
            if constexpr (p == Piece::ROOK) { return black_rooks | white_rooks; }
            if constexpr (p == Piece::KNIGHT) { return black_knights | white_knights; }
            if constexpr (p == Piece::BISHOP) { return black_bishops | white_bishops; }
            if constexpr (p == Piece::QUEEN) { return black_queens | white_queens; }
            if constexpr (p == Piece::KING) { return black_king | white_king; }
        };

        template <Piece p, Color c>
        inline ull get_pieces_template() const {
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
        inline ull get_pieces_template(Color c) const {
            if (c == Color::BLACK) {
                return get_pieces_template<p, Color::BLACK>();
            }
            return get_pieces_template<p, Color::WHITE>();
        };

        template <Color c>
        inline ull get_pieces_template() const {
            if constexpr (c == Color::WHITE)      { return white_pawns | white_rooks | white_knights | white_bishops | white_queens | white_king; }
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
        inline ull get_pieces() const {
            return white_pawns | white_rooks | white_knights | white_bishops | white_queens | white_king | 
                   black_pawns | black_rooks | black_knights | black_bishops | black_queens | black_king;
        }

        inline Piece get_piece_type_on_bitboard(ull bitboard) {
            //assert(bits_in(bitboard) == 1);
            if (bitboard & (white_pawns   | black_pawns))   return Piece::PAWN;
            if (bitboard & (white_rooks   | black_rooks))   return Piece::ROOK;
            if (bitboard & (white_knights | black_knights)) return Piece::KNIGHT;
            if (bitboard & (white_bishops | black_bishops)) return Piece::BISHOP;
            if (bitboard & (white_queens  | black_queens))  return Piece::QUEEN;
            if (bitboard & (white_king    | black_king))    return Piece::KING;
            return Piece::NONE;
        }

        template <Color c>
        inline Piece get_piece_type_on_bitboard_template(ull bitboard)
        {
            assert(bits_in(bitboard) == 1);

            if constexpr (c == Color::WHITE) {
                if (bitboard & white_pawns)   return Piece::PAWN;
                if (bitboard & white_rooks)   return Piece::ROOK;
                if (bitboard & white_knights) return Piece::KNIGHT;
                if (bitboard & white_bishops) return Piece::BISHOP;
                if (bitboard & white_queens)  return Piece::QUEEN;
                if (bitboard & white_king)    return Piece::KING;
                return Piece::NONE;
            } else if constexpr (c == Color::BLACK) {
                if (bitboard & black_pawns)   return Piece::PAWN;
                if (bitboard & black_rooks)   return Piece::ROOK;
                if (bitboard & black_knights) return Piece::KNIGHT;
                if (bitboard & black_bishops) return Piece::BISHOP;
                if (bitboard & black_queens)  return Piece::QUEEN;
                if (bitboard & black_king)    return Piece::KING;
                return Piece::NONE;
            } else {
                assert(0);
                return Piece::NONE;
            }
        }

        // Returns the first matching Piece type found.
        inline Piece get_piece_type_on_bitboard(Color c, ull bb1) {
            if (c == Color::WHITE) {
                if (bb1 & white_pawns) return Piece::PAWN;
                if (bb1 & white_knights) return Piece::KNIGHT;
                if (bb1 & white_bishops) return Piece::BISHOP;
                if (bb1 & white_rooks) return Piece::ROOK;
                if (bb1 & white_queens) return Piece::QUEEN;
                if (bb1 & white_king) return Piece::KING;
            } else {
                if (bb1 & black_pawns) return Piece::PAWN;
                if (bb1 & black_knights) return Piece::KNIGHT;
                if (bb1 & black_bishops) return Piece::BISHOP;
                if (bb1 & black_rooks) return Piece::ROOK;
                if (bb1 & black_queens) return Piece::QUEEN;
                if (bb1 & black_king) return Piece::KING;
            }
            return Piece::NONE;
        };     


        Color get_color_on_bitboard(ull);

        bool are_bit_boards_valid() const;
        
        bool insufficient_material_simple();

        template<Color c> int king_edgeness_cp_t();
        int king_edgeness_cp(Color color);
        int piece_edgeness(ull pieces);
        template<Color c> int queenOnCenterSquare_cp_t();
        int queenOnCenterSquare_cp(Color c);
        template<Color c> int moved_f_pawn_early_cp_t() const;
        int moved_f_pawn_early_cp(Color c) const;

        int center_closeness_bonus(Color c);

        template<Color c> int pawns_attacking_square_t(int sq);
        int pawns_attacking_square(Color c, int sq);
        template<Color c> int pawns_attacking_center_squares_cp_t();
        int pawns_attacking_center_squares_cp(Color c);

        template<Color c> int knights_attacking_square_t(int sq);
        int knights_attacking_square(Color c, int sq);
        template<Color c> int knights_attacking_center_squares_cp_t();
        int knights_attacking_center_squares_cp(Color for_color);

        template<Color c> int bishops_attacking_center_squares_cp_t();
        int bishops_attacking_center_squares_cp(Color c);
        template<Color c> int bishops_attacking_square_t(int sq);
        int bishops_attacking_square(Color c, int sq);

        int king_castle_happiness(Color c) const;

        template<Color c> int bishop_pawn_pattern_cp_t();
        int bishop_pawn_pattern_cp(Color color);       // Stupid bishop blocking pawn
        int queen_still_home(Color color);          // Stupid queen move too early

        //bool is_king_in_check_new(Color color);

        template<Color c> int two_bishops_cp_t() const;
        int two_bishops_cp(Color c) const;
        template<Color c> int rook_connectiveness_cp_t() const;
        int rook_connectiveness_cp(Color c) const;
        template<Color c> int rook_7th_rankness_cp_t();
        int rook_7th_rankness_cp(Color c);

        bool build_pawn_file_summary(Color c, PInfo& p);
        template<Color c> bool build_pawn_file_summary_fast_t(PInfo& p);
        bool build_pawn_file_summary_fast(Color c, PInfo& p);
        void dump_pinfo_mismatch(const PInfo& a, const PInfo& b);
        void validate_row_col_masks_h1_0();

        template<Color c> bool any_piece_ahead_on_file_t(int sq, ull pieces) const;
        bool any_piece_ahead_on_file(Color c, int sq, ull pieces) const;
        std::string sqToString(int f, int r) const; // H1=0, 

        // "Positional "pawn" routines.
        template<Color c> int count_isolated_pawns_cp_t(const PawnFileInfo& pawnInfo) const;
        int count_isolated_pawns_cp(Color c, const PawnFileInfo& pawnInfo) const;
        int count_pawn_holes_cp_old(Color c, const PawnFileInfo& pawnInfo
                                , ull& holes);  // output
        template<Color c> int count_pawn_holes_cp2_t(const PawnFileInfo& pawnInfo, ull& holes);
        int count_pawn_holes_cp2(Color c, const PawnFileInfo& pawnInfo
                                , ull& holes);  // output
        template<Color c> int count_knights_on_holes_cp_t(ull holes_bb);
        int count_knights_on_holes_cp(Color c, ull holes_bb);

        template<Color c> int count_doubled_pawns_cp_t(const PawnFileInfo& pawnInfo);
        int count_doubled_pawns_cp(Color c, const PawnFileInfo& pawnInfo);

        template<Color c> int count_passed_pawns_cp_t(const PawnFileInfo& pawnInfo, ull& passed_pawns);
        int count_passed_pawns_cp(Color c, const PawnFileInfo& pawnInfo,
                                    ull& passed_pawns);
        template<Color c> int rooks_file_status_cp_t(const PawnFileInfo& pawnInfo);
        int rooks_file_status_cp(Color c, const PawnFileInfo& pawnInfo);


        std::string random_kqk_fen(bool doQueen);
        std::string random_960_FEN_strict(void);


        template<Color c> int get_king_near_squares_t(int king_near_squares_out[9]);
        int get_king_near_squares(Color defender_color, int king_near_squares_out[9]);
        int kings_in_opposition(Color defender_color);
        int sliders_and_knights_attacking_square(Color attacker_color, int sq);
        template<Color c> int sliders_and_knights_attacking_square2_t(int sq);
        int sliders_and_knights_attacking_square2(Color attacker_color, int sq);
        template<Color c> int attackers_on_enemy_king_near_cp_t();
        int attackers_on_enemy_king_near_cp(Color attacker_color);
        int attackers_on_enemy_passed_pawns(Color attacker_color,
                                               ull passed_white_pwns,
                                               ull passed_black_pswns);

        #define MAX_DIST 10     // Varies based on method used. 14 for Manhatten, 8 for Chebyshev
        double distance_between_squares(int enemySq, int frienSq);

        int get_Chebyshev_distance(int x1, int y1, int x2, int y2);
        int get_Manhattan_distance(int x1, int y1, int x2, int y2);   
        double get_board_distance(int x1, int y1, int x2, int y2);
        //int get_board_distance_100(int x1, int y1, int x2, int y2) const;
        template<Color c> double kings_close_toegather_cp_t();
        double kings_close_toegather_cp(Color attacker_color);
        template<Color c> double kings_far_apart_t();
        double kings_far_apart(Color attacker_color);
        int king_center_manhattan_dist(Color c);
        template<Color c> int is_knight_on_edge_cp_t();
        int is_knight_on_edge_cp(Color color);
        template<Color c> int development_opening_cp_t();
        int development_opening_cp(Color color);
        template<Color c> bool hasNoMajorPieces_t();
        bool hasNoMajorPieces(Color attacker_color);
        bool is_king_highest_piece();
        //bool IsSimpleEndGame(Color for_color);
        
        bool isReversableMove(const Move& m);
        
        inline ull get_major_pieces(Color c) const {
            return (c == Color::WHITE) ? (white_rooks | white_queens) : (black_rooks | black_queens);
        }


        std::mt19937 rng;       // 32-bit Mersenne Twister PRNG. For randomness. This is fine. Let it go.
        int rand_new();

        template<Color c> int get_castled_bonus_cp_t(int phase) const;
        int get_castled_bonus_cp(Color color1, int phase) const;
        template<Color c> int get_material_for_color_t(int& cp_pawns_only_temp);
        int get_material_for_color(ShumiChess::Color color1, int& cp_pawns_only_temp);
        //bool bHasCastled(Color color1) const;
        template<Color c> bool bHasCastled_fake_t() const;
        bool bHasCastled_fake(Color color1) const;

        // returns 0 if sq has no attackers. 
        int SEE_for_capture(Color side, const Move &mv, FILE* fp);
        int SEE_for_capture_new(Color side, const Move &mv, FILE* fpDebug);

        struct SEEBoards
        {
            ull wp, wn, wb, wr, wq, wk;
            ull bp, bn, bb, br, bq, bk;
        };

        ull SEE_attackers_on_square_local(Color c,
                                             int sq,
                                             ull occ_now,
                                             const SEEBoards& b) const;
        int SEE_recursive(Color stm,
                            Color root_side,
                            int to_sq,
                            Piece target_piece,
                            Color target_color,
                            ull occ_local,
                            const SEEBoards& b_local,
                            int balance_local);

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
        // static constexpr int  MAX_CP_PER_SIDE = ( (8*centipawn_score_of(ShumiChess::Piece::PAWN)) 
        //                             + (2*centipawn_score_of(ShumiChess::Piece::KNIGHT)) 
        //                             + (2*centipawn_score_of(ShumiChess::Piece::BISHOP)) 
        //                             + (2*centipawn_score_of(ShumiChess::Piece::ROOK)) 
        //                             + (1*ShumiChess::Piece::QUEEN)
        //                         );
        // Total material in centipawns for one side at game start
        // 8 pawns, 2 knights, 2 bishops, 2 rooks, 1 queen
        #define MAX_CP_PER_SIDE ( \
            (8 * 100)  + \
            (2 * 320)  + \
            (2 * 330)  + \
            (2 * 500)  + \
            (1 * 900)    \
        )


        // Three possibilities here: Which is best? Must use fastest one.
        // inline int GameBoard::bits_in(ull bitboard) const {
        //     auto bs = bitset<64>(bitboard);
        //     return (int) bs.count();
        // }
        // It translates directly to the POPCNT machine instruction on supported x86-64 processors, providing 
        // high-performance bit counting. 
        inline int bits_in(ull bb) const {
            #if defined(_MSC_VER)
                return (int)__popcnt64(bb);     // bb is NOT mutated here
            #else
                return (int)__builtin_popcountll(bb);
            #endif
        }

        inline int bits_in_fast(ull bb) const {
            if (!bb) return 0;

            ull bb2 = bb & (bb - 1);
            if (!bb2) return 1;

            if ((bb2 & (bb2 - 1)) == 0) return 2;

            return 3;   // means "3 or more"
        }

        // inline int bits_in(ull bb) const {
        //     int n = 0;
        //     while (bb) { bb &= (bb - 1); ++n; }    // The K&R or Kernighan trick
        //     return n;
        // }

        // Square identities (in h1=0 bit board lingo)
        // "static constexpr" means, only one set of contants for multiple instances

        static constexpr int square_h1 = 0;
        static constexpr int square_g1 = 1;
        static constexpr int square_f1 = 2;
        static constexpr int square_e1 = 3;
        static constexpr int square_d1 = 4;
        static constexpr int square_c1 = 5;
        static constexpr int square_b1 = 6;
        static constexpr int square_a1 = 7;

        static constexpr int square_f2 = 10;
        static constexpr int square_e2 = 11;
        static constexpr int square_d2 = 12;

        static constexpr int square_h3 = 16;   // e3 - 3
        static constexpr int square_g3 = 17;   // e3 - 2
        static constexpr int square_f3 = 18;   // e3 - 1
        static constexpr int square_e3 = 19;   // e4 - 8
        static constexpr int square_d3 = 20;   // d4 - 8

        static constexpr int square_f4 = 26;   // e4 - 1
        static constexpr int square_e4 = 27;
        static constexpr int square_d4 = 28;
        static constexpr int square_c4 = 29;   // d4 + 1

        static constexpr int square_f5 = 34;   // e5 - 1
        static constexpr int square_e5 = 35;
        static constexpr int square_d5 = 36;
        static constexpr int square_c5 = 37;   // d5 + 1

        static constexpr int square_f6 = 42;   // f4 + 16
        static constexpr int square_e6 = 43;   // e4 + 16
        static constexpr int square_d6 = 44;   // d4 + 16

        static constexpr int square_f7 = 50;
        static constexpr int square_e7 = 51;
        static constexpr int square_d7 = 52;

        static constexpr int square_h8 = 56;
        static constexpr int square_g8 = 57;
        static constexpr int square_f8 = 58;
        static constexpr int square_e8 = 59;
        static constexpr int square_d8 = 60;
        static constexpr int square_c8 = 61;
        static constexpr int square_b8 = 62;
        static constexpr int square_a8 = 63;

        // center squares
        static constexpr ull squares_e4_d4 = (1ull<<square_e4) | (1ull<<square_d4); 
        static constexpr ull squares_e5_d5 = (1ull<<square_e5) | (1ull<<square_d5); 

        // "advanced" center squares
        static constexpr ull squares_e6_d6 = (1ull<<square_e6) | (1ull<<square_d6); 
        static constexpr ull squares_e3_d3 = (1ull<<square_e3) | (1ull<<square_d3); 

        // "flanking" center squares
        static constexpr ull squares_f4_c4 = (1ull<<square_f4) | (1ull<<square_c4); 
        static constexpr ull squares_f5_c5 = (1ull<<square_f5) | (1ull<<square_c5);   

        Weights wghts;

        // Used only in crazy Ivan.
        int CENTER_SCORE[64] = {
                    /* h1=0 layout assumed */
                    0,1,1,2,2,1,1,0,
                    1,2,2,3,3,2,2,1,
                    1,2,3,4,4,3,2,1,
                    2,3,4,5,5,4,3,2,
                    2,3,4,5,5,4,3,2,
                    1,2,3,4,4,3,2,1,
                    1,2,2,3,3,2,2,1,
                    0,1,1,2,2,1,1,0
        };


};
} // end namespace ShumiChess

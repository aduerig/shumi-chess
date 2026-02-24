#pragma once

#include <cassert>
#include <iostream>
#include <stack>
#include <vector>
#include <chrono>

#include "globals.hpp"
#include "gameboard.hpp"
#include "move_tables.hpp"
#include "utility.hpp"

#include "endgameTables.hpp"


using namespace std;

using MoveAndScore     = std::pair<ShumiChess::Move, double>;
using MoveAndScoreList = std::vector<MoveAndScore>;


/////////// Debug ////////////////////////////////////////////////////////////////////////////////////
// These should all be off, except for temporary debug.

//#define _DEBUGGING_PUSH_POP_FAST

//////////////////////////////////////////////////////////////////////////////////////////////////////

// #include <cstddef>  // size_t
// #include <cmath>    // std::abs

inline constexpr int MAX_MOVES = 256;

inline constexpr double TINY_SCORE       = 1.0e-15;  // pawns
inline constexpr double VERY_SMALL_SCORE = 1.0e-5;   // pawns (0.001 centipawns)
inline constexpr double HUGE_SCORE       = 10000.0;  // pawns

inline bool IS_MATE_SCORE(double x) {return std::abs(x) > (HUGE_SCORE - 200.0);}  // pawns. Why 200? This would be a mate in 100.

inline constexpr double ABORT_SCORE     = HUGE_SCORE + 1.0;  // abort analysis
inline constexpr double ONLY_MOVE_SCORE = HUGE_SCORE + 2.0;  // short-circuit when only one legal move

inline constexpr std::size_t _MAX_ALGEBRIAC_SIZE = 16;
inline constexpr std::size_t _MAX_MOVE_PLUS_SCORE_SIZE = _MAX_ALGEBRIAC_SIZE + 32;



#ifdef _SUPRESSING_MOVE_HISTORY_RESULTS
    #define FIFTY_MOVE_RULE_PLY 5000      // This should be 100 as the units are ply.
    #define THREE_TIME_REP 3333            // Should be 3
#else
    #define FIFTY_MOVE_RULE_PLY 50      // This should be 100 as the units are ply.
    #define THREE_TIME_REP 3            // Should be 3
#endif





namespace ShumiChess {


class Engine;


class PGN {
    public:
        PGN();
        void clear();
        int addMe(Move& m, Engine& e);
        string spitout();
    private:
        std::string text;
};



class Engine {
    public:
        // Members
        GameBoard game_board;

        // Stacks
        std::stack<Move> move_history;
        std::stack<int> halfway_move_state;
        std::stack<ull> en_passant_history;

        //TODO This way of tracking is inconsistent with gameboard, think over. Requires splitting and merging of  castle_opportunity_
        std::stack<uint8_t> castle_opportunity_history; // Bits right to left: white kingside, white queenside, black kingside, black queenside
    
        // Constructors
        //? Should the engine be tied to a single boardstate
        //? Should engine functions act independently and take board objects?
        explicit Engine();
        explicit Engine(const string&);


        // Member methods
        void reset_engine();                // New game
        void reset_engine(const string&);   // new game (with FEN)
        void reset_all_but_FEN();


        void pushMove(const Move&);
        void popMove();

        // Same as above, Omits castling, zobrist, history stacks, halfmove/fullmove counters, repetition, etc.
        void pushMoveFast(const Move&);
        void popMoveFast();
        
        GameState is_game_over();
        GameState is_game_over(vector<Move>&);
        int i_randomize_next_move = 0;

        // Returns direct pointer (reference) to a bit board.
        ull& access_pieces_of_color(Piece, Color);
        template <Piece P> ull& access_pieces_of_color_tp(Color color);

        void add_move_to_vector(vector<Move>&, ull, ull, Piece, Color, bool, bool, ull, bool, bool);

        vector<Move> get_legal_moves();
        void get_legal_moves(Color c, vector<Move>& MovesOut);
        void get_legal_moves_fast(Color color, vector<Move>& MovesOut);
        bool assert_same_moves(const std::vector<Move>& a,
                                const std::vector<Move>& b);

        void get_psuedo_legal_moves(Color, vector<Move>& all_psuedo_legal_moves);

        // Storage buffers (they live here to avoid extra allocation during the game)        

   
        vector<Move> psuedo_legal_moves; 
        
        #define MAX_PLY0 100
        vector<Move> all_legal_moves[MAX_PLY0];

        vector<Move> all_unquiet_moves[MAX_PLY0];   

        bool in_check_after_move(Color color, const Move& move);
        bool in_check_after_move_fast(Color color, const Move& move);
        bool in_check_after_king_move(Color color, const Move& move);


        string syzygy_path = "C:\\tb\\syzygy\\";

        int computer_ply_so_far = 0;       // real moves in whole game
        int ply_so_far = 0;     // ply played in game so far
        ull game_white_time_msec = 0;     // total white thinking time for game
        ull game_black_time_msec = 0;     // total black thinking time for game

        PGN gamePGN;

        // Centipawn to pawn conversions
        inline int convert_to_CP(double dd) {return (int)( (dd * 100.0) + (dd >= 0.0 ? 0.5 : -0.5) );}
        inline double convert_from_CP(int ii) {return (static_cast<double>(ii) / 100.0);}

        bool is_king_in_check(const Color);
        bool is_king_in_check2(const Color);
        bool is_square_in_check0(const Color, const ull);
        bool is_square_in_check(const Color, const ull);
        bool is_square_in_check2(const Color, const ull);
        bool is_square_attacked_with_masks(
            const ShumiChess::Color enemy_color,
            const ull square_bb,     // 1-bit bb of target square
            const int square,        // 0..63, must match square_bb
            const ull occ_BB,        // occupancy bitboard to use (can be "after-move")
            const ull themKnights,
            const ull themKing,
            const ull themPawns,
            const ull themQueens,
            const ull themRooks,
            const ull themBishops
        );


        void add_pawn_moves_to_vector(vector<Move>&, Color);
        void add_knight_moves_to_vector(vector<Move>&, Color);
        void add_bishop_moves_to_vector(vector<Move>&, Color);
        void add_queen_moves_to_vector(vector<Move>&, Color);
        void add_king_moves_to_vector(vector<Move>&, Color);
        void add_rook_moves_to_vector(vector<Move>&, Color);

        ull all_enemy_pieces;
        ull all_own_pieces;
        ull all_pieces; 

        // This is the "classical" approach
        // !TODO: https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html, currently implemented with slow method at top
        
        

        inline ull squares_between_exclusive(int kingSq, int checkerSq) const
        {
            // Returns squares strictly between kingSq and checkerSq if aligned (rank/file/diag),
            // else returns 0.
            // Uses your ray tables: north_square_ray[], etc.

            const ull kingBB    = (1ULL << kingSq);
            const ull checkerBB = (1ULL << checkerSq);

            // Same file/rank:
            if (north_square_ray[kingSq] & checkerBB) {
                return north_square_ray[kingSq] & ~north_square_ray[checkerSq];
            }
            if (south_square_ray[kingSq] & checkerBB) {
                return south_square_ray[kingSq] & ~south_square_ray[checkerSq];
            }
            if (east_square_ray[kingSq] & checkerBB) {
                return east_square_ray[kingSq] & ~east_square_ray[checkerSq];
            }
            if (west_square_ray[kingSq] & checkerBB) {
                return west_square_ray[kingSq] & ~west_square_ray[checkerSq];
            }

            // Diagonals:
            if (north_east_square_ray[kingSq] & checkerBB) {
                return north_east_square_ray[kingSq] & ~north_east_square_ray[checkerSq];
            }
            if (north_west_square_ray[kingSq] & checkerBB) {
                return north_west_square_ray[kingSq] & ~north_west_square_ray[checkerSq];
            }
            if (south_east_square_ray[kingSq] & checkerBB) {
                return south_east_square_ray[kingSq] & ~south_east_square_ray[checkerSq];
            }
            if (south_west_square_ray[kingSq] & checkerBB) {
                return south_west_square_ray[kingSq] & ~south_west_square_ray[checkerSq];
            }

            (void)kingBB; // quiet unused warning if you remove some branches
            return 0ULL;
        }



        void move_into_string(ShumiChess::Move m);
        void move_into_string_full(ShumiChess::Move m);
        string moves_into_string(const std::vector<Move>& mvs);

        std::string move_string;             // longest text possible? -> "exd8=Q#" or "axb8=R+"
        Move users_last_move = {};

 
        //int bishops_attacking_square_old(Color c, int sq);         
        
        void bitboards_to_algebraic(ShumiChess::Color color_that_moved, const ShumiChess::Move move
                                    , GameState state 
                                    , bool isCheck
                                    , bool bPadTrailing
                                    , const vector<ShumiChess::Move>* p_legal_moves   // from this position                                    
                                    , std::string& MoveText) const;           // output

        char get_piece_char(Piece p) const;
        char file_from_move(const Move& m) const;
        char rank_from_move(const Move& m) const;

        char file_to_move(const Move& m) const;
        char rank_to_move(const Move& m) const;

        void set_random_on_next_move(int randomMoveCount);
        
        int user_request_next_move = 7;    // Note: make me go aways. I am for changing the deepening inbetweem moves
        void killTheKing(Color color);     // Note: The idea. is that you can resign by deleting your king. Sort of works.

        void print_moves_and_scores_to_file(const MoveAndScoreList move_and_scores_list
            , bool b_convert_to_abs_score, bool b_sort_descending, FILE* fp);
        void print_move_and_score_to_file(const MoveAndScore move_and_score, bool b_convert_to_abs_score, FILE* fp);

        void move_and_score_to_string(const Move best_move, double d_best_move_value, bool b_convert_to_abs_score);
      
        void print_bitboard_to_file(ull bb, FILE* fp);

        bool has_unquiet_move(const vector<ShumiChess::Move>& moves);

        // Note: Should we also include:
        //  captures of queen
        //  captures of rook (optional)
        //  only checking captures (a capture that gives check).
        //  only checks that are “near king” (checker lands in king zone: 1–2 squares away / same ring). 
        bool inline is_unquiet_move(ShumiChess::Move mv) {
           return (mv.capture != ShumiChess::Piece::NONE || mv.promotion != ShumiChess::Piece::NONE); 
        }


        //void reduce_to_unquiet_moves(const vector<ShumiChess::Move>& moves, vector<ShumiChess::Move>& MovesOut);
        void reduce_to_unquiet_moves_MVV_LVA(
                                        const vector<ShumiChess::Move>& moves,      // Input
                                        //const Move& move_last,                      // input
                                        int qPlys,
                                        vector<ShumiChess::Move>& MovesOut           // output
                                    );

        double d_bestScore_at_root = 0.0;       // in abs coordinates
        //
        // Returns "an ordering key", for a capture, using MVV-LVA. "Top range" of key is the victim piece value,                          
        // "Bottom range" is the negative of the attacker piece value. 
        // Asserts it’s only used for captures.
        // Based on the fact that we prefer taking the biggest victim with the smallest attacker.
        inline int mvv_lva_key(const Move& m) {

            if (m.capture == ShumiChess::Piece::NONE) assert(0);    // non-captures

            int victim  = game_board.centipawn_score_of(m.capture);
            int attacker= game_board.centipawn_score_of(m.piece_type);

            // Victim dominates (shift by a few bits, note: a few?, 10?), attacker is a tiebreaker penalty
            int key = (victim << 10) - attacker;

            // Promotions: capturing + promoting should go even earlier, but it doesnt now
            if (m.promotion != ShumiChess::Piece::NONE) {
                key += game_board.centipawn_score_of(m.promotion) - game_board.centipawn_score_of(ShumiChess::Piece::PAWN);
            }

            // En passant: treat as a pawn capture
            // (no extra handling needed if m.capture == Piece::PAWN is already set for EP). Note: what?
            return key;
        }

        // display crap
        //string reason_for_draw = "------------";
        #define DRAW_NULL        0 
        #define DRAW_STALEMATE   1
        #define DRAW_3TIME_REP   2
        #define DRAW_50MOVERULE  3
        #define DRAW_INSUFFMATER 4
        #define DRAW_AGREEMENT   5
        #define DRAW_ADMIN       6    // tounyement director is closing the hall.
        int reason_for_draw = DRAW_NULL;

        int get_best_score_at_root();
        int material_centPawns = 0;

        int rand_int(int, int);
        bool flip_a_coin(void);
   
        //std::unordered_map<uint64_t, int> repetition_table;
        //void debug_print_repetition_table() const;

        // These stacks handled in parallel
        std::vector<uint64_t> three_time_rep_stack; // zobrist keys representing positions
        std::vector<int> boundary_stack; // indices into three_time_rep_stack

        struct PinnedInfo
        {
            ull pinnedMask;          // bit i = 1 => my piece on square i is pinned
            ull allowedMask[64];     // for pinned square i: squares it may move to (including capture of pinner)
                                    // for non-pinned squares: can be 0

            void clear()
            {
                pinnedMask = 0ULL;
                for (int i = 0; i < 64; i++) allowedMask[i] = 0ULL;
            }

            bool isPinned(int fromSq) const
            {
                // fromSq must be 0..63
                return (pinnedMask & (1ULL << fromSq)) != 0ULL;
            }

            bool moveObeysPinLine(int fromSq, int toSq) const
            {
                // Only valid if isPinned(fromSq) is true
                return (allowedMask[fromSq] & (1ULL << toSq)) != 0ULL;
            }
        };

        struct CheckInfo
        {
            int numCheckers = 0;
            ull checkerBB   = 0ULL;   // 1-bit if single check, else 0
            ull captureMask = 0ULL;   // squares that can capture checker (single check)
            ull blockMask   = 0ULL;   // squares that block slider check (single check)
            ull toHelpMask  = 0ULL;   // captureMask | blockMask

            bool toSquareHelps(int toSq) const
            {
                return (toHelpMask & (1ULL << toSq)) != 0ULL;
            }
        };

        PinnedInfo compute_pins(Color color);


        inline int get_king_square(Color c)
        {
            ull k = game_board.get_pieces_template<Piece::KING>(c);
            assert(k != 0ULL);
            return utility::bit::bitboard_to_lowest_square_fast(k);
        }

        CheckInfo find_checkers_and_blockmask(Color color);

        std::mt19937 rng;       // 32-bit Mersenne Twister PRNG. For randomness. This is fine. Let it go.

        void print_move_history_to_buffer(char *out, size_t out_size);
        void print_move_history_to_file(FILE* fp, const char* psz);
        void print_move_history_to_file0(FILE* fp, std::stack<ShumiChess::Move> tmp);

        int print_move_to_file(const ShumiChess::Move m, int nPly, ShumiChess::GameState gs
                            , bool isInCheck, bool bFormated, bool bFlipColor
                            , FILE* fp);

        int print_move_to_file2(const ShumiChess::Move m, int nPly, ShumiChess::GameState gs
                            , bool isInCheck, bool bFlipColor
                            , const char* preString
                            , FILE* fp);
        void print_tabOver(int nPly, FILE* fp);

        void print_move_to_file_from_string(const char* p_move_text, ShumiChess::Color turn, int nPly
                                            , const char* preString
                                            , char postCharacter
                                            , bool b_right_Pad
                                            , FILE* fp);

        void debug_SEE_for_all_captures(FILE* fp);

    };




} // end namespace ShumiChess
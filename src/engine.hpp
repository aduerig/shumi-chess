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

//#define _DEBUGGING_PUSH_POP_FAST

#define MAX_MOVES 128

// Note: make me go away.
using MoveAndScore     = std::pair<ShumiChess::Move, double>;
using MoveAndScoreList = std::vector<MoveAndScore>;
   
#define TINY_SCORE 1.0e-15          // In pawns.
#define VERY_SMALL_SCORE 1.0e-5     // In pawns. (its 0.001 centipawns)
#define HUGE_SCORE 10000.0          // In pawns. (its a million centipawns!)       //  DBL_MAX    // A relative score
#define IS_MATE_SCORE(x) ( std::abs((x)) > (HUGE_SCORE - 200) )

#define ABORT_SCORE (HUGE_SCORE+1)      // Used only to abort the analysis
#define ONLY_MOVE_SCORE (HUGE_SCORE+2)  // Used to short circuit anlysis when only one legal move

#define _MAX_ALGEBRIAC_SIZE 16        // longest SAN text possible? -> "exd8=Q#" or "axb8=R+"
#define _MAX_MOVE_PLUS_SCORE_SIZE (_MAX_ALGEBRIAC_SIZE+32)        // Move text plus a score




namespace ShumiChess {
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
        void reset_engine();
        void reset_engine(const string&);

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
        //template <Piece P, Color C> ull& Engine::access_pieces_of_color()

        void add_move_to_vector(vector<Move>&, ull, ull, Piece, Color, bool, bool, ull, bool, bool);

        vector<Move> get_legal_moves();
        vector<Move> get_legal_moves(Color);
        void get_psuedo_legal_moves(Color, vector<Move>& all_psuedo_legal_moves);

        // Storage buffers (they live here to avoid extra allocation during the game)        
        vector<Move> psuedo_legal_moves; 
        vector<Move> all_legal_moves;

        int get_minor_piece_move_number (const vector <Move> mvs);       

        inline bool in_check_after_move(Color color, const Move& move) {
            // NOTE: is this the most effecient way to do this (push()/pop())?
            #ifdef _DEBUGGING_PUSH_POP_FAST
                std::string temp_fen_before = game_board.to_fen();
            #endif
            pushMoveFast(move);           
            bool bReturn = is_king_in_check(color);
            popMoveFast();
            #ifdef _DEBUGGING_PUSH_POP_FAST
                std::string temp_fen_after = game_board.to_fen();
                if (temp_fen_before != temp_fen_after) {
                    std::cout << "\x1b[31m";
                    std::cout << "PROBLEM WITH PUSH POP FAST!!!!!" << std::endl;
                    std::cout << "FEN before  push/pop: " << temp_fen_before  << std::endl;
                    std::cout << "FEN after   push/pop: " << temp_fen_after   << std::endl;
                    std::cout << "\x1b[0m";
                    assert(0);
                }
            #endif
            return bReturn;
        }

        string syzygy_path = "C:\\tb\\syzygy\\";

        int g_iMove = 0;       // real moves in whole game
        ull totalWhiteTimeMsec = 0;     // total white thinking time for game
        ull totalBlackTimeMsec = 0;     // total black thinking time for game

        // Centipawn to pawn conversions
        inline int convert_to_CP(double dd) {return (int)( (dd * 100.0) + (dd >= 0.0 ? 0.5 : -0.5) );}
        inline double convert_from_CP(int ii) {return (static_cast<double>(ii) / 100.0);}

        bool is_king_in_check(const Color&);
        bool is_square_in_check(const Color&, const ull&);

        void add_pawn_moves_to_vector(vector<Move>&, Color);
        void add_knight_moves_to_vector(vector<Move>&, Color);
        void add_bishop_moves_to_vector(vector<Move>&, Color);
        void add_queen_moves_to_vector(vector<Move>&, Color);
        void add_king_moves_to_vector(vector<Move>&, Color);
        void add_rook_moves_to_vector(vector<Move>&, Color);

        // helpers for move generation (inlined)
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

        void move_into_string(ShumiChess::Move m);

        std::string move_string;             // longest text possible? -> "exd8=Q#" or "axb8=R+"
        Move users_last_move = {};

        int bishops_attacking_center_squares(Color c);
        int bishops_attacking_square(Color c, int sq);    
        void bitboards_to_algebraic(ShumiChess::Color color_that_moved, const ShumiChess::Move move
                                    , GameState state 
                                    , bool isCheck
                                    //, const vector<ShumiChess::Move>* p_legal_moves   // from this position
                                    , bool bPadTrailing
                                    , std::string& MoveText);           // output

        char get_piece_char(Piece p);
        char file_from_move(const Move& m);
        char rank_from_move(const Move& m);

        char file_to_move(const Move& m);
        char rank_to_move(const Move& m);

        void set_random_on_next_move(int randomMoveCount);
        
        int user_request_next_move = 7;    // Note: make me go aways. I am for changing the deepening inbetweem moves
        void killTheKing(Color color);     // Note: The idea. is that you can resign by deleting your king. Sort of works.

        void print_moves_and_scores_to_file(const MoveAndScoreList move_and_scores_list, bool b_convert_to_abs_score, FILE* fp);
        void print_move_and_score_to_file(const MoveAndScore move_and_score, bool b_convert_to_abs_score, FILE* fp);

        void move_and_score_to_string(const Move best_move, double d_best_move_value, bool b_convert_to_abs_score);
      
        void print_bitboard_to_file(ull bb, FILE* fp);

        bool has_unquiet_move(const vector<ShumiChess::Move>& moves);
        bool inline is_unquiet_move(ShumiChess::Move mv) {
           return (mv.capture != ShumiChess::Piece::NONE || mv.promotion != ShumiChess::Piece::NONE); 
        }
        vector<ShumiChess::Move> reduce_to_unquiet_moves(const vector<ShumiChess::Move>& moves);
        vector<ShumiChess::Move> reduce_to_unquiet_moves_MVV_LVA(
                                        const vector<ShumiChess::Move>& moves,      // Input
                                        //const Move& move_last,                      // input
                                        int qPlys,
                                        vector<ShumiChess::Move>& vReturn           // output
                                    );

        double d_bestScore_at_root = 0.0;
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
        int get_best_score_at_root();
        int material_centPawns = 0;

        int rand_int(int, int);
        bool flip_a_coin(void);

        //ShumiChess::Move make_enpassant_move_from_bit_boards( Piece p, ull bitTo, ull bitFrom, Color color);
    
        std::unordered_map<uint64_t, int> repetition_table;

        void debug_print_repetition_table() const;

        std::mt19937 rng;       // 32-bit Mersenne Twister PRNG. For randomness. This is fine. Let it go.


        void print_move_history_to_file(FILE* fp);

        int print_move_to_file(const ShumiChess::Move m, int nPly, ShumiChess::GameState gs
                            , bool isInCheck, bool bFormated, bool bFlipColor, FILE* fp);

        void print_move_to_file_from_string(const char* p_move_text, ShumiChess::Color turn, int nPly
                                            , char preCharacter
                                            , char postCharacter
                                            , bool b_right_Pad
                                            , FILE* fp);

        void debug_SEE_for_all_captures(FILE* fp);

    };
} // end namespace ShumiChess
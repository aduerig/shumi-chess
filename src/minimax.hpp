#pragma once

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>
#include <algorithm>

#include <vector>
#include <unordered_map>
#include <string>
#include <limits>

class RandomAI {
public:
    ShumiChess::Engine engine;
    
    //unordered_map<ull, int> piece_values;

    RandomAI(ShumiChess::Engine&);
    int rand_int(int, int);
    ShumiChess::Move& get_move(vector<ShumiChess::Move>&);
};



class MinimaxAI {
public:

    int nodes_visited = 0;
    int g_evals = 0;
    
    int top_depth = 0;         // thhis is depth at top of recursion (depth==0 at bottom of recursion)

    // The chess engine
    ShumiChess::Engine& engine;

    // NOTE: Can these be more accurate (i.e. decimals like 2.8 for knight)
    // NOTE: Can these be more accurate as to "2 bishops" etc.
    // std::array<std::tuple<ShumiChess::Piece, double>, 6> piece_values = {
    //     make_tuple(ShumiChess::Piece::PAWN, 1),
    //     make_tuple(ShumiChess::Piece::ROOK, 5),
    //     make_tuple(ShumiChess::Piece::KNIGHT, 3.2),
    //     make_tuple(ShumiChess::Piece::BISHOP, 3.3),
    //     make_tuple(ShumiChess::Piece::QUEEN, 9),
    //     make_tuple(ShumiChess::Piece::KING, 0),
    // };

    unordered_map<uint64_t, std::string> seen_zobrist;

    // NOTE: move me to top of class declaration.
    MinimaxAI(ShumiChess::Engine&);
    ~MinimaxAI();

    double evaluate_board(ShumiChess::Color);     // ShumiChess::Move& last_move, 
                                                  // ,vector<ShumiChess::Move>&);   // Output

    ShumiChess::Move get_move_iterative_deepening(double);

    std::tuple<double, ShumiChess::Move> store_board_values_negamax(int depth, double alpha, double beta
                                            //, unordered_map<uint64_t, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &move_scores
                                            , unordered_map<std::string, unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>> &move_scores
                                            , ShumiChess::Move& move_last
                                            , int nPly);


    double get_value(int depth, int color_multiplier, double alpha, double beta);
    ShumiChess::Move get_move(int);
    ShumiChess::Move get_move();


    void sort_moves_by_score(
        std::vector<ShumiChess::Move>& moves,  // reorder this in place
        const std::unordered_map<std::string,
            std::unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>
        >& move_scores,
        bool sort_descending  // true = highest relative score first
    );

    std::tuple<double, ShumiChess::Move>
    best_move_static(ShumiChess::Color color,
                                const std::vector<ShumiChess::Move>& moves,
                                bool in_Check
                            );



    void Add_result_to_print_tree(FILE* fpStatistics
                                    ,ShumiChess::Move& move_last
                                    ,std::tuple<double, ShumiChess::Move> final_result
                                    , ShumiChess::GameState state
                                    ,int level
                                    ,int depth);

    void clear_stats_file(FILE*& fpStatistics, const char* path);

    void print_move_to_file(ShumiChess::Move m, int nPly, ShumiChess::GameState gs, bool isInCheck);

    void print_move_to_file_from_string(const char* p_move_text, ShumiChess::Color turn, int nPly
                                            , char preCharacter
                                            , char postCharacter
                                            , bool b_right_Pad);


    void print_moves_to_print_tree(std::vector<ShumiChess::Move> mvs, int depth, char* szHeader, char* szTrailer);


    void print_move_scores_to_file(
        FILE* fpStatistics,
        const std::unordered_map<std::string,std::unordered_map<ShumiChess::Move, double, utility::representation::MoveHash>>& move_scores
    );



};
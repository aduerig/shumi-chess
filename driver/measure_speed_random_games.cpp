
#include <cstdio>
#include <ostream>
#include <iostream>
#include <chrono> 
#include <math.h>

#include <globals.hpp>
#include <engine.hpp>
#include <utility.hpp>

using namespace std;
using namespace ShumiChess;
using namespace std::chrono;


int rand_int(int min, int max) {
    return min + (rand() % static_cast<int>(max - min + 1));
}


Move& get_random_move(Move* all_moves, int num_moves) {
    int index = rand_int(0, num_moves - 1);
    return all_moves[index];
}


GameState play_game(Engine& engine, int& total_moves) {
    GameState result = engine.game_over();

    while (result == GameState::INPROGRESS) {
        LegalMoves legal_moves = engine.get_legal_moves();
        if (legal_moves.num_moves == 0) { // draw
            result = GameState::DRAW;
            break;
        }
        total_moves++;
        engine.push(get_random_move(legal_moves.moves, legal_moves.num_moves));
        // for (const auto & [ key, value ] : engine.count_zobrist_states) {
        //     cout << "key: " << key << " value: " << value << endl;
        // }
        result = engine.game_over();
    }
    engine.reset_engine();
    return result;
}


int main() {
    Engine engine;


    int total = 5000;
    int black_wins = 0;
    int white_wins = 0;
    int draws = 0;

    int total_moves = 0;


    auto start = high_resolution_clock::now();
    for (int i = 0; i < total; i++) {
        GameState result = play_game(engine, total_moves);
        if (result == GameState::WHITEWIN) {
            white_wins++;
        }
        else if (result == GameState::BLACKWIN) {
            black_wins++;
        }
        else {
            draws++;
        }
    }
    auto stop = high_resolution_clock::now();
    auto duration_microsec = duration_cast<microseconds>(stop - start);
    double seconds_passed = duration_microsec.count() / 1000000.0f;

    double our_nps = (total_moves / seconds_passed);
    double stockfish_nps = 2000000;
    double speed_comparison = ((our_nps / stockfish_nps) * 100);
    double speed_comparison_rounded_2_dec = round(speed_comparison * 100) / 100.0f;

    cout << "played: " << total << " random games in " << duration_microsec.count() << " microseconds." << endl;
    cout << "results in " << our_nps << " moves per second (nps)" << endl;
    cout << speed_comparison_rounded_2_dec << "% of stockfish's speed" << endl;

    return 0;
}



#define VERY_SMALL_SCORE 1.0e-9     // 10 micro centipawns?
#define HUGE_SCORE 10000            // A million centipawns!       //  DBL_MAX    // A relative score

tuple<double, Move> MinimaxAI::store_board_values_negamax(
    int depth, double alpha, double beta,
    unordered_map<std::string, unordered_map<Move, double, MoveHash>> &move_scores,
    ShumiChess::Move& move_last,
    bool debug)
{
    assert(alpha <= beta);

    double d_end_score = 0.0;
    Move the_best_move = {};
    std::tuple<double, ShumiChess::Move> final_result;

    int level = (top_depth - depth);
    assert(level >= 0);
    double d_level = static_cast<double>(level);

    std::vector<Move> moves_to_loop_over;
    std::vector<Move> unquiet_moves;

    nodes_visited++;

    std::vector<Move> legal_moves = engine.get_legal_moves();
    GameState state = engine.game_over(legal_moves);

    moves_to_loop_over = legal_moves;

    assert(level < 22); // runaway recursion stop

    // =====================================================================
    // Terminal positions (game over)
    // =====================================================================
    if (state != GameState::INPROGRESS) {
        if (state == GameState::WHITEWIN) {
            d_end_score = (engine.game_board.turn == ShumiChess::WHITE)
                            ? (+HUGE_SCORE - d_level)
                            : (-HUGE_SCORE + d_level);
        } else if (state == GameState::BLACKWIN) {
            d_end_score = (engine.game_board.turn == ShumiChess::BLACK)
                            ? (+HUGE_SCORE - d_level)
                            : (-HUGE_SCORE + d_level);
        } else if (state == GameState::DRAW) {
            d_end_score = 0.0;
        } else {
            assert(0);
        }

        final_result = std::make_tuple(d_end_score, the_best_move);
        return final_result; // FIX: return here
    }

    // =====================================================================
    // Hard node-limit sentinel
    // =====================================================================
    if (nodes_visited > 1e10) {
        // FIX: do a well-defined return
        final_result = std::make_tuple(d_end_score, the_best_move);
        return final_result;
    }

    // =====================================================================
    // Quiescence entry when depth <= 0
    // =====================================================================
    if (depth <= 0) {
        bool in_check = engine.is_king_in_check(engine.game_board.turn);

        d_end_score = evaluate_board(engine.game_board.turn, legal_moves);
        unquiet_moves = engine.reduce_to_unquiet_moves(legal_moves);

        // If quiet (not in check & no tactics), just return stand-pat
        if (!in_check && unquiet_moves.empty()) {
            return { d_end_score, Move{} }; // FIX: actual return
        }

        if (!in_check) {
            // Stand-pat cutoff
            if (d_end_score >= beta) {
                return { d_end_score, Move{} };
            }
            alpha = std::max(alpha, d_end_score);
            // Extend on captures/promotions only
            moves_to_loop_over = unquiet_moves;
        } else {
            // In check: use all legal moves
            moves_to_loop_over = legal_moves;
        }
    } else {
        // depth > 0: already have moves_to_loop_over = legal_moves
        // nothing to change here
    }

    // =====================================================================
    // Recurse over selected move set
    // =====================================================================
    if (!moves_to_loop_over.empty()) {
        std::vector<Move> sorted_moves = moves_to_loop_over;

        // (optional) move ordering
        sort_moves_by_score(sorted_moves, move_scores, true);

        d_end_score = -HUGE_SCORE;
        the_best_move = sorted_moves[0];

        for (Move &m : sorted_moves) {
            #ifdef _DEBUGGING_PUSH_POP
                std::string temp_fen_before = engine.game_board.to_fen();
            #endif

            engine.push(m);

            #ifdef _DEBUGGING_MOVE_TREE
                print_move_to_print_tree(m, depth);
            #endif

            auto ret_val = store_board_values_negamax(
                (depth > 0 ? depth - 1 : 0),
                -beta, -alpha,
                move_scores,
                m, debug
            );

            double d_score_value = -std::get<0>(ret_val);

            engine.pop();

            #ifdef _DEBUGGING_PUSH_POP
                std::string temp_fen_after = engine.game_board.to_fen();
                if (temp_fen_before != temp_fen_after) {
                    std::cout << "PROBLEM WITH PUSH POP!!!!!" << std::endl;
                    cout_move_info(m);
                    std::cout << "FEN before  push/pop: " << temp_fen_before  << std::endl;
                    std::cout << "FEN after   push/pop: " << temp_fen_after   << std::endl;
                    assert(0);
                }
            #endif

            // Store score
            std::string temp_fen_now = engine.game_board.to_fen();
            move_scores[temp_fen_now][m] = d_score_value;

            #ifndef NDEBUG
            {
                auto& row = move_scores[temp_fen_now];
                auto it = row.find(m);
                assert(it != row.end());
                assert(std::fabs(it->second - d_score_value) <= VERY_SMALL_SCORE);
            }
            #endif

            bool b_use_this_move = (d_score_value > d_end_score);
            if (b_use_this_move) {
                d_end_score = d_score_value;
                the_best_move = m;
            }

            alpha = std::max(alpha, d_end_score);
            if (alpha >= beta + VERY_SMALL_SCORE) {
                break;
            }
        }
    }

    final_result = std::make_tuple(d_end_score, the_best_move);
    return final_result;
}




//////////////////////////////////////////////////////////////////////////////////
//
// NOTE: This the entry point into the C to get a minimax AI move.
//   It does "Iterative deepening".
//
//////////////////////////////////////////////////////////////////////////////////

Move MinimaxAI::get_move_iterative_deepening(double time) {

    seen_zobrist.clear();
    nodes_visited = 0;

    uint64_t zobrist_key_start = engine.game_board.zobrist_key;
    //cout << "zobrist_key at start of get_move_iterative_deepening is: " << zobrist_key_start << endl;

    auto start_time = chrono::high_resolution_clock::now();
    auto required_end_time = start_time + chrono::duration<double>(time);

    // Results of previous searches
    //unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> move_scores;
    unordered_map<std::string, unordered_map<Move, double, MoveHash>> move_scores;

    size_t nFENS = move_scores.size();    // This returns the number of FEN rows.
    assert(nFENS == 0);

    Move best_move = {};
    double d_best_move_value;

    //Move null_move = Move{};
    Move null_move = engine.users_last_move;

    int maximum_depth = 4;        // Note: because i said so.
    assert(maximum_depth>=1);

    // NOTE: this should be an option: depth .vs. time.
    int depth = 1;
    do {
        

        #ifdef _DEBUGGING_MOVE_TREE
            fputs("\n\n---------------------------------------------------------------------------", fpStatistics);
            print_move_to_print_tree(null_move, depth);
        #endif

        cout << "Deepening to " << depth << " half moves" << endl;

        move_scores.clear();   // before calling store_board_values_negamax for a new depth

        top_depth = depth;
        auto ret_val = store_board_values_negamax(depth, -HUGE_SCORE, HUGE_SCORE
                                                , move_scores
                                                , null_move, false);

        // ret_val is a tuple of the score and the move.
        d_best_move_value = get<0>(ret_val);
        best_move = get<1>(ret_val);

        depth++;
    //} while (chrono::high_resolution_clock::now() <= required_end_time);
    } while (depth < (maximum_depth+1));

    cout << "Went to depth " << (depth - 1) << endl;

    //vector<Move> top_level_moves = engine.get_legal_moves();
    //Move move_chosen = top_level_moves[0];     // Note: Assumes the moves are sorted?

    //cout << "Found " << move_scores.size() << " items inside of move_scores" << endl;

    // if (move_scores.find(engine.game_board.zobrist_key) == move_scores.end()) {
    //     cout << "Did not find zobrist key in move_scores, this is bad" << endl;
    //     exit(1);
    // }

    // auto stored_move_values = move_scores[engine.game_board.zobrist_key];
    // for (Move& m : top_level_moves) {
    //     if (stored_move_values.find(m) == stored_move_values.end()) {
    //         cout << "did not find move " << move_to_string(m) << " in the stored moves. this is probably bad" << endl;
    //         exit(1);
    //     }
    // }
    string color = engine.game_board.turn == Color::BLACK ? "BLACK" : "WHITE";

    // Convert to absolute score
    double d_best_move_value_abs = d_best_move_value;
    if (engine.game_board.turn == Color::BLACK) d_best_move_value_abs = -d_best_move_value_abs;
    
    if (std::fabs(d_best_move_value_abs) < VERY_SMALL_SCORE) d_best_move_value_abs = 0.0;        // avoid negative zero
    string abs_score_string = to_string(d_best_move_value_abs);


    engine.bitboards_to_algebraic(engine.game_board.turn, best_move, (GameState::INPROGRESS)
                //, NULL
                , engine.move_string);

    cout << colorize(AColor::BRIGHT_CYAN,engine.move_string) << "   ";

    cout << colorize(AColor::BRIGHT_CYAN, abs_score_string + "= score, Minimax AI chose move: for " + color + " to move") << endl;
    cout << colorize(AColor::BRIGHT_YELLOW, "Visited: " + format_with_commas(nodes_visited) + " nodes total") << endl;
    // cout << colorize(AColor::BRIGHT_GREEN, "Time it was supposed to take: " + to_string(time) + " s") << endl;
    // cout << colorize(AColor::BRIGHT_GREEN, "Actual time taken: " + to_string(chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count()) + " s") << endl;
    //cout << "get_move_iterative_deepening zobrist_key at begining: " << zobrist_key_start << ", at end: " << engine.game_board.zobrist_key << endl;
    




    return best_move;
}

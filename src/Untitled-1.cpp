




tuple<double, Move> MinimaxAI::store_board_values_negamax(int depth, double alpha, double beta
                                    //, unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> &move_scores
                                    , unordered_map<std::string, unordered_map<Move, double, MoveHash>> &move_scores
                                    , ShumiChess::Move& move_last
                                    , bool debug) {
    assert(depth>=0);

    // These are are the final result, returned from here. A best score, and best move.
    // If I didnt look at any moves, then the best move is, default, or "none".
    double d_end_score = 0.0;
    Move the_best_move = {};

    // default-initializes the tuple: the double becomes 0.0 and Move is default-constructed.
    std::tuple<double, ShumiChess::Move> final_result;

    int level = (top_depth-depth);
    assert (level >= 0);
    double d_level = static_cast<double>(level);
    //d_level = 0.0;   // Stupid debug. Delete me.

    nodes_visited++;
    
    vector<Move> legal_moves = engine.get_legal_moves();

    GameState state = engine.game_over(legal_moves);


    std::vector<ShumiChess::Move> unquiet_moves;

    if (state != GameState::INPROGRESS) {
        
        // Game is over.  Here recursive analysis must stop.

        // Relative (negamax): score is from the side-to-move’s perspective.
        // NOTE: I added "mate-distance bookkeeping":
        // Use a large mate score reduced by d_level so shorter mates have larger magnitude.
        if (state == GameState::WHITEWIN) {
            d_end_score = (engine.game_board.turn == ShumiChess::WHITE)
                            ? (+HUGE_SCORE - d_level)   // (edge case, normally loser to move)
                            : (-HUGE_SCORE + d_level);  // side to move is mated
        }
        else if (state == GameState::BLACKWIN) {
            d_end_score = (engine.game_board.turn == ShumiChess::BLACK)
                            ? (+HUGE_SCORE - d_level)   // (edge case, normally loser to move)
                            : (-HUGE_SCORE + d_level);  // side to move is mated
        }
        else if (state == GameState::DRAW) { 
            // This is Stalemate.
            //print_gameboard(engine.game_board);
            d_end_score = 0.0;    // I am not needed because of initilazation
        }
        else
        {
            assert(0);
        }

        //Move defaultMove = Move{};       // Use a default move", as there is no "best move" in this position. Its the end.
        final_result = make_tuple(d_end_score, the_best_move);

    } else if (depth == 0) {

        // depth == 0 is the deepest level of analysis. Here recursive analysis must stop.
        //
        // Evaluate end node "relative score". Relative score is always positive for great positions for the specified player. 
        // Absolute score is always positive for great positions for white.


        d_end_score = evaluate_board(engine.game_board.turn, legal_moves);

        //Move defaultMove = Move{};       // Use a default move", as there is no "best move" in this position. Its the end.
        final_result = make_tuple(d_end_score, the_best_move);

    } else {
        //
        // Keep analyzing moves (recurse another level)
        //

        std::vector<Move> sorted_moves;              // Local array to store sorted moves
        sorted_moves = legal_moves;


        // Sort the moves by relative score (a descending sort over doubles)
        // "iterive deepining"
        sort_moves_by_score(sorted_moves, move_scores, true);

        //
        // Loop over all moves
        //
        d_end_score = -HUGE_SCORE;
        // Note: by default pick the first move? Or best move from last time? Does it matter?
        // I guess note, if we are not sorted.
        the_best_move =  sorted_moves[0]; 

        for (Move &m : sorted_moves) {
            
            #ifdef _DEBUGGING_PUSH_POP
                string temp_fen_before = engine.game_board.to_fen();
            #endif
            
            // Push data
            engine.push(m);

            #ifdef _DEBUGGING_PUSH_POP
                string temp_fen_between = engine.game_board.to_fen();
            #endif

            // Recursive call down another level.
            auto ret_val = store_board_values_negamax((depth - 1), -beta, -alpha
                                                    , move_scores
                                                    , m, debug);
            
            // ret_val is a tuple of the score and the move.
            double d_score_value = -get<0>(ret_val);

            // Debug   NOTE: this if check reduces speed
            // if (debug == true) {
            //     cout << colorize(AColor::BRIGHT_GREEN, "On depth " + to_string(depth) + ", move is: " + move_to_string(m) + ", score_value below is: " + to_string(d_score_value) + ", color perspective: " + color_str(engine.game_board.turn)) << endl;
            //     print_gameboard(engine.game_board);
            // }

            // Pop data
            engine.pop();

            // Digest (store) score result
            string temp_fen_now = engine.game_board.to_fen();
            move_scores[temp_fen_now][m] = d_score_value;

            // uint64_t someInt = engine.game_board.zobrist_key;
            // move_scores[someInt][m] = d_score_value;

            // Note: Should this comparison be done with a fuzz?
            bool b_use_this_move = (d_score_value > d_end_score);


            if (b_use_this_move) {
                d_end_score = d_score_value;
                the_best_move = m;
            }

            #define VERY_SMALL 1.0e-9
            alpha = max(alpha, d_end_score);
            // NOTE: Is this the best way to do this comparison?
            //if ((alpha) >= beta) {
            if ((alpha) >= (beta+VERY_SMALL)) {   
                // Stop looking for new moves - (break out of the for loop over all legal moves)
                #ifdef _DEBUGGING_MOVE_TREE
                    // Append to the move tree
                    snprintf(szDebug, sizeof(szDebug), "   alpha= %f,  beta %f      %f", alpha, beta, d_end_score);
                    int ierr = fputs(szDebug, fpStatistics);
                    assert (ierr!=EOF);
                #endif
                break;
            }

        }

        // Assemble return
        final_result = make_tuple(d_end_score, the_best_move);
    }


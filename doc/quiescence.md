



tuple<double, Move> MinimaxAI::store_board_values_negamax(int depth, double alpha, double beta
                                    //, unordered_map<uint64_t, unordered_map<Move, double, MoveHash>> &move_scores
                                    , unordered_map<std::string, unordered_map<Move, double, MoveHash>> &move_scores
                                    , ShumiChess::Move& move_last
                                    , bool debug) {






    // Distribution that mimics rand(): 0 .. RAND_MAX

    GameState state = engine.game_over(legal_moves);

    double d_end_score = 0.0;
    Move the_best_move = {};


    Vector<Move> moves_to_loop_over = legal_moves;


    If game over


        // Game is over.  Here recursive analysis must stop.

        // Relative (negamax): score is from the side-to-move’s perspective.
        // "mate-distance bookkeeping": (use a large mate score reduced by d_level so shorter mates have larger magnitude.

        return { d_end_score, Move{} };


    } else if (depth < 0) {     // We already tried to stop earlier, but were forced to continue.

   
        unquiet_moves = engine.reduce_to_unquiet_moves(legal_moves);

        Vector<Move> moves_to_loop_over = unquiet_moves;   

        // decrement depth, and 
        // continue recursive search. 


    } else if (depth == 0) {     // This is where analysis should stop UNLESS unqueit moves



        bool in_check = engine.is_king_in_check(engine.game_board.turn);


        if in_check {
            // when in check, by definition all moves are check esscapes, and all escspes are legal moves.
            Vector<Move> moves_to_loop_over = legal_moves;   

            // decrement depth, and 
            // continue recursive search. 

        }

        double d_end_score = evaluate_board(engine.game_board.turn, legal_moves);

        // stand-pat cutoff
        // If d_end_score ≥ beta ⇒ return d_end_score (fail-high).
        if (d_end_score >= beta) {
            return { d_end_score, Move{} };
        }

        // raise alpha to stand-pat, then search captures/promotions only
        alpha = std::max(alpha, d_end_score);

        unquiet_moves = engine.reduce_to_unquiet_moves(legal_moves);


        Vector<Move> moves_to_loop_over = unquiet_moves;   

        //if (unquiet_moves.size() > 0) {
        //    
        //}

    } else {
        //
    
        // decrement depth, and 
        // continue recursive search. 

    }









}

// takes a move, but tracks it so pop() can undo
void Engine::pushMove(const Move& move) {

    assert(move.piece_type != NONE);

    move_history.push(move);

    // Switch color
    this->game_board.turn = utility::representation::opposite_color(move.color);

    // zobrist_key "push" update (side to move)
    game_board.zobrist_key ^= zobrist_side;

    // Update full move "clock" (used for display)
    this->game_board.fullmove += static_cast<int>(move.color == ShumiChess::Color::BLACK); //Fullmove incs on white only
  
    // Push full move "clock"
    this->halfway_move_state.push(this->game_board.halfmove);

    // Update half move status (used only to apply the "fifty-move draw")
    ++this->game_board.halfmove;
    if(move.piece_type == ShumiChess::Piece::PAWN) {
        this->game_board.halfmove = 0;
    }
    
    // Remove the piece from where it was
    ull& moving_piece = access_pieces_of_color(move.piece_type, move.color);
    moving_piece &= ~move.from;

    // Returns the number of trailing zeros in the binary representation of a 64-bit integer.
    int square_from = utility::bit::bitboard_to_lowest_square(move.from);
    int square_to   = utility::bit::bitboard_to_lowest_square(move.to);

      // zobrist_key "push" update (for normal moves, remove piece from from square)
      game_board.zobrist_key ^= zobrist_piece_square_get(move.piece_type + move.color * 6, square_from);

    // Put the piece where it will go.
    if (move.promotion == Piece::NONE) {

        moving_piece |= move.to;

        // zobrist_key "push" update (for promotions, new piece on square)
        game_board.zobrist_key ^= zobrist_piece_square_get(move.piece_type + move.color * 6, square_to);
    }
    else {
        // Promote the piece
        ull& promoted_piece = access_pieces_of_color(move.promotion, move.color);
        promoted_piece |= move.to;

        // zobrist_key "push" update (for promotions, new piece on square)
        game_board.zobrist_key ^= zobrist_piece_square_get(move.promotion + move.color * 6, square_to);
    }

    if (move.capture != Piece::NONE) {
        // The move is a capture
        this->game_board.halfmove = 0;

        if (move.is_en_passent_capture) {
            // Enpassent capture
            
            // Looks at the rank just "forward" of this pawn, on the same file?
            ull target_pawn_bitboard = (move.color == ShumiChess::Color::WHITE ? move.to >> 8 : move.to << 8);

            // Gets the number of leading zeros in the pawn butboard. So this is the first pawn in the list?
            int target_pawn_square = utility::bit::bitboard_to_lowest_square(target_pawn_bitboard);
            access_pieces_of_color(move.capture, utility::representation::opposite_color(move.color)) &= ~target_pawn_bitboard;

            game_board.zobrist_key ^= zobrist_piece_square_get(move.capture + utility::representation::opposite_color(move.color) * 6, target_pawn_square);

        } else {
            // Regular capture

            // remove piece from where it was.
            ull& where_I_was = access_pieces_of_color(move.capture, utility::representation::opposite_color(move.color));
            where_I_was &= ~move.to;
            
            game_board.zobrist_key ^= zobrist_piece_square_get(move.capture + utility::representation::opposite_color(move.color) * 6, square_to);
        }
    } else if (move.is_castle_move) {

        if (move.color == ShumiChess::Color::WHITE) {
           game_board.bCastledWhite = true;  // I dont care which side i castled.
        } else {
           game_board.bCastledBlack = true;  // I dont care which side i castled.
        }

        // !TODO zobrist update for castling

        ull& friendly_rooks = access_pieces_of_color(ShumiChess::Piece::ROOK, move.color);
        //TODO  Figure out the generic 2 if (castle side) solution, not 4 (castle side x color)
        // cout << "PUSHING: Friendly rooks are:";
        // utility::representation::print_bitboard(friendly_rooks);
        if (move.to & 0b00100000'00000000'00000000'00000000'00000000'00000000'00000000'00100000) {
            //          rnbqkbnr                                                       RNBQKBNR
            // Queenside Castle (black or white)
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<7);
                friendly_rooks |= (1ULL<<4);
            } else {
                friendly_rooks &= ~(1ULL<<63);
                friendly_rooks |= (1ULL<<60);
            }
        } else if (move.to & 0b00000010'00000000'00000000'00000000'00000000'00000000'00000000'00000010) {
             //                rnbqkbnr                                                       RNBQKBNR
            // Kingside castle
            if (move.color == ShumiChess::Color::WHITE) {
                friendly_rooks &= ~(1ULL<<0);
                friendly_rooks |= (1ULL<<2);
                //assert (this->game_board.white_castle);
            } else {
                friendly_rooks &= ~(1ULL<<56);
                friendly_rooks |= (1ULL<<58);
            }
        } else {        // Something wrong, its not a castle
            assert(0);
        }
    }

    this->en_passant_history.push(this->game_board.en_passant);
    // !TODO zobrist update for en_passant
    this->game_board.en_passant = move.en_passant;
    
    // Manage castling rights
    uint8_t castle_opp = (this->game_board.black_castle << 2) | this->game_board.white_castle;
    this->castle_opportunity_history.push(castle_opp);
    
    // Manage castling status
    this->game_board.black_castle &= move.black_castle;
    this->game_board.white_castle &= move.white_castle;
}
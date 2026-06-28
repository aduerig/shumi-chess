

Related: Debug why FEN creation to OPENING_FEN fails tests (see the GameBoard class)

In reality a chess player spends more time in "complex" positions. In the first order, complexity is simply the total number of moves (FOR BOTH SIDES). This would be fine for now. It would play a lot better if it spent more time in positions, in ratio to the total number of moves (for both sides), at the starting position. This is what human chessplayers do.
SOLVED: There are far far more sophisticated ways to do this, see bug list.


Castling needs refinements:
   1. "5"s role in guard pawns (f,c, pawns only 1/5, a,b,g,h file 2/5). This may encourage guard pawn (f,c)movement.
   2. Account for guard pawns, in castle priviledge. Be careful, this may suppress guard pawn moves too much.
   3. Castling when king on 2cnd rank (but behind guards?). This may encourage guard pawn movement. This may subvert the whole castling motivation. 
   4. (DONE) Castling privledge shoultn't be 1/2 but 2/3 if one privledge still there. This may encourage guard pawn movement.

Currently, we obtain moves by piece. But later we sort the captures up front, isn't it faster to generate captures first, to avoid the sort? Yes and no, the generation algorithm is very effecient and would be comprimised in speed, by forcing captures first, then quiets. So routines like "add_knight_moves_to_vector_t()" would have to be called twice not once, like now. But maybe instead you maintain two lists, captures and quiet moves, and only call the routines once, then glue them toegather at the end. Now when this sort happens, sort_moves_for_search(), we can glue toegather four lists:
   1. TT move      (only one move)
   2. PV move      (only one move)
   3. Captures     (many moves)
   4. Quiet moves  (many moves)
But a problem here, is that both lists: captures, and quiets, are sorted for other reasons. For what reasons?
   1. Captures  (sorted by SEE)
   2. Quiets    (killer moves bubbled up)

Note that sort_moves_for_search() is not called in qsearch. However of course we do move generation in qsearch also. SO we have to look at this, here again we would have two lists. In qsearch we call: sort_unquiet_moves_qsearch(). This has the potential of saving a lot of time in qsearch. Now there is no reduction (other than removing SEE bad captures).
   1. Captures in quissence (MVV-LVA  used to sort them. SEE used to reject some captures). 
      Also Recapture bias, 
Note the Quiets in quissence, were not generated.
I note that we definitly would save an if statment or two (In a for loop) by this trnasition

The build_pawn_file_summary_fast() is too slow. Maybe it should be managed incremently, although than that gives work to the push/pop to maintain it. It only needs updating if a pawn move, pawn capture, or promotion. 

Explore "magic bitboards"
// !TODO: https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html, 


Explore use of SEE in regular search

Explore possibility of cloning "recursive_negamax", one for qsearch and one for regular search. Would this save time? You remove an if internally (if depth==0), but add one in the call? Maybe. This can only
result in a little improvmenet, but is it worth the doubling on common code confusion.

LESSONS:
==========

From game 45:

   draw. Shumi fails to see that he can't castle, because squares between king and rook are in check.
   ANd shouldnt middlegames also (not just openeing and early middlegame) take castling into account?
1. e4 e5 2. Nf3 Nc6 3. Bb5 f6 4. Nc3 Bb4 5. a3 Ba5 6. O-O Bxc3 7. dxc3 Nge7 8. Bc4 d6 9. Qd3 f5 10. Ng5 fxe4 11. Bf7 Kf8 12. Qxe4 Bf5 13. Qf3 Qb8 14. Be6 e4 15. Nxe4 Ne5 16. Qf4 N5g6 17. Qf3 Ne5 18. Qf4 N5g6 19. Qf3 Ne5  *


These is a bug around the routine set_random_on_next_move() and i_randomize_next_move. Notice the calls to get_move_iterative_deepening() and the parameter iRandomMoves. The idea is that a "randommove" is executed and the count decremented, so if IrandoMoves is 2, the first two moves only are random moves.  Withe code here,  if (iRandomMoves > 0) {
        engine.set_random_on_next_move(iRandomMoves);
    } 
Then it works as expected when get_move_iterative_deepening is called from shumi_uci.cpp. Butthe "main line", via engine_communicatormodule. It keeps doing a random move every move. If I comment out the above lines, then it works from the main line but not from shumi_driver.cpp.




Games with me and chess.com:  (1 means i won)
==================================


My problem is this. At the beginning of every deepeining I must decide wether to contune or not, despite a time control of n moves per t minutes. The algorithm should make use of "estimated_elapsed_time". Let k be the number of seconds per move, that the time control decrees is the average per move. Now the algorithm cannot just try to make sure that each move uses less than k seconds. That is to crude. But it must not run out of time either (gievn the limitations of the estimating) Some algorithm must be used to allow "borrowing" of time. Any suggestions?



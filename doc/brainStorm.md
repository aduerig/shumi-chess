
**some thoughts. None of these have been done yet**

It irritates me that "depth" counts down, not up. I understand why, and i don't want to change it but "depth" indicates a value increasing in value with more analysis. This is just something to keep in mind. In my debug I use "level", as in: "level= top_level-depth" But these names are messed up.
SOLVED: There is now depth, and nPly. Both are units of "ply", depth is more complex, started at 1 then increases for each iteration of iterive deepeing. BUT decreased with every node down in analysis. Depth can never go to zero or below.

I understand that using bitmap boards (in GameSetup.hpp) may interfere with the tests. But from a chess player's point of view, sometimes bitboards, sometimes FENs are preferable. So far, having a "load FEN" button, and allowing safe bitboard overrides (of the initial position) are both useful and fine.
SOLVED: Bug fixed that allowed illegal bitboards is solved. No reason not to keep both inut methods now. I prefer FENS now anyway.

Here is a related point. In reality a chess player spends more time in "complex" positions. In the first order, complexity is simply the total number of moves (FOR BOTH SIDES). This would be fine for now. It would play a lot better if it spent more time in positions, in ratio to the total number of moves (for both sides), at the starting position. This is what human chessplayers do.
SOLVED: There are far far more sophisticated ways to do this, see bug list.



-------------------------------------------------------------------------------------------

add_move_to_vector() must take an argument, and an "if" to discard anything but captures and promotions.

add_move_to_vector() is ONLY called from the the family of routines: add_knight_moves_to_vector_t(). 

the family of routines: add_knight_moves_to_vector_t(), are called ONLY from get_psuedo_legal_moves_t().

get_psuedo_legal_moves_t()  ONLY called by get_legal_moves_fast_t()

get_legal_moves_fast_t() is called by:
    get_legal_moves
    move_into_string_full
    recursive_negamax

the only one we care about is recursive_negamax(). The others "pass in" a flag of "false" for b_unquiet_moves_only. For recursive_negamax(), we pass in (depth==0).
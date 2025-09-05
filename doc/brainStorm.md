
**some thoughts. None of these have been done yet**

It irritates me that "depth" counts down, not up. I understand why, and i don't want to change it but "depth" indicates a value increasing in value with more analysis. This is just something to keep in mind. In my debug I use "level", as in: "level= top_level-depth" But these names are messed up.

I understand that using bitmap boards (in GameSetup.hpp) may interfere with the tests. But from a chess player's point of view, sometimes bitboards, sometimes FENs are preferable. So far, having a "load FEN" button, and allowing safe bitboard overrides (of the initial position) are both useful and fine.

I think you allow a "time" to be input on the command line. Don't know the syntax. Great!  But it would be nice to have an integer input that can be a time or depth. as in "-d5", or "-t3.5"?

Here is a related point. In reality a chess player spends more time in "complex" positions. In the first order, complexity is simply the total number of moves (FOR BOTH SIDES). This would be fine for now. It would play a lot better if it spent more time in positions, in ratio to the total number of moves (for both sides), at the starting position. This is what human chessplayers do.

Requests to the UI developer (please),    
   1. Would love to see an indication of what the computer moved, after it moves. Even if just highlighting the square moved to.     
   2. PLease provide a connection to "esc", to close the window WITHOUT printing silly messages to terminal.    
   3. Please start the board in a fixed place, not in some crappy windows random place.

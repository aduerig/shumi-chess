# 趣味
we're back

## Quickstart
### Dependancies
* Python
* IF WINDOWS: 
  * [mingw](https://github.com/niXman/mingw-builds-binaries)
  * lld: error: unable to find library -lvcruntime140, find out how to fix this...

### How to run gui
* `python scripts\run_gui.py`

### How to run tests
* `python scripts\run_tests.py`


## intro
This project is a hobby C++ engine written by OhMesch and ADuerig, and later refined and maintained by PDuerig. It functions as a fast library exposing useful chess functions such as `generate_legal_moves`. A python module `engine_communicator` is also available. There is a serviceable python gui to see the engine in action at `driver/show_board.py`.

This project has AI to make intelligent chess moves, it is called "MinimaxAI". MinimaxAI uses at least the following technologies: Bit Boards, Nwegamax, Alpha/Beta, Iterative deepening, acquiescence, descending MVV-LVA order, PV ordering, killer moves, SEE.

See players (minimaxAI) in * See [players](doc/players.md) for more better desciptions of checked in "MinimaxAI", as far as speed or "intelligence".

## current issue log 
    Not prioritized. No particular order to them. All issues classified as either: a. Bug, or b. Failure (to chess requirements), or c. Sloth (slowdown) or d. Feature. ~~Crossed out~~ items are done, but under testing.
  * Feature: Get Shumi to think while its your move. This is fascinating. I suppose you start a search for the enemy, and the enemy does that, somehow transfer into that analysis.
  * Feature: Takeback button.
  * Sloth: Think on opponents move (human opponents only)
  * Make Shumi transportable.
  * Bug: Get FEN button gives scrambled result when computer player is playing. Maybe this is OK, The scrambled FEN is seen both in the FEN box and the terminal of VSC. Probably just needs screening. 
  * Sloth: Move structure is too big (11 bytes). More could be saved condesing flags. But overhead coding/decoding.
  * Bug: --debug builds fail miserably. I need asserts(0), so I build release (default), but force asserts() on each file with a "#undef NDEBUG". This is really no problem to anyone.
  * Bug: Forces promotions for the human to be to a queen. Problem with the interface. Need a new feature here.
  * Feature: Can't seem to get evaluator to want to trade when it is ahead.
  * Bug: Random AI seems broken (she stalls). This must be a problem caused by the threading, the threading somehow excludes the randomAI as opposed to minimaxAI move. Maybe easy change See get_ai_move_threaded().
  * Sloth: Scores still handles as double, outside of evaluation. This is a more serious problem than it appears, as the TT/TT2 has to convert and unconvert (it stores in centipawns like it should, and has to). I know, just double multiplies and divides, but All scores should be int, in centipawns unless its display. Big job, but not hard, some gain in speed and cleaner code for sure. Big obstacle here is that alpha and beta are still double. Can we change these to centipawns? How do we test the change?
  * Bug: "Windows Close box" (X) fails, upper right corner of window hangs the thread. Bug In Interface.
  * Failure: The 50 ply the unit uses for 50 move rep, should be in moves. Again, so what. I dont care. It makes games too slow.
  * Bug  TT2 still burp2s after dozens of games. 
  * JUST RENAME ALL "double" to "Score" its
  just a refactor and it has to help. There is no way we need double as opposed to float!
  * Feature: Aspiration is coded, but not tested at all, disabled now. This is only a speedup, no improveent in play. Does it ruin the TT2?
  * Feature: No pruning done. At least "delta pruning", maybe should be done, but, coding but never tested, but perhaps this destroys the TT2? Perhaps it destoys creativily. 
  * Failure: Fifty move rule, and 3 time rep, technically need a player to call it. Here the computer just calls it when it sees it. This is normal for chess engines and will never be fixed. 
  * Failure: bitboards_to_algebriac() does not postfix checks with a "+". The function is debug for human consumption only.
  * Failure: Trading motivator in eval only looks at pieces, not pawns.
  * Failure: See to put rooks on passed pawn files. or maybe all pieces to put attack on passed pawns.
  * Bug: Disappearing pieces. Only in the python, display, peices are really there. Infrequent. But repeatible, 
    just enter the FEN: "3qk3/8/8/8/8/8/8/3Q1K1B w KQkq - 0 1", then move Ke1 for white. King "disappears".
  * Feature: Finish bottom/top/white/black/ match scores. Auto flip board at end of game?
  * Bug: No disambigouation in moves printed to terminal. (only a display thing). Look its alreaday in the .PGN.
## todo
* See [brainStorm](doc/brainStorm.md) for more future directions.

## recommended usage (for python scripts/run_tests.py -dD and -tT)

As to app buttons. Flow follows from top to bottom. Always follow this order: flip board (optional), choose players, reset FEN (optional), gets Shumi to start thinking of a move, Wake up to force a move (Shumi plays worse), choose auto play (optional). After game you can optionaly play Export FEN (position), or Export PGN (game), for analysis. Do not use the Windows "X", just hit ESC key.

As to options: There are three: -d, -t, and -r. All must have an integer after them.
  bullet play use:        -d5 or -d6, and     -t200 to -t500 
  fast 5 minute play use: -d7,        and     -t3000    (plays even with PRD)
  for 15 minute game:     -d8,        and     -t5000
  for 60 minute game:     -d9         and     -t9000
Default -d is 8, default -t is 3000.
Use -r only to force it to chose randomly from different moves within a certain delta For "-rn" it applies only to the first n moves. Slows Shumi down, and worsens play. But first move or two who cares, all we want is variety. Must use -r0 for repeatible play. Use cautiosly. Default=2.

Old stuff related to options:
  or you can use "-wd", "-wt", "bd", or "bt", to set the t and d arguments for one side only (black or white)
  You can also use "-rN" where N is some integer like 2, when it will play N "random moves" in a row. This move is randomly
  chosen as one of the best withen a small delta. Watch out, these "random moves" reduce ability. Also of note is the "f0xF" feature for a hexidecimal number F (see Features.hpp for constants).

## Keystrokes active in app
  * esc  - Exit app
  * 1    - Pause play. Useful when playing computers against each other so you can look at the position before the game moves on. You can then hit "Get FEN" button, which will deposit the FEN into the clipboard. Timers not paused, so times will include the pause interval. One key hit again, releases the pause.

## change log
  * Abandoned 10/11/2025
  * see ...

## building
This project uses CMAKE for C++ parts of the engine. It also exposes a python module written in C++ that can access the board state, and several simple engine commands for the purposes of a GUI (a somewhat simple GUI is provided). There is also two AI's builty on the engine: a. RandomAI, and MinimaxAI. RandomAI is what you think, looks at all legal moves and selects a random response. MinimaxAI is much different using many technologies such as alpha/beta, negamax, iterative deepening, quiessence, SEE, and so on, for move choice.

* use the python files in the `scripts/` folder
* Run before compilation to compile with clang
```
export CXX=/usr/bin/clang++
export CC=/usr/bin/clang
```

## useful argument examples
* `python scripts/run_tests.py "LegalPositionsByDepthParam/LegalPositionsByDepth.LegalPositionsByDepth/5"`
* `python scripts/run_gui.py --human --fen "4k3/pp1p1ppp/8/4pP2/1Pp1P3/8/P1PP2PP/4K3 w - - 0 1"`


## benchmarking
* note only works if you have callgrind / perf installed
* `python scripts/callgrind_minimax.py`
* `python scripts/perf_minimax.py`

## developing
* if you are facing "unresolved imports" on vscode in the `show_board.py` python file, you can create the file `.env` and add as the contents
** `PYTHONPATH=driver`

## technical notes
[probably a good resource](https://github.com/GunshipPenguin/shallow-blue)
  * [his blog on magic bitboards](https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html)

### using this as CMAKE + gtest template
https://github.com/bast/gtest-demo

### using cpython to compile C extensions to communicate to c++ engine for gui
https://docs.python.org/3/extending/

### bit twiddling hacks
http://graphics.stanford.edu/%7Eseander/bithacks.html

### for gtest, maybe want to use this
https://gist.github.com/elliotchance/8215283


## profiling
* perf (by function)
  * perf record -o ./bin/perf.data -g ./bin/run_minimax_depth $1
  * perf report -g  -i ./bin/perf.data
* valgrind (line by line hopefully)
  * valgrind --tool=callgrind ./bin/run_minimax_depth 5
  * kcachegrind callgrind.out.pid
* gprof (line by line)
  * add `-pg` to complilation, and -l
  * `gprof -l ./my_program > analysis.txt`




# laser

example websocket requests
```
SEND (periodically, looks like every 9s): {
  "messageType":"ping"
}

SEND: {
  "messageType":"games"
}

GET (x2): {
  "status":"games","data":{"games":[],"nConnections":4}
}

GET: {
  "status": "games",
  "data": {
    "games": [
      {
        "fen": "7q/4pnk1/4prn1/5pp1/1PP5/1NRP4/1KNP4/Q7 b - - 0 1",
        "turn": "b",
        "times": [180000, 180000],
        "increments": [0, 0],
        "lastMoveTime": null,
        "name": [null, "idk"],
        "allFens": { "7q/4pnk1/4prn1/5pp1/1PP5/1NRP4/1KNP4/Q7": 1 },
        "id": "Y6FiSixi56az-oi57EZGH"
      }
    ],
    "nConnections": 4
  }
}

SEND: {
  "messageType":"join","data":{"id":"Y6FiSixi56az-oi57EZGH","name":"lmfao"}
}

GET: {
  "status": "joined",
  "data": {
    "fen": "7q/4pnk1/4prn1/5pp1/1PP5/1NRP4/1KNP4/Q7 b - - 0 1",
    "key": "C28LgQd9apnQ0sP7JGToP",
    "id": "Y6FiSixi56az-oi57EZGH",
    "names": ["lmfao", "idk"],
    "times": [180000, 180000],
    "index": 0,
    "color": "w"
  }
}

GET: {
  "status":"moved","data":{"times":[180000,156899],"move":["f5","e4"],"winner":null}
}

SEND: {
  "messageType":"move","data":{"key":"C28LgQd9apnQ0sP7JGToP","id":"Y6FiSixi56az-oi57EZGH","name":"lmfao","fen":"7q/4pnk1/4prn1/3P2p1/1P2p3/1NRP4/1KNP4/Q7 b - - 0 1","move":["c4","d5"]}
}

GET: {
  "status":"moved","data":{"times":[179901,156899],"move":["c4","d5"],"winner":null}
}

GET: {
  "status":"gameOver","data":{"winner":"w","times":[179901,0]}
}
```


TO CREATE GAME:

```
SEND: {"messageType":"create","data":{"fen":"7q/4pnk1/4prn1/5pp1/1PP5/1NRP4/1KNP4/Q7 b - - 0 1","turn":"b","times":[180000,180000],"increments":[0,0],"color":"b","name":"lmfao"}}

GET: {"status":"created","data":{"key":"8p2Lt1nEgcwclSeWQ2XQQ","id":"j2CENKH6XNcmUDtu72VTS"}}
GET: {"status":"games","data":{"games":[{"fen":"7q/4pnk1/4prn1/5pp1/1PP5/1NRP4/1KNP4/Q7 b - - 0 1","turn":"b","times":[180000,180000],"increments":[0,0],"lastMoveTime":null,"name":[null,"lmfao"],"allFens":{"7q/4pnk1/4prn1/5pp1/1PP5/1NRP4/1KNP4/Q7":1},"id":"j2CENKH6XNcmUDtu72VTS"}],"nConnections":3}}

someone joins

GET: {"status":"joined","data":{"fen":"7q/4pnk1/4prn1/5pp1/1PP5/1NRP4/1KNP4/Q7 b - - 0 1","key":"8p2Lt1nEgcwclSeWQ2XQQ","id":"j2CENKH6XNcmUDtu72VTS","names":["idk","lmfao"],"times":[180000,180000],"index":1,"color":"b"}}
GET: {"status":"games","data":{"games":[],"nConnections":3}}
```






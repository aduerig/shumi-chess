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
This project is a hobby C++ engine written by OhMesch and ADuerig, and later refined and maintained by PDuerig. It functions as a fast library exposing useful chess functions such as `generate_legal_moves`. A python module `engine_communicator` is also avaliable. There is a serviceable python gui to see the engine in action at `driver/show_board.py`.

This project has AI to make intelligent chess moves, it is called "MinimaxAI". MinimaxAI uses at least the following technologies: Bit Boards, Alpha/Beta, Iterative deepening, acquiescence, descending MVV-LVA order, PV ordering (at root), killer moves. 

See players (minimaxAI) in * See [players](doc/players.md) for more better desciptions of checked in "MinimaxAI", as far as speed or "intelligence".

## todo
* See [brainStorm](doc/brainStorm.md) for more future directions.


## current issue log 
    All issues classified as either: a. bug, or b. failure (to chess requirements), or c. sloth (slowdown) or d. feature to add. ~~Crossed out~~ items are done, but under testing.
    
  * Bug: --debug builds fail miserably. I need asserts(0), so I build release (default), but force asserts() on
  each file with a "#undef NDEBUG".
  * Bug: Serious after one game, (human .vs. shumi, how do you start the next game) (reuse the "pop!" button).
  * Bug:  Forces promotions for the human to be to a queen. Problem with the interface I suppose.
  * Bug: Weird interface bug prevents AI from underpromotion to queen. "7b/7b/8/8/1pk5/1n6/2p5/K7 w - - 0 1". (after white moves Ka2).  The AI and engine come up with c1=N. c1=Q leads to a stalemate and is not considered by the engine. c1=N is the correct move and is checkmate. At every depth, the AI and engine correctly choses c1=N. But the interface somehow makes the c1=N move, translates it to c1=Q which is stalemate and the game is over and a draw. 
  * Bug: cant seem to get evaluator to want to trade when it should.
  * Feature: Ability to set one minimaxai to a different setup than the other. Very useful feature. Excellent for testing.
  ~~* Feature: No ability to truely randomize response without ruining capability. This is much harder than it sounds. RANDOMIZING_MOVES does not work at all to do that, it just does a small delta.~~
  * Bug: "Some time repetition" (when playing in the game) crashes the system. Over leveling, runs off to the 256 level trap. Only seen in autoplay. Not always seen., not frequent.
  * Feature: Cant play black from bottom of board.
  * Feature: A "best score" field up in top of the showboard.py. Connect it to the double field used for root PV?
  ~~* Failure: does not recognize draw by insuffecent material.~~
  * Bug: Random AI seems broken (she stalls). This must be a problem caused by the threading, the threading somehow excludes the random move. Maybe easy change See get_ai_move_threaded().
  * Feature: Output or matybe input .pgn files. Most important is output.
  * Sloth: Scores still handles as double, outside of evaluation. This is a more serious problem than it appears, as the TT has to convert and uncovert (it stores in centipawns like it should, and has to. I know, just double multiplies and divides, but All scores should be int, in centipawns unless its display. Big job, but not hard, some gain in speed and cleaner code for sure).
  ~~* Feature: Interface needs better stuff for autoplay. Like a "current games won for both white and black", Also needs a "best absolute score" field.~~
  * Bug: Moveing Thread hangs after 12-25 moves or so, in long chess game. Only way out is to cut/paste the fen into a restarted app. Then its all fine, for 10 or so moves more. Tedius. Would be nice to have a "load last fen" button, but it would have to be stored in a file. What a pain. This is rare, and does not seem to happen in autoplay.
  * Bug: "Windows Close box" fails, upper left corner of window hangs the thread. Bug In Interface.
  * ~~Failure: Doesn't recognize three fold position repitition. (Stalemate works fine). Note that the "move_history" should allow us to get this. This is a problem in the engine. Wrong. uses the zobrist.~~ 
  * Failure: The 50 ply the unit uses for 50 move rep, should be in moves. Again, so what. Its now 20, for testing only.
  * ~~Failure: Doesnt recognize Enpassent (its not implemented). This is a problem in the engine.~~
  * Sloth: No Transposition table (TT) implemented. This is a problem in MinimaxAI. Comment: Wrong its "slightly" implemented. Its used for repeat position ID, but not for move sorting. 
  * ~~Sloth: Zobrist is not maintained for castling or en passant.~~
  * ~~Bug: The trap: "! NODES VISITED trap#2 ..." is horrible in its choice of best move. Needs a "Wake up grampa"~~ ~~functionality. This is a problem in MinimaxAI.~~
  * ~~Sloth: Should use "Anytime behavior" of iterive deepinging, to make a "Wake up grampa" button. (use last levels of deepeinings results.) Related to the above "trap #2", as both of these situations should do this.  This is a problem in MinimaxAI.~~
  * Sloth: Use other "speedups", that result from iterive deepening. (~~Killer moves~~ + History heuristics, aspiration). These changes do not rely on TT or transposition tables. This is a problem in MinimaxAI.
  * Sloth: Does not move immediatly if only one legal move. Only problem in fixing it, what is the evaluation? can you present a move to the user, with no evaluation? Zero evaluation? And what about Zobrist?
  * Bug: bitboards_to_algebriac() does not do disambiguation. Too hard for now. Does not show checks either. The function is debug for human consumption only.

## recommended usage (for python scripts/run_tests.py -dD and -tT)
  fast 5 minute play: -d=4 -t=10
  for half hour game: -d=6 -t=1000
  for long game     : -d=8 -t=2000
  for end games     : -d=10 -t=2000   (takes too long unless matrial very small)
   

## change log
  * Abandoned 10/11/2025
  * see ...

## building
This project uses CMAKE for C++ parts of the engine. It also exposes a python module written in C++ that can access the board state, and several simple engine commands for the purposes of a GUI (a somewhat simple GUI is provided). There is also two AI's builty on the engine: a. RandomAI, and MinimaxAI. RandomAI is what you think, looks at all legal moves and selects a random response. MinimaxAI is much different using many technologies such as alpha/beta, iterive deepening, and so on, for move choice.

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






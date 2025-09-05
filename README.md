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
This project is a hobby C++ engine written by OhMesch and I. It functions as a fast library exposing useful chess functions such as `generate_legal_moves`. A python module `engine_communicator` is also avaliable. There is a serviceable python gui to see the engine in action at `driver/show_board.py`.

This project has no AI to make intelligent chess moves. There is another repo written by OhMesch and I that consumes this library to write both a Minimax based AI and a Montecarlo AI based on Deepmind's AlphaZero:
* https://github.com/aduerig/chess-ai

## todo
* i disabled enpassant and ~~castling~~ temporarily until zobrist hashing works
* zobrist hashing not implemented for enpassant RIGHTS (it works for capture) or castling. it might not be set correctly for turn stuff either.
* See [Brainstorm](docs/Brainstorm.md) for more future directions.

## change log
  * Farnsworth 8/14/2025    Reallowed castling so as to play it. Fixes some tests. (not sure of the fate of enpassent)
  * Farnsworth 8/15/2025    Fixed "rook promotion bug" (pawn now always promotes to queen). Increased baseline minimax time. Commented out "Move shouldnt be legal" logic. so as to play it. This always comes up around castling. Seperated out board initial setup.
  * Farnsworth 8/20/2025    BASELINE 1.  All tests now pass. Most castle bugs now fixed. Updated bug log. Refactored.  NOTE: This means the tests don't test enpassant, nor do they test "magical rook appearence3" bug.
 * Farnsworth 8/22/2025     Jailed the "Magical rook appearence3" By disabling zobrist code, see: "if (board_values.find(engine.game_board.zobrist_key) != board_values.end()) {". Simplfied the evaluator to just
 see material. Did not yet remove the ""Magical rook appearence3" bug debug code, awaiting more testing.

## current bug log (unless otherwise specified, happens in minimax AI)
  * I think that the pawn masks for check when the king is on the left and right columns is busted (you can move into check)
  * Edge of the board false checkmates. Checkmate piece is queen with no support, Kxq not allowed as king move. Happens occasionally. Related to above? This one seems to alwaus be in left and right columns also.
  * Doesnt recognize three fold position repitition.
  * ~~Magical rook appearence3 bug: Reproduce: Load fen rnb1kbnr/pppp3p/5qp1/4pp2/8/3PPN2/PPP1BPPP/RNBQK2R w KQkq - 2 5, then move white Nc3. Extra rook shows up on f1. (in debug now it crashes with debug displays)~~
  * ~~Load fen "4k2r/7p/8/8/8/8/8/4K2R w KQkq - 0 1" make white king move, crash. Same castling bug as above most likley. Happens to black in this FEN.~~
  * Doesnt recognize Enpassent (disallowed)
  


## building
This project uses CMAKE for C++ parts of the engine. It also exposes a python module written in C++ that can access the board state, and simple engine commands for the purposes of a GUI.

* use the files in the `scripts/` folder
* Run before compilation to compile with clang
```
export CXX=/usr/bin/clang++
export CC=/usr/bin/clang
```

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
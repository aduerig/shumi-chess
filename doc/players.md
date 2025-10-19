

* "grampa Shumi"    Grampa is a bit board based (h1=0), chess player using Bit Boards, Alpha/Beta, Iterative deepening, acquiescence, descending MVV-LVA order, PV ordering (at root). The engine uses "negamax" technology designed for maximum speed. (no maximizing/minimizing players). Grampa also is fast for other reasons and is able to maintain 200,000 nodes per second on my laptop. Most important, Grampa has acquiesensce. Grampa has almost no chess knowledge (as to evaluation). He understands material. He also understands (or values/disvalues) the following: 1. He values getting the king away from the center and to the corners.


* "gramma shumi". Gramma is cloned from Grampa so has his characteristics inheriated from him. Gramma is faster than Grampa (nodes/sec). As to evaluation, Gramma also has an intelligent (working) "rooks connected" algorithm. This, toegather with grampas' lust for getting the king to a corner, cause gramma to clear its back rank pretty effectivly. Gramma also uses "PV PUSH", or "PV promotion", unlike grampa. And most important, Unlike grampa, gramma hates double and tripled isolated pawns. 


* "uncle shumi 2". Uncle is cloned from Gramma so has her characteristics inheriated from him. But Uncle shumi knows huge amounts more about positional evaluation including at least:
   castling/castled happiness (vast improvement over old "king-distance" method). No more stupid rook or king moves.
   connected rooks
   isolated pawns sadness (single, double and triple)
   attacking of 4-center squares by knights, bishops, or pawns.
   rook occuption of open and semi open files.
The uncle is even faster, about 350,000 nodes per second on my laptop. Uncle shumi also has a "Wake up" button! Works magnicantly!
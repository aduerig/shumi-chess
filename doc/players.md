

* "grampa Shumi"    Grampa is a bit board based (h1=0), chess player using Bit Boards, Alpha/Beta, Iterative deepening, acquiescence, descending MVV-LVA order, PV ordering (at root). The engine uses "negamax" technology designed for maximum speed. (no maximizing/minimizing players). Grampa also is fast for other reasons and is able to maintain 200,000 nodes per second on my laptop. Most important, Grampa has acquiesensce. Grampa has almost no chess knowledge (as to evaluation). He understands material. He also understands (or values/disvalues) the following: 1. He values getting the king away from the center and to the corners.


* "gramma shumi". Gramma is cloned from Grampa so has his characteristics inheriated from him. Gramma is faster than Grampa (nodes/sec). As to evaluation, Gramma also has an intelligent (working) "rooks connected" algorithm. This, toegather with grampas' lust for getting the king to a corner, cause gramma to clear its back rank pretty effectivly. Gramma also uses "PV PUSH", or "PV promotion", unlike grampa. And most important, Unlike grampa, gramma hates double and tripled isolated pawns. 


* "uncle shumi 2". Uncle is cloned from Gramma so has her characteristics inheriated from him. But Uncle shumi knows huge amounts more about positional evaluation including at least:
   castling/castled happiness (vast improvement over old "king-distance" method). No more stupid rook or king moves.
   connected rooks
   isolated pawns sadness (single, double and triple)
   attacking of 4-center squares by knights, bishops, or pawns.
   rook (or queen) occuption of open and semi open files.
The uncle is even faster, about 350,000 nodes per second on my laptop. Uncle shumi also has a "Wake up" button! Works magnicantly!


* "Brother shumi" is significantly faster, using "quiescence delta pruning", and "depth==0" does not force non-queen promotions in reduce_to_unquiet_moves_MVV_LVA(). Brother shumi realized through testing that 85% of all nodes are in "quiescence", or "depth==0" territory, so speedups are there. Also, the brother has a better understanding of passed pawns: brother shumi believes passed pawns should be pushed. Tried "FAST_EVALUATIONS", but no help in speed, so didnt do it. 

* "Sister shumi" uses a transposition table to screen repeats, but does not seem much faster. Sister shumi does attack squares near the king, after the opening. Three time rep now operational. Sister shumi has a dramatic difference: MVV_LVA sorting on (depth>0) nodes. This makes it MUCH faster, but it plays differently. I had to add two eval tricks 
that are new, 1. Stop playing the Scandinavian as black, 2. Stop putting the bishop in front of the queeen pwn. But the speed is indispensible.
You can now use deepening 7 to play rapid games for the first time. Deepening 8 is for long games. (used to be 6 and 7).
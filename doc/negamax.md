Negamax assumes and enforces a whole set of conventions:

    1. Zero-sum symmetry: The game is modeled so one side’s gain is the other’s loss (V = −V’). That symmetry is why negation works at every ply.

    2. Single “maximize” routine: You eliminate separate min/max cases; every node is a max node, and child values are negated when returned to the parent.

    3. Window flip with αβ: When recursing you pass the negated window: use [-β, −α] at the child, then negate the child’s result when you return. Cutoff remains α ≥ β.

    4. Evaluation perspective: Your eval must be from side-to-move’s point of view (or you negate a fixed “white-centric” eval at each ply). Consistency here is critical.

    5. Mate score bookkeeping: Use mate scores like ±(MATE − ply) and, when negating across a ply, also adjust the mate distance by 1 so “faster mates” are preferred correctly for both sides.

    6. Transposition table semantics: Store scores as side-to-move values with bound flags (EXACT / LOWERBOUND / UPPERBOUND) and depth. Retrieval must respect the current αβ window; no extra negation is needed if the side-to-move is part of your Zobrist key (it should be).

    7. Quiescence integrates the same way: Stand-pat and capture search also use negamax with the same [-β, −α] window flip and negation on return.

    8. Pruning heuristics fit naturally: Null-move, late-move reductions, aspiration windows, etc., all plug in with the same negate-and-flip pattern.

    9. PV handling: The principal variation you propagate is the sequence that produced the max value at each node; the score is negated between plies, but the move sequence itself isn’t.

In short: negamax = one uniform recursion, zero-sum symmetry, consistent sign-flip of scores and windows at every ply, plus careful handling of mate distances, TT bounds, and quiescence under that same convention.
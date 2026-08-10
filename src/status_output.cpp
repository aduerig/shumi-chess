

#include "status_output.hpp"

// Cute Chess uses stdout as the UCI communication channel, so std::cout
// is reserved exclusively for valid UCI output.
//
// Shumi writes human-readable status and debugging output to sout.
// By default, sout uses std::clog's stream buffer and therefore writes
// to stderr, which appears in the VS Code terminal when run from Python.
//
// The UCI driver redirects sout to uci_debug.txt when run by Cute Chess.

std::ostream sout(std::clog.rdbuf());
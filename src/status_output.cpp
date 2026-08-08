

#include "status_output.hpp"
//
// cutechess uses "cout" exclusivily for communication with the engines. So the engine cannot use cout.
// We use sout instead, and when playing with cutechess, we divert sout to a file.

// By default, status output goes to stderr.
// When run from Python, this should appear in the VS Code terminal.
std::ostream sout(std::clog.rdbuf());
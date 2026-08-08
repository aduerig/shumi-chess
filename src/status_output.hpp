
#pragma once
//
// cutechess uses "cout" exclusivily for communication with the engines. So the engine cannot use cout.
// We use sout instead, and when playing with cutechess, we divert sout to a file.
#include <iostream>

extern std::ostream sout;
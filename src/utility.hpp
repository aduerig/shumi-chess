#pragma once

//TODO shares a name with builtin lib
#include <string>
#include <vector>
#include <unordered_map>

#include "globals.hpp"

using namespace std;

namespace utility {

namespace representation {

ull acn_to_bit_conversion(const std::string&);
ull bitshift_by_color(ull, ShumiChess::Color, int);
ull lsb_and_pop(ull&);
std::string square_to_position_string(ull square);
std::string move_to_string(ShumiChess::Move);

} // representation

// ? should we really clash in namespace name here
namespace string {

/* 
/  Split an input string according to some string delimiter.
/  Uses space by default. 
*/
std::vector<std::string> split(const std::string&, const std::string& = " ");
bool starts_with(const std::string& main_str, const std::string& smaller_str);

} // end namespace string
} // end namespace utility
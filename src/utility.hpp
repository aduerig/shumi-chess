#pragma once

//TODO shares a name with builtin lib
#include <string>
#include <vector>

#include "globals.hpp"

namespace utility {

namespace representation {

ull acn_to_bit_conversion(const std::string&);

} // representation

namespace string {

/* 
/  Split an input string according to some string delimiter.
/  Uses space by default. 
*/
std::vector<std::string> split(const std::string&, const std::string& = " ");
bool starts_with(const std::string& main_str, const std::string& smaller_str);

} // end namespace string
} // end namespace utility
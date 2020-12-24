#pragma once

//TODO shares a name with builtin lib
#include <string>
#include <vector>

namespace utility {
namespace string {

// Split an input string according to some string delimiter.
// Uses space by default.
const std::vector<std::string> split(const std::string&, const std::string& = " ");

} // end namespace string
} // end namespace utility
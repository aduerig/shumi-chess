#pragma once

#include <string>
#include <vector>

namespace utility {
namespace string {

// Split an input string according to some string delimiter.
// Uses space by default.
const std::vector<std::string> split(const std::string&, const std::string& = " ");

} // string
} // utility
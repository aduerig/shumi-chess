#include <algorithm>
#include <sstream>

#include "utility.hpp"

namespace utility {
namespace string {

const std::vector<std::string> split(const std::string& str) {
    std::stringstream spaced_str(str);

    std::vector<std::string> split_str_sol;
    std::string temp;
    while (spaced_str >> temp) {
        split_str_sol.push_back(temp);
    }

    return split_str_sol;
}

const std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    const std::string space = " ";
    if (delimiter == " ") {
        return utility::string::split(str);
    }

    const std::string space_replacement = "DHIHKH_NO_COLLISION";
    
    std::replace(str.begin(), str.end(), space, space_replacement);
    std::replace(str.begin(), str.end(), delimiter, space);
    std::stringstream spaced_str(str);

    std::vector<std::string> split_str_sol;
    std::string temp;
    while (spaced_str >> temp) {
        std::replace(temp.begin(), temp.end(), space_replacement, space);
        split_str_sol.push_back(temp);
    }

    return split_str_sol;
}

} // string
} // utility
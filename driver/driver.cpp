#include <cstdio>
#include <ostream>
#include <iostream>

#include <utility.hpp>

int main()
{
    printf("ok\n");
    auto val = utility::string::split("fishorbanana", "or");
    for (auto x : val) {
        std::cout << x << std::endl;
    }
    return 0;
}
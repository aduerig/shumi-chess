#include <cstdio>
#include <ostream>
#include <iostream>

#include <engine.hpp>
#include <utility.hpp>

using namespace std;

int main()
{
    auto val = utility::string::split("fishorbanana", "or");
    for (auto x : val) {
        cout << x << endl;
    }

    ShumiChess::Engine test_engine;

    int num_legal_moves = test_engine.get_legal_moves().size(); 
    cout << "number of legal moves turn one: " << num_legal_moves << endl;
    return 0;
}
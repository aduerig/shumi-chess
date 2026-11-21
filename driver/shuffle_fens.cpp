#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "fen_database.hpp"
#include "status_output.hpp"

#ifdef _WIN32
    #include <conio.h>
#else
    static inline int _getch() { return std::cin.get(); }
#endif

using namespace std;
using namespace ShumiChess;

//////////////////////////////////////////////////////////////////////////////////////////

//const filesystem::path filename = "NNUE/FENs/100K_Shumi_vs_othersA.txt";
const filesystem::path filename = "NNUE/FENs/evaluated_FENs.txt";

//////////////////////////////////////////////////////////////////////////////////////////

// make with: cmake --build C:\programming\shumi-chess\build --config Release --target shuffle_fens

int main() {
    sout << "Shuffling FEN file: " << filename.string() << endl;
    sout << "..." << endl;

    string error;
    if (!shuffle_FEN_file(filename, &error)) {
        sout << error << endl;
        sout << "Press any key to exit..." << endl;
        _getch();
        return EXIT_FAILURE;
    }

    sout << "SUCCESS - press any key" << endl << endl;
    _getch();
    return EXIT_SUCCESS;
}

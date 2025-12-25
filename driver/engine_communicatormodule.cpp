// using https://docs.python.org/3/extending/extending.html as template
// these lines must come first
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <algorithm>
#include <ostream>
#include <iostream>
#include <utility>
#include <string>
#include <chrono>

#include "globals.hpp"
#include <engine.hpp>
#include <minimax.hpp>

#include <gameboard.hpp>

#undef NDEBUG
//#define NDEBUG         // Define (uncomment) this to disable asserts
#include <assert.h>


using namespace std;

static PyObject*
engine_communicator_systemcall(PyObject* self, PyObject* args) {
    const char *command;
    int sts;

    if (!PyArg_ParseTuple(args, "s", &command)) {
        return NULL;
    }

    // return of 0 is success, else error.
    sts = system(&command[1]);      // First character is a singling character (used by python)

    //cout << " sts " << sts;
    
    return PyLong_FromLong(sts);
}




ShumiChess::Engine python_engine;
vector<ShumiChess::Move> last_moves;

static PyObject*
engine_communicator_get_legal_moves(PyObject* self, PyObject* args) {
    vector<ShumiChess::Move> moves = python_engine.get_legal_moves();
    last_moves = moves;
    vector<string> moves_readable;
    // map function that loads into moves_string
    transform(
        moves.begin(),
        moves.end(),
        back_inserter(moves_readable),
        utility::representation::move_to_string
    );

    PyObject* python_move_list = PyList_New(0);
    for (string move : moves_readable)
    {
        PyList_Append(python_move_list, Py_BuildValue("s", move.c_str()));
    }
    return python_move_list;
}


static PyObject* engine_communicator_print_from_c(PyObject* self, PyObject* args) {

    //python_engine.       
    
    return Py_BuildValue(""); // this is None in Python

}



static PyObject*
engine_communicator_get_piece_positions(PyObject* self, PyObject* args) {
    vector<pair<string, ull>> pieces = {
        make_pair("black_pawn", python_engine.game_board.black_pawns),
        make_pair("black_rook", python_engine.game_board.black_rooks),
        make_pair("black_knight", python_engine.game_board.black_knights),
        make_pair("black_bishop", python_engine.game_board.black_bishops),
        make_pair("black_queen", python_engine.game_board.black_queens),
        make_pair("black_king", python_engine.game_board.black_king),
        make_pair("white_pawn", python_engine.game_board.white_pawns),
        make_pair("white_rook", python_engine.game_board.white_rooks),
        make_pair("white_knight", python_engine.game_board.white_knights),
        make_pair("white_bishop", python_engine.game_board.white_bishops),
        make_pair("white_queen", python_engine.game_board.white_queens),
        make_pair("white_king", python_engine.game_board.white_king)
    };

    PyObject* python_all_pieces_dict = PyDict_New();
    for (pair<string, ull> piece_pair : pieces)
    {
        string piece_name = piece_pair.first;
        ull piece_bitboard = piece_pair.second;
        PyObject* python_piece_list = PyList_New(0);
        while (piece_bitboard) {
            ull single_piece /*  */= utility::bit::lsb_and_pop(piece_bitboard);
            string pos_string = utility::representation::square_to_position_string(single_piece);
            PyList_Append(python_piece_list, Py_BuildValue("s", pos_string.c_str()));
        }
        auto python_string_name = Py_BuildValue("s", piece_name.c_str());
        PyDict_SetItem(python_all_pieces_dict, python_string_name, python_piece_list);
    }
    return python_all_pieces_dict;
}

static PyObject*
engine_communicator_make_move_two_acn(PyObject* self, PyObject* args) {
    char* from_square_c_str;
    char* to_square_c_str;

    if(!PyArg_ParseTuple(args, "ss", &from_square_c_str, &to_square_c_str)) {
        return NULL;
    }

    string from_square_acn(from_square_c_str);
    string to_square_acn(to_square_c_str);

    ShumiChess::Move found_move = {};
    for (const auto move : last_moves) {
        if (from_square_acn == utility::representation::bitboard_to_acn_conversion(move.from) && 
                to_square_acn == utility::representation::bitboard_to_acn_conversion(move.to)) {
            found_move = move;
        }
    }

    // Get, and store in engine, the algebriac (SAN) text form of the user's move.
    // string tempString;
    // python_engine.bitboards_to_algebraic(ShumiChess::WHITE, found_move, ShumiChess::GameState::INPROGRESS
    //                                     , false
    //                                     , false
    //                                     , tempString);      // Output
    python_engine.users_last_move = found_move;

    python_engine.move_history = stack<ShumiChess::Move>();
    //python_engine.move_history.push(found_move);

    if (found_move.piece_type == 6)       // Piece::NONE is 6
    {
        cout << "\x1b[1;31m" << " You are full of it " << "\x1b[0m" << endl;
    } else {
        // Tell the engine the move
        python_engine.pushMove(found_move);
        
        ++python_engine.repetition_table[python_engine.game_board.zobrist_key];
    }
    return Py_BuildValue("");
}

static PyObject*
engine_communicator_set_random_number_of_moves(PyObject* self, PyObject* args) {
    int randomMoveCount;
  
    if(!PyArg_ParseTuple(args, "i", &randomMoveCount)) {
        return NULL;
    }
    //cout << "random!" << randomMoveCount << endl;

    python_engine.set_random_on_next_move(randomMoveCount);

    return Py_BuildValue("");
}

static PyObject*
engine_communicator_game_over(PyObject* self, PyObject* args) {
    // returns constant (GameState C constant) 
    return Py_BuildValue("i", (int) python_engine.is_game_over());
}

// By "reset engine" is meant: "new game". 
static PyObject*
engine_communicator_reset_engine(PyObject* self, PyObject* args) {
    //cout << "reset_engine" << endl;
    const char* fen_string = nullptr;
    if (!PyArg_ParseTuple(args, "|s", &fen_string)) {
        return nullptr; 
    }
    if (fen_string != nullptr) {
        python_engine.reset_engine(fen_string);
    } else {
        python_engine.reset_engine();
    }
    Py_RETURN_NONE;
}



static PyObject*
engine_communicator_get_fen(PyObject* self, PyObject* args) {
    return Py_BuildValue("s", python_engine.game_board.to_fen().c_str());
}

static PyObject*
engine_communicator_get_features_default(PyObject* self, PyObject* args) {
    PyObject* ret = Py_BuildValue("i", _DEFAULT_FEATURES_MASK);
    return ret;
}

static PyObject*
engine_communicator_get_move_number(PyObject* self, PyObject* args) {
    PyObject* ret = Py_BuildValue("i", python_engine.g_iMove);    // real moves in whole game
    return ret;
}

static PyObject*
engine_communicator_get_engine(PyObject* self, PyObject* args) {
    // Create Python capsule with a pointer to the Engine object
    PyObject* engine_capsule = PyCapsule_New((void * ) &python_engine, "engineptr", NULL);
    return engine_capsule;
}


MinimaxAI* minimax_ai = new MinimaxAI(python_engine);
static PyObject*
minimax_ai_get_move(PyObject* self, PyObject* args) {
    ShumiChess::Move gotten_move = minimax_ai->get_move();
    string move_in_acn_notation = utility::representation::move_to_string(gotten_move);
    return Py_BuildValue("s", move_in_acn_notation.c_str());
}


static PyObject*
minimax_ai_get_move_iterative_deepening(PyObject* self, PyObject* args)
{
    double milliseconds;      // required
    int    max_deepening;     // required
    int    argument = 0;      // optional, default 0

    // required double, required int, optional int
    if (!PyArg_ParseTuple(args, "di|i",
                          &milliseconds,
                          &max_deepening,
                          &argument))
    {
        return NULL;
    }

    ShumiChess::Move gotten_move;
    std::string move_in_acn_notation;

    Py_BEGIN_ALLOW_THREADS;

    // Pass arguments through to the engine, get the opponents move.
    gotten_move = minimax_ai->get_move_iterative_deepening(
        milliseconds,
        max_deepening,
        argument
    );

    move_in_acn_notation = utility::representation::move_to_string(gotten_move);

    Py_END_ALLOW_THREADS;

    return Py_BuildValue("s", move_in_acn_notation.c_str());
}

static PyObject*
engine_communicator_wakeup(PyObject* self, PyObject* args) {
    minimax_ai->wakeup();
    return Py_BuildValue(""); // this is None in Python
}

// NOTE: unfinished
static PyObject*
engine_communicator_get_draw_reason(PyObject* self, PyObject* args) {
    const char* reason = "3 time rep";

    // "s" builds a Python str from a C null-terminated char*
    return Py_BuildValue("s", reason);
}

static PyObject*
engine_communicator_evaluate(PyObject* self, PyObject* args) {

    int nPhase = minimax_ai->phaseOfGame();
    std::vector<ShumiChess::Move> legal_moves = minimax_ai->engine.get_legal_moves();
    bool b_is_Quiet = !minimax_ai->engine.has_unquiet_move(legal_moves);
    
    int cp_score_best = minimax_ai->evaluate_board( minimax_ai->engine.game_board.turn, nPhase, false, b_is_Quiet);
    double pawnScore =  minimax_ai->engine.convert_from_CP(cp_score_best);

    cout << "eval = " << pawnScore << endl;
    
    return Py_BuildValue(""); // this is None in Python
}


static PyObject*
engine_communicator_get_game_timew(PyObject* self, PyObject* args) {
    //cout << "wsec=" << minimax_ai->engine.game_white_time_msec << endl;
    return Py_BuildValue("i", minimax_ai->engine.game_white_time_msec);
}
static PyObject*
engine_communicator_get_game_timeb(PyObject* self, PyObject* args) {
    //cout << "wsec=" << minimax_ai->engine.game_black_time_msec << endl;
    return Py_BuildValue("i", minimax_ai->engine.game_black_time_msec);
}




static PyObject* 
engine_communicator_get_best_score_at_root(PyObject* self, PyObject* args) {
    int iCPScore = python_engine.get_best_score_at_root();
    double iPawnScore = iCPScore / 100.0;        // e.g. 243 -> 2.43, -1234 -> -12.34
    double absip;

    bool isMateScore = IS_MATE_SCORE(iPawnScore);
    char szScore[16];

    if (isMateScore) {
        if (iPawnScore >= 0.0) {
            absip = iPawnScore;
        } else {
            absip = -iPawnScore;
        }
 
        double mate_in_number = ((HUGE_SCORE - absip) / 2.0);
        assert (mate_in_number > 0.0);

        if (iPawnScore >= 0.0) {
            sprintf(szScore, "#%.1f", mate_in_number);
        } else {
            sprintf(szScore, "-#%.1f", mate_in_number);
        }
    } else {
        sprintf(szScore, "%.2f", iPawnScore);
    }

    return Py_BuildValue("s", szScore);
    //return Py_BuildValue("d", iPawnScore);    // "d" = Python float
}

static PyObject*
engine_communicator_resign(PyObject* self, PyObject* args) {
    minimax_ai->resign();
    return Py_BuildValue(""); // this is None in Python
}



static PyMethodDef engine_communicator_methods[] = {
    {"systemcall",  engine_communicator_systemcall, METH_VARARGS, ""},

    // These are move generation engines
    {"minimax_ai_get_move_iterative_deepening", minimax_ai_get_move_iterative_deepening, METH_VARARGS, ""},
    {"minimax_ai_get_move",  minimax_ai_get_move, METH_VARARGS, ""},
    
    {"reset_engine",  engine_communicator_reset_engine, METH_VARARGS, ""},      // new game
    {"print_from_c",  engine_communicator_print_from_c, METH_VARARGS, ""},
    {"get_legal_moves",  engine_communicator_get_legal_moves, METH_VARARGS, ""},
    {"is_game_over",  engine_communicator_game_over, METH_VARARGS, ""},         // returns constant (GameState C constant) 
    {"get_piece_positions",  engine_communicator_get_piece_positions, METH_VARARGS, ""},
    {"make_move_two_acn",  engine_communicator_make_move_two_acn, METH_VARARGS, ""},
    {"get_fen",  engine_communicator_get_fen, METH_VARARGS, ""},
    {"get_features_default",  engine_communicator_get_features_default, METH_VARARGS, ""},
    {"get_move_number",  engine_communicator_get_move_number, METH_VARARGS, ""},
    {"get_engine",  engine_communicator_get_engine, METH_VARARGS, ""},
    {"wakeup",  engine_communicator_wakeup, METH_VARARGS, ""},                  //  force a move if thinking
    {"get_best_score_at_root",  engine_communicator_get_best_score_at_root, METH_VARARGS, ""},
    {"get_draw_reason",  engine_communicator_get_draw_reason, METH_VARARGS, ""},
    {"evaluate",  engine_communicator_evaluate, METH_VARARGS, ""},
    {"get_game_timew",   engine_communicator_get_game_timew, METH_VARARGS, ""},     // Total time thinking, since match start
    {"get_game_timeb",   engine_communicator_get_game_timeb, METH_VARARGS, ""},     // Total time thinking, since match start
    {"resign",  engine_communicator_resign, METH_VARARGS, ""},
    {"set_random_number_of_moves",  engine_communicator_set_random_number_of_moves, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef engine_communicatormodule = {
    PyModuleDef_HEAD_INIT,
    "engine_communicator",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    engine_communicator_methods
};


PyMODINIT_FUNC
PyInit_engine_communicator(void) {

    // Seed randomization (using microseconds since ?)
    using namespace std::chrono;
    auto now = chrono::high_resolution_clock::now().time_since_epoch();
    auto us  = duration_cast<microseconds>(now).count();
    std::srand(static_cast<unsigned>(us));  // higher-resolution seed

    PyObject* m = PyModule_Create(&engine_communicatormodule);
    if (m == NULL) {return NULL;}

    // Export GameState enum values to Python as module-level constants
    // Python usage: engine_communicator.DRAW, etc.s
    if (PyModule_AddIntConstant(m, "INPROGRESS", (int)ShumiChess::INPROGRESS) < 0) return NULL;
    if (PyModule_AddIntConstant(m, "WHITEWIN",   (int)ShumiChess::WHITEWIN)   < 0) return NULL;
    if (PyModule_AddIntConstant(m, "DRAW",       (int)ShumiChess::DRAW)       < 0) return NULL;
    if (PyModule_AddIntConstant(m, "BLACKWIN",   (int)ShumiChess::BLACKWIN)   < 0) return NULL;


    return m;
}



// don't need for now, from tutorial

// int
// main(int argc, char *argv[])
// {
//     wchar_t *program = Py_DecodeLocale(argv[0], NULL);
//     if (program == NULL) {
//         fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
//         exit(1);
//     }

//     /* Add a built-in module, before Py_Initialize */
//     if (PyImport_AppendInittab("spam", PyInit_spam) == -1) {
//         fprintf(stderr, "Error: could not extend in-built modules table\n");
//         exit(1);
//     }

//     /* Pass argv[0] to the Python interpreter */
//     Py_SetProgramName(program);

//     /* Initialize the Python interpreter.  Required.
//        If this step fails, it will be a fatal error. */
//     Py_Initialize();

//     /* Optionally import the module; alternatively,
//        import can be deferred until the embedded script
//        imports it. */
//     /* pmodule = PyImport_ImportModule("spam"); */
//     /* if (!pmodule) { */
//         /* PyErr_Print(); */
//         /* fprintf(stderr, "Error: could not import module 'spam'\n"); */
//     /* } */

//     PyMem_RawFree(program);
//     return 0;
// }

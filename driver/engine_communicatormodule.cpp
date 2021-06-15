// using https://docs.python.org/3/extending/extending.html as template
// these lines must come first
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <algorithm>
#include <ostream>
#include <iostream>
#include <utility>

#include <engine.hpp>

using namespace std;

// testing functions
static PyObject*
engine_communicator_systemcall(PyObject* self, PyObject* args)
{
    const char *command;
    int sts;

    if (!PyArg_ParseTuple(args, "s", &command)) {
        return NULL;
    }

    sts = system(command);
    return PyLong_FromLong(sts);
}

static PyObject*
engine_communicator_print_from_c(PyObject* self, PyObject* args)
{
    cout << "this is from C" << endl;
    return Py_BuildValue(""); // this is None in Python
}


// ! actual chess functionality
ShumiChess::Engine python_engine;
vector<ShumiChess::Move> last_moves;

static PyObject*
engine_communicator_get_legal_moves(PyObject* self, PyObject* args)
{
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

static PyObject*
engine_communicator_get_piece_positions(PyObject* self, PyObject* args)
{
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
engine_communicator_make_move_two_acn(PyObject* self, PyObject* args)
{
    char* from_square_c_str;
    char* to_square_c_str;

    if(!PyArg_ParseTuple(args, "ss", &from_square_c_str, &to_square_c_str)) {
        return NULL;
    }

    string from_square_acn(from_square_c_str);
    string to_square_acn(to_square_c_str);

    ShumiChess::Move found_move;
    for (const auto move : last_moves) {
        if (from_square_acn == utility::representation::bitboard_to_acn_conversion(move.from) && 
                to_square_acn == utility::representation::bitboard_to_acn_conversion(move.to)) {
            found_move = move;
        }
    }
    python_engine.push(found_move);
    return Py_BuildValue("");
}

static PyObject*
engine_communicator_pop(PyObject* self, PyObject* args)
{
    python_engine.pop();
    return Py_BuildValue("");
}

static PyObject*
engine_communicator_game_over(PyObject* self, PyObject* args)
{
    return Py_BuildValue("i", (int) python_engine.game_over());
}

static PyObject*
engine_communicator_reset_engine(PyObject* self, PyObject* args)
{
    python_engine.reset_engine();
    return Py_BuildValue("");
}

static PyObject*
engine_communicator_get_move_number(PyObject* self, PyObject* args)
{
    return Py_BuildValue("i", (int) python_engine.game_board.fullmove);
}


static PyObject*
engine_communicator_get_engine(PyObject* self, PyObject* args)
{
    // Create Python capsule with a pointer to the Engine object
    PyObject* engine_capsule = PyCapsule_New((void * ) &python_engine, "engineptr", NULL);
    std::cout << "get_engine called in engine_communicator, returning pointer: " << static_cast<void*>(&python_engine) << endl;
    return engine_capsule;
}


static PyMethodDef engine_communicator_methods[] = {
    {"systemcall",  engine_communicator_systemcall, METH_VARARGS,
        "Execute a shell command."},
    {"print_from_c",  engine_communicator_print_from_c, METH_VARARGS,
        "just prints"},
    {"get_legal_moves",  engine_communicator_get_legal_moves, METH_VARARGS,
        "gets all legal moves"},    
    {"game_over",  engine_communicator_game_over, METH_VARARGS,
        "reports the state of the game"},
    {"get_piece_positions",  engine_communicator_get_piece_positions, METH_VARARGS,
        "gets all piece positions"},
    {"make_move_two_acn",  engine_communicator_make_move_two_acn, METH_VARARGS,
        "takes two acn coordinates and makes the move on the board"},
    {"reset_engine",  engine_communicator_reset_engine, METH_VARARGS,
        "resets the engine to the begining"},
    {"get_move_number",  engine_communicator_get_move_number, METH_VARARGS,
        "gets the current move number"},
    {"pop",  engine_communicator_pop, METH_VARARGS,
        "undoes the last move"},
    {"get_engine",  engine_communicator_get_engine, METH_VARARGS,
        "just returns the raw engine pointer"},
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
PyInit_engine_communicator(void)
{
    return PyModule_Create(&engine_communicatormodule);
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

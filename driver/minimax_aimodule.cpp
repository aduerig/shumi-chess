// using https://docs.python.org/3/extending/extending.html as template
// these lines must come first
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pycapsule.h>

#include <algorithm>
#include <ostream>
#include <iostream>
#include <utility>

#include <engine.hpp>
#include "minimax.hpp"

using namespace std;

// testing functions
static PyObject*
minimax_ai_systemcall(PyObject* self, PyObject* args)
{
    const char *command;
    int sts;

    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;
    sts = system(command);
    return PyLong_FromLong(sts);
}

static PyObject*
minimax_ai_print_from_c(PyObject* self, PyObject* args)
{
    cout << "this is from C (minimax)" << endl;
    return Py_BuildValue(""); // this is None in Python
}


MinimaxAI* minimax_ai = NULL;

// ! actual chess functionality
static PyObject*
minimax_ai_get_move(PyObject* self, PyObject* args)
{
    // Get the pointer to Engine object
    if (minimax_ai == NULL) {
        // Capsule with the pointer to Engine object
        PyObject* engine_capsule_;

        if (!PyArg_ParseTuple(args, "O!", &PyCapsule_Type, &engine_capsule_)) {
            return NULL;
        }

        ShumiChess::Engine* passed_in_engine = reinterpret_cast<ShumiChess::Engine* >(PyCapsule_GetPointer(engine_capsule_, "engineptr"));
        minimax_ai = new MinimaxAI(*passed_in_engine);
    }

    ShumiChess::Move gotten_move = minimax_ai->get_move();
    string move_in_acn_notation = utility::representation::move_to_string(gotten_move);
    return Py_BuildValue("s", move_in_acn_notation.c_str());
}

static PyMethodDef minimax_ai_methods[] = {
    {"systemcall",  minimax_ai_systemcall, METH_VARARGS,
        "Execute a shell command."},
    {"print_from_c",  minimax_ai_print_from_c, METH_VARARGS,
        "just prints"},
    {"get_move",  minimax_ai_get_move, METH_VARARGS,
        "gets a move"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef minimax_aimodule = {
    PyModuleDef_HEAD_INIT,
    "minimax_ai",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    minimax_ai_methods
};

PyMODINIT_FUNC
PyInit_minimax_ai(void)
{
    return PyModule_Create(&minimax_aimodule);
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

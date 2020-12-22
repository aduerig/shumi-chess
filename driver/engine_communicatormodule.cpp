// these lines must come first
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <ostream>
#include <iostream>

#include <engine.hpp>

static PyObject *
engine_communicator_systemcall(PyObject *self, PyObject *args)
{
    const char *command;
    int sts;

    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;
    sts = system(command);
    return PyLong_FromLong(sts);
}

static PyObject *
engine_communicator_print_from_c(PyObject *self, PyObject *args)
{
    std::cout << "this is from C" << std::endl;
    return Py_BuildValue(""); // this is None in Python
}

static PyMethodDef engine_communicator_methods[] = {
    {"systemcall",  engine_communicator_systemcall, METH_VARARGS,
     "Execute a shell command."},
    {"print_from_c",  engine_communicator_print_from_c, METH_VARARGS,
     "just prints"},
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

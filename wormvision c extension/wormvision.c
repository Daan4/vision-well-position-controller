#include "../venv/Include/Python.h"

//https://mail.python.org/pipermail/python-list/2008-June/501130.html


// C version of well bottom features evaluate function.
// Inputs: self -> WellBottomFeaturesEvaluator instance
//         imgdata -> 1d list with grayscale pixel values (8-bit) from LT to BR
//         imgrows -> image row count
//         imgcols -> image col count
//         target -> tuple with target coordinates {x, y}
// Returns: Python tuple with (found offset, found centroid)
//static PyObject * WellBottomFeaturesEvaluator_evaluate(PyObject *self, PyObject *args) {
//    // Create new basic image container and fill it with data from passed image argument
//    PyObject *imgdata_list;
//    PyObject *imgrows_int;
//    PyObject *imgcols_int;
//    PyObject *target_tuple;
//    // O! = python object of type &PyXXX_Type
//    // i = int
//    if(!PyArg_ParseTuple(args, "O!iiO!", &PyList_Type, &imgdata_list,
//                         &imgrows_int, &imgcols_int, &PyTuple_Type, &target_tuple)) {
//        return NULL;
//    }
//}

static PyObject * WBFE_evaluate(PyObject *self, PyObject *args) {
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }
    printf("Hello %s!\n", name);
    Py_RETURN_NONE;
}

static PyMethodDef functions[] = {
    {"WBFE_evaluate", WBFE_evaluate, METH_VARARGS, "Vision algorithm implementation for the well bottom features evaluator."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef wormvision = {
    PyModuleDef_HEAD_INIT,
    "wormvision", // name of module
    "",  // module documentation, may be NULL
    -1, // size of per-interpreter state of the module, or -1 i the module keeps state in global vars
    functions
};

PyMODINIT_FUNC PyInit_wormvision(void) {
    return PyModule_Create(&wormvision);
}
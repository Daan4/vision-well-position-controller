#include "D:\\Program Files\\Python36\\include\\Python.h"
#include "operators/operators_basic.h"

//https://mail.python.org/pipermail/python-list/2008-June/501130.html


// C version of well bottom features evaluate function.
// Inputs: self -> WellBottomFeaturesEvaluator instance
//         imgdata -> 1d list with grayscale pixel values (8-bit) from LT to BR
//         imgrows -> image row count
//         imgcols -> image col count
//         target -> tuple with target coordinates {x, y}
// Returns: Python tuple with (found offset, found centroid)
static PyObject * WellBottomFeaturesEvaluator_evaluate(PyObject *self, PyObject *args) {
    // Create new basic image container and fill it with data from passed image argument
    PyObject *imgdata_list;
    PyObject *imgrows_int;
    PyObject *imgcols_int;
    PyObject *target_tuple;
    // O! = python object of type &PyXXX_Type
    // i = int
    if(!PyArg_ParseTuple(args, "O!iiO!", &PyList_Type, &imgdata_list,
                         &imgrows_int, &imgcols_int, &PyTuple_Type, &target_tuple)) {
        return NULL;
    }
}

static PyMethodDef functions[] = {
    {"WellBottomFeaturesEvaluator_evaluate", (PyCFunction)WellBottomFeaturesEvaluator_evaluate, METH_VARARGS},
    {NULL, NULL, 0, NULL},
};

DL_EXPORT(void)
init_wormvision(void) {
    Py_InitModule("_wormvision", functions);
}

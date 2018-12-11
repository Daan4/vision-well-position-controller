// desktop includes
//#include "D:/Program Files/Python36/include/Python.h"
//#include "D:/Libraries/Documents/Google Drive/ELT/semester 7 minor/EVD1/svn blok 2/evd1/evdk2/EVDK2 v1.0 PC (Qt)/evdk2/operators/operators_basic.h"

// laptop includes
#include "../venv/Include/Python.h"
#include "operators_basic.h"


// Create an image_t struct from python arguments
// Inputs: self -> WellBottomFeaturesEvaluator instance
//         data -> 1d list with grayscale pixel values (8-bit) from LT to BR
//         cols -> image col count
//         rows -> image row count
// Returns: pointer to image_t struct with given imgdata, cols and rows
image_t *newBasicImagePython(PyObject *data, int32_t cols, int32_t rows) {
    // Create image_t struct
    image_t *img = newBasicImage(cols, rows);

    if(img == NULL) { return NULL; }
    // Parse data from python list to c array
    // printf("data\n");
    for(int32_t i=0; i<rows*cols; i++) {
        img->data[i] = (basic_pixel_t) PyLong_AsLong(PyList_GetItem(data, i));
        // printf("%d\n", img->data[i]);
    }
    // printf("rows %d cols %d\n", rows, cols);
    return(img);
}


// C version of well bottom features evaluate function.
// Inputs: self -> WellBottomFeaturesEvaluator instance
//         imgdata -> 1d list with grayscale pixel values (8-bit) from LT to BR
//         imgcols -> image col count
//         imgrows -> image row count
//         target -> tuple with target coordinates {x, y}
// Returns: Python tuple with (found offset, found centroid)
static PyObject *WBFE_evaluate(PyObject *self, PyObject *args) {
    PyObject *imgdata_list;
    int32_t imgrows;
    int32_t imgcols;
    PyObject *target_tuple;
    int32_t target[2];
    if(!PyArg_ParseTuple(args, "O!iiO!", &PyList_Type, &imgdata_list,
                          &imgcols, &imgrows, &PyTuple_Type, &target_tuple)) { return NULL; }
    // Parse args to image_t struct
    image_t *img = newBasicImagePython(imgdata_list, imgcols, imgrows);
    // Parse target from python tuple to array
    target[0] = (int32_t) PyLong_AsLong(PyTuple_GetItem(target_tuple, 0));
    target[1] = (int32_t) PyLong_AsLong(PyTuple_GetItem(target_tuple, 1));
    // printf("target %d, %d\n", target[0], target[1]);

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
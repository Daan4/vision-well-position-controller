// desktop includes
//#include "D:/Program Files/Python36/include/Python.h"
//#include "D:/Libraries/Documents/Google Drive/ELT/semester 7 minor/EVD1/svn blok 2/evd1/evdk2/EVDK2 v1.0 PC (Qt)/evdk2/operators/operators_basic.h"

// laptop includes
//#include "../venv/Include/Python.h"
//#include "operators_basic.h"

// raspberry pi includes
#include "Python.h"
#include "operators_basic.h"

// Manually define M_PI
#define M_PI		3.14159265358979323846

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
//         blur_kernelsize -> kernel size for blur
//         blur_sigma -> sigma for blur
//         c -> constant for gamma operation
//         gamma -> constant for gamma operation
//         open_kernelsize -> morphology kernel size
//         metric_threshold -> threshold for classification
// Returns: Python tuple with (found offset, found centroid)
static PyObject *WBFE_evaluate(PyObject *self, PyObject *args) {
    PyObject *imgdata_list;
    int32_t imgrows;
    int32_t imgcols;
    int32_t kernel_size;
    double sigma;
    basic_pixel_t threshold_param;
    float c;
    float g;
    //int32_t morphology_kernel_size;
    //float metric_threshold;
    int32_t area_threshold;

    PyObject *target_tuple;
    int32_t target[2];
    if(!PyArg_ParseTuple(args, "O!iiO!idffii", &PyList_Type, &imgdata_list,
                          &imgcols, &imgrows, &PyTuple_Type, &target_tuple,
                          &kernel_size, &sigma, &c, &g, &threshold_param,
                          &area_threshold)) { return NULL; }
    // Parse args to image_t struct
    image_t *src = newBasicImagePython(imgdata_list, imgcols, imgrows);
    // Parse target from python tuple to array
    target[0] = (int32_t) PyLong_AsLong(PyTuple_GetItem(target_tuple, 0));
    target[1] = (int32_t) PyLong_AsLong(PyTuple_GetItem(target_tuple, 1));

    // uncomment to print arguments for testing
//    printf("target %d, %d\n", target[0], target[1]);
//    printf("blurkernelsize %d\n", kernel_size);
//    printf("sigma %f\n", sigma);
//    printf("c %f\n", c);
//    printf("g %f\n", g);
//    printf("morphkernelsize %d\n", morphology_kernel_size);
//    printf("metric_threshold %f\n", metric_threshold);
//    printf("area_threshold %d\n", area_threshold);
//    printf("threshold_param %d\n", threshold_param);



    //image_t *dst = newBasicImage(src->cols, src->rows);

    // 1. Gaussian blur
    gaussianBlur(src, src, kernel_size, sigma);

    // 2. Contrast stretch
    contrastStretchFast(src, src);

    // 3. Gamma
    gamma_evdk(src, src, c, g);

    // 4. Threshold
    //thresholdOtsu(src, src, BRIGHT);
    threshold(src, src, 0, threshold_param);
    invert(src, src);

    // fill holes
    fillHoles(src, src, EIGHT);

    // 5. Morphology
    // first generate 49x49 ellipse (ish?) kernel. note: not quite the same as opencv kernel yet
//    int32_t radius = (int32_t) (morphology_kernel_size / 2.0f + 0.5);
//    image_t *kernel = newBasicImage(2*radius-1, 2*radius-1);
//    kernel->view = IMGVIEW_BINARY;
//    uint32_t i = (2*radius-1) * (2*radius-1);
//    int32_t row = 0;
//    int32_t col = 0;
//    basic_pixel_t *k = (basic_pixel_t *) kernel->data;
//    while(i-- > 0) {
//        if((int32_t) (sqrt((row-radius+1)*(row-radius+1) + (col-radius+1)*(col-radius+1)) + 0.5) < radius) {
//            *k++ = 1;
//        } else {
//            *k++ = 0;
//        }
//        if(++col == kernel->cols) {
//            col = 0;
//            row++;
//        }
//    }
//    morph_open(src, dst, kernel);

    // 6. Labelling, feature extraction, classification to select correct blob
//    uint32_t blob_count;
//    blob_count = labelBlobs(dst, dst, EIGHT);
//    float metric;
//    int32_t largest_area_above_threshold = -1;
//    int8_t best_match = -1;
//    blobinfo_t info;
//    for(i = 1; i <= blob_count; i++) {
//        blobAnalyse(dst, i, &info);
//        metric = 4 * M_PI * info.nof_pixels / (info.perimeter * info.perimeter);
//        if(metric > metric_threshold && info.nof_pixels > largest_area_above_threshold) {
//            largest_area_above_threshold = info.nof_pixels;
//            best_match = i;
//        }
//    }
//    if(best_match == -1) {
//        // no valid blob found
//        // return None
//        Py_RETURN_NONE;
//    }
    int8_t best_match = -1;
    float best_score = 1000.0f;
    float roundness_metric, eccentricity_metric, score;
    float m20, m02, m11;
    uint32_t blob_count;
    blob_count = labelBlobs(src, src, EIGHT);
    blobinfo_t info;
    for(uint32_t i = 1; i <= blob_count; i++) {
        blobAnalyse(src, i, &info);
        if(info.nof_pixels < area_threshold) {
            continue;
        }
        // Calculate roundness metric
        roundness_metric = 4 * M_PI * info.nof_pixels / (info.perimeter * info.perimeter);
        // Calculate eccentricity metric using moments
        m20 = normalizedCentralMoments(src, i, 2, 0);
        m02 = normalizedCentralMoments(src, i, 0, 2);
        m11 = normalizedCentralMoments(src, i, 1, 1);
        eccentricity_metric = ((m20 - m02) * (m20 - m02) + 4 * m11 * m11) / ((m20 + m02) * (m20 + m02));
        score = (1-roundness_metric + eccentricity_metric) / 2;

        if(score < best_score) {
            best_score = score;
            best_match = i;
        }
    }

    // 7. Calculate centroid / offset
    int32_t cc, rc;
    int32_t offset_x, offset_y;
    centroid(src, best_match, &cc, &rc);
    offset_x = cc - target[0];
    offset_y = rc - target[1];

    // Cleanup
    deleteImage(src);
    //deleteImage(dst);
    //deleteImage(kernel);

    // Return tuple (offset_x, offset_y)
    PyObject *offset_tuple = PyTuple_New(2);
    PyTuple_SetItem(offset_tuple, 0, PyLong_FromLong(offset_x));
    PyTuple_SetItem(offset_tuple, 1, PyLong_FromLong(offset_y));
    return offset_tuple;
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

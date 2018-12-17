/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Header file for image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#ifndef _OPERATORS_H_
#define _OPERATORS_H_

#include "stdint.h"
#include "stdlib.h"
#include "limits.h"

// ----------------------------------------------------------------------------
// Defines
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Type definitions
// ----------------------------------------------------------------------------
typedef enum
{
    IMGTYPE_BASIC  = 0,  // Unsigned Character
    IMGTYPE_INT16  = 1,  // Integer
    IMGTYPE_FLOAT  = 2,  // Float
    IMGTYPE_RGB888 = 3,  // RGB 8-bit per pixel
    IMGTYPE_RGB565 = 4,
  
    IMGTYPE_MAX    = 2147483647 // Max 32-bit int value,
                                // forces enum to be 4 bytes
}eImageType;

typedef enum
{
    IMGVIEW_STRETCH = 0,
    IMGVIEW_CLIP    = 1,
    IMGVIEW_BINARY  = 2,
    IMGVIEW_LABELED = 3,
  
    IMGVIEW_MAX     = 2147483647 // Max 32-bit int value,
                                 // forces enum to be 4 bytes
}eImageView;

// Pixel types
typedef uint8_t basic_pixel_t;
typedef int16_t int16_pixel_t;
typedef float float_pixel_t;

typedef struct rgb888_pixel_t
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
}rgb888_pixel_t;

typedef uint16_t rgb565_pixel_t;

typedef struct complex_pixel_t
{
    float real;
    float imaginary;
    
}complex_pixel_t;

union pixel
{
    basic_pixel_t  basic_pixel;
    int16_pixel_t  int16_pixel;
    float_pixel_t  float_pixel;
    rgb888_pixel_t rgb888_pixel;
    rgb565_pixel_t rgb565_pixel;
};

typedef union pixel pixel_t;

// Get a single pixel given the image pointer, a column and a row
#define BASIC_PIXEL(img,c,r)  (*(((basic_pixel_t  *)(img->data)) + ((r) * (img->cols) + (c))))
#define INT16_PIXEL(img,c,r)  (*(((int16_pixel_t  *)(img->data)) + ((r) * (img->cols) + (c))))
#define FLOAT_PIXEL(img,c,r)  (*(((float_pixel_t  *)(img->data)) + ((r) * (img->cols) + (c))))
#define RGB888_PIXEL(img,c,r) (*(((rgb888_pixel_t *)(img->data)) + ((r) * (img->cols) + (c))))
#define RGB565_PIXEL(img,c,r) (*(((rgb565_pixel_t *)(img->data)) + ((r) * (img->cols) + (c))))

// Image type
typedef struct
{
    int32_t     cols;
    int32_t     rows;
    eImageView  view;
    eImageType  type;
    uint8_t    *data;
    
}image_t;

// Brightness
typedef enum
{
    BRIGHT = 0,
    DARK
    
}eBrightness;

// Connectivity
typedef enum
{
    FOUR  = 4,
    EIGHT = 8
    
}eConnected;

// Filter operations
typedef enum
{
    AVERAGE = 0,
    HARMONIC,
    MAX,
    MEDIAN,
    MIDPOINT,
    MIN,
    RANGE
    
}eFilterOperation;

// BLOB info structure
typedef struct blobinfo_t
{
    uint16_t height;
    uint16_t width;
    uint16_t nof_pixels;
    float    perimeter;
    
}blobinfo_t;

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------

// unique operator: watershed transformation
uint32_t waterShed(const image_t *src,
                         image_t *dst,
                         const eConnected connected,
                         basic_pixel_t minh,
                         basic_pixel_t maxh);

// ----------------------------------------------------------------------------
// Memory (de)allocation
// ----------------------------------------------------------------------------

// These functions can be used for creating new images
// Memory is allocated within these functions
//
// Precondition : -
// Postcondition: User must free allocated memory by calling the deleteImage()
//                function when appropriate
image_t *newBasicImage( const uint32_t cols, const uint32_t rows );
image_t *newInt16Image( const uint32_t cols, const uint32_t rows );
image_t *newFloatImage( const uint32_t cols, const uint32_t rows );
image_t *newRGB888Image( const uint32_t cols, const uint32_t rows );
image_t *newRGB565Image( const uint32_t cols, const uint32_t rows );

// These functions can be used for copying images
// Memory is allocated within these functions
//
// Precondition : -
// Postcondition: User must free allocated memory by calling the deleteImage()
//                function when appropriate
image_t *toBasicImage( image_t *src );
image_t *toInt16Image( image_t *src );
image_t *toFloatImage( image_t *src );
image_t *toRGB888Image( image_t *src );
image_t *toRGB565Image( image_t *src );

// Use the function deleteImage() for freeing memory
// This function will automatically call the appropriate function based on the
// image type
//
// Precondition : -
// Postcondition: -
void deleteImage( image_t *img );
void deleteBasicImage( image_t *img );
void deleteInt16Image( image_t *img );
void deleteFloatImage( image_t *img );
void deleteRGB888Image( image_t *img );
void deleteRGB565Image( image_t *img );

// ----------------------------------------------------------------------------
// Contrast stretching
// ----------------------------------------------------------------------------

// The contrast of the src image is stretch to 0-255 in the dst image
// This function is required by the Qt PC application. It is used to display
// the source- and destination image. If this function is not implemented, the
// user will see random data.
//
// Precondition : img is a single channel image
// Postcondition: dst is a single channel image
void contrastStretch( const image_t *src
                    ,       image_t *dst
                    , const int32_t bottom
                    , const int32_t top
                    );
// This function does the same as contrastStretch. However, it always
// stretches from 0 to 255 and implements a fast algorithm.
//
// Precondition : img is a single channel image
// Postcondition: dst is a single channel image
void contrastStretchFast( const image_t *src, image_t *dst );

// This function stretches the contrast for the R, G, and B channels
// separately
//
// Precondition : img is a RGB888 image
// Postcondition: dst is a RGB888 image
void contrastStretchRGB888( const image_t *src
                          ,       image_t *dst
                          , const rgb888_pixel_t bottom
                          , const rgb888_pixel_t top
                          );

// This function stretches the contrast for the R, G, and B channels
// separately
//
// Precondition : img is a RGB565 image
// Postcondition: dst is a RGB565 image
void contrastStretchRGB565( const image_t *src
                          ,       image_t *dst
                          , const rgb565_pixel_t bottom
                          , const rgb565_pixel_t top
                          );

// ----------------------------------------------------------------------------
// Rotation
// ----------------------------------------------------------------------------

// Rotates an image 180 degrees
//
// Precondition : -
// Postcondition: -
void rotate180( const image_t *img );

// ----------------------------------------------------------------------------
// Thresholding
// ----------------------------------------------------------------------------

// All pixels in src image with value between 'low' and 'high' are
// set to 1 in dst image. Rest is set to 0
// This function is used in all VisionSets. Without it, initially nothing will
// seem to happen.
//
// Precondition : img is a single channel image
// Postcondition: dst is a binary image
void threshold( const image_t *src
              ,       image_t *dst
              , const int32_t low
              , const int32_t high
              );

// Threshold values are automatically generated based on the 2-means method
//
// Precondition : img is a single channel image
// Postcondition: dst is a binary image
void threshold2Means( const image_t *src
                    ,       image_t *dst
                    , const eBrightness brightness
                    );

// Threshold values are automatically generated based on Otsu's method
//
// Precondition : img is a single channel image
// Postcondition: dst is a binary image
void thresholdOtsu( const image_t *src
                  ,       image_t *dst
                  , const eBrightness brightness
                  );

// ----------------------------------------------------------------------------
// Miscellaneous
// ----------------------------------------------------------------------------

// All pixels are set to the value 0
//
// Precondition : -
// Postcondition: -
void erase( const image_t *img );

// Source image is copied to dst image
//
// Precondition : -
// Postcondition: -
void copy( const image_t *src, image_t *dst );

// First, src image is copied to dst image
// Then, all pixels in dst image with value 'selected' are set to 'value'
//
// Precondition : img is a single channel image
// Postcondition: dst is a single channel image
void setSelectedToValue( const image_t *src
                       ,       image_t *dst
                       , const int32_t selected
                       , const int32_t value
                       );

// This function checks the number of pixels around pixel (x,y) and returns the
// number of pixels that are equal to value
//
// Precondition : img is a single channel image
// Postcondition: -
uint32_t neighbourCount( const image_t *img
                       , const int32_t c
                       , const int32_t r
                       , const int32_t pixel
                       , const eConnected connected
                       );

// Make a histogram of the source image and calculate the sum of all pixel
// values
//
// Precondition : img is a single channel image
// Postcondition: -
void histogram( const image_t *img, uint16_t *hist );

// ----------------------------------------------------------------------------
// Arithmetic
// ----------------------------------------------------------------------------

// Pixels are added
//
// Precondition : -
// Postcondition: -
void add( const image_t *src, image_t *dst );

// the sum of all pixels is returned
//
// Precondition : -
// Postcondition: -
uint32_t sum( const image_t *img );

// Pixels are multiplied
//
// Precondition : -
// Postcondition: -
void multiply( const image_t *src, image_t *dst );

// src image is binary inverted
//
// Precondition : img is a binary image
// Postcondition: dst is a binary image
void invert( const image_t *src, image_t *dst);

void gamma( const image_t *src, image_t *dst, const float c, const float g);


// ----------------------------------------------------------------------------
// Filters
// ----------------------------------------------------------------------------

// A set of nonlinear filters
// n equals the mask size
//
// Precondition : img is a single channel image
//                n must be an odd number
// Postcondition: dst is a single channel image
void nonlinearFilter( const image_t *src
                    ,       image_t *dst
                    , const eFilterOperation fo
                    , const uint8_t n
                    );

void gaussianBlur( const image_t *src
                       ,       image_t *dst
                       , const int32_t kernelSize
                       , const double sigma);

void convolution( const image_t *src
                      , image_t *dst
                        , const image_t *kernel);

void morph_erode(const image_t *src, image_t *dst, const image_t *kernel);

void morph_dilate(const image_t *src, image_t *dst, const image_t *kernel);

void morph_open(const image_t *src, image_t *dst, const image_t *kernel);

void morph_close(const image_t *src, image_t *dst, const image_t *kernel);

// ----------------------------------------------------------------------------
// Binary
// ----------------------------------------------------------------------------

// Remove the blobs connected to one or more borders
//
// Precondition : img is a binary image
// Postcondition: dst is a binary image
void removeBorderBlobs( const image_t *src
                      ,       image_t *dst
                      , const eConnected connected
                      );

// Fill holes
//
// Precondition : img is a binary image
// Postcondition: dst is a binary image
void fillHoles( const image_t *src
              ,       image_t *dst
              , const eConnected connected
              );

// Label all blobs, returns the number of labeled blobs
//
// Precondition : img is a binary image
// Postcondition: dst is a labeled image
uint32_t labelBlobs( const image_t *src
                   ,       image_t *dst
                   , const eConnected connected
                   );

// Find the edges of binary objects
//
// Precondition : img is a binary image
// Postcondition: dst is a binary image
void binaryEdgeDetect( const image_t *src
                     ,       image_t *dst
                     , const eConnected connected
                     );

// ----------------------------------------------------------------------------
// Analysis
// ----------------------------------------------------------------------------

// Analyse blobs
//
// Precondition : img is a labeled image
//                pBlobInfo points to a blobinfo_t struct declared by the
//                calling program
// Postcondition: -
void blobAnalyse( const image_t *img
                , const uint8_t blobnr
                ,       blobinfo_t *blobInfo);

// Calculates the centroid of a blob
//
// Precondition : img is a labeled image
// Postcondition: -
void centroid( const image_t *img
             , const uint8_t blobnr
             ,       int32_t *cc
             ,       int32_t *rc
             );

// Calculates the normalized central moments of a blob specified by p and q
// Ref.: Gonzalez, R. (). 11.3.4 Moment Invariants.
//       In Digital Image Processing. pp. 839-842.
//       New Jersey: Pearson Prentice Hall.
//
// Precondition : img is a binary or labeled image
//                blobnr must be '1' if img is binary
// Postcondition: -
float normalizedCentralMoments( const image_t *img
                              , const uint8_t blobnr
                              , const int32_t p
                              , const int32_t q
                              );

#endif // _OPERATORS_H_
// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

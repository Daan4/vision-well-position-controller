/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Header file for basic image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#ifndef _OPERATORS_BASIC_H_
#define _OPERATORS_BASIC_H_

#include "stdint.h"
#include "operators.h"

// unique operator: watershed transformation
uint32_t waterShed_basic(const image_t *src,
                         image_t *dst,
                         const eConnected connected,
                         basic_pixel_t minh,
                         basic_pixel_t maxh);

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------

void contrastStretch_basic( const image_t *src
                          ,       image_t *dst
                          , const basic_pixel_t bottom
                          , const basic_pixel_t top
                          );

void contrastStretchFast_basic( const image_t *src
                              ,       image_t *dst
                              );

// ----------------------------------------------------------------------------
// Rotation
// ----------------------------------------------------------------------------

void rotate180_basic( const image_t *img );

// ----------------------------------------------------------------------------
// Thresholding
// ----------------------------------------------------------------------------


void threshold_basic( const image_t *src
                    ,       image_t *dst
                    , const basic_pixel_t low
                    , const basic_pixel_t high
                    );

void threshold2Means_basic( const image_t *src
                          ,       image_t *dst
                          , const eBrightness brightness
                          );

void thresholdOtsu_basic( const image_t *src
                        ,       image_t *dst
                        , const eBrightness brightness
                        );

// ----------------------------------------------------------------------------
// Miscellaneous
// ----------------------------------------------------------------------------

void erase_basic( const image_t *img );

void copy_basic( const image_t *src, image_t *dst );

void setSelectedToValue_basic( const image_t *src
                             ,       image_t *dst
                             , const basic_pixel_t selected
                             , const basic_pixel_t value
                             );

uint32_t neighbourCount_basic( const image_t *img
                             , const int32_t c
                             , const int32_t r
                             , const basic_pixel_t pixel
                             , const eConnected connected
                             );

void histogram_basic( const image_t *img, uint16_t *hist );

// ----------------------------------------------------------------------------
// Arithmetic
// ----------------------------------------------------------------------------

void add_basic( const image_t *src, image_t *dst );

uint32_t sum_basic( const image_t *img );

void multiply_basic( const image_t *src, image_t *dst );

void invert_basic( const image_t *src, image_t *dst);

void gamma_basic( const image_t *src, image_t *dst, const float c, const float g);


// ----------------------------------------------------------------------------
// Filters
// ----------------------------------------------------------------------------

void nonlinearFilter_basic( const image_t *src
                          ,       image_t *dst
                          , const eFilterOperation fo
                          , const uint8_t n
                          );

void gaussianBlur_basic( const image_t *src
                       ,       image_t *dst
                       , const int32_t kernelSize
                       , const double sigma);

void convolution_basic( const image_t *src
                      , const image_t *dst
                        , const image_t *kernel);

// ----------------------------------------------------------------------------
// Morphology
// ----------------------------------------------------------------------------
void erode_basic(const image_t *src, image_t *dst, const image_t *kernel);

void dilate_basic(const image_t *src, image_t *dst, const image_t *kernel);

void open_basic(const image_t *src, image_t *dst, const image_t *kernel);

void close_basic(const image_t *src, image_t *dst, const image_t *kernel);

// ----------------------------------------------------------------------------
// Binary
// ----------------------------------------------------------------------------

void removeBorderBlobs_basic( const image_t *src
                            ,       image_t *dst
                            , const eConnected connected
                            );

void fillHoles_basic( const image_t *src
                    ,       image_t *dst
                    , const eConnected connected
                    );

uint32_t labelBlobs_basic( const image_t *src
                         ,       image_t *dst
                         , const eConnected connected);

void binaryEdgeDetect_basic( const image_t *src
                           ,       image_t *dst
                           , const eConnected connected
                           );

// ----------------------------------------------------------------------------
// Analysis
// ----------------------------------------------------------------------------

void blobAnalyse_basic( const image_t *img
                      , const uint8_t blobnr
                      ,       blobinfo_t *blobInfo);

void centroid_basic( const image_t *img
                   , const uint8_t blobnr
                   ,       int32_t *cc
                   ,       int32_t *rc
                   );

float normalizedCentralMoments_basic( const image_t *img
                                    , const uint8_t blobnr
                                    , const int32_t p
                                    , const int32_t q
                                    );
                            
#endif // _OPERATORS_BASIC_H_
// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

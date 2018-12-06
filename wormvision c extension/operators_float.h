/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Header file for float image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#ifndef _OPERATORS_FLOAT_H_
#define _OPERATORS_FLOAT_H_

#include "stdint.h"
#include "operators.h"

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------

void contrastStretch_float( const image_t *src
                          ,       image_t *dst
                          , const float_pixel_t bottom
                          , const float_pixel_t top
                          );

void threshold_float( const image_t *src
                    ,       image_t *dst
                    , const float_pixel_t low
                    , const float_pixel_t high
                    );

void erase_float( const image_t *img );

void copy_float( const image_t *src, image_t *dst );

void setSelectedToValue_float( const image_t *src
                             ,       image_t *dst
                             , const float_pixel_t selected
                             , const float_pixel_t value
                             );

#endif // _OPERATORS_FLOAT_H_
// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

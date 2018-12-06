/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Header file for int16 image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#ifndef _OPERATORS_INT16_H_
#define _OPERATORS_INT16_H_

#include "stdint.h"
#include "operators.h"

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------

void contrastStretch_int16( const image_t *src
                          ,       image_t *dst
                          , const int16_pixel_t bottom
                          , const int16_pixel_t top
                          );

void threshold_int16( const image_t *src
                    ,       image_t *dst
                    , const int16_pixel_t low
                    , const int16_pixel_t high
                    );

void erase_int16( const image_t *img );

void copy_int16( const image_t *src, image_t *dst );

void setSelectedToValue_int16( const image_t *src
                             ,       image_t *dst
                             , const int16_pixel_t selected
                             , const int16_pixel_t value
                             );

#endif // _OPERATORS_INT_H_
// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

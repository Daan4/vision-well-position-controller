/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Header file for RGB888 image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#ifndef _OPERATORS_RGB888_H_
#define _OPERATORS_RGB888_H_

#include "stdint.h"
#include "operators.h"

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------

void contrastStretch_rgb888( const image_t *src
                           ,       image_t *dst
                           , const rgb888_pixel_t bottom
                           , const rgb888_pixel_t top
                           );

void threshold_rgb888( const image_t *src
                     ,       image_t *dst
                     , const rgb888_pixel_t low
                     , const rgb888_pixel_t high
                     );

void erase_rgb888( const image_t *img );

void copy_rgb888( const image_t *src, image_t *dst );

#endif // _OPERATORS_RGB888_H_
// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

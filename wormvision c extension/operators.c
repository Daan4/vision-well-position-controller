/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Implementation file for image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#include <stdio.h>

#include "operators.h"
#include "operators_basic.h"
#include "operators_int16.h"
#include "operators_float.h"
#include "operators_rgb888.h"
#include "operators_rgb565.h"
#include "math.h"

// ----------------------------------------------------------------------------
// Function implementations
// ----------------------------------------------------------------------------

void deleteImage(image_t *img)
{
    switch(img->type)
    {
    case IMGTYPE_BASIC:
        deleteBasicImage(img);
    break;
    case IMGTYPE_INT16:
        deleteInt16Image(img);
    break;
    case IMGTYPE_FLOAT:
        deleteFloatImage(img);
    break;
    case IMGTYPE_RGB888:
        deleteRGB888Image(img);
    break;
    case IMGTYPE_RGB565:
        deleteRGB565Image(img);
    break;
    default:
        fprintf(stderr, "deleteImage(): image type %d not supported\n", img->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// Contrast stretching
// ----------------------------------------------------------------------------

void contrastStretch( const image_t *src
                    ,       image_t *dst
                    , const int32_t bottom
                    , const int32_t top)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "contrastStretch(): src and dst are of different type\n");
    }

    switch(src->type)
    {
    case IMGTYPE_BASIC:
        contrastStretch_basic(src, dst, (basic_pixel_t)bottom, (basic_pixel_t)top);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "contrastStretch(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "contrastStretch(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void contrastStretchFast( const image_t *src, image_t *dst )
{
    switch(src->type)
    {
    case IMGTYPE_BASIC:
        contrastStretchFast_basic(src, dst);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "contrastStretchFast(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "contrastStretchFast(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void contrastStretchRGB888( const image_t *src
                          ,       image_t *dst
                          , const rgb888_pixel_t bottom
                          , const rgb888_pixel_t top)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "contrastStretchRGB888(): src and dst are of different type\n");
    }

    switch(src->type)
    {
    case IMGTYPE_RGB888:
        contrastStretch_rgb888(src, dst, bottom, top);
    break;
    default:
        fprintf(stderr, "contrastStretchRGB888(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void contrastStretchRGB565( const image_t *src
                          ,       image_t *dst
                          , const rgb565_pixel_t bottom
                          , const rgb565_pixel_t top)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "contrastStretchRGB888(): src and dst are of different type\n");
    }

    switch(src->type)
    {
    case IMGTYPE_RGB565:
        contrastStretch_rgb565(src, dst, bottom, top);
    break;
    default:
        fprintf(stderr, "contrastStretchRGB565(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// Rotation
// ----------------------------------------------------------------------------

void rotate180(const image_t *img)
{
    // Build function call table
    // (make sure order matches the order in eImageType)
    void (*fp[])(const image_t *) =
    {
        rotate180_basic,
        //rotate180_int16,
        //rotate180_float,
        //rotate180_rgb888
        //rotate180_rgb565
    };

    // Call the function
    fp[img->type](img);
}

// ----------------------------------------------------------------------------
// Thresholding
// ----------------------------------------------------------------------------

void threshold( const image_t *src
              ,       image_t *dst
              , const int32_t low
              , const int32_t high)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "threshold(): src and dst are of different type\n");
    }

    switch(src->type)
    {
    case IMGTYPE_BASIC:
        if(low < 0)
        {
            fprintf(stderr, "threshold(): low < 0 is invalid for IMGTYPE_BASIC\n");
        }

        if(high > 255)
        {
            fprintf(stderr, "threshold(): high > 255 is invalid for IMGTYPE_BASIC\n");
        }

        threshold_basic(src, dst, (basic_pixel_t)low, (basic_pixel_t)high);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "threshold(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    {
        rgb888_pixel_t rgb888_low;
        rgb888_low.r = rgb888_low.g = rgb888_low.b = (unsigned char)low;
        rgb888_pixel_t rgb888_high;
        rgb888_high.r = rgb888_high.g = rgb888_high.b = (unsigned char)high;

        threshold_rgb888(src, dst, rgb888_low, rgb888_high);
    }
    break;
    case IMGTYPE_RGB565:
    {
        rgb565_pixel_t rgb565_low = 0x0000;
        rgb565_pixel_t rgb565_high = 0xFFFF;
        threshold_rgb565(src, dst, rgb565_low, rgb565_high);
    }
    break;
    default:
        fprintf(stderr, "threshold(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void threshold2Means( const image_t *src
                    ,       image_t *dst
                    , const eBrightness brightness)
{
    switch(src->type)
    {
    case IMGTYPE_BASIC:
        threshold2Means_basic(src, dst, brightness);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "threshold2Means(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "threshold2Means(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void thresholdOtsu( const image_t *src
                  ,       image_t *dst
                  , const eBrightness brightness)
{
    switch(src->type)
    {
    case IMGTYPE_BASIC:
        thresholdOtsu_basic(src, dst, brightness);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "thresholdOtsu(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "thresholdOtsu(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// Miscellaneous
// ----------------------------------------------------------------------------

void erase(const image_t *img)
{
    // Build function call table
    // (make sure order matches the order in eImageType)
    void (*fp[])(const image_t *) =
    {
        erase_basic,
        erase_int16,
        erase_float,
        erase_rgb888,
        erase_rgb565
    };

    // Call the function
    fp[img->type](img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void copy(const image_t *src, image_t *dst)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "copy(): src and dst are of different type\n");
    }

    // Build function call table
    // (make sure order matches the order in eImageType)
    void (*fp[])(const image_t *, image_t *) =
    {
        copy_basic,
        copy_int16,
        copy_float,
        copy_rgb888,
        copy_rgb565
    };

    // Call the function
    fp[src->type](src, dst);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void setSelectedToValue( const image_t *src
                       ,       image_t *dst
                       , const int32_t selected
                       , const int32_t value)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "setSelectedToValue(): src and dst are of different type\n");
    }
    switch(src->type)
    {
    case IMGTYPE_BASIC:    
        setSelectedToValue_basic(src, dst, (basic_pixel_t)selected, (basic_pixel_t)value);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "setSelectedToValue(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "setSelectedToValue(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint32_t neighbourCount( const image_t *img
                       , const int32_t c
                       , const int32_t r
                       , const int32_t pixel
                       , const eConnected connected)
{
    switch(img->type)
    {
    case IMGTYPE_BASIC:
        return neighbourCount_basic(img, c, r, (basic_pixel_t)pixel, connected);
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "neighbourCount(): image type %d not yet implemented\n", img->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "neighbourCount(): image type %d not supported\n", img->type);
    break;
    }

    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void histogram( const image_t *img, uint16_t *hist )
{
    switch(img->type)
    {
    case IMGTYPE_BASIC:
        histogram_basic(img, hist);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "histogram(): image type %d not yet implemented\n", img->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "histogram(): image type %d not supported\n", img->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// Arithmetic
// ----------------------------------------------------------------------------

void add( const image_t *src, image_t *dst )
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "add(): src and dst are of different type\n");
    }

    // Build function call table
    // (make sure order matches the order in eImageType)
    void (*fp[])(const image_t *, image_t *) =
    {
        add_basic,
      //add_int16,
      //add_float,
      //add_rgb888,
      //add_rgb565
    };

    // Call the function
    fp[src->type](src, dst);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint32_t sum( const image_t *img )
{
    // Build function call table
    // (make sure order matches the order in eImageType)
    uint32_t (*fp[])(const image_t *) =
    {
        sum_basic,
      //sum_int16,
      //sum_float,
      //sum_rgb888,
      //sum_rgb565
    };

    // Call the function
    return fp[img->type](img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void multiply( const image_t *src, image_t *dst )
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "multiply(): src and dst are of different type\n");
    }

    // Build function call table
    // (make sure order matches the order in eImageType)
    void (*fp[])(const image_t *, image_t *) =
    {
        multiply_basic,
      //multiply_int16,
      //multiply_float,
      //multiply_rgb888,
      //multiply_rgb565
    };

    // Call the function
    fp[src->type](src, dst);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void invert( const image_t *src, image_t *dst)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "invert(): src and dst are of different type\n");
    }

    // Build function call table
    // (make sure order matches the order in eImageType)
    void (*fp[])(const image_t *, image_t *) =
    {
        invert_basic,
      //invert_int16,
      //invert_float,
      //invert_rgb888,
      //invert_rgb565
    };

    // Call the function
    fp[src->type](src, dst);
}

// ----------------------------------------------------------------------------
// Filters
// ----------------------------------------------------------------------------

void nonlinearFilter( const image_t *src
                    ,       image_t *dst
                    , const eFilterOperation fo
                    , const uint8_t n)
{
    switch(src->type)
    {
    case IMGTYPE_BASIC:
        nonlinearFilter_basic(src, dst, fo, n);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "nonlinearFilter(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "nonlinearFilter(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// Binary
// ----------------------------------------------------------------------------

void removeBorderBlobs( const image_t *src
                      ,       image_t *dst
                      , const eConnected connected)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "removeBorderBlobs(): src and dst are of different type\n");
    }

    switch(src->type)
    {
    case IMGTYPE_BASIC:
        removeBorderBlobs_basic(src, dst, connected);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "removeBorderBlobs(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "removeBorderBlobs(): image type %d not supported\n", src->type);
    break;
    }    
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void fillHoles( const image_t *src
              ,       image_t *dst
              , const eConnected connected)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "fillHoles(): src and dst are of different type\n");
    }

    switch(src->type)
    {
    case IMGTYPE_BASIC:
        fillHoles_basic(src, dst, connected);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "fillHoles(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "fillHoles(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint32_t labelBlobs( const image_t *src
                   ,       image_t *dst
                   , const eConnected connected)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "labelBlobs(): src and dst are of different type\n");
    }

    switch(src->type)
    {
    case IMGTYPE_BASIC:
        return labelBlobs_basic(src, dst, connected);
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "labelBlobs(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "labelBlobs(): image type %d not supported\n", src->type);
    break;
    }

    return 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void binaryEdgeDetect( const image_t *src
                     ,       image_t *dst
                     , const eConnected connected)
{
    if(src->type != dst->type)
    {
        fprintf(stderr, "binaryEdgeDetect(): src and dst are of different type\n");
    }

    switch(src->type)
    {
    case IMGTYPE_BASIC:
        binaryEdgeDetect_basic(src, dst, connected);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "binaryEdgeDetect(): image type %d not yet implemented\n", src->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "binaryEdgeDetect(): image type %d not supported\n", src->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// Analysis
// ----------------------------------------------------------------------------

void blobAnalyse( const image_t *img
                , const uint8_t blobnr
                ,       blobinfo_t *blobInfo)
{
    switch(img->type)
    {
    case IMGTYPE_BASIC:
        blobAnalyse_basic(img, blobnr, blobInfo);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "blobAnalyse(): image type %d not yet implemented\n", img->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "blobAnalyse(): image type %d not supported\n", img->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void centroid( const image_t *img
             , const uint8_t blobnr
             ,       int32_t *cc
             ,       int32_t *rc)
{
    switch(img->type)
    {
    case IMGTYPE_BASIC:
        centroid_basic(img, blobnr, cc, rc);
    break;
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "centroid(): image type %d not yet implemented\n", img->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "centroid(): image type %d not supported\n", img->type);
    break;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
float normalizedCentralMoments( const image_t *img
                              , const uint8_t blobnr
                              , const int32_t p
                              , const int32_t q)
{
    switch(img->type)
    {
    case IMGTYPE_BASIC:
        return normalizedCentralMoments_basic(img, blobnr, p, q);
    case IMGTYPE_INT16:
    case IMGTYPE_FLOAT:
        fprintf(stderr, "normalizedCentralMoments(): image type %d not yet implemented\n", img->type);
    break;
    case IMGTYPE_RGB888:
    case IMGTYPE_RGB565:
    default:
        fprintf(stderr, "normalizedCentralMoments(): image type %d not supported\n", img->type);
    break;
    }

    return 0.0f;
}

// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

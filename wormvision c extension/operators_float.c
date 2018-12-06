/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Implementation file for float image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#include "operators_float.h"
#include "float.h"
#include "math.h"

// ----------------------------------------------------------------------------
// Function implementations
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
image_t *newFloatImage(const uint32_t cols, const uint32_t rows)
{
    image_t *img = (image_t *)malloc(sizeof(image_t));
    if(img == NULL)
    {
        // Unable to allocate memory for new image
        return NULL;
    }

    img->data = (unsigned char *)malloc((rows * cols) * sizeof(float_pixel_t));
    if(img->data == NULL)
    {
        // Unable to allocate memory for data
        free(img);
        return NULL;
    }

    img->cols = cols;
    img->rows = rows;
    img->view = IMGVIEW_CLIP;
    img->type = IMGTYPE_FLOAT;
    return(img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
image_t *toFloatImage(image_t *src)
{
    register long int  i = src->rows * src->cols;

    image_t * dst = newFloatImage(src->cols, src->rows);
    if(dst == NULL)
        return NULL;

    dst->view= src->view;

    register float_pixel_t *d = (float_pixel_t *)dst->data;

    switch(src->type)
    {
    case IMGTYPE_BASIC:
    {
        basic_pixel_t *s = (basic_pixel_t *)src->data;
        // Loop all pixels and copy
        while(i-- > 0)
            *d++ = (float)(*s++);

    }break;
    case IMGTYPE_INT16:
    {
        int16_pixel_t *s = (int16_pixel_t *)src->data;
        // Loop all pixels and copy
        while(i-- > 0)
            *d++ = (float)(*s++);

    }break;
    case IMGTYPE_FLOAT:
    {
        copy_float(src, dst);

    }break;
    case IMGTYPE_RGB888:
    {
        rgb888_pixel_t *s = (rgb888_pixel_t *)src->data;
        // Loop all pixels, convert and copy
        while(i-- > 0)
        {
            unsigned char r = s->r;
            unsigned char g = s->g;
            unsigned char b = s->b;

            *d++ = (float_pixel_t)(0.212671f * r + 0.715160f * g + 0.072169f * b);
            s++;
        }

    }break;
    case IMGTYPE_RGB565:
    {
        // TODO
    }
    break;
    default:
    break;
    }

    return dst;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void deleteFloatImage(image_t *img)
{
    free(img->data);
    free(img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void contrastStretch_float(const image_t *src,
                                 image_t *dst,
                           const float_pixel_t bottom,
                           const float_pixel_t top)
{
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)src;
    (void)dst;
    (void)bottom;
    (void)top;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void erase_float(const image_t *img)
{
    register long int  i = img->rows * img->cols;
    register float_pixel_t *s = (float_pixel_t *)img->data;

    // Loop through the image and set all pixels to the value 0
    while(i-- > 0)
        *s++ = 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void threshold_float(const image_t *src,
                           image_t *dst,
                     const float_pixel_t low,
                     const float_pixel_t high)
{
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)src;
    (void)dst;
    (void)low;
    (void)high;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void copy_float(const image_t *src, image_t *dst)
{
    register long int  i = src->rows * src->cols;
    register float_pixel_t *s = (float_pixel_t *)src->data;
    register float_pixel_t *d = (float_pixel_t *)dst->data;

    dst->rows = src->rows;
    dst->cols = src->cols;
    dst->type = src->type;
    dst->view = src->view;

    // Loop all pixels and copy
    while(i-- > 0)
        *d++ = *s++;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void setSelectedToValue_float(const image_t *src,
                                    image_t *dst,
                              const float_pixel_t selected,
                              const float_pixel_t value)
{
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)src;
    (void)dst;
    (void)selected;
    (void)value;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

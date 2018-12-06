/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Implementation file for RGB888 image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#include "operators_rgb888.h"
#include "math.h"

// ----------------------------------------------------------------------------
// Function implementations
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
image_t *newRGB888Image(const uint32_t cols, const uint32_t rows)
{
    image_t *img = (image_t *)malloc(sizeof(image_t));
    if(img == NULL)
    {
        // Unable to allocate memory for new image
        return NULL;
    }

    img->data = (unsigned char *)malloc((rows * cols) * sizeof(rgb888_pixel_t));
    if(img->data == NULL)
    {
        // Unable to allocate memory for data
        free(img);
        return NULL;
    }

    img->cols = cols;
    img->rows = rows;
    img->view = IMGVIEW_CLIP;
    img->type = IMGTYPE_RGB888;
    return(img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
image_t *toRGB888Image(image_t *src)
{
    register long int  i = src->rows * src->cols;

    image_t * dst = newRGB888Image(src->cols, src->rows);
    if(dst == NULL)
        return NULL;

    dst->view= src->view;

    register rgb888_pixel_t *d = (rgb888_pixel_t *)dst->data;

    switch(src->type)
    {
    case IMGTYPE_BASIC:
    {
        basic_pixel_t *s = (basic_pixel_t *)src->data;
        // Loop all pixels and copy all channels with same value
        while(i-- > 0)
        {
            d->r = *s;
            d->g = *s;
            d->b = *s;
            
            s++;
            d++;
        }

    }break;
    case IMGTYPE_INT16:
    {
        // TODO

    }break;
    case IMGTYPE_FLOAT:
    {
        // TODO

    }break;
    case IMGTYPE_RGB888:
    {
        copy_rgb888(src, dst);

    }break;
    default:
    break;
    }

    return dst;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void deleteRGB888Image(image_t *img)
{
    free(img->data);
    free(img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void contrastStretch_rgb888(const image_t *src,
                                  image_t *dst,
                            const rgb888_pixel_t bottom,
                            const rgb888_pixel_t top)
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
void erase_rgb888(const image_t *img)
{
    register long int i = img->rows * img->cols;
    register rgb888_pixel_t *s = (rgb888_pixel_t *)img->data;

    // Loop through the image and set all pixels to the value 0
    while(i-- > 0)
    {
        s->r = 0;
        s->g = 0;
        s->b = 0;
        
        s++;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void threshold_rgb888(const image_t *src,
                            image_t *dst,
                      const rgb888_pixel_t low,
                      const rgb888_pixel_t high)
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
void copy_rgb888(const image_t *src, image_t *dst)
{
    register long int i = src->rows * src->cols;
    register rgb888_pixel_t *s = (rgb888_pixel_t *)src->data;
    register rgb888_pixel_t *d = (rgb888_pixel_t *)dst->data;

    dst->rows = src->rows;
    dst->cols = src->cols;
    dst->type = src->type;
    dst->view = src->view;

    // Loop all pixels and copy
    while(i-- > 0)
        *d++ = *s++;
}

// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

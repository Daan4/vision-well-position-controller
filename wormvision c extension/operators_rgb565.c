/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Implementation file for RGB565 image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#include "operators_rgb565.h"
#include "math.h"

// ----------------------------------------------------------------------------
// Function implementations
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
image_t *newRGB565Image(const uint32_t cols, const uint32_t rows)
{
    image_t *img = (image_t *)malloc(sizeof(image_t));
    if(img == NULL)
    {
        // Unable to allocate memory for new image
        return NULL;
    }

    img->data = (unsigned char *)malloc((rows * cols) * sizeof(rgb565_pixel_t));
    if(img->data == NULL)
    {
        // Unable to allocate memory for data
        free(img);
        return NULL;
    }

    img->cols = cols;
    img->rows = rows;
    img->view = IMGVIEW_CLIP;
    img->type = IMGTYPE_RGB565;
    return(img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
image_t *toRGB565Image(image_t *src)
{
    register long int  i = src->rows * src->cols;

    image_t * dst = newRGB565Image(src->cols, src->rows);
    if(dst == NULL)
        return NULL;

    dst->view= src->view;

    register rgb565_pixel_t *d = (rgb565_pixel_t *)dst->data;

    switch(src->type)
    {
    case IMGTYPE_BASIC:
    {
        basic_pixel_t *s = (basic_pixel_t *)src->data;
        // Loop all pixels and copy all channels with same value
        while(i-- > 0)
        {
            *d = 0;

            *d |= ((*s & 0x1F) << 11);
            *d |= ((*s & 0x3F) <<  5);
            *d |=  (*s & 0x1F);

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
        // TODO

    }break;
    case IMGTYPE_RGB565:
    {
        copy_rgb565(src, dst);

    }break;
    default:
    break;
    }

    return dst;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void deleteRGB565Image(image_t *img)
{
    free(img->data);
    free(img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void contrastStretch_rgb565(const image_t *src,
                                  image_t *dst,
                            const rgb565_pixel_t bottom,
                            const rgb565_pixel_t top)
{
    (void)src;
    (void)dst;
    (void)bottom;
    (void)top;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void erase_rgb565(const image_t *img)
{
    register long int i = img->rows * img->cols;
    register rgb565_pixel_t *s = (rgb565_pixel_t *)img->data;

    // Loop through the image and set all pixels to the value 0
    while(i-- > 0)
        *s++ = 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void threshold_rgb565(const image_t *src,
                            image_t *dst,
                      const rgb565_pixel_t low,
                      const rgb565_pixel_t high)
{
    (void)src;
    (void)dst;
    (void)low;
    (void)high;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void copy_rgb565(const image_t *src, image_t *dst)
{
    register long int i = src->rows * src->cols;
    register rgb565_pixel_t *s = (rgb565_pixel_t *)src->data;
    register rgb565_pixel_t *d = (rgb565_pixel_t *)dst->data;

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

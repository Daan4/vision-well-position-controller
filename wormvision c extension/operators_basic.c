/******************************************************************************
 * Project    : Embedded Vision Design
 * Copyright  : 2017 HAN Electrical and Electronic Engineering
 * Author     : Hugo Arends
 *
 * Description: Implementation file for basic image processing operators
 *
 ******************************************************************************
  Change History:

    Version 1.0 - November 2017
    > Initial revision

******************************************************************************/
#ifdef DEBUG_ENABLE
#include <QDebug>
#endif

#include "operators_basic.h"
#include "math.h"
#include "limits.h"

// custom operator: watershed

// ----------------------------------------------------------------------------
// Function implementations
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
image_t *newBasicImage(const uint32_t cols, const uint32_t rows)
{
    image_t *img = (image_t *)malloc(sizeof(image_t));
    if(img == NULL)
    {
        // Unable to allocate memory for new image
        return NULL;
    }

    img->data = (basic_pixel_t *)malloc((rows * cols) * sizeof(basic_pixel_t));
    if(img->data == NULL)
    {
        // Unable to allocate memory for data
        free(img);
        return NULL;
    }

    img->cols = cols;
    img->rows = rows;
    img->view = IMGVIEW_CLIP;
    img->type = IMGTYPE_BASIC;
    return(img);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
image_t *toBasicImage(image_t *src)
{
    register long int  i = src->rows * src->cols;

    image_t *dst = newBasicImage(src->cols, src->rows);
    if(dst == NULL)
        return NULL;

    dst->view = src->view;

    register basic_pixel_t *d = (basic_pixel_t *)dst->data;

    switch(src->type)
    {
    case IMGTYPE_BASIC:
    {
        copy_basic(src, dst);

    }break;
    case IMGTYPE_INT16:
    {
        int16_pixel_t *s = (int16_pixel_t *)src->data;
        // Loop all pixels and copy
        while(i-- > 0)
          *d++ = (basic_pixel_t)(*s++);

    }break;
    case IMGTYPE_FLOAT:
    {
        float_pixel_t *s = (float_pixel_t *)src->data;
        // Loop all pixels and copy
        while(i-- > 0)
          *d++ = (basic_pixel_t)(*s++);

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

            *d++ = (basic_pixel_t)(0.212671f * r + 0.715160f * g + 0.072169f * b);
            s++;
        }
        
    }break;
    case IMGTYPE_RGB565:
    {
        rgb565_pixel_t *s = (rgb565_pixel_t *)src->data;
        // Loop all pixels, convert and copy
        while(i-- > 0)
        {
            unsigned char r = *s >> 11;
            unsigned char g = (*s >>  5) & (rgb565_pixel_t)0x003F;
            unsigned char b = (*s)       & (rgb565_pixel_t)0x001F;

            *d++ = (basic_pixel_t)(0.212671f * r + 0.715160f * g + 0.072169f * b);
            s++;
        }

    }break;
    default:
    break;
    }

    return dst;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void deleteBasicImage(image_t *img)
{
    free(img->data);
    free(img);
}

// ----------------------------------------------------------------------------
// Function prototypes
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void contrastStretch_basic( const image_t *src
                          ,       image_t *dst
                          , const basic_pixel_t bottom
                          , const basic_pixel_t top)
{
    // Find the min and max pixel values
    register basic_pixel_t min = 255;
    register basic_pixel_t max = 0;
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = src->data;
    while(i-- > 0) {
        if(*s < min) {
            min = *s;
        }
        if(*s > max) {
            max = *s;
        }
        s++;
    }
    // Prevent zero division
    if(min == max) {
        max += 1;
    }
    // Calculate stretch factor
    register float stretch_factor = (float) (top - bottom) / (max - min);
    // Calculate new pixel values in a LUT
    basic_pixel_t LUT[256];
    i = 256;
    while(i-- > 0) {
        LUT[i] = (basic_pixel_t) ((i - min) * stretch_factor + (float) 0.5);
    }
    // Assign new pixel values in destination image
    i = src->rows * src->cols;
    s = dst->data;
    while(i-- > 0) {
        *s = LUT[*s];
        s++;
    }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time: 3.2ms
void contrastStretchFast_basic(const image_t *src, image_t *dst)
{
    // Find the min and max pixel values
    register basic_pixel_t min = 255;
    register basic_pixel_t max = 0;
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = src->data;
    while(i-- > 0) {
        if(*s < min) {
            min = *s;
        }
        if(*s > max) {
            max = *s;
        }
        s++;
    }
    // Prevent zero division
    if(min == max) {
        max += 1;
    }
    // Calculate stretch factor
    register float stretch_factor = 255.0f / (max - min);

    // Calculate new pixel values in a LUT
    basic_pixel_t LUT[256];
    i = 256;
    while(i-- > 0) {
        LUT[i] = (uint8_t) ((i - min) * stretch_factor + 0.5f);
    }
    // Assign new pixel values in destination image
    i = src->rows * src->cols;
    s = src->data;
    register basic_pixel_t *d = dst->data;
    while(i-- > 0) {
        *d++ = LUT[*s++];
    }
}

// ----------------------------------------------------------------------------
// Rotation
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time: 0.529ms
void rotate180_basic(const image_t *img)
{
    register basic_pixel_t temp;
    register basic_pixel_t *l = img->data;
    register basic_pixel_t *h = img->data + img->rows * img->cols - 1;
    register uint32_t i = img->rows * img-> cols / 2;
    while(i-- > 0) {
        temp = *l;
        *l++ = *h;
        *h-- = temp;
    }

}

// ----------------------------------------------------------------------------
// Thresholding
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time: 1.779ms
void threshold_basic( const image_t *src
                    ,       image_t *dst
                    , const basic_pixel_t low
                    , const basic_pixel_t high)
{
    dst->view = IMGVIEW_BINARY;
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = src->data;
    register basic_pixel_t *d = dst->data;
    while(i-- > 0) {
        if(*s >= low && *s <= high) {
            *d++ = 1;
        } else {
            *d++ = 0;
        }
        s++;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time: 6-12ms depending on iterations
void threshold2Means_basic( const image_t *src
                          ,       image_t *dst
                          , const eBrightness brightness)
{
    // Select initial mean position halfway between lowest and highest pixel
    register basic_pixel_t min = 255;
    register basic_pixel_t max = 0;
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = src->data;
    while(i-- > 0) {
        if(*s < min) {
            min = *s;
        }
        if(*s > max) {
            max = *s;
        }
        s++;
    }
    register basic_pixel_t T = (max - min) / 2;

    // Find threshold value via iterative 2 means algorithm.
    register basic_pixel_t new_T;
    i = 256;
    register uint32_t left = 0;
    register uint32_t left_count = 0;
    register uint32_t right = 0;
    register uint32_t right_count = 0;
    uint16_t hist[256];
    while(1) {
        // Get histogram
        histogram_basic(src, hist);
        // Calculate the 2 means
        while(i-- > 0) {
            if(i > T) {
                right += *(hist + i) * i;
                right_count += *(hist + i);
            } else if(i < T) {
                left += *(hist + i) * i;
                left_count += *(hist + i);
            }
        }
        right /= right_count;
        left /= left_count;
        // Calculate new T and continue the next loop or finish
        new_T = (right + left) / 2;
        if(new_T != T) {
            T = new_T;
            left = 0;
            right = 0;
            left_count = 0;
            right_count = 0;
            i = 256;
        } else {
            break;
        }
    }
    // Threshold on T, taking the low or high values based on the brightness setting.
    // 0 = bright, 1 = dark
    dst->view = IMGVIEW_BINARY;
    i = src->rows * src->cols;
    s = src->data;
    register basic_pixel_t *d = dst->data;
    while(i-- > 0) {
        if(*s >= T) {
            *d++ = 1 - brightness;
        } else {
            *d++ = brightness;
        }
        s++;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time 3.3ms
void thresholdOtsu_basic( const image_t *src
                        ,       image_t *dst
                        , const eBrightness brightness)
{
    // Create histogram
    uint16_t hist[256];
    histogram_basic(src, hist);
    // Calculate number of pixels and sum of pixel values.
    register uint32_t N = src->rows * src->cols;
    register uint32_t sum = 0;
    register uint32_t i = 256;
    while(i-- > 0) {
        sum += *(hist + i) * i;
    }
    // Calculate BCV for each possible threshold
    register float max_BCV = 0;
    register uint32_t best_threshold = 0;
    register uint32_t N_object = 0;
    register uint32_t sum_object = 0;
    register float mean_object;
    register uint32_t N_back;
    register uint32_t sum_back;
    register float mean_back;
    register float BCV;
    i = 256;
    register uint16_t *h = hist;
    while(i-- > 0) {
        N_object += *h;
        sum_object += *h++ * (255 - i);
        if (N_object == 0) {
            mean_object = 0;
        } else {
            mean_object = (float) sum_object / N_object;
        }
        N_back = N - N_object;
        sum_back = sum - sum_object;
        if (N_back == 0) {
            mean_back = 0;
        } else {
            mean_back = (float) sum_back / N_back;
        }
        BCV = N_back * N_object * ((mean_back - mean_object) * (mean_back - mean_object));
        if(BCV > max_BCV) {
            max_BCV = BCV;
            best_threshold = 255 - i;
        }
    }
    // Threshold on best_threshold, taking the low or high values based on the brightness setting.
    // 0 = bright, 1 = dark
    dst->view = IMGVIEW_BINARY;
    i = src->rows * src->cols;
    register basic_pixel_t *s = src->data;
    register basic_pixel_t *d = dst->data;
    while(i-- > 0) {
        if(*s >= best_threshold) {
            *d++ = 1 - brightness;
        } else {
            *d++ = brightness;
        }
        s++;
    }
}

// ----------------------------------------------------------------------------
// Miscellaneous
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void erase_basic(const image_t *img)
{

// #warning TODO: erase_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)img;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void copy_basic(const image_t *src, image_t *dst)
{
    register long int i = src->rows * src->cols;
    register basic_pixel_t *s = (basic_pixel_t *)src->data;
    register basic_pixel_t *d = (basic_pixel_t *)dst->data;

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
void setSelectedToValue_basic(const image_t *src,
                                    image_t *dst,
                              const basic_pixel_t selected,
                              const basic_pixel_t value)
{
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = src->data;
    register basic_pixel_t *d = dst->data;
    while(i-- > 0) {
        if(*s++ == selected) {
            *d++ = value;
        } else {
            *d++ = *(s-1);
        }
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// c = pixel column, r = pixel row, pixel = neighbour pixel value to count
uint32_t neighbourCount_basic(const image_t *img,
                              const int32_t c,
                              const int32_t r,
                              const basic_pixel_t pixel,
                              const eConnected connected)
{
    register uint32_t count = 0;
    // Count 4 neighbors
    if(r - 1 >= 0 && BASIC_PIXEL(img, c, r-1) == pixel) {
        count++;
    }
    if(c - 1 >= 0 && BASIC_PIXEL(img, c-1, r) == pixel) {
        count++;
    }
    if(r + 1 < img->rows && BASIC_PIXEL(img, c, r+1) == pixel) {
        count++;
    }
    if(c + 1 < img->cols && BASIC_PIXEL(img, c+1, r) == pixel) {
        count++;
    }
    // Also count 8 neighbors if this was chosen
    if(connected == EIGHT) {
        if(r - 1 >= 0 && c - 1 >= 0 && BASIC_PIXEL(img, c-1, r-1) == pixel) {
            count++;
        }
        if(r - 1 >= 0 && c + 1 < img->cols && BASIC_PIXEL(img, c+1, r-1) == pixel) {
            count++;
        }
        if(r + 1 < img->rows && c - 1 >= 0 && BASIC_PIXEL(img, c-1, r+1) == pixel) {
            count++;
        }
        if(r + 1 < img->rows && c + 1 < img->cols && BASIC_PIXEL(img, c+1, r+1) == pixel) {
            count++;
        }
    }
    return count;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void histogram_basic( const image_t *img, uint16_t *hist )
{
    // Initialize hist array to all zeroes
    register uint32_t i = 256;
    while(i-- > 0) {
        *(hist + i) = 0;
    }
    // Calculate histogram
    i = img->cols * img->rows;
    register basic_pixel_t *s = img->data;
    while(i-- > 0) {
        *(hist + *s++) += 1;
    }
}

// ----------------------------------------------------------------------------
// Arithmetic
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void add_basic( const image_t *src, image_t *dst )
{

// #warning TODO: add_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)src;
    (void)dst;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint32_t sum_basic( const image_t *img )
{

// #warning TODO: sum_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)img;

    return 0;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void multiply_basic( const image_t *src, image_t *dst )
{

// #warning TODO: multiply_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)src;
    (void)dst;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void invert_basic( const image_t *src, image_t *dst )
{

// #warning TODO: invert_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)src;
    (void)dst;

    return;
// ********************************************

}

void gamma_basic( const image_t *src, image_t *dst, const int8_t c, const int8_t g)
{

}


// ----------------------------------------------------------------------------
// Filters
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmarks
// avg: 188ms
// harmonic: 250ms
// max: 177ms
// min: 183ms
// midpoint: 200ms
// median: 480ms
// range: 200ms
void nonlinearFilter_basic( const image_t *src
                          ,       image_t *dst
                          , const eFilterOperation fo
                          , const uint8_t n)
{
    // src =/= data, n = odd
    register uint32_t i = src->cols * src->rows;
    register uint32_t w_counter;
    register int32_t w_row;
    register int32_t w_col;
    register int32_t row = 0;
    register int32_t col = 0;
    register basic_pixel_t *w;
    register basic_pixel_t *s = src->data;
    register basic_pixel_t *d = dst->data;
    // x, y, z and arr are variables used for calculations depending on the selected operation.
    register int32_t x;
    register int32_t y;
    register float z;
    register int32_t j;// used as a counter to loop through arr
    int32_t *arr = (int32_t *) malloc(n * n * sizeof(int32_t));
    // loop through pixels
    while(i-- > 0) {
        w_counter = n * n;
        w_row = -n / 2;
        w_col = -n / 2;
        x = 0;
        y = 255;
        z = 0;
        // loop through all window pixels for each pixel, ignoring pixels outside of the image
        while(w_counter-- > 0) {
            if(col + w_col < 0 || col + w_col >= src->cols
                    || row + w_row < 0 || row + w_row >= src->rows) {
                // skip pixels outside the image border.
                w_col++;
                if(w_col > n / 2) {
                    w_col = -n / 2;
                    w_row++;
                }
                continue;
            }
            w = s + w_row * src->cols + w_col++;
            if(w_col > n / 2) {
                w_col = -n / 2;
                w_row++;
            }
            switch(fo) {
            case AVERAGE:
                // sum windows values in x
                x += *w;
                break;
            case HARMONIC:
                // sum inverse of window values in x
                  if(*w == 0) {
                     z = 0.0f;
                  } else {
                     z += (float) 1 / *w;
                  }
                break;
            case MAX:
                // store max window value in x
                if(*w > x) {
                    x = *w;
                }
                break;
            case MIN:
                // store min window value in x
                if(*w < y) {
                    y = *w;
                }
                break;
            case MIDPOINT:
                // store max window value in x and min window value in y
                if(*w > x) {
                    x = *w;
                }
                if(*w < y) {
                    y = *w;
                }
                break;
            case MEDIAN:
                // store window values in arr (sorted small to large)
                // store current array length in x
                for(j=x-1; (j >= 0 && arr[j] > *w); j--) {
                    arr[j+1] = arr[j];
                }
                arr[j+1] = *w;
                x++;
                break;
            case RANGE:
                // store max value in x and min value in y
                if(*w > x) {
                    x = *w;
                }
                if(*w < y) {
                    y = *w;
                }
                break;
            }
        }
        // Set destination pixel
        switch(fo) {
        case AVERAGE:
            *d = x / (n * n);
            break;
        case HARMONIC:
              if(z == 0) {
                 *d = 0;
              } else {
                 *d = (basic_pixel_t) n * n / z;
              }
            break;
        case MAX:
            *d = x;
            break;
        case MIN:
            *d = y;
            break;
        case MIDPOINT:
            *d = (x + y) / 2;
            break;
        case MEDIAN:
            if(x % 2 == 1) {
                // if x = odd -> find middle number
                *d = arr[x / 2];
            } else {
                // if x = even -> find mean of middle 2 numbers (this can happen for pixels along the edges)
                *d = (arr[x / 2] + arr[(x - 1) / 2]) / 2;
            }
            break;
        case RANGE:
            j = x - y;
            if(j < 0) {
               j = 0;
            }
            *d = j;
            break;
        }
        // increment pointers and row/column trackers
        s++;
        d++;
        col++;
        if(col >= src->cols) {
            col = 0;
            row++;
        }
    }
}

void gaussianBlur_basic( const image_t *src
                       ,       image_t *dst
                       , const uint8_t kernelSize
                       , const uint8_t sigma) {

}


// ----------------------------------------------------------------------------
// Binary
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void removeBorderBlobs_basic( const image_t *src
                            ,       image_t *dst
                            , const eConnected connected)
{
    // set border pixels with value 1 to value 2
    register uint32_t i = src->cols;
    register basic_pixel_t *s = src->data;
    register basic_pixel_t *d = dst->data;
    // top row
    while(i-- > 0) {
        if(*s++ == 1) {
            *d = 2;
        }
        d++;
    }
    // left column (except corners)
    i = src->rows - 2;
    s = src->data + src->cols;
    d = dst->data + src->cols;
    while(i-- > 0) {
        if(*s == 1) {
            *d = 2;
        }
        s += src->cols;
        d += src->cols;
    }
    // right column (except corners)
    i = src->rows - 2;
    s = src->data + src->cols * 2 - 1;
    d = dst->data + src->cols * 2 - 1;
    while(i-- > 0) {
        if(*s == 1) {
            *d = 2;
        }
        s += src->cols;
        d += src->cols;
    }
    // bottom row
    i = src->cols;
    s = src->data + src->cols * (src->rows - 1);
    d = dst->data + src->cols * (src->rows - 1);
    while(i-- > 0) {
        if(*s++ == 1) {
            *d = 2;
        }
        d++;
    }
    register uint32_t changes = 1; // Flag set to 1 if the image was changed in this iteration
    register uint32_t row;
    register uint32_t col;
    while(changes--) {
        // Flag pixels LT -> RB
        row = 1;
        col = 1;
        s = src->data + src->cols + 1;
        d = dst->data + src->cols + 1;
        i = (src->cols - 2) * (src->rows - 2);
        while(i-- > 0) {
            if(*s == 1 && neighbourCount_basic(dst, col, row, 2, connected) > 0) {
                *d = 2;
                changes = 1;
            }
            s++;
            d++;
            col++;
            if(col == (uint32_t) src->cols - 1) {
                row++;
                col = 1;
                d += 2;
                s += 2;
            }
        }
        // Flag pixels RB -> LT
        row = src->rows - 2;
        col = src->cols - 2;
        d = dst->data + src->cols * (src->rows - 1) - 2;
        s = src->data + src->cols * (src->rows - 1) - 2;
        i = (src->cols - 2) * (src->rows - 2);
        while(i-- > 0) {
            if(*s == 1 && neighbourCount_basic(dst, col, row, 2, connected) > 0) {
                *d = 2;
                changes = 1;
            }
            s--;
            d--;
            col--;
            if(col == 0) {
                row--;
                col = src->cols - 2;
                d -= 2;
                s -= 2;
            }
        }
    }
    // Set all flagged pixels to value 0
    setSelectedToValue_basic(dst, dst, 2, 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void fillHoles_basic( const image_t *src
                    ,       image_t *dst
                    , const eConnected connected)
{
    // set border pixels with value 1 that have no non-border neighbors to value 2
    register uint32_t i = src->cols;
    register basic_pixel_t *d = dst->data;
    register basic_pixel_t *s = src->data;
    register uint32_t border_marked = 0;
    // first set all border pixels with value 1 to value 2
    // top row
    while(i-- > 0) {
        if(*s++ == 0) {
            *d = 2;
            border_marked = 1;
        }
        d++;
    }
    // left column (except corners)
    i = src->rows - 2;
    s = src->data + src->cols;
    d = dst->data + dst->cols;
    while(i-- > 0) {
        if(*s == 0) {
            *d = 2;
            border_marked = 1;
        }
        s += src->cols;
        d += src->cols;
    }
    // right column (except corners)
    i = src->rows - 2;
    s = src->data + src->cols * 2 - 1;
    d = dst->data + src->cols * 2 - 1;
    while(i-- > 0) {
        if(*s == 0) {
            *d = 2;
            border_marked = 1;
        }
        s += src->cols;
        d += src->cols;
    }
    // bottom row
    i = src->cols;
    s = src->data + src->cols * (src->rows - 1);
    d = dst->data + src->cols * (src->rows - 1);
    while(i-- > 0) {
        if(*s++ == 0) {
            *d = 2;
            border_marked = 1;
        }
        d++;
    }
    if(!border_marked) {
        // All border pixels were set -> simply fill the entire image and return
        setSelectedToValue(dst, dst, 0, 1);
        return;
    }
    // Copy over ones where dst has zeroes (in case src != dst)
    i = src->cols * src->rows;
    s = src->data;
    d = dst->data;
    while(i-- > 0) {
        if(*s++ == 1 && *d == 0) {
            *d = 1;
        }
        d++;
    }
    // Mark background pixels 2 and hole pixels 1
    register uint32_t row;
    register uint32_t col;
    register uint32_t changes = 1;
    while(changes--) {
        row = 1;
        col = 1;
        d = dst->data + src->cols + 1;
        i = (src->cols - 2) * (src->rows - 2);
        // Mark 2s LT->BR
        while(i-- > 0) {
            if(*d == 0 && neighbourCount_basic(dst, col, row, 2, connected) > 0) {
                *d = 2;
                changes = 1;
            }
            d++;
            col++;
            if(col == (uint32_t) src->cols - 1) {
                col = 1;
                row++;
                d += 2;
            }
        }
        // Mark 2s BR->LT
        row = src->rows - 2;
        col = src->cols - 2;
        d = dst->data + src->cols * (src->rows - 1) - 2;
        s = src->data + src->cols * (src->rows - 1) - 2;
        i = (src->cols - 2) * (src->rows - 2);
        while(i-- > 0) {
            if(*d == 0 && neighbourCount_basic(dst, col, row, 2, connected) > 0) {
                *d = 2;
                changes = 1;
            }
            d--;
            col--;
            if(col == 0) {
                row--;
                col = src->cols - 2;
                d -= 2;
            }
        }
    }
    // Set the remaining zeroes to ones
    setSelectedToValue(dst, dst, 0, 1);
    // Set all marked pixels to zeroes
    setSelectedToValue(dst, dst, 2, 0);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
uint32_t labelBlobs_basic( const image_t *src
                         ,       image_t *dst
                         , const eConnected connected)
{
    
// #warning TODO: labelBlobs_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)src;
    (void)dst;
    (void)connected;

    return 0;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void binaryEdgeDetect_basic( const image_t *src
                           ,       image_t *dst
                           , const eConnected connected)
{

// #warning TODO: binaryEdgeDetect_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)src;
    (void)dst;
    (void)connected;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// Analysis
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void blobAnalyse_basic( const image_t *img
                      , const uint8_t blobnr
                      ,       blobinfo_t *blobInfo)
{

// #warning TODO: blobAnalyse_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)img;
    (void)blobnr;
    (void)blobInfo;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void centroid_basic( const image_t *img
                   , const uint8_t blobnr
                   ,       int32_t *cc
                   ,       int32_t *rc)
{

// #warning TODO: centroid_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)img;
    (void)blobnr;
    (void)cc;
    (void)rc;

    return;
// ********************************************

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
float normalizedCentralMoments_basic( const image_t *img
                                    , const uint8_t blobnr
                                    , const int32_t p
                                    , const int32_t q)
{

// #warning TODO: normalizedCentralMoments_basic
// ********************************************
// Added to prevent compiler warnings
// Remove these when implementation starts
    (void)img;
    (void)blobnr;
    (void)p;
    (void)q;

    return 0.0f;
// ********************************************

}

// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

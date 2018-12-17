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
#ifdef QDEBUG_ENABLE
#include <QDebug>
#else
// define M_PI for keil
#define M_PI		3.14159265358979323846
#endif

#include "operators_basic.h"
#include "math.h"
#include "limits.h"

// precondition: dst cannot point to the same data as src
// unique operator: watershed transformation
// src -> source image
// dst -> destination image
// connected -> use FOUR or EIGHT neighbour connections
// minh -> minimum grayscale value to start flooding from
// maxh -> maximum grayscale value to reach with flooding
//
// output: labeled image with catchment basins numbered (1 to 254)
//         watershed lines / background labeled 0
// Returns the number of basins or 0 if zero or more than 254 basins were found.
// todo determine hmin and hmax preconditions
uint32_t waterShed_basic(const image_t *src,
                         image_t *dst,
                         const eConnected connected,
                         basic_pixel_t minh,
                         basic_pixel_t maxh) {
    //https://imagej.net/Classic_Watershed

    // set all pixels in src with minh <= value <= maxh to 255 in dst
    // set all pixels outside that range to 0 in dst
    // also find the max pixel values so that maxh can be adjusted if necessary
    register basic_pixel_t max = 0;
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
    while(i-- > 0) {
        if(*s <= maxh) {
            *d++ = 255;
        } else {
            *d++ = 0;
        }
        if(*s > max) {
            max = *s;
        }
        s++;
    }

    // set view and adjust maxh
    dst->view = IMGVIEW_LABELED;
    if(max < maxh) {
        maxh = max;
    }

    // determine max possible neighbours for each pixel position
    register uint32_t max_neighbours;
    register uint32_t max_neighbours_corner;
    register uint32_t max_neighbours_edge;
    register uint32_t current_max_neighbours;
    if(connected == FOUR) {
        max_neighbours = 4;
        max_neighbours_corner = 2;
        max_neighbours_edge = 3;
    } else {
        max_neighbours = 8;
        max_neighbours_corner = 3;
        max_neighbours_edge = 5;
    }

    register uint32_t changes = 1;
    register basic_pixel_t h = minh;  // current water height
    register uint32_t current_blob = 1;
    register uint32_t k;
    register int32_t row;
    register int32_t col;
    register uint32_t blob_neighbour;
    //    // todo implement RB->LT for all steps
    // Step 1: create initial blobs @ height hmin
    //         if blobs touch merge them together
    while(changes--) {
        i = src->rows * src->cols;
        s = (basic_pixel_t *) src->data;
        d = (basic_pixel_t *) dst->data;
        row = 0;
        col = 0;
        while(i-- > 0) {
            if((row == 0 && col == 0) || (row == dst->rows - 1 && col == dst->cols - 1) ||
                    (row == dst->rows - 1 && col == 0) || (row == 0 && col == dst->cols - 1)) {
                // corner pixel
                current_max_neighbours = max_neighbours_corner;
            } else if(row == 0 || col == 0 || row == dst->rows - 1 || col == dst->cols - 1) {
                // edge pixel
                current_max_neighbours = max_neighbours_edge;
            } else {
                // center pixel
                current_max_neighbours = max_neighbours;
            }
            if(*s == h) {
                if(neighbourCount(dst, col, row, 0, connected) + neighbourCount(dst, col, row, 255, connected) == current_max_neighbours && *d == 255) {
                    // this pixel is part of a new blob
                    if(current_blob == 255) {
                        // too many blobs found
                        return 0;
                    }
                    *d = current_blob++;
                    changes = 1;
                } else {
                    // this pixel is part of an existing neighbouring blob with the lowest label
                    blob_neighbour = 255;
                    for(k = 1; k < current_blob; k++) {
                        if(neighbourCount(dst, col, row, k, connected) > 0 && k < blob_neighbour)  {
                            blob_neighbour = k;
                        }
                    }
                    if(blob_neighbour < 255 && *d != blob_neighbour) {
                        *d = blob_neighbour;
                        changes = 1;
                    }
                }
            }
            // increment row/col trackers
            if(++col >= src->cols) {
                col = 0;
                row++;
            }
            // increment data pointers
            s++;
            d++;
        }
    }

    // Step 2: extend initial blobs to lower surrounding values
    //         if blobs touch at this point -> merge into 1 blob
    changes = 1;
    while(changes--) {
        i = src->rows * src->cols;
        s = (basic_pixel_t *) src->data;
        d = (basic_pixel_t *) dst->data;
        row = 0;
        col = 0;
        while(i-- > 0) {
            if((row == 0 && col == 0) || (row == dst->rows - 1 && col == dst->cols - 1) ||
                    (row == dst->rows - 1 && col == 0) || (row == 0 && col == dst->cols - 1)) {
                // corner pixel
                current_max_neighbours = max_neighbours_corner;
            } else if(row == 0 || col == 0 || row == dst->rows - 1 || col == dst->cols - 1) {
                // edge pixel
                current_max_neighbours = max_neighbours_edge;
            } else {
                // center pixel
                current_max_neighbours = max_neighbours;
            }
            if(*s <= h) {
                if(!(neighbourCount(dst, col, row, 0, connected) + neighbourCount(dst, col, row, 255, connected) == current_max_neighbours)) {
                    // this pixel is part of an existing neighbouring blob with the lowest label
                    blob_neighbour = 255;
                    for(k = 1; k < current_blob; k++) {
                        if(neighbourCount(dst, col, row, k, connected) > 0)  {
                            if(k < blob_neighbour) {
                                blob_neighbour = k;
                            } else {

                            }
                        }
                    }
                    if(blob_neighbour < 255 && *d != blob_neighbour) {
                        *d = blob_neighbour;
                        changes = 1;
                    }
                }
            }
            // increment row/col trackers
            if(++col >= src->cols) {
                col = 0;
                row++;
            }
            // increment data pointers
            s++;
            d++;
        }
    }
    // Step 3: Count the number of blobs and set current_blob accordingly
    //         Make sure the blob numbers are continuous from 1 through current_blob
    //         The current_blob and actual number of blobs can differ when blobs are merged.
    uint16_t hist[256];
    basic_pixel_t label_mapping[256];  // map old labels to new labels, index is old label, value is new label, index 0 and 255 are unused
    label_mapping[0] = 0;
    label_mapping[255] = 0;
    histogram(dst, hist);
    register uint32_t blob_count = 0;
    for(i = 1; i < 255; i++) {
        if(hist[i] > 0) {
            blob_count++;
            label_mapping[i] = blob_count;
        } else {
            label_mapping[i] = 0;
        }
    }
    // set new labels
    current_blob = blob_count + 1;
    i = src->cols * src->rows;
    d = (basic_pixel_t *) dst->data;
    while(i-- > 0) {
        if(label_mapping[*d] != 0) {
            *d = label_mapping[*d];
        }
        d++;
    }

    // raise the height of the water from minh+1 trough maxh
    // creating watersheds where different blobs meet.
    register uint32_t j = maxh - minh;
    //register uint32_t j = 1;
    h++; // start at height minh + 1 (since minh and lower were processed in step 1-3);
    while(j-- > 0) {
        changes = 1;
        // Step 4: extend existing blobs up to height h until no changes
        //         creating watersheds where 2 blobs meet
        while(changes--) {
            // first step on height h: extend existing blobs up to height h un-til no changes
            i = src->rows * src->cols;
            s = (basic_pixel_t *) src->data;
            d = (basic_pixel_t *) dst->data;
            row = 0;
            col = 0;
            while(i-- > 0) {
                if((row == 0 && col == 0) || (row == dst->rows - 1 && col == dst->cols - 1) ||
                        (row == dst->rows - 1 && col == 0) || (row == 0 && col == dst->cols - 1)) {
                    // corner pixel
                    current_max_neighbours = max_neighbours_corner;
                } else if(row == 0 || col == 0 || row == dst->rows - 1 || col == dst->cols - 1) {
                    // edge pixel
                    current_max_neighbours = max_neighbours_edge;
                } else {
                    // center pixel
                    current_max_neighbours = max_neighbours;
                }
                if(*s <= h && *d == 255) {
                    if(!(neighbourCount(dst, col, row, 0, connected) + neighbourCount(dst, col, row, 255, connected) == current_max_neighbours)) {
                        // this pixel is part of an existing blob or part of a watershed
                        // if 1 blob neighbour -> part of that blob
                        // if more than 1 blob neighbour -> part of watershed
                        blob_neighbour = 0;
                        for(k = 1; k < current_blob; k++) {
                            if(neighbourCount(dst, col, row, k, connected) > 0)  {
                                if(blob_neighbour == 0) {
                                    // first blob neighbour found -> if no second blob neighbour this pixel is part of blob k
                                    blob_neighbour = k;
                                } else {
                                    // second blob neighbour found -> pixel is part of watershed
                                    blob_neighbour = 0;
                                    *d = 0;
                                    changes = 1;
                                    break;
                                }
                            }
                            // pixel is part of neighbouring blob
                            if(blob_neighbour != 0) {
                                *d = blob_neighbour;
                                changes = 1;
                            }
                        }
                    }
                }
                // increment row/col trackers
                if(++col >= src->cols) {
                    col = 0;
                    row++;
                }
                // increment data pointers
                s++;
                d++;
            }
        }
        // Step 5: create new blobs at height H and extend them at height h until no changes
        //         creating watersheds if it touches another blob
        changes = 1;
        while(changes--) {
            i = src->rows * src->cols;
            s = (basic_pixel_t *) src->data;
            d = (basic_pixel_t *) dst->data;
            row = 0;
            col = 0;
            while(i-- > 0) {
                if((row == 0 && col == 0) || (row == dst->rows - 1 && col == dst->cols - 1) ||
                        (row == dst->rows - 1 && col == 0) || (row == 0 && col == dst->cols - 1)) {
                    // corner pixel
                    current_max_neighbours = max_neighbours_corner;
                } else if(row == 0 || col == 0 || row == dst->rows - 1 || col == dst->cols - 1) {
                    // edge pixel
                    current_max_neighbours = max_neighbours_edge;
                } else {
                    // center pixel
                    current_max_neighbours = max_neighbours;
                }
                if(*s == h && *d != 0) {
                    if(neighbourCount(dst, col, row, 0, connected) + neighbourCount(dst, col, row, 255, connected) == current_max_neighbours && *d == 255) {
                        // this pixel is part of a new blob
                        if(current_blob == 255) {
                            // too many blobs found
                            return 0;
                        }
                        *d = current_blob++;
                        changes = 1;
                    } else {
                        // this pixel is part of an existing blob or part of a watershed
                        if(neighbourCount(src, col, row, *s, connected) > 0) {
                            // If the neighbour pixel value is the same as the current pixel value -> this pixel is part of the lowest neighbouring blob
                            // todo: or part of a watershed if two blob neighbours
                            blob_neighbour = 255;
                            for(k = 1; k < current_blob; k++) {
                                if(neighbourCount(dst, col, row, k, connected) > 0 && k < blob_neighbour)  {
                                    blob_neighbour = k;
                                }
                            }
                            if(blob_neighbour < 255 && *d != blob_neighbour) {
                                *d = blob_neighbour;
                                changes = 1;
                            }
                        } else {
                            // if 1 blob neighbour -> part of that blob
                            // if more than 1 blob neighbour -> part of watershed
                            blob_neighbour = 0;
                            for(k = 1; k < current_blob; k++) {
                                if(neighbourCount(dst, col, row, k, connected) > 0)  {
                                    if(blob_neighbour == 0) {
                                        // first blob neighbour found -> if no second blob neighbour this pixel is part of blob k
                                        blob_neighbour = k;
                                    } else {
                                        // second blob neighbour found -> pixel is part of watershed
                                        blob_neighbour = 0;
                                        *d = 0;
                                        changes = 1;
                                        break;
                                    }
                                }
                            }
                            // pixel is part of neighbouring blob
                            if(blob_neighbour != 0 && *d != blob_neighbour) {
                                *d = blob_neighbour;
                                changes = 1;
                            }
                        }
                    }
                }
                // increment row/col trackers
                if(++col >= src->cols) {
                    col = 0;
                    row++;
                }
                // increment data pointers
                s++;
                d++;
            }
        }
        // increment water height
        h++;
    }

    // Set any remaining markers (255) to 0, these can be isolated regions lower than minh
    setSelectedToValue(dst, dst, 255, 0);

    // Relabel and recount blobs so no numbers are skipped.
    // Also label from LT to RB similar to labelBlobs()
    i = 256;
    while(i-- > 0) {
        label_mapping[i] = 0;
    }

    blob_count = 0;
    i = src->cols * src->rows;
    d = (basic_pixel_t *) dst->data;
    while(i-- > 0) {
        if(*d != 0 && *d != 255 && label_mapping[*d] == 0) {
            label_mapping[*d] = ++blob_count;
        }
        d++;
    }

    // set new labels
    i = src->cols * src->rows;
    d = (basic_pixel_t *) dst->data;
    while(i-- > 0) {
        if(label_mapping[*d] != 0) {
            *d = label_mapping[*d];
        }
        d++;
    }

    return blob_count;
}

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
// initial benchmark time: 3ms
void contrastStretch_basic( const image_t *src
                            ,       image_t *dst
                            , const basic_pixel_t bottom
                            , const basic_pixel_t top)
{
    // Find the min and max pixel values
    register basic_pixel_t min = 255;
    register basic_pixel_t max = 0;
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
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
    s = (basic_pixel_t *) dst->data;
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
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
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
    s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
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
    register basic_pixel_t *l = (basic_pixel_t *) img->data;
    register basic_pixel_t *h = (basic_pixel_t *) (img->data + img->rows * img->cols - 1);
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
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
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
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
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
    s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
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
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
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
// Initial benchmark time: 0.2ms
void erase_basic(const image_t *img)
{
    register uint32_t *ip = (uint32_t *) img->data;
    register int32_t i = img->cols * img->rows / 4;
    while(i-- > 0) {
        *ip++ = 0;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// copy src into dst
// If dst dimensions > src dimensions -> copy in the topleft corner, rest 0
// If dst dimensions < src dimension -> copy only the part that fits
// initial benchmark times:
// copy same size img: 3.6ms
void copy_basic(const image_t *src, image_t *dst)
{
    if(src->data == dst->data) {
        // the images are already the same
        dst->rows = src->rows;
        dst->cols = src->cols;
        dst->view = src->view;
        dst->type = src->type;
        return;
    }
    register long int i = src->rows * src->cols;
    register basic_pixel_t *s = (basic_pixel_t *)src->data;
    register basic_pixel_t *d = (basic_pixel_t *)dst->data;
    register int32_t col = 0;
    register int32_t row = 0;

    if(dst->rows == 0 && dst->cols == 0) {
        dst->rows = src->rows;
        dst->cols = src->cols;
    }
    dst->type = src->type;
    dst->view = src->view;

    // If dst dimensions > src dimensions, initialize to 0
    if(dst->rows > src->rows || dst->cols > src->cols) {
        erase(dst);
    }

    // Loop all pixels and copy
    while(i-- > 0) {
        // Only copy the pixel value if the current row and column exists in the destination image
        if(col < dst->cols && row < dst->rows) {
            *d++ = *s;
        }
        // Increment col/row counters, also handle cases where dst image is larger than src image
        if(++col >= src->cols) {
            col = 0;
            row++;
            if(dst->cols > src->cols) {
                // Increment dst pointer to start of next row
                d += dst->cols - src->cols;
            }
        }
        s++;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time: 1.6ms
void setSelectedToValue_basic(const image_t *src,
                              image_t *dst,
                              const basic_pixel_t selected,
                              const basic_pixel_t value)
{
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
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
// initial benchmark time: 0.002ms
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
// initial benchmark time: 1.6ms
void histogram_basic( const image_t *img, uint16_t *hist )
{
    // Initialize hist array to all zeroes
    register uint32_t i = 256;
    while(i-- > 0) {
        *(hist + i) = 0;
    }
    // Calculate histogram
    i = img->cols * img->rows;
    register basic_pixel_t *s = (basic_pixel_t *) img->data;
    while(i-- > 0) {
        *(hist + *s++) += 1;
    }
}

// ----------------------------------------------------------------------------
// Arithmetic
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// add pixels in src and dst and set the pixel in dst to the result
// values above 255 will be truncated
// initial benchmark time: 1.5ms
void add_basic( const image_t *src, image_t *dst )
{
    register int32_t i = src->cols * src->rows / 4;
    register uint32_t *s = (uint32_t *) src->data;
    register uint32_t *d = (uint32_t *) dst->data;
    register uint32_t temp;
    register uint32_t result;
    while(i-- > 0) {
        temp = *((uint8_t *) s) + *((uint8_t *) d);
        if(temp > 255) { temp = 255; }
        result = temp;

        temp = (*((uint8_t *) s+1) + *((uint8_t *) d+1));
        if(temp > 255) { temp = 255; }
        result |= temp << 8;

        temp = (*((uint8_t *) s+2) + *((uint8_t *) d+2));
        if(temp > 255) { temp = 255; }
        result |= temp << 16;

        temp = (*((uint8_t *) s++ +3) + *((uint8_t *) d+3));
        if(temp > 255) { temp = 255; }
        result |= temp << 24;

        *d++ = result;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time 0.6ms
uint32_t sum_basic( const image_t *img )
{
    register int32_t i = img->rows * img->cols / 4;
    register uint32_t *ip = (uint32_t *) img->data;
    register uint32_t sum = 0;
    while(i-- > 0) {
        sum += *((uint8_t *) ip);
        sum += *((uint8_t *) ip + 1);
        sum += *((uint8_t *) ip + 2);
        sum += *((uint8_t *) ip++ +3);
    }
    return sum;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time: 1.4ms
void multiply_basic( const image_t *src, image_t *dst )
{
    register int32_t i = src->cols * src->rows / 4;
    register uint32_t *s = (uint32_t *) src->data;
    register uint32_t *d = (uint32_t *) dst->data;
    register uint32_t temp;
    register uint32_t result;
    while(i-- > 0) {
        temp = *((uint8_t *) s) * *((uint8_t *) d);
        if(temp > 255) { temp = 255; }
        result = temp;

        temp = (*((uint8_t *) s+1) * *((uint8_t *) d+1));
        if(temp > 255) { temp = 255; }
        result |= temp << 8;

        temp = (*((uint8_t *) s+2) * *((uint8_t *) d+2));
        if(temp > 255) { temp = 255; }
        result |= temp << 16;

        temp = (*((uint8_t *) s++ +3) * *((uint8_t *) d+3));
        if(temp > 255) { temp = 255; }
        result |= temp << 24;

        *d++ = result;
    }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// precondition: src is a binary image
// initial benchmark time: 0.79ms
void invert_basic( const image_t *src, image_t *dst )
{
    dst->view = IMGVIEW_BINARY;
    register uint32_t *s = (uint32_t *) src->data;
    register uint32_t *d = (uint32_t *) dst->data;
    register int32_t i = src->cols * src->rows / 4;
    register uint32_t result;
    while(i-- > 0) {
        result = 1 - *((uint8_t *) s);
        result |= (1 - *((uint8_t *) s + 1)) << 8;
        result |= (1 - *((uint8_t *) s + 2)) << 16;
        result |= (1 - *((uint8_t *) s++ + 3)) << 24;
        *d++ = result;
    }
}

// benchmark time without LUT: 3s!
// benchmark time with LUT: 2.2ms!!!!!!
void gamma_basic( const image_t *src, image_t *dst, const float c, const float g)
{
    register uint32_t *s = (uint32_t *) src->data;
    register uint32_t *d = (uint32_t *) dst->data;
    register uint32_t result;
    register int32_t temp;

    // create look up table
    basic_pixel_t LUT[256];
    register uint32_t i = 256;
    while(i-- > 0) {
        //LUT[i] = (basic_pixel_t) ((i - min) * stretch_factor + (float) 0.5);
        temp = (int32_t) (powf(i/255.0, g) * c * 255 + 0.5);
        if(temp > 255) {
            LUT[i] = 255;
        } else if(temp < 0) {
            LUT[i] = 0;
        } else {
            LUT[i] = temp;
        }
    }

    i = src->cols * src->rows / 4;
    while(i-- > 0) {
        result = (uint32_t) LUT[*((uint8_t *) s)];

        result |= (uint32_t) LUT[*((uint8_t *) s + 1)] << 8;

        result |= (uint32_t) LUT[*((uint8_t *) s + 2)] << 16;

        result |= (uint32_t) LUT[*((uint8_t *) s++ + 3)] << 24;

        *d++ = result;
    }
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
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
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

// initial benchmark time: 284ms
void gaussianBlur_basic( const image_t *src
                         ,       image_t *dst
                         , const int32_t kernelSize
                         , const double sigma) {
    // implementation taken from https://www.geeksforgeeks.org/gaussian-filter-generation-c/
    // Create gaussian kernel image with size kernelsize and given sigma
    image_t *kernel = newFloatImage(kernelSize, kernelSize);
    register float_pixel_t *k = (float_pixel_t *) kernel->data;
    register double s = 2.0 * sigma * sigma;
    register double r;
    register double sum = 0.0; // sum used for normalization

    // generate kernel
    for (int x = -kernelSize / 2; x <= kernelSize / 2; x++) {
        for (int y = -kernelSize / 2; y <= kernelSize / 2; y++) {
            r = sqrt(x * x + y * y);
            *k = (float_pixel_t) ((exp(-(r * r) / s)) / (M_PI * s));
            sum += *k++;
        }
    }

    // normalising the Kernel
    k = (float_pixel_t *) kernel->data;
    for (int i = 0; i < kernelSize; ++i) {
        for (int j = 0; j < kernelSize; ++j) {
            *k++ /= (float_pixel_t) sum;
        }
    }

    // perform a convolution with this gaussian kernel
    convolution_basic(src, dst, kernel);
}

// Only use normalized kernels of imgtype float
void convolution_basic( const image_t *src
                        , const image_t *dst
                        , const image_t *kernel) {
    register uint32_t i = src->cols * src->rows;
    register uint32_t w_counter;
    register int32_t w_row;
    register int32_t w_col;
    register int32_t row = 0;
    register int32_t col = 0;
    register basic_pixel_t *w; // window pixel
    if(kernel->type != IMGTYPE_FLOAT){
#ifdef QDEBUG_ENABLE
        fprintf(stderr, "Convolution_basic is only implemented for float kernels for now.");
#endif
        return;
    }
    register float_pixel_t *k; // kernel pixel
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
    register double result;
    // loop through image pixels
    while(i-- > 0) {
        w_counter = kernel->cols * kernel->rows;
        w_row = -kernel->rows / 2;
        w_col = -kernel->cols / 2;
        result = 0.0;
        k = (float_pixel_t *) kernel->data;
        // loop through window pixels
        while(w_counter-- > 0) { // w_counter is used as kernel data index
            if(col + w_col < 0 || col + w_col >= src->cols
                    || row + w_row < 0 || row + w_row >= src->rows) {
                // skip pixels outside the image border
                k++;
                if(++w_col > kernel->cols / 2) {
                    w_col = -kernel->cols / 2;
                    w_row++;
                }
                continue;
            }
            w = s + w_row * src->cols + w_col++;
            result += *w * *k++;
            if(w_col > kernel->cols / 2) {
                w_col = -kernel->cols / 2;
                w_row++;
            }
        }
        // Set destination pixel
        if(result > 255) { result = 255; }
        else if(result < 0) { result = 0; }
        *d++ = (basic_pixel_t) (result + 0.5);
        s++;
        col++;
        if(col >= src->cols) {
            col = 0;
            row++;
        }
    }
}

// ----------------------------------------------------------------------------
// Morphology
// ----------------------------------------------------------------------------
// precondition: src, dst and kernel are binary images
//               src and dst point to different images
void erode_basic(const image_t *src, image_t *dst, const image_t *kernel) {
    dst->view = IMGVIEW_BINARY;
    register uint32_t i = src->cols * src->rows;
    register uint32_t w_counter;
    register int32_t w_row;
    register int32_t w_col;
    register int32_t row = 0;
    register int32_t col = 0;
    register basic_pixel_t *w; // window pixel
    register basic_pixel_t WTEST;
    register basic_pixel_t *k; // kernel pixel
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
    register basic_pixel_t result;
    // loop through image pixels
    while(i-- > 0) {
        w_counter = kernel->cols * kernel->rows;
        w_row = -kernel->rows / 2;
        w_col = -kernel->cols / 2;
        result = 1;
        k = (basic_pixel_t *) kernel->data;
        // loop through window pixels
        while(w_counter-- > 0) { // w_counter is used as kernel data index
            if(col + w_col < 0 || col + w_col >= src->cols
                    || row + w_row < 0 || row + w_row >= src->rows) {
                // skip pixels outside the image border
                k++;
                if(++w_col > kernel->cols / 2) {
                    w_col = -kernel->cols / 2;
                    w_row++;
                }
                continue;
            }
            w = s + w_row * src->cols + w_col;
            if(*k++ == 1 && *w == 0) {
                result = 0;
                break;
            }
            if(++w_col > kernel->cols / 2) {
                w_col = -kernel->cols / 2;
                w_row++;
            }
        }
        // Set destination pixel
        *d++ = result;
        s++;
        if(++col == src->cols) {
            col = 0;
            row++;
        }
    }
}

// precondition: src, dst and kernel are binary images
//               src and dst point to different images
void dilate_basic(const image_t *src, image_t *dst, const image_t *kernel) {
    dst->view = IMGVIEW_BINARY;
    register uint32_t i = src->cols * src->rows;
    register uint32_t w_counter;
    register int32_t w_row;
    register int32_t w_col;
    register int32_t row = 0;
    register int32_t col = 0;
    register basic_pixel_t *w; // window pixel
    register basic_pixel_t *k; // kernel pixel
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
    register basic_pixel_t result;
    // loop through image pixels
    while(i-- > 0) {
        w_counter = kernel->cols * kernel->rows;
        w_row = -kernel->rows / 2;
        w_col = -kernel->cols / 2;
        result = 0;
        k = (basic_pixel_t *) kernel->data;
        // loop through window pixels
        while(w_counter-- > 0) { // w_counter is used as kernel data index
            if(col + w_col < 0 || col + w_col >= src->cols
                    || row + w_row < 0 || row + w_row >= src->rows) {
                // skip pixels outside the image border
                k++;
                if(++w_col > kernel->cols / 2) {
                    w_col = -kernel->cols / 2;
                    w_row++;
                }
                continue;
            }
            w = s + w_row * src->cols + w_col;
            if(*k++ == 1 && *w == 1) {
                result = 1;
                break;
            }
            if(++w_col > kernel->cols / 2) {
                w_col = -kernel->cols / 2;
                w_row++;
            }
        }
        // Set destination pixel
        *d++ = result;
        s++;
        if(++col == src->cols) {
            col = 0;
            row++;
        }
    }
}

// precondition: src, dst and kernel are binary images
//               src and dst point to different images
void open_basic(const image_t *src, image_t *dst, const image_t *kernel) {
    image_t *tmp = newBasicImage(src->cols, src->rows);
    erode_basic(src, tmp, kernel);
    dilate_basic(tmp, dst, kernel);
}

// precondition: src, dst and kernel are binary images
//               src and dst point to different images
void close_basic(const image_t *src, image_t *dst, const image_t *kernel) {
    image_t *tmp = newBasicImage(src->cols, src->rows);
    dilate_basic(src, tmp, kernel);
    erode_basic(tmp, dst, kernel);
}


// ----------------------------------------------------------------------------
// Binary
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time: 35-50ms
void removeBorderBlobs_basic( const image_t *src
                              ,       image_t *dst
                              , const eConnected connected)
{
    // set border pixels with value 1 to value 2
    register uint32_t i = src->cols;
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
    // top row
    while(i-- > 0) {
        if(*s++ == 1) {
            *d = 2;
        }
        d++;
    }
    // left column (except corners)
    i = src->rows - 2;
    s = (basic_pixel_t *) (src->data + src->cols);
    d = (basic_pixel_t *) (dst->data + src->cols);
    while(i-- > 0) {
        if(*s == 1) {
            *d = 2;
        }
        s += src->cols;
        d += src->cols;
    }
    // right column (except corners)
    i = src->rows - 2;
    s = (basic_pixel_t *) (src->data + src->cols * 2 - 1);
    d = (basic_pixel_t *) (dst->data + src->cols * 2 - 1);
    while(i-- > 0) {
        if(*s == 1) {
            *d = 2;
        }
        s += src->cols;
        d += src->cols;
    }
    // bottom row
    i = src->cols;
    s = (basic_pixel_t *) (src->data + src->cols * (src->rows - 1));
    d = (basic_pixel_t *) (dst->data + src->cols * (src->rows - 1));
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
        s = (basic_pixel_t *) (src->data + src->cols + 1);
        d = (basic_pixel_t *) (dst->data + src->cols + 1);
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
        d = (basic_pixel_t *) (dst->data + src->cols * (src->rows - 1) - 2);
        s = (basic_pixel_t *) (src->data + src->cols * (src->rows - 1) - 2);
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
// initial benchmark time: 31ms
void fillHoles_basic( const image_t *src
                      ,       image_t *dst
                      , const eConnected connected)
{
    // set border pixels with value 1 that have no non-border neighbors to value 2
    register uint32_t i = src->cols;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
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
    s = (basic_pixel_t *) (src->data + src->cols);
    d = (basic_pixel_t *) (dst->data + dst->cols);
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
    s = (basic_pixel_t *) (src->data + src->cols * 2 - 1);
    d = (basic_pixel_t *) (dst->data + src->cols * 2 - 1);
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
    s = (basic_pixel_t *) (src->data + src->cols * (src->rows - 1));
    d = (basic_pixel_t *) (dst->data + src->cols * (src->rows - 1));
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
    s = (basic_pixel_t *) src->data;
    d = (basic_pixel_t *) dst->data;
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
        d = (basic_pixel_t *) (dst->data + src->cols + 1);
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
        d = (basic_pixel_t *) (dst->data + src->cols * (src->rows - 1) - 2);
        s = (basic_pixel_t *) (src->data + src->cols * (src->rows - 1) - 2);
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
// 254 blobs max, return the number of blobs found or 0 if 0 or more than 254 blobs are found
// 255 used as marker
// precondition: image is a binary image
// initial benchmark time: 50ms
uint32_t labelBlobs_basic( const image_t *src
                           ,       image_t *dst
                           , const eConnected connected)
{
    register uint32_t i;
    register uint32_t j;
    register basic_pixel_t *d;
    register int32_t row;
    register int32_t col;
    register uint32_t changes = 1;
    register uint32_t currentBlob = 1;

    register uint32_t max_neighbours;
    register uint32_t max_neighbours_corner;
    register uint32_t max_neighbours_edge;

    register uint32_t current_max_neighbours;
    if(connected == FOUR) {
        max_neighbours = 4;
        max_neighbours_corner = 2;
        max_neighbours_edge = 3;
    } else {
        max_neighbours = 8;
        max_neighbours_corner = 3;
        max_neighbours_edge = 5;
    }

    // set all 1 to 255 and copy src into dst.
    copy(src, dst);
    setSelectedToValue(dst, dst, 1, 255);
    dst->view = IMGVIEW_LABELED;

    while(changes--) {
        // process image left top to right bottom
        row = 0;
        col = 0;
        i = dst->cols * dst->rows;
        d = (basic_pixel_t *) dst->data;
        while(i-- > 0) {
            if((row == 0 && col == 0) || (row == dst->rows - 1 && col == dst->cols - 1) ||
                    (row == dst->rows - 1 && col == 0) || (row == 0 && col == dst->cols - 1)) {
                // corner pixel
                current_max_neighbours = max_neighbours_corner;
            } else if(row == 0 || col == 0 || row == dst->rows - 1 || col == dst->cols - 1) {
                // edge pixel
                current_max_neighbours = max_neighbours_edge;
            } else {
                // center pixel
                current_max_neighbours = max_neighbours;
            }
            if(*d == 255) {
                if(neighbourCount(dst, col, row, 0, connected) + neighbourCount(dst, col, row, 255, connected) == current_max_neighbours) {
                    // this pixel is part of a new blob
                    if(currentBlob == 255) {
                        // too many (intermediate) blobs found
                        return 0;
                    }
                    *d = currentBlob++;
                    changes = 1;
                } else {
                    // this pixel is part of an existing blob -> find the lowest neighbour (not 0) to find the blob it belongs to
                    for(j = 1; j < currentBlob; j++) {
                        if(neighbourCount(dst, col, row, j, connected) > 0) {
                            *d = j;
                            changes = 1;
                            break;
                        }
                    }
                }
            } else if(*d > 1) {
                // check if the current pixel has a neighbour with a lower value -> if so it should be part of that blob instead
                for(j = 1; j < *d; j++) {
                    if(neighbourCount(dst, col, row, j, connected) > 0) {
                        *d = j;
                        changes = 1;
                        break;
                    }
                }
            }
            // increment row, col and data pointer
            d++;
            if(++col >= dst->cols) {
                col = 0;
                row++;
            }
        }
        // process image right bottom to top left
        row = dst->rows - 1;
        col = dst->cols - 1;
        d = (basic_pixel_t *) (dst->data + dst->cols * src->rows - 1);
        i = dst->rows * dst->cols;
        while(i-- > 0) {
            if((row == 0 && col == 0) || (row == dst->rows - 1 && col == dst->cols - 1) ||
                    (row == dst->rows - 1 && col == 0) || (row == 0 && col == dst->cols - 1)) {
                // corner pixel
                current_max_neighbours = max_neighbours_corner;
            } else if(row == 0 || col == 0 || row == dst->rows - 1 || col == dst->cols - 1) {
                // edge pixel
                current_max_neighbours = max_neighbours_edge;
            } else {
                // center pixel
                current_max_neighbours = max_neighbours;
            }
            if(*d == 255) {
                if(neighbourCount(dst, col, row, 0, connected) + neighbourCount(dst, col, row, 255, connected) == current_max_neighbours) {
                    // this pixel is part of a new blob
                    if(currentBlob == 255) {
                        // too many (intermediate) blobs found
                        return 0;
                    }
                    *d = currentBlob++;
                    changes = 1;
                } else {
                    // this pixel is part of an existing blob -> find the lowest neighbour (not 0) to find the blob it belongs to
                    for(j = 1; j < 255; j++) {
                        if(neighbourCount(dst, col, row, j, connected) > 0) {
                            *d = j;
                            changes = 1;
                            break;
                        }
                    }
                }
            } else if(*d > 1) {
                // check if the current pixel has a neighbour with a lower value -> if so it should be part of that blob instead
                for(j = 1; j < *d; j++) {
                    if(neighbourCount(dst, col, row, j, connected) > 0) {
                        *d = j;
                        changes = 1;
                        break;
                    }
                }
            }
            // increment row, col and data pointer
            d--;
            if(--col == -1) {
                col = dst->cols - 1;
                row--;
            }
        }
    }
    if(currentBlob == 1) {
        // no blobs were found
        return 0;
    }

    // Make sure the blob labels are continuous using a histogram of the result
    uint16_t hist[256];
    histogram_basic(dst, hist);
    register uint32_t blobCount = 0; // Count the number of blobs to determine the new label
    // generate label mapping entries
    for(i = 1; i < 255; i++) {
        if(hist[i] > 0) {
            blobCount++;
            setSelectedToValue(dst, dst, i, blobCount);
        }
    }
    return blobCount;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// precondition: src is a binary image
// initial benchmark time 12ms
void binaryEdgeDetect_basic( const image_t *src
                             ,       image_t *dst
                             , const eConnected connected)
{
    register uint32_t i = src->rows * src->cols;
    register basic_pixel_t *s = (basic_pixel_t *) src->data;
    register basic_pixel_t *d = (basic_pixel_t *) dst->data;
    register int32_t row = 0;
    register int32_t col = 0;
    while(i-- > 0) {
        if(*s++ == 1) {
            if(neighbourCount_basic(src, col, row, 0, connected) == 0) {
                *d++ = 2;
            } else {
                *d++ = 1;
            }
        } else {
            *d++ = 0;
        }
        // increment row and col trackers
        if(++col >= src->cols) {
            col = 0;
            row++;
        }
    }
    setSelectedToValue_basic(dst, dst, 2, 0);
}

// ----------------------------------------------------------------------------
// Analysis
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// perimeter calculated as:
// pixel with one edge connected to background -> perimeter++
// pixel with two edges connected to background -> perimeter += sqrt(2)
// pixel with three edges connected to background -> perimeter += 0.5 * (1+sqrt(2))
// initial benchmark time: 3.5ms
void blobAnalyse_basic( const image_t *img
                        , const uint8_t blobnr
                        ,       blobinfo_t *blobInfo)
{
    register uint16_t min_row = img->rows - 1;
    register uint16_t max_row = 0;
    register uint16_t min_col = img->cols - 1;
    register uint16_t max_col = 0;
    register uint16_t pixel_count = 0;
    register basic_pixel_t *ip = (basic_pixel_t *) img->data;
    register uint32_t i = img->rows * img->cols;
    register int32_t row = 0;
    register int32_t col = 0;
    register float perimeter = 0.0f;
    register uint32_t neighbours;
    while(i-- > 0) {
        if(*ip++ == blobnr) {
            // track bounding rows / columns
            if(col < min_col) {
                min_col = col;
            } else if(col > max_col) {
                max_col = col;
            }
            if(row < min_row) {
                min_row = row;
            } else if(row > max_row) {
                max_row = row;
            }
            // increment pixel count
            pixel_count++;
            // keep track of perimeter
            neighbours = neighbourCount_basic(img, col, row, 0, FOUR);
            if(neighbours == 1) {
                perimeter += 1.0f;
            } else if(neighbours == 2) {
                perimeter += (float) sqrt(2);
            } else if(neighbours == 3) {
                perimeter += (float) 0.5f / (1.0f + (float) sqrt(2));
            }
        }
        if(++col >= img->cols) {
            col = 0;
            row++;
        }
    }
    blobInfo->height = max_row - min_row + 1;
    blobInfo->width = max_col - min_col + 1;
    blobInfo->nof_pixels = pixel_count;
    blobInfo->perimeter = perimeter;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// initial benchmark time 1.8ms
void centroid_basic( const image_t *img
                     , const uint8_t blobnr
                     ,       int32_t *cc
                     ,       int32_t *rc)
{
    register uint32_t m_00 = 0;
    register uint32_t m_01 = 0;
    register uint32_t m_10 = 0;
    register uint32_t i = img->rows * img->cols;
    register basic_pixel_t *ip = (basic_pixel_t *) img->data;
    register int32_t col = 0;
    register int32_t row = 0;
    // calculate m_00, m_01 and m_10
    while(i-- > 0) {
        if(*ip++ == blobnr) {
            m_00 += 1;
            m_01 += row;
            m_10 += col;
        }
        if(++col >= img->cols) {
            col = 0;
            row++;
        }
    }
    *cc = (int32_t) ((float) m_10 / (float) m_00 + 0.5f);
    *rc = (int32_t) ((float) m_01 / (float) m_00 + 0.5f);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// calculate the normalized central moment eta_pq, normalized from mu_pq, calculated from m_pq
// initial benchmark time: 7.5ms
float normalizedCentralMoments_basic( const image_t *img
                                      , const uint8_t blobnr
                                      , const int32_t p
                                      , const int32_t q)
{
    if((p == 0 && q == 1) || (p == 1 && q == 0)) {
        return 0.0f;
    }
    if(p == 0 && q == 0) {
        return 1.0f;
    }
    register uint32_t m_00 = 0;
    register uint32_t m_01 = 0;
    register uint32_t m_10 = 0;
    register uint32_t i = img->rows * img->cols;
    register basic_pixel_t *ip = (basic_pixel_t *) img->data;
    register int32_t col = 0;
    register int32_t row = 0;
    // calculate m_00, m_01 and m_10
    while(i-- > 0) {
        if(*ip++ == blobnr) {
            m_00 += 1;
            m_01 += row;
            m_10 += col;
        }
        if(++col >= img->cols) {
            col = 0;
            row++;
        }
    }
    // calculate centroid
    register float cc = (float) m_10 / (float) m_00;
    register float rc = (float) m_01 / (float) m_00;
    // calculate central moment µ20 or µ02
    register float central_moment = 0.0f;
    i = img->rows * img->cols;
    ip = (basic_pixel_t *) img->data;
    col = 0;
    row = 0;
    while(i-- > 0) {
        if(*ip++ == blobnr) {
            central_moment += powf(col - cc, p) * powf(row - rc, q);
        }
        if(++col >= img->cols) {
            col = 0;
            row++;
        }
    }
    return central_moment / powf(m_00, (p + q) / 2.0f + 1.0f);
}

// ----------------------------------------------------------------------------
// EOF
// ----------------------------------------------------------------------------

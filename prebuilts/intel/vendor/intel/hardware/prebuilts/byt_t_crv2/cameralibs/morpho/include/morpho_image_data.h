/**
 * @file     morpho_image_data.h
 * @brief    Image data structure definition
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_IMAGE_DATA_H
#define MORPHO_IMAGE_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    void * y;               /**< Pointer for Y image data */
    void * u;               /**< Pointer for U image data */
    void * v;               /**< Pointer for V image data */
} morpho_ImageYuvPlanar;
    
typedef struct{
    void * y;               /**< Pointer for Y image data */
    void * uv;              /**< Pointer for UV image data */
} morpho_ImageYuvSemiPlanar;

/** image data. */
typedef struct {
    int width;              /**< Width */
    int height;             /**< Height */
    union{
        void * p;           /**< Pointer for image data */
        morpho_ImageYuvPlanar planar;
        morpho_ImageYuvSemiPlanar semi_planar;
    } dat;
} morpho_ImageData;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_IMAGE_DATA_H */

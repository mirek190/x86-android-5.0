//====================================================================
// morpho_image_stabilizer3.h
// [SJIS/CRLF] { Ç† ïÑçÜâªï˚éÆé©ìÆîªíËóp }
//
// Copyright(c) 2006-2012 Morpho,Inc.
//====================================================================

#ifndef MORPHO_IMAGE_STABILIZER3_H
# define MORPHO_IMAGE_STABILIZER3_H

//--------------------------------------------------------------------

# include "morpho_api.h"
# include "morpho_error.h"
# include "morpho_image_data.h"
# include "morpho_motion_data.h"
# include "morpho_rect_int.h"

//--------------------------------------------------------------------

# ifdef __cplusplus
extern "C" {
# endif

//====================================================================

/** Version string */
# define MORPHO_IMAGE_STABILIZER3_VERSION "Morpho PhotoSolid Ver.3.1.0 INTEL 2013/01/15"

/** Version specified */
# define MORPHO_IMAGE_STABILIZER_VERSION_3_1

/** Use Hardware I/F */
# undef MORPHO_IMAGE_STABILIZER3_USE_HW_IF

/** Use sharpness analysis */
# define MORPHO_USE_SHARPNESS_ANALYSIS

//--------------------------------------------------------------------

/** NR Type */
enum
{
    MORPHO_IMAGE_STABILIZER3_NR_NORMAL = 0,
#ifdef MORPHO_IMAGE_STABILIZER_VERSION_3_1
    MORPHO_IMAGE_STABILIZER3_NR_FINE,
    MORPHO_IMAGE_STABILIZER3_NR_LITE,
    MORPHO_IMAGE_STABILIZER3_NR_SUPERFINE,
#endif
    MORPHO_IMAGE_STABILIZER3_NR_NUM,
};

/** Input sequence type */
enum
{
    MORPHO_IMAGE_STABILIZER3_INPUT_SEQUENCE_NORMAL = 0,
    MORPHO_IMAGE_STABILIZER3_INPUT_SEQUENCE_LITE,
    MORPHO_IMAGE_STABILIZER3_INPUT_SEQUENCE_NUM,
};

/** Motion Detection Failure Code */
enum
{
    /** Failed because global subject blur was detected */
    MORPHO_IMAGE_STABILIZER3_FAILURE_CODE_OBJBLUR     = 1 << 0,
    /** Failed because too much rotation blur */
    MORPHO_IMAGE_STABILIZER3_FAILURE_CODE_ROTATION    = 1 << 1,
    /** Failed because small amount of features */
    MORPHO_IMAGE_STABILIZER3_FAILURE_CODE_FEATURELESS = 1 << 2,
    /** Failed because blur exceeded MarginOfMotion */
    MORPHO_IMAGE_STABILIZER3_FAILURE_CODE_OVER_MARGIN = 1 << 3,
    /** Failed because high-frequency line detected */
    MORPHO_IMAGE_STABILIZER3_FAILURE_CODE_HIGHFREQ    = 1 << 4,
};

/** Image Stabilizer */
typedef struct
{
    void *p; /**< Pointer to PhotoSolid internal processes */
} morpho_ImageStabilizer3;

//--------------------------------------------------------------------

/**
 * Gets the version string 
 *
 * @return  Version string(MORPHO_IMAGE_STABILIZER_VERSION)
 */
MORPHO_API(const char *)
morpho_ImageStabilizer3_getVersion(void);

/**
 * Get the memory size required for image image stabilizer
 * Referring to TRM, please check the image format that can be specified.
 *
 * @param[in] width      Input image width
 * @param[in] height     Input image height
 * @param[in] image_num  Number of input images
 * @param[in] format     Image format
 * @return  The size of the memory area required for image stabilization(byte)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getBufferSize(
    int width,
    int height,
    int image_num,
    const char *format);

/**
 * Initializes the image stabilizer
 *
 * @param[out] stabilizer   Pointer to the image stabilizer
 * @param[out] buffer       Pointer to the memory allocated to the image stabilizer
 * @param[in]  buffer_size  The size of the memory allocated to the image stabilizer
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_initialize(
    morpho_ImageStabilizer3 *stabilizer,
    void *buffer,
    int buffer_size);

/**
 * Destruction of the image stabilizer
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_finalize(
    morpho_ImageStabilizer3 *stabilizer);

/**
 * Image stabilizer: processing start
 * output_image may be the same as a input image.
 *
 * @param[in,out] stabilizer    Pointer to the image stabilizer
 * @param[out]    output_image  Pointer to the output image
 * @param[in]     image_num     Number of input images
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_start(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_ImageData *output_image,
    int image_num);

/**
 * Image stabilizer: motion detection
 *
 * @param[in,out] stabilizer   Pointer to the image stabilizer
 * @param[in]     input_image  Pointer to the input image
 * @param[out]    motion_data  moiton data
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_detectMotion(
    morpho_ImageStabilizer3 *stabilizer,
    const morpho_ImageData *input_image,
    morpho_MotionData *motion_data);

/**
 * Image stabilizer: alpha blending
 *
 * @param[in,out] stabilizer   Pointer to the image stabilizer
 * @param[in]     input_image  Pointer to the input image
 * @param[in]     motion_data  moiton data
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_render(
    morpho_ImageStabilizer3 *stabilizer,
    const morpho_ImageData *input_image,
    morpho_MotionData *motion_data);

/**
 * Image stabilizer: noise reduction
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_reduceNoise(
    morpho_ImageStabilizer3 *stabilizer);

#ifdef MORPHO_IMAGE_STABILIZER_VERSION_3_1

# ifdef MORPHO_IMAGE_STABILIZER3_USE_HW_IF
/**
 * Image stabilizer: blending area acquisition
 *
 * @param[in,out] stabilizer   Pointer to the image stabilizer
 * @param[in]     input_image  Pointer to the input image
 * @param[out]    rect         Rectangular area to blend
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_calcAccumRect(
    morpho_ImageStabilizer3 *stabilizer,
    const morpho_ImageData *input_image,
    morpho_RectInt *rect);

/**
 * Image stabilizer: alignment
 *
 * @param[in,out] stabilizer    Pointer to the image stabilizer
 * @param[in]     input_image   Pointer to the input image
 * @param[in]     output_image  Pointer to the output image
 * @param[out]    rect          Rectangular area to blend
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_alignImage(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_ImageData *output_image,
    const morpho_ImageData *input_image,
    morpho_RectInt *rect);

/**
 * Image stabilizer: ghost detection & blending mask generation
 *
 * @param[in,out] stabilizer   Pointer to the image stabilizer
 * @param[in]     input_image  Pointer to the input image
 * @param[in]     alpha_tbl    blending mask
 * @param[in]     rect         Rectangular area to blend
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_detectGhost(
    morpho_ImageStabilizer3 *stabilizer,
    unsigned char *alpha_tbl,
    const morpho_ImageData *input_image,
    const morpho_RectInt *rect);

/**
 * Image stabilizer: alpha blending
 *
 * @param[in,out] stabilizer   Pointer to the image stabilizer
 * @param[in]     input_image  Pointer to the input image
 * @param[in]     alpha_tbl    blending mask
 * @param[in]     rect         Rectangular area to blend
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_blend(
    morpho_ImageStabilizer3 *stabilizer,
    const morpho_ImageData *input_image,
    const unsigned char *alpha_tbl,
    const morpho_RectInt *rect);
# endif    /* !MORPHO_IMAGE_STABILIZER3_USE_HW_IF */

/**
 * Image stabilizer: noise reduction for Y image
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_reduceNoiseLuma(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_ImageData *output_image);

/**
 * Image stabilizer: noise reduction for C image
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_reduceNoiseChroma(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_ImageData *output_image);
#endif    /* MORPHO_IMAGE_STABILIZER_VERSION_3_1 */

/**
 * âÊëúèkè¨(1/2 x 1/2)
 *
 * @param[in,out] stabilizer    Pointer to the image stabilizer
 * @param[out]    output_image  Pointer to the output image
 * @param[in]     input_image   Pointer to the input image
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_shrinkImage(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_ImageData *output_image,
    const morpho_ImageData *input_image);

#ifdef MORPHO_USE_SHARPNESS_ANALYSIS
/**
 * Get the sharpness
 *
 * @param[in,out] stabilizer    Pointer to the image stabilizer
 * @param[out]    sharpness     Sharpness array
 * @param[in]     input_images  Input images array
 * @param[in]     image_num     Number of input images
 *
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getSharpness(
    morpho_ImageStabilizer3 *stabilizer,
    int*   sharpness_vals,
    morpho_ImageData *input_images,
    int    image_num);
#endif

/**
 * Get the image format
 * After executing morpho_ImageStabilizer3_initialize().
 * Buffer size should be at least 32.
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] format         Image format string
 * @param[in] buffer_size     Buffer size
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getImageFormat(
    morpho_ImageStabilizer3 *stabilizer,
    char *format,
    const int buffer_size);

/**
 * Get the degree of freedom of motion detection
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] dof            The degree of freedom of motion detection
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getDoF(
    morpho_ImageStabilizer3 *stabilizer,
    int *dof);

/**
 * Get the rectangular for motion detection
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] rect           The rectangular for motion detection
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getDetectionRect(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_RectInt *rect);

/**
 * Get the minimum rectangle S / N ratio is ensured
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] rect           The minimum rectangle S / N ratio is ensured
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getMarginOfMotion(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_RectInt *rect);

#ifndef MORPHO_IMAGE_STABILIZER_VERSION_3_1
/**
 * Get the rectangle area to cut out as the output image
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] rect           The rectangular area to be cut
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getRenderingRect(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_RectInt *rect);

/**
 * Get the rectangular area to be  blended
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] rect           The rectangular area to be blended
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getCompositionRect(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_RectInt *rect);


/**
 * Get the global subject blur detection level
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] level          The global subject blur detection level
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getObjBlurLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int *level);
#endif

/**
 * Get the rotation blur detection level
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] level          The rotation blur detection level
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getRotationLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int *level);

/**
 * Get the high-frequency straight lines detection level
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] level          The high-frequency straight lines detection level
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getHighFreqLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int *level);

/**
 * Get the feature value detection level
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] level  The feature value detection level
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getFeaturelessLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int *level);

/**
 * Get the local subject blur correction level
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] level          The local subject blur correction level
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getObjBlurCorrectionLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int *level);

/**
 * Get the level of noise reduction strength
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] level          The level of noise reduction strength
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getNoiseReductionLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int *level);

/**
 * Get the brightness amplification factor
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] gain           The brightness amplification factor
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getGain(
    morpho_ImageStabilizer3 *stabilizer,
    int *gain);

/**
 * Get the noise reduction type
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] type           The noise reduction type
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getNoiseReductionType(
    morpho_ImageStabilizer3 *stabilizer,
    int *type);

/**
 * Get the input sequence type
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] type           The input sequence type
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getInputSequenceType(
    morpho_ImageStabilizer3 *stabilizer,
    int *type);

#ifdef MORPHO_IMAGE_STABILIZER_VERSION_3_1
/**
 * Get the block size of ghost detection
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] block_size     The block size of ghost detection
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getObjBlurBlockSize(
    morpho_ImageStabilizer3 *stabilizer,
    int *block_size);

/**
 * Get the shrink ratio
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] ratio          The shrink ratio
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getShrinkRatio(
    morpho_ImageStabilizer3 *stabilizer,
    int *ratio);

/**
 * Get the Y image size
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] width          Y image width
 * @param[out] height         Y image height
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getFinestSize(
    morpho_ImageStabilizer3 *stabilizer,
    int *width, int *height);

/**
 * Get the level of noise reduction strength for Y image
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] level          The level of noise reduction strength for Y image
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getLumaNoiseReductionLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int *level);

/**
 * Get the level of noise reduction strength for C image
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] level          The level of noise reduction strength for C image
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getChromaNoiseReductionLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int *level);

/**
 * Get the noise reduction type for Y image
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] type           The noise reduction type for Y image
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getLumaNoiseReductionType(
    morpho_ImageStabilizer3 *stabilizer,
    int *type);

/**
 * Get the noise reduction type for C image
 * After executing morpho_ImageStabilizer3_initialize().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] type           The noise reduction type for C image
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_getChromaNoiseReductionType(
    morpho_ImageStabilizer3 *stabilizer,
    int *type);
#endif    /* !MORPHO_IMAGE_STABILIZER_VERSION_3_1 */

/**
 * Set the image format
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 * Referring to TRM, please check the image format that can be specified.
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] format          Image format
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setImageFormat(
    morpho_ImageStabilizer3 *stabilizer,
    const char *format);

/**
 * Set the degree of freedom of motion detection
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] dof The degree of freedom of motion detection(2, 3, 4, or 6)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setDoF(
    morpho_ImageStabilizer3 *stabilizer,
    int dof);

/**
 * Set the rectangular for motion detection
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] rect            The rectangular for motion detection
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setDetectionRect(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_RectInt *rect);

/**
 * Set the minimum rectangle S / N ratio is ensured
 * If NULL is set to rect does not guarantee the ratio S / N.
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] rect            The minimum rectangle S / N ratio is ensured
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setMarginOfMotion(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_RectInt *rect);

#ifndef MORPHO_IMAGE_STABILIZER_VERSION_3_1
/**
 * Set the rectangle area to cut out as the output image
 * In the output image, alpha blending is performed only within this area.
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] rect            The rectangular area to be cut
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setRenderingRect(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_RectInt *rect);

/**
 * Set the rectangular area to be blended
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] rect            The rectangular area to be blended
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setCompositionRect(
    morpho_ImageStabilizer3 *stabilizer,
    morpho_RectInt *rect);

/**
 * Set the global subject blur detection level
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] level           The global subject blur detection level(0-7)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setObjBlurLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int level);
#endif

/**
 * Set the rotation blur detection level
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] level           The rotation blur detection level(0-7)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setRotationLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int level);

/**
 * Set the high-frequency straight lines detection level
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] level           The high-frequency straight lines detection level(0-7)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setHighFreqLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int level);

/**
 * Set the feature value detection level
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] level           The feature value detection level(0-7)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setFeaturelessLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int level);

/**
 * Set the local subject blur correction level
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] level           The local subject blur correction level(0-7)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setObjBlurCorrectionLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int level);

/**
 * Set the level of noise reduction strength
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] level           The level of noise reduction strength(0-7)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setNoiseReductionLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int level);

/**
 * Set the brightness amplification factor
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] gain            The brightness amplification factor(0-25500)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setGain(
    morpho_ImageStabilizer3 *stabilizer,
    int gain);

/**
 * Set the noise reduction type
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] type            The noise reduction type
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setNoiseReductionType(
    morpho_ImageStabilizer3 *stabilizer,
    int type);

/**
 * Set the input sequence type
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] type            The input sequence type
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setInputSequenceType(
    morpho_ImageStabilizer3 *stabilizer,
    int type);

#ifdef MORPHO_IMAGE_STABILIZER_VERSION_3_1
/**
 * Set the block size of ghost detection
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[out] block_size     The block size of ghost detection
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setObjBlurBlockSize(
    morpho_ImageStabilizer3 *stabilizer,
    int block_size);

/**
 * Set the shrink ratio
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] ratio           Shrink ratio
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setShrinkRatio(
    morpho_ImageStabilizer3 *stabilizer,
    int ratio);

/**
 * Set the Y image size
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] width           Y image width
 * @param[in] height          Y image height
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setFinestSize(
    morpho_ImageStabilizer3 *stabilizer,
    int width, int height);


/**
 * Set the level of noise reduction strength for Y image
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] level           The level of noise reduction strength for Y image(0-7)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setLumaNoiseReductionLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int level);

/**
 * Set the level of noise reduction strength for C image
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] level           The level of noise reduction strength for C image(0-7)
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setChromaNoiseReductionLevel(
    morpho_ImageStabilizer3 *stabilizer,
    int level);

/**
 * Set the noise reduction type for Y image
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] type            The noise reduction type for Y image
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setLumaNoiseReductionType(
    morpho_ImageStabilizer3 *stabilizer,
    int type);


/**
 * Set the noise reduction type for C image
 * After calling morpho_ImageStabilizer3_initialize(),
 * but before calling morpho_ImageStabilizer3_start().
 *
 * @param[in,out] stabilizer  Pointer to the image stabilizer
 * @param[in] type            The noise reduction type for C image
 * @return  Error code(morpho_error.h)
 */
MORPHO_API(int)
morpho_ImageStabilizer3_setChromaNoiseReductionType(
    morpho_ImageStabilizer3 *stabilizer,
    int type);
#endif    /* !MORPHO_IMAGE_STABILIZER_VERSION_3_1 */

//====================================================================

# ifdef __cplusplus
} // extern "C"
# endif

//--------------------------------------------------------------------

#endif // !MORPHO_IMAGE_STABILIZER3_H

//====================================================================
// [EOF]

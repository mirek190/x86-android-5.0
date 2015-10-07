/**
 * @file     morpho_motion_data.h
 * @brief    Motion data structure definition
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_MOTION_DATA_H
#define MORPHO_MOTION_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

/** Motion data. */
typedef struct {
    int v[6];  /**< Motion data */
    int fcode; /**< Failure code */
} morpho_MotionData;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_MOTION_DATA_H */

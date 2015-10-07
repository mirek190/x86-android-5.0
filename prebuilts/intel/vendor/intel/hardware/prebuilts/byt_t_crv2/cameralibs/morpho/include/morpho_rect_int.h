/**
 * @file     morpho_rect_int.h
 * @brief    Rectangular data structure definition
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006, 2007, 2008 Morpho, Inc.
 */

#ifndef MORPHO_RECT_INT_H
#define MORPHO_RECT_INT_H

#ifdef __cplusplus
extern "C" {
#endif

/** Rectangular data. */
typedef struct {
    int sx; /**< left */
    int sy; /**< top */
    int ex; /**< right */
    int ey; /**< bottom */
} morpho_RectInt;

/** Set the upper left coordinates and the lower right coordinates of the rectangular area. */
#define morpho_RectInt_setRect(rect,l,t,r,b) do { \
    (rect)->sx=(l);\
    (rect)->sy=(t);\
    (rect)->ex=(r);\
    (rect)->ey=(b);\
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* #ifndef MORPHO_RECT_INT_H */

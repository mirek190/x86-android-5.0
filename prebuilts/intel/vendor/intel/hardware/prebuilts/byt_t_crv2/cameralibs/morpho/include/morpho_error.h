/**
 * @file     morpho_error.h
 * @brief    Error code definitions
 * @version  1.0.0
 * @date     2008-06-09
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_ERROR_H
#define MORPHO_ERROR_H

/** Error code */
#define MORPHO_OK                   (0x00000000)  /**< Successful */
#define MORPHO_DOPROCESS            (0x00000001)  /**< Processing */
#define MORPHO_CANCELED             (0x00000002)  /**< Canceled */
#define MORPHO_SUSPENDED            (0x00000008)  /**< Suspended */

#define MORPHO_ERROR_GENERAL_ERROR  (0x80000000)  /**< General error */
#define MORPHO_ERROR_PARAM          (0x80000001)  /**< Invalid argument */
#define MORPHO_ERROR_STATE          (0x80000002)  /**< Invalid internal state or function call order */
#define MORPHO_ERROR_MALLOC         (0x80000004)  /**< Memory allocation error */
#define MORPHO_ERROR_IO             (0x80000008)  /**< Input/output error */
#define MORPHO_ERROR_UNSUPPORTED    (0x80000010)  /**< Unsupported function */
#define MORPHO_ERROR_NOTFOUND       (0x80000020)  /**< Is not found to be searched */
#define MORPHO_ERROR_INTERNAL       (0x80000040)  /**< Internal error */
#define MORPHO_ERROR_UNKNOWN        (0xC0000000)  /**< Error other than the above */

#endif /* #ifndef MORPHO_ERROR_H */

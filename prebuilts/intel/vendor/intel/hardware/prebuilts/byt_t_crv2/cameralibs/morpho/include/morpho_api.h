/**
 * @file     morpho_api.h
 * @brief    Defined macro API function
 * @version  1.0.0
 * @date     Tue Sep 21 17:37:35 2010
 *
 * Copyright (C) 2006-2012 Morpho, Inc.
 */

#ifndef MORPHO_API_H
#define MORPHO_API_H

/**  
 * Used to define the function API.
 * Can be switched by rewriting when you create a DLL in Windows.
 */
#if defined(MORPHO_DLL) && defined(_WIN32)
#define MORPHO_API(type) __declspec(dllexport) extern type
#else
#define MORPHO_API(type) extern type
#endif

#endif /* #ifndef MORPHO_API_H */

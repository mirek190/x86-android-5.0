/*
** Copyright 2012 Intel Corporation
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef __CPF_DRV_H__
#define __CPF_DRV_H__

#include <stdint.h>  /* defines integer types with specified widths */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t next_offset;
    uint16_t flags;
    uint16_t data_offset;
    uint16_t data_size;
} cpf_drv_header_t;

typedef enum
{
    MSR = (1 << 0)
} cpf_drv_flags_t;

#ifdef __cplusplus
}
#endif

#endif // __CPF_DRV_H__

#ifndef _ADD_TYPES_H_
#define _ADD_TYPES_H_

#include <stdint.h>

typedef struct {
    uint32_t acc_ctrl; /* Mandatory field for CSS runtime */
    uint32_t in_ptr_a;
    uint32_t in_ptr_b;
    uint32_t out_ptr;
    uint32_t num_values;
} add_params;

#endif /* _ADD_TYPES_H_ */

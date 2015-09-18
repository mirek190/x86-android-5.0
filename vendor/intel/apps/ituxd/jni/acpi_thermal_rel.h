#ifndef __ACPI_THERMAL_REL_H
#define __ACPI_THERMAL_REL_H

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>

#define ACPI_THERMAL_REL_MAGIC 's'

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

#define ACPI_THERMAL_REL_GET_TRT_LEN    _IOR(ACPI_THERMAL_REL_MAGIC, 1, unsigned long long)
#define ACPI_THERMAL_REL_GET_ART_LEN    _IOR(ACPI_THERMAL_REL_MAGIC, 2, unsigned long long)
#define ACPI_THERMAL_REL_GET_TRT_COUNT  _IOR(ACPI_THERMAL_REL_MAGIC, 3, unsigned long long)
#define ACPI_THERMAL_REL_GET_ART_COUNT  _IOR(ACPI_THERMAL_REL_MAGIC, 4, unsigned long long)
#define ACPI_THERMAL_REL_GET_TRT        _IOR(ACPI_THERMAL_REL_MAGIC, 5, unsigned long long)
#define ACPI_THERMAL_REL_GET_ART        _IOR(ACPI_THERMAL_REL_MAGIC, 6, unsigned long long)

union art_object {
    struct {
        char source_device[8]; /* ACPI single name */
        char target_device[8]; /* ACPI single name */
        u64 weight;
        u64 ac0_max_level;
        u64 ac1_max_level;
        u64 ac2_max_level;
        u64 ac3_max_level;
        u64 ac4_max_level;
        u64 ac5_max_level;
        u64 ac6_max_level;
        u64 ac7_max_level;
        u64 ac8_max_level;
        u64 ac9_max_level;
    }art;
    u64 __data[13];
};

union trt_object {
    struct trt {
        char source_device[8]; /* ACPI single name */
        char target_device[8]; /* ACPI single name */
        u64 influence;
        u64 sample_period;
        u64 reserved[4];
    }trt;
    u64 __data[8];
};

#endif /* __ACPI_ACPI_THERMAL_REL_H */

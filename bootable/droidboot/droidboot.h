#ifndef _DROIDBOOT_H_
#define _DROIDBOOT_H_

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#define MEGABYTE	(1024 * 1024)

#define DROIDBOOT_VERSION       "2.0"

#define MSEC_PER_SEC            (1000LL)

#define BATTERY_UNKNOWN_TIME    (2 * MSEC_PER_SEC)
#define POWER_ON_KEY_TIME       (2 * MSEC_PER_SEC)
#define UNPLUGGED_SHUTDOWN_TIME (30 * MSEC_PER_SEC)
#define CAPACITY_POLL_INTERVAL  (5 * MSEC_PER_SEC)
#define MODE_NON_CHARGER        0

#endif

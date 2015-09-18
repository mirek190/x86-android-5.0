
# Makefile for sensor general plugin driver
# This file is provided under a GPLv2 license

DRIVER_BASE=sensor_general_plugin
DRIVER_MAJOR=1
DRIVER_MINOR=0
DRIVER_NAME=${DRIVER_BASE}${DRIVER_MAJOR}_${DRIVER_MINOR}
INSTALL = .
EXTRA_CFLAGS += -I$(M)
EXTRA_CFLAGS += -Idrivers/external_drivers/drivers/hwmon

KERNEL_VERSION=$(shell uname -r | awk 'BEGIN{FS="[^0-9]"} {print $$3}')

obj-m := $(DRIVER_NAME).o
$(DRIVER_NAME)-objs := sensor_general_plugin.o

.PHONY: kernel_check

kernel_check:
ifeq "$(KERNEL_SRC_DIR)" ""
	echo "Error: makefile MUST NOT be invoked directly! Use the \"build_driver\" script instead."
	@exit 255
endif

default: kernel_check
	echo "************************************************************"
	echo "KERNEL_SRC_DIR=$(KERNEL_SRC_DIR)"
	make -C $(KERNEL_SRC_DIR) M=$(PWD) modules PWD=$(PWD)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

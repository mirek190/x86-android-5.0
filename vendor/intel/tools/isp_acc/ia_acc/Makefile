PREFIX_DIRS = 1 
USE_CSS_1_5 = 0

METHODS = host_lib
SYSTEM  = hive_isp_css_2400A0_system
# DLI_SYSTEM doesn't change between 2400, 2400A0 and 2401
DLI_SYSTEM = hive_isp_css_2400_system
TARGETS = crun sched
LIB     = ia_acc
CFLAGS  = -W -Wall -Werror

ifeq ("$(USE_CSS_1_5)", "1")
FILES   = ia_acc_csim_css_1_5.c
else
FILES   = ia_acc_csim_css_2_0.c
FILES   += $(HIVE_ACC_PKG)/build/dli_env/dli_env.c \
	   $(HIVE_ACC_PKG)/build/dli_env/$(DLI_SYSTEM)/memory_access.c \
	   $(HIVE_ACC_PKG)/build/dli_env/$(DLI_SYSTEM)/device_access.c

CPPFLAGS = -I$(HIVE_ACC_PKG)/build/ \
	   -I$(HIVE_ACC_PKG)/build/dli_env

CPPFLAGS += -DHRT_ADDRESS_WIDTH=64
endif

CPPFLAGS += -I$(HIVE_ACC_PKG)/build/css \
            -I$(HIVE_ACC_PKG)/build/css/hive_isp_css_include \
            -I$(HIVE_ACC_PKG)/build/css/hive_isp_css_include/host \
            -I$(HIVE_ACC_PKG)/build/css/hive_isp_css_include/device_access \
            -I$(HIVE_ACC_PKG)/build/css/hive_isp_css_include/memory_access \
            -I$(HIVE_ACC_PKG)/build/css/$(DLI_SYSTEM) \
            -I$(HIVE_ACC_PKG)/build/css/$(DLI_SYSTEM)/host \
            -DSYSTEM_$(SYSTEM) -D__HOST__

INSTALL_DIR = $(HIVE_CVSWORK)

include $(HIVEBIN)/../share/apps/hive_make.mk

ANDROID_ISP_ACC = $(ANDROID_BUILD_TOP)/vendor/intel/tools/isp_acc
ANDROID_IA_ACC = $(ANDROID_ISP_ACC)/ia_acc
ANDROID_LIB_SYSTEM = $(ANDROID_ISP_ACC)/ia_acc/$(SYSTEM)
ANDROID_ACC_TEST = $(ANDROID_ISP_ACC)/acc_test
package:
	# create content for $(ANDROID_IA_ACC)
	mkdir -p            $(ANDROID_IA_ACC)
	cp Makefile         $(ANDROID_IA_ACC)
	cp Android.mk       $(ANDROID_IA_ACC)
	cp ia_acc.h         $(ANDROID_IA_ACC)
	cp ia_acc_android.c $(ANDROID_IA_ACC)
	# copy library .a
	mkdir -p                            $(ANDROID_LIB_SYSTEM)
	cp host_lib/sched/libia_acc.sched.a $(ANDROID_LIB_SYSTEM)
	cp host_lib/crun/libia_acc.crun.a   $(ANDROID_LIB_SYSTEM)
	# create content for $(ANDROID_ACC_TEST)
	mkdir -p                       $(ANDROID_ACC_TEST)
	cp ../acc_test/*.c             $(ANDROID_ACC_TEST)
	cp ../acc_test/*.h             $(ANDROID_ACC_TEST)
	cp ../acc_test/Makefile        $(ANDROID_ACC_TEST)
	cp ../acc_test/Makefile.fw     $(ANDROID_ACC_TEST)
	cp ../acc_test/Android.mk      $(ANDROID_ACC_TEST)
	cp -rf ../acc_test/isp_include $(ANDROID_ACC_TEST)


PREFIX_DIRS = 1 
USE_CSS_RUNTIME=1
NO_SHARE_INTERMEDIATES=1
SYSTEM         = hive_isp_css_2400_system
METHODS        = crun sched 
HOST_FILES     = host.c

ifeq ("$(USE_CSS_RUNTIME)", "1")
HOST_CPPFLAGS += -DCSS_RUNTIME
crun_fw:
	$(MAKE) firmware -f Makefile.fw METHOD=crun HOST_FILES=

sched_fw:
	$(MAKE) firmware -f Makefile.fw METHOD=sched HOST_FILES=

clean_fw:
	$(MAKE) clean -f Makefile.fw

android_fw:
	$(MAKE) firmware -f Makefile.fw METHOD=target HOST_FILES= HOST_LDFLAGS=

crun: crun_fw
sched: sched_fw
clean: clean_fw
android: android_fw


else
PROGRAMS   = add
add_CELL    = isp2400_mamoiada
add_FILES   = add.hive.c
add_CFLAGS  = -verbose -html -O2 -I../isp_include
add_LDFLAGS = -embed -verbose
endif

ifeq ("$(USE_CSS_RUNTIME)", "1")
HOST_CPPFLAGS += -DSYSTEM=$(SYSTEM) -I ../../ia_acc
HOST_CPPFLAGS += -D__HOST__ -DCSS_ACC_SIMULATION -DENABLE_DMA_PROXY
endif

HOST_LDFLAGS   = -lhmm_64 -lhil
ifeq ("$(USE_CSS_RUNTIME)", "1")
HOST_LDFLAGS  += -lia_acc -L$(HIVE_CVSWORK)/lib \
                 -L$(HIVE_CVSWORK)/ia_acc/$(SYSTEM) \
                 -L$(HIVE_ACC_PKG)/lib/css/$(SYSTEM)_host_lib/crun \
                 -L$(HIVE_ACC_PKG)/lib/css/$(SYSTEM)_host_lib/sched -lsh_css
endif

SIMULATOR_ARGS = -no-sdk-version-check -no-warnings -no-volatile-check -dma-listing -listing-per-processor

crun_SIMULATOR_ARGS = $(SIMULATOR_ARGS) \
                      -add-binary-path $(HIVE_ACC_PKG)/crun \
                      -add-binary-path $(HIVE_CVSWORK)/acc_test/$(SYSTEM)_crun

sched_SIMULATOR_ARGS = $(SIMULATOR_ARGS) \
                       -add-binary-path $(HIVE_ACC_PKG)/sched \
                       -add-binary-path $(HIVE_CVSWORK)/acc_test/$(SYSTEM)_sched

HIVE_OVERLOADING = 1

include $(HIVEBIN)/../share/apps/hive_make.mk



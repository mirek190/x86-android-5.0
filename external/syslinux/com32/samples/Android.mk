LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/../com32_build_prebuilt.mk

C32_MODULES := \
	advdump.c32 \
	entrydump.c32 \
	fancyhello.c32 \
	hello.c32 \
	keytest.c32 \
	localboot.c32 \
	resolv.c32 \
	serialinfo.c32

$(foreach module,$(C32_MODULES),$(eval $(call com32_build_prebuilt,$(module))))

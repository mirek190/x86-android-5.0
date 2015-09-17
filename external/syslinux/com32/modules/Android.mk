LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/../com32_build_prebuilt.mk

C32_MODULES := \
	android.c32 \
	cat.c32 \
	chain.c32 \
	cmd.c32 \
	config.c32 \
	cpuid.c32 \
	cpuidtest.c32 \
	disk.c32 \
	dmitest.c32 \
	elf.c32 \
	ethersel.c32 \
	gpxecmd.c32 \
	host.c32 \
	ifcpu64.c32 \
	ifcpu.c32 \
	ifplop.c32 \
	kbdmap.c32 \
	linux.c32 \
	ls.c32 \
	meminfo.c32 \
	pcitest.c32 \
	pmload.c32 \
	pwd.c32 \
	reboot.c32 \
	sanboot.c32 \
	sdi.c32 \
	vesainfo.c32 \
	vpdtest.c32 \
	whichsys.c32

$(foreach module,$(C32_MODULES),$(eval $(call com32_build_prebuilt,$(module))))

#
# Makefile for gc class sensor driver
#
obj-m += gc_class.o

gc_class-objs := gc.o 


ccflags-y += $(addprefix -DCONFIG_GC, $(subst m, _0310, $(CONFIG_VIDEO_GC0310SOC)))
ccflags-y += $(addprefix -DCONFIG_GC, $(subst m, _2155, $(CONFIG_VIDEO_GC2155)))


ccflags-y += -Werror

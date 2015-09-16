LOCAL_PATH := $(call my-dir)

# Valgrind
include $(CLEAR_VARS)
LOCAL_MODULE := valgrind_pack
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := \
    libvex-$(TARGET_ARCH)-linux \
    libcoregrind-$(TARGET_ARCH)-linux \
    libreplacemalloc_toolpreload-$(TARGET_ARCH)-linux \
    vgpreload_core-$(TARGET_ARCH)-linux \
    memcheck-$(TARGET_ARCH)-linux \
    vgpreload_memcheck-$(TARGET_ARCH)-linux \
    cachegrind-$(TARGET_ARCH)-linux \
    callgrind-$(TARGET_ARCH)-linux \
    helgrind-$(TARGET_ARCH)-linux \
    vgpreload_helgrind-$(TARGET_ARCH)-linux \
    drd-$(TARGET_ARCH)-linux \
    vgpreload_drd-$(TARGET_ARCH)-linux \
    massif-$(TARGET_ARCH)-linux \
    vgpreload_massif-$(TARGET_ARCH)-linux \
    none-$(TARGET_ARCH)-linux \
    valgrind \
    default.supp
include $(BUILD_PHONY_PACKAGE)


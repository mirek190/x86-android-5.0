
LOCAL_MODULE_SUFFIX := .efi

EFI_TARGET := efi-app-$(EFI_ARCH)

LOCAL_STATIC_LIBRARY := libgnuefi
LOCAL_IMPORT_C_INCLUDE_DIRS_FROM_STATIC_LIBRARIES := libgnuefi
LOCAL_MODULE_EXTENSION := so

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_FORCE_STATIC_EXECUTABLE := true

GNUEFI_PATH := $(call intermediates-dir-for, STATIC_LIBRARIES, libgnuefi)
EFI_CRT0 := $(GNUEFI_PATH)/gnuefi/crt0-efi-$(EFI_ARCH).o
LDS := $(PRODUCT_OUT)/elf_$(EFI_ARCH)_efi.lds
EFI_SIGNING_TOOL := $(HOST_OUT_EXECUTABLES)/isu

LOCAL_CFLAGS += $(EFI_EXTRA_CFLAGS)

include $(BUILD_SYSTEM)/binary.mk

$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_TARGET_GLOBAL_CFLAGS :=

EFI_APP_OBJS := $(patsubst %.c, %.o , $(LOCAL_SRC_FILES))
EFI_APP_OBJS := $(patsubst %.S, %.o , $(EFI_APP_OBJS))
EFI_APP_OBJS := $(patsubst %.bmp, %.o , $(EFI_APP_OBJS))
EFI_APP_OBJS := $(addprefix $(intermediates)/, $(EFI_APP_OBJS))
EFI_APP_OBJS += $(built_static_libraries)

# rules for splash.bmp
$(intermediates)/%.o: $(LOCAL_PATH)/%.bmp | $(HOST_OUT_EXECUTABLES)/prebuilt-bin-to-hex
	$(hide) mkdir -p $(@D)
	$(hide) prebuilt-bin-to-hex $(basename $(notdir $@)) < $< | $(TARGET_CC) -x c - -c $(TARGET_GLOBAL_CFLAGS) $(EFI_ARCH_CFLAGS_$(EFI_ARCH)) -o $@

EFI_LINKED := $(basename $(LOCAL_BUILT_MODULE))
$(EFI_LINKED): EFI_OBJS := $(EFI_APP_OBJS) $(EFI_CRT0)
$(EFI_LINKED): $(EFI_APP_OBJS) $(GNUEFI_PATH)/libgnuefi.a $(LDS) $(all_objects)
	@echo "linking $(notdir $@)"
	$(hide) $(TARGET_TOOLS_PREFIX)ld$(HOST_EXECUTABLE_SUFFIX).bfd \
		-Bsymbolic \
		-shared \
		-z relro -z now \
		-fstack-protector \
		-fPIC -pie \
		-nostdlib \
		-znocombreloc \
		-L$(EFI_TARGET_LIBGCC) \
		-L$(GNUEFI_PATH) \
		-T $(LDS) \
		$(EFI_OBJS) -lgnuefi -lgcc -o $@
	@echo "checking symbols in $(notdir $@)"
	@$(hide) unknown=`nm -u $@` ; if [ -n "$$unknown" ] ; then echo "Unknown symbol(s): $$unknown" && exit -1 ; fi

EFI_UNSIGNED := $(basename $(LOCAL_BUILT_MODULE)).unsigned.efi
$(EFI_UNSIGNED): $(EFI_LINKED)
	@echo "objcopy $(notdir $@)"
	$(hide) $(TARGET_OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		-j .rela -j .reloc --target=$(EFI_TARGET) -S $< $@

$(LOCAL_BUILT_MODULE): EFI_MANIFEST_OUT := $(EFI_LINKED)_manifest__OS_manifest.bin
$(LOCAL_BUILT_MODULE): $(EFI_UNSIGNED) $(EFI_SIGNING_TOOL)
	@mkdir -p $(@D)
	@echo "Signing $(notdir $@) with $(EFI_SIGNING_TOOL)"
	$(hide) $(EFI_SIGNING_TOOL) -h 0 -i $< -o $@ \
		-l $(TARGET_BOOT_LOADER_PRIV_KEY) -k $(TARGET_BOOT_LOADER_PUB_KEY) -t 11 -p 2 -v 1 > /dev/null
	$(hide) cat $< $@_OS_manifest.bin > $@


# At the moment we only generate prebuilts for userdebug builds
# this is a safety feature, eng, user, and userdebug binaries should be the same
# so intel_prebuilts should be used only for one variant anyway.
ifeq (userdebug,$(TARGET_BUILD_VARIANT))


# Prebuilts generation for external release is now done in two steps
# 1/ A full build is done
# 2/ Based on the files generated in 1/, prepare the prebuilts
#
# Some modules do specific processing when prebuilts are generated.
# Therefore, specific variables must be set for both all prebuilt targets.
#
# * intel_prebuilts target is built together with full image
#   It also restarts a make with publish_intel_prebuilts target
# * generate_intel_prebuilts target will pick built files and package them
#   in out folder.
# * publish_intel_prebuilts target takes the prebuilts from out folder
#   packages them in a zip and publishes the zip in pub


ifneq (,$(filter \
    intel_prebuilts generate_intel_prebuilts publish_intel_prebuilts, \
    $(MAKECMDGOALS)))
# GENERATE_INTEL_PREBUILTS is used to indicate we are generating intel_prebuilts
# so that the tests above are not duplicated in different portions of the code.
GENERATE_INTEL_PREBUILTS:=true

TARGET_OUT_prebuilts := $(PRODUCT_OUT)/prebuilts/intel

# for easy porting to legacy branches, we setup REF_PRODUCT_NAME
ifeq ($(REF_PRODUCT_NAME),)
REF_PRODUCT_NAME:=$(TARGET_PRODUCT)
endif

endif # intel_prebuilts || generate_intel_prebuilts || publish_intel_prebuilts


ifneq (,$(filter generate_intel_prebuilts publish_intel_prebuilts,$(MAKECMDGOALS)))
# Projects that require prebuilt are defined in manifest as follow:
# - project belongs to bsp-priv manifest group
# - and project has g_external annotation set to 'bin' ('g' meaning 'generic' customer)
$(eval _prebuilt_projects := $(shell repo forall -g bsp-priv -a g_external=bin -c 'echo $$REPO_PATH'))

# get the original path of the hooked build makefiles
define original-metatarget
$(strip \
  $(eval _LOCAL_BUILD_MAKEFILE := $$(lastword $$(MAKEFILE_LIST))) \
  $(BUILD_SYSTEM)/$(notdir $(_LOCAL_BUILD_MAKEFILE)))
endef

# $(1) : line to echo to the makefile
define external-echo-makefile
       echo $(1) >>$@;
endef

# $(1) : metatarget
# $(2) : MULTI_PREBUILT variable
# $(3) : LOCAL_BUILT_MODULE suffix variable
#
# .LOCAL_STRIP_MODULE is forced to false
#
# Two modules can have identical LOCAL_MODULE (e.g. target and host modules)
# or same LOCAL_MODULE_STEM with 2 different LOCAL_MODULE.
# To be sure to use a unique key when gathering files,
# the key used is $(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM)
#
define external-gather-files
$(if $(filter $(1),$(_metatarget)), \
    $(if $(findstring /PRIVATE/, $(LOCAL_SRC_FILES) $(LOCAL_PREBUILT_MODULE_FILE)), \
        $(info PRIVATE module [$(LOCAL_MODULE)] cannot use another PRIVATE module files directly to prevent IP violation) \
        $(foreach s, $(LOCAL_SRC_FILES) $(LOCAL_PREBUILT_MODULE_FILE), $(if $(findstring /PRIVATE/, $(s)), $(info >    [$(s)]))) \
        $(error Stop) \
    ) \
    $(eval LOCAL_INSTALLED_MODULE_STEM := $(my_installed_module_stem)) \
    $(eval $(my).$(module_type).$(2).LOCAL_INSTALLED_STEM_MODULES := $($(my).$(module_type).$(2).LOCAL_INSTALLED_STEM_MODULES) $(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM)) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_MODULE := $(strip $(LOCAL_MODULE))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_IS_HOST_MODULE := $(strip $(LOCAL_IS_HOST_MODULE))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_MODULE_CLASS := $(strip $(LOCAL_MODULE_CLASS))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_MODULE_TAGS := $(strip $(LOCAL_MODULE_TAGS))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).OVERRIDE_BUILT_MODULE_PATH := $(strip $(subst $(HOST_OUT),$$$$(HOST_OUT),$(subst $(PRODUCT_OUT),$$$$(PRODUCT_OUT),$(OVERRIDE_BUILT_MODULE_PATH))))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_UNINSTALLABLE_MODULE := $(strip $(LOCAL_UNINSTALLABLE_MODULE))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_BUILT_MODULE_STEM := $(strip $(my_built_module_stem))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_STRIP_MODULE := ) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_REQUIRED_MODULES := $(strip $(LOCAL_REQUIRED_MODULES))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_SHARED_LIBRARIES := $(strip $(LOCAL_SHARED_LIBRARIES))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_INSTALLED_MODULE_STEM := $(strip $(LOCAL_INSTALLED_MODULE_STEM))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_CERTIFICATE := $(strip $(notdir $(LOCAL_CERTIFICATE)))) \
    $(eval $(my).$(module_type).$(2).$(LOCAL_MODULE).$(LOCAL_INSTALLED_MODULE_STEM).LOCAL_MODULE_PATH := $(strip $(subst $(HOST_OUT),$$$$(HOST_OUT),$(subst $(PRODUCT_OUT),$$$$(PRODUCT_OUT),$(my_module_path))))) \
    $(eval ### prebuilts must copy the original source file as some post-processing may happen on the built file - others copy the built file) \
    $(if $(filter prebuilt,$(_metatarget)), \
        $(eval _input_file := $(ext_prebuilt_src_file)), \
        $(if $(filter java_library,$(_metatarget)), \
            $(eval ### get unstripped jar) \
            $(eval _input_file := $(common_javalib.jar)), \
            $(eval _input_file := $(LOCAL_BUILT_MODULE)$(3)) \
        ) \
    ) \
    $(eval $(my).copyfiles := $($(my).copyfiles) $(_input_file):$(dir $(my))$(module_type)/$(LOCAL_INSTALLED_MODULE_STEM)) \
    $(eval ### for java libraries, also keep jar with classes) \
    $(if $(filter java_library,$(_metatarget)), \
        $(eval $(my).copyfiles := $($(my).copyfiles) $(full_classes_jar):$(dir $(my))$(module_type)/$(LOCAL_INSTALLED_MODULE_STEM).classes.jar) \
    ) \
)
endef

define external-phony-package-boilerplate
  $(call external-echo-makefile, '') \
  $(call external-echo-makefile,'include $$(CLEAR_VARS)') \
  $(call external-echo-makefile,'LOCAL_MODULE:=$(strip $(1))') \
  $(call external-echo-makefile,'LOCAL_MODULE_TAGS:=optional') \
  $(call external-echo-makefile,'LOCAL_REQUIRED_MODULES:=$(strip $(2))') \
  $(call external-echo-makefile,'include $$(BUILD_PHONY_PACKAGE)')
endef

# $(1): file list
# $(2): IS_HOST_MODULE
# $(3): MODULE_CLASS
# $(4): MODULE_TAGS
# $(5): OVERRIDE_BUILT_MODULE_PATH
# $(6): UNINSTALLABLE_MODULE
# $(7): BUILT_MODULE_STEM
# $(8): LOCAL_STRIP_MODULE
# $(9): LOCAL_MODULE
# $(10): LOCAL_MODULE_STEM
# $(11): LOCAL_CERTIFICATE
# $(12): LOCAL_MODULE_PATH
# $(13): LOCAL_REQUIRED_MODULES
# $(14): LOCAL_SHARED_LIBRARIES
#
define external-auto-prebuilt-boilerplate
$(if $(filter %: :%,$(1)), \
  $(error $(LOCAL_PATH): Leading or trailing colons in "$(1)")) \
$(foreach t,$(1), \
  $(call external-echo-makefile, '') \
  $(call external-echo-makefile, 'include $$(CLEAR_VARS)') \
  $(call external-echo-makefile, 'LOCAL_IS_HOST_MODULE:=$(strip $(2))') \
  $(call external-echo-makefile, 'LOCAL_MODULE_CLASS:=$(strip $(3))') \
  $(call external-echo-makefile, 'LOCAL_MODULE_TAGS:=$(strip $(4))') \
  $(call external-echo-makefile, 'OVERRIDE_BUILT_MODULE_PATH:=$(strip $(5))') \
  $(call external-echo-makefile, 'LOCAL_UNINSTALLABLE_MODULE:=$(strip $(6))') \
  $(call external-echo-makefile, 'LOCAL_SRC_FILES:=$(strip $(t))') \
  $(if $(7), \
    $(call external-echo-makefile, 'LOCAL_BUILT_MODULE_STEM:=$(strip $(7))') \
   , \
    $(call external-echo-makefile, 'LOCAL_BUILT_MODULE_STEM:=$(strip $(notdir $(t)))') \
   ) \
  $(call external-echo-makefile, 'LOCAL_STRIP_MODULE:=$(strip $(8))') \
  $(call external-echo-makefile, 'LOCAL_MODULE:=$(strip $(9))') \
  $(call external-echo-makefile, 'LOCAL_MODULE_STEM:=$(strip $(10))') \
  $(call external-echo-makefile, 'LOCAL_CERTIFICATE:=$(strip $(11))') \
  $(call external-echo-makefile, 'LOCAL_MODULE_PATH:=$(strip $(12))') \
  $(call external-echo-makefile, 'LOCAL_REQUIRED_MODULES:=$(strip $(13))') \
  $(call external-echo-makefile, 'LOCAL_SHARED_LIBRARIES:=$(strip $(14))') \
  $(call external-echo-makefile, 'LOCAL_EXPORT_C_INCLUDE_DIRS:=$$(LOCAL_PATH)/include') \
  $(call external-echo-makefile, 'include $$(BUILD_PREBUILT)') \
 )
endef

# Copy several files.
# $(1): The files to copy.  Each entry is a ':' separated src:dst pair
# Note: Explicitly fail when attempting to copy a directory as acp does not return an error
define copy-several-files
$(foreach f, $(1), \
    $(eval _cmf_tuple := $(subst :, ,$(f))) \
    $(eval _cmf_src := $(word 1,$(_cmf_tuple))) \
    $(eval _cmf_dest := $(word 2,$(_cmf_tuple))) \
    ( mkdir -p $(dir $(_cmf_dest)); \
    $(ACP) $(_cmf_src) $(_cmf_dest) && \
    test ! -d $(_cmf_src) ) && ) true;
endef

# List several files dependencies.
# $(1): The files to compute.  Each entry is a ':' separated src:dst pair
define several-files-deps
$(foreach f, $(1), $(strip \
    $(eval _cmf_tuple := $(subst :, ,$(f))) \
    $(eval _cmf_src := $(word 1,$(_cmf_tuple))) \
    $(_cmf_src)))
endef

EXTERNAL_BUILD_SYSTEM=device/intel/common/external

# hook all the build makefiles with our own version
# most of them are only symlinks to "unsupported.mk", which will generate an
# error if included from a "PRIVATE" dir
# others are symlink to generic_rules.mk
# we cannot directly point to unsupported or generic_rules, because we would loose
# the information on what we are building
BUILD_HOST_STATIC_LIBRARY:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/host_static_library.mk
BUILD_HOST_SHARED_LIBRARY:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/host_shared_library.mk
BUILD_STATIC_LIBRARY:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/static_library.mk
BUILD_RAW_STATIC_LIBRARY := $(EXTERNAL_BUILD_SYSTEM)/symlinks/raw_static_library.mk
BUILD_SHARED_LIBRARY:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/shared_library.mk
BUILD_EXECUTABLE:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/executable.mk
BUILD_RAW_EXECUTABLE:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/raw_executable.mk
BUILD_HOST_EXECUTABLE:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/host_executable.mk
BUILD_PACKAGE:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/package.mk
BUILD_PHONY_PACKAGE:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/phony_package.mk
BUILD_HOST_PREBUILT:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/host_prebuilt.mk
BUILD_PREBUILT:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/prebuilt.mk
BUILD_JAVA_LIBRARY:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/java_library.mk
BUILD_STATIC_JAVA_LIBRARY:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/static_java_library.mk
BUILD_HOST_JAVA_LIBRARY:= $(EXTERNAL_BUILD_SYSTEM)/symlinks/host_java_library.mk
BUILD_COPY_HEADERS := $(EXTERNAL_BUILD_SYSTEM)/symlinks/copy_headers.mk
BUILD_NATIVE_TEST := $(EXTERNAL_BUILD_SYSTEM)/symlinks/native_test.mk
BUILD_HOST_NATIVE_TEST := $(EXTERNAL_BUILD_SYSTEM)/symlinks/host_native_test.mk
BUILD_CUSTOM_EXTERNAL := $(EXTERNAL_BUILD_SYSTEM)/symlinks/custom_external.mk

# No need to define rules for wrappers around targets we already support
# BUILD_MULTI_PREBUILT -> relies on BUILD_PREBUILT

endif # generate_intel_prebuilts || publish_intel_prebuilts
endif # userdebug

# Convenient function to translate the path from internal to external.
# It's available regardless of the prebuilt generation.
#
# $(1) : Local path to be translated in prebuilt
#
define intel-prebuilts-path
prebuilts/intel/$(subst /PRIVATE/,/prebuilts/$(REF_PRODUCT_NAME)/,$(1))
endef

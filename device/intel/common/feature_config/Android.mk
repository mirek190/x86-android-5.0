# Add features to the list as new features are supported in the build tree
INTEL_SUPPORTED_FEATURES := \
	INTEL_FEATURE_ARKHAM \
	INTEL_FEATURE_AWARESERVICE \
	INTEL_FEATURE_LPAL \
	INTEL_FEATURE_ASF \
	INTEL_FEATURE_PERM_LIC \
	INTEL_FEATURE_ADAPTIVE_AUTHENTICATION \
	INTEL_FEATURE_SUPL20_ECID

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := com.intel.config
LOCAL_MODULE_TAGS := optional

# Get the directory where intermediates will be built
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
intermediates := $(local-intermediates-dir)

# Prepare the java code to inject - pack syntax to avoid extra white spaces
str :=
$(foreach flag, $(INTEL_SUPPORTED_FEATURES), \
	$(eval val:=$(if $($(flag)),$($(flag)),false)) \
	$(eval str:=$(str)\n    public static final boolean $(flag) = $(val);\n) \
)

# Generate the java source from the template
TEMPLATE := $(LOCAL_PATH)/FeatureConfig.template
GEN := $(intermediates)/com/intel/config/FeatureConfig.java
$(GEN): P_FLAGS_STR := $(str)
$(GEN): P_TEMPLATE := $(TEMPLATE)
$(GEN): $(TEMPLATE)
	@echo "target Generated: $(PRIVATE_MODULE) <= $<"
	@mkdir -p $(dir $@)
	$(hide) sed "s|^.*//__FEATURE_FLAGS__|$(P_FLAGS_STR)|" $(P_TEMPLATE) > $@

LOCAL_GENERATED_SOURCES := $(GEN)

# The build system requires at least one sourcefile
LOCAL_SRC_FILES := Comments.java
LOCAL_JAVA_LIBRARIES := core-libart
LOCAL_NO_STANDARD_LIBRARIES := true

include $(BUILD_JAVA_LIBRARY)

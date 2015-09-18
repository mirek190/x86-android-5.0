ifeq ($(call intel-target-need-intel-libraries),)
LOCAL_PATH := $(call my-dir)

# Use libraries from ICC version 13.0
ICC_LIBCHKP_SO    := lib/libchkp.so
ICC_LIBCILKRTS_SO := lib/gnustl/libcilkrts.so
ICC_LIBIMF_SO     := lib/libimf.so
ICC_LIBINTLC_SO   := lib/libintlc.so
ICC_LIBIRNG_SO    := lib/libirng.so
ICC_LIBPDBX_SO    := lib/gnustl/libpdbx.so
ICC_LIBSVML_SO    := lib/libsvml.so

ICC_LIBBFP754_A   := lib/libbfp754.a
ICC_LIBDECIMAL_A  := lib/libdecimal.a
ICC_LIBIMF_A      := lib/libimf.a
ICC_LIBIPGO_A     := lib/libipgo.a
ICC_LIBIRNG_A     := lib/libirng.a
ICC_LIBIRC_A      := lib/libirc.a
ICC_LIBIRC_S_A    := lib/libirc_s.a
# Renamed to aviod conflict with vendor/intel/PRIVATE/ipp/libsvml.a
ICC_LIBSVML_A     := lib/libsvml_14_0.a

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS :=                       \
                       $(ICC_LIBCHKP_SO)     \
                       $(ICC_LIBCILKRTS_SO)  \
                       $(ICC_LIBIMF_SO)      \
                       $(ICC_LIBINTLC_SO)    \
                       $(ICC_LIBIRNG_SO)     \
                       $(ICC_LIBPDBX_SO)     \
                       $(ICC_LIBSVML_SO)     \
                                             \
                       $(ICC_LIBBFP754_A)    \
                       $(ICC_LIBDECIMAL_A)   \
                       $(ICC_LIBIMF_A)       \
                       $(ICC_LIBIPGO_A)      \
                       $(ICC_LIBIRC_A)       \
                       $(ICC_LIBIRC_S_A)     \
                       $(ICC_LIBIRNG_A)      \
                       $(ICC_LIBSVML_A)

include $(BUILD_MULTI_PREBUILT)
endif


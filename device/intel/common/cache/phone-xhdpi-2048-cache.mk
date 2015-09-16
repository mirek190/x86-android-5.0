# please modify to appropriate value based on tuning

PRODUCT_PROPERTY_OVERRIDES += \
    ro.hwui.texture_cache_size=72 \
    ro.hwui.layer_cache_size=48 \
    ro.hwui.r_buffer_cache_size=8 \
    ro.hwui.gradient_cache_size=1 \
    ro.hwui.path_cache_size=32 \
    ro.hwui.drop_shadow_cache_size=6 \
    ro.hwui.texture_cache_flushrate=0.4 \
    ro.hwui.text_small_cache_width=1024 \
    ro.hwui.text_small_cache_height=1024 \
    ro.hwui.text_large_cache_width=2048 \
    ro.hwui.text_large_cache_height=1024
#    ro.hwui.fbo_cache_size=16
#    ro.hwui.patch_cache_size=128
#    ro.hwui.disable_scissor_opt=false
#    ro.hwui.use_gpu_pixel_buffers=true
#    hwui.text_gamma_correction=lookup
#    hwui.text_gamma=1.4
#    hwui.text_gamma.black_threshold=64
#    hwui.text_gamma.white_threshold=192

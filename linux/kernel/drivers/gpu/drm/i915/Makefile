#
# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI) in XFree86 4.1.0 and higher.

ccflags-y := -Iinclude/drm -Iinclude/uapi/drm -I../modules/intel_media/common -Werror
i915-y := i915_drv.o i915_dma.o i915_irq.o \
	  i915_debugfs.o \
	  i915_gpu_error.o \
          i915_suspend.o \
	  i915_gem.o \
	  i915_gem_userptr.o \
	  i915_gem_context.o \
	  i915_gem_debug.o \
	  i915_gem_evict.o \
	  i915_gem_execbuffer.o \
	  i915_gem_gtt.o \
	  i915_gem_stolen.o \
	  i915_gem_tiling.o \
	  i915_sysfs.o \
	  i915_trace_points.o \
	  i915_ums.o \
	  i915_rpm.o \
	  i915_perfmon.o \
	  intel_display.o \
	  intel_crt.o \
	  intel_lvds.o \
	  intel_dsi.o \
	  intel_dsi_cmd.o \
	  intel_dsi_pll.o \
	  intel_bios.o \
	  intel_ddi.o \
	  intel_dp.o \
	  intel_hdmi.o \
	  intel_sdvo.o \
	  intel_modes.o \
	  intel_panel.o \
	  intel_pm.o \
	  intel_i2c.o \
	  intel_fb.o \
	  intel_tv.o \
	  intel_dvo.o \
	  intel_ringbuffer.o \
	  intel_overlay.o \
	  intel_sprite.o \
	  intel_opregion.o \
	  intel_sideband.o \
	  intel_uncore.o \
	  intel_clrmgr.o \
	  intel_sync.o \
	  dvo_ch7xxx.o \
	  dvo_ch7017.o \
	  dvo_ivch.o \
	  dvo_tfp410.o \
	  dvo_sil164.o \
	  dvo_ns2501.o \
	  hdmi_audio_if.o \
	  i915_gem_dmabuf.o	\
	  i915_dpst.o \
	  i915_cmd_parser.o \
	  dsi_mod_auo_b080xat.o	\
	  dsi_mod_panasonic_vvx09f006a00.o	\
	  dsi_mod_auo_b101uan01.o	\
	  dsi_mod_jdi_lpm070w425b.o	\
	  dsi_mod_vbt_generic.o	\
	  dsi_mod_auo_b080ean011.o \
	  dsi_mod_auo_b080xan020.o \
	  dsi_mod_rayken_rk785x32a1ci.o \
	  dsi_mod_mjk_m080wx01b.o \
	  dsi_mod_kdt_kd070d30.o \
	  dsi_mod_cpt_claa080wq05.o \
	  dsi_mod_innolux_n080ice_gb1.o \
	  dsi_mod_sdc_bp080wx7.o \
	  dsi_mod_boe_bp080wx7.o \
	  dsi_mod_rayken_rk089wu45j1ai.o \
	  dsi_mod_cpt_claa080fp01.o \
	  dsi_mod_boe_kd079d5_31nb_a9.o	\
	  tc35876x_vid.o \
	  edp_bridge_ps8622.o	\
	  dsi_mod_kdt_kd080d10_31na_a11.o \
	  dsi_mod_auo_b101uan017.o \
	  dsi_mod_ivo_kr079ia1t.o

i915-$(CONFIG_COMPAT)   += i915_ioc32.o

i915-$(CONFIG_ACPI)	+= intel_acpi.o

obj-$(CONFIG_DRM_I915)  += i915.o

i915-$(CONFIG_DSI_MODE_P088PW_3G081I)   += dsi_mod_p088pw.o
i915-$(CONFIG_DSI_MODE_KW101JEA_3G086I)   += dsi_mod_kw101jea.o
i915-$(CONFIG_DSI_MODE_AUO_3G086I)   += dsi_mod_auo.o
i915-$(CONFIG_DSI_MODE_NT35521_QC7_3G069I)   += dsi_mod_auo_nt35521_qc7.o
i915-$(CONFIG_DSI_MODE_KW080KD_3G080IH)   += dsi_mod_kw080kd.o
i915-$(CONFIG_DSI_MODE_NT35521_QC10_3G085I)   += dsi_mod_auo_nt35521_qc10.o
i915-$(CONFIG_DSI_MODE_KD080D14_H8I)   += dsi_mod_kd080d14.o
i915-$(CONFIG_DSI_MODE_LSL1080A102_NB31)   += dsi_mod_ps_lsl080al02.o


CFLAGS_i915_trace_points.o := -I$(src)

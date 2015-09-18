#EXTRA CFLAGS
ccflags-y += -Werror

# Medfield board
snd-soc-mfld-machine-objs := mfld_common.o mfld_machine.o
obj-$(CONFIG_SND_MFLD_MACHINE) += snd-soc-mfld-machine.o

# Gilligan Island/Lexington board
snd-soc-mfld-machine-gi-objs := mfld_common.o mfld_machine_gi.o
obj-$(CONFIG_SND_MFLD_MACHINE_GI) += snd-soc-mfld-machine-gi.o

# Yukkabeach board
snd-soc-yukkabeach-machine-objs := mfld_common.o yukkabeach_machine.o
obj-$(CONFIG_SND_YUKKABEACH_MACHINE) += snd-soc-yukkabeach-machine.o

# Cloverview/Clovertrail+/Bodegabay board
snd-soc-ctp-machine-objs := ctp_common.o ctp_rhb_cs42l73.o ctp_vb_cs42l73.o ctp_comms_common.o merr_bb_cs42l73.o ctp_ht_wm5102.o ctp_lt_wm8994.o
snd-soc-ctp-machine-5671-objs := ctp_common_rt5671.o ctp_rhb_rt5671.o
obj-$(CONFIG_SND_CTP_MACHINE) += snd-soc-ctp-machine.o
obj-$(CONFIG_SND_CTP_MACHINE_5671) += snd-soc-ctp-machine-5671.o

# Merrifield board
snd-merr-saltbay-lm49453-objs := merr_saltbay_lm49453.o
snd-merr-saltbay-wm8958-objs := merr_saltbay_wm8958.o
snd-merr-dpcm-wm8958-objs := merr_dpcm_wm8958.o

# Cherrytrail board
snd-cht-bl-rt5672-objs := cht_bl_rt5672.o
snd-cht-dpcm-rt5672-objs := cht_bl_dpcm_rt5672.o
snd-cht-cr-dpcm-aic3100-objs := cht_cr_dpcm_aic3100.o

# Moorefield board
snd-moor-dpcm-florida-objs := moor_dpcm_florida.o

ifdef CONFIG_SST_DPCM
	obj-$(CONFIG_SND_CHT_MACHINE) += snd-cht-dpcm-rt5672.o snd-cht-cr-dpcm-aic3100.o
	obj-$(CONFIG_SND_MRFLD_MACHINE) += snd-merr-dpcm-wm8958.o
else
	obj-$(CONFIG_SND_CHT_MACHINE) += snd-cht-bl-rt5672.o
	obj-$(CONFIG_SND_MRFLD_MACHINE) += snd-merr-saltbay-lm49453.o snd-merr-saltbay-wm8958.o
endif

# Moorefield board
obj-$(CONFIG_SND_MOOR_MACHINE) += snd-merr-dpcm-wm8958.o snd-moor-dpcm-florida.o
obj-$(CONFIG_SND_MOOR_MACHINE) += snd-merr-saltbay-wm8958.o

# Baytrail board
snd-byt-bl-rt5642-objs := byt_bl_rt5642.o
snd-byt-cr-aic3100-objs := byt_cr_aic3100.o
obj-$(CONFIG_SND_BYT_MACHINE) += snd-byt-bl-rt5642.o snd-byt-cr-aic3100.o

snd-byt-catalog-objs := byt_common.o byt_comms_common.o
obj-$(CONFIG_SND_BYT_MACHINE) += snd-byt-catalog.o

snd-byt-bl-rt5645-objs := byt_bl_rt5645.o
obj-$(CONFIG_SND_BYT_RT5645) += snd-byt-bl-rt5645.o

snd-byt-cr-rt5651-objs := byt_cr_rt5651.o
obj-$(CONFIG_SND_BYT_RT5651) += snd-byt-cr-rt5651.o

snd-byt-bl-es8396-objs := byt_bl_es8396.o
obj-$(CONFIG_SND_BYT_ES8396) += snd-byt-bl-es8396.o

snd-byt-cr-es8316-objs := byt_cr_es8316.o
obj-$(CONFIG_SND_BYT_ES8316) += snd-byt-cr-es8316.o

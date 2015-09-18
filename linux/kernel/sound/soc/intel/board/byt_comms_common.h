
#ifndef _BYT_COMMS_COMMON_H_
#define _BYT_COMMS_COMMON_H_

/*
 * For Mixing 1 slot of 32 bits is used
 * to transfer stereo 16 bits PCM samples
 */
#define BYT_SSP_MIXING_SLOT_NB_SLOT	1
#define BYT_SSP_MIXING_SLOT_WIDTH	32
#define BYT_SSP_MIXING_SLOT_RX_MASK	0x1
#define BYT_SSP_MIXING_SLOT_TX_MASK	0x1

/*
 * For BT SCO 1 slot of 16 bits is used
 * to transfer mono 16 bits PCM samples
 */
#define BYT_SSP_BT_SLOT_NB_SLOT		1
#define BYT_SSP_BT_SLOT_WIDTH		16
#define BYT_SSP_BT_SLOT_RX_MASK		0x1
#define BYT_SSP_BT_SLOT_TX_MASK		0x1


/* comms dai link and ops */

extern struct snd_soc_ops byt_comms_common_dai_link_ops;
#define COMMS_DAI_LINK	\
	[BYT_COMMS_BT] = { \
		.name = "Baytrail Comms BT SCO", \
		.stream_name = "BYT_BTSCO", \
		.cpu_dai_name = SSP_BT_DAI_NAME, \
		.codec_dai_name = "snd-soc-dummy-dai", \
		.codec_name = "snd-soc-dummy", \
		.platform_name = "mid-ssp-dai", \
		.init = NULL, \
		.ops = &byt_comms_common_dai_link_ops, \
	}, \
	[BYT_COMMS_MODEM] = { \
		.name = "Baytrail Comms MODEM", \
		.stream_name = "BYT_MODEM_MIXING", \
		.cpu_dai_name = SSP_MODEM_DAI_NAME, \
		.codec_dai_name = "snd-soc-dummy-dai", \
		.codec_name = "snd-soc-dummy", \
		.platform_name = "mid-ssp-dai", \
		.init = NULL, \
		.ops = &byt_comms_common_dai_link_ops, \
	},

#endif

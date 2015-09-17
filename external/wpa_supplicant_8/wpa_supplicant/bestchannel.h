/*
 * WPA Supplicant - best channel module
 * Copyright (c) 2013, Intel Corporation. All rights reserved.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef __BEST_CHANNEL_H__
#define __BEST_CHANNEL_H__

/* Channels from 1 to 13 per step of 1*/
#define NUM_CHANNELS_2_4_GHZ  13

/* Channels from 36 to 64 per step of 4*/
#define NUM_CHANNELS_5_1_GHZ  8

/* Channels from 149 to 165 per step of 4*/
#define NUM_CHANNELS_5_7_GHZ  5

/* Weight of handled channel N */
#define W0_CHAN 1

/* Weight of channel N-1 and N+1  */
#define W1_CHAN 4

/* Weight of channel N-2 and N+2  */
#define W2_CHAN 2

/**
 * Structure defining a channel
 */
struct best_op_channel {
	unsigned int nb_of_aps;
	unsigned int channel_number;
	unsigned char enabled;

};

struct wpa_supplicant;
struct wpa_global;

#ifdef CONFIG_BESTCHANNEL

int best_safe_channel_list_set(struct wpa_global *global,
				unsigned int channel_list);
int best_channel_init(struct wpa_global *global);
void best_channel_uninit(struct wpa_global *global);
void best_channel_reset_ap(struct wpa_supplicant *wpa_s);
int best_channel_add_ap(struct wpa_supplicant *wpa_s, unsigned int channel);
int best_channel_get_freq_list(struct wpa_supplicant *wpa_s);
int best_channel_compute(struct wpa_supplicant *wpa_s);


#else /* CONFIG_BESTCHANNEL */

static inline int best_channel_init(struct wpa_global *global)
{
	return 0;
}

static inline void best_channel_uninit(struct wpa_global *global)
{
}

static inline void best_channel_reset_ap(struct wpa_supplicant *wpa_s)
{
}

static inline int best_channel_add_ap(struct wpa_supplicant *wpa_s,
		unsigned int channel)
{
	return 0;
}

static inline int best_channel_get_freq_list(struct wpa_supplicant *wpa_s)
{
	return 0;
}

static inline int best_channel_compute(struct wpa_supplicant *wpa_s)
{
	return 0;
}

#endif /* CONFIG_BESTCHANNEL */

#endif

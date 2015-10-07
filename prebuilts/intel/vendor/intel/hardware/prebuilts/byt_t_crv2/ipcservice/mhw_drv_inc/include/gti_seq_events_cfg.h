/*  -------------------------------------------------------------------------
    Copyright (C) 2011-2013 Intel Mobile Communications GmbH
    
         Sec Class: Intel Confidential (IC)
    
    All rights reserved.
    -------------------------------------------------------------------------
    This document contains proprietary information belonging to IMC.
    Passing on and copying of this document, use and communication of its
    contents is not permitted without prior written authorization.
    -------------------------------------------------------------------------
    Revision Information:
       $File name:  /mhw_drv_inc/inc/target_test/gti_seq_events_cfg.h $
       $CC-Version: .../oint_drv_target_interface/23 $
       $Date:       2013-07-19    14:03:32 UTC $
    ------------------------------------------------------------------------- */

/*** User events ***/
/*** USE ONLY "small" letters in event names ***/
/*** Sort lines alphabetically (can e.g. be done in SourceInsight by "sort selection" custom command */
/*****************************************************************************************************/
/*** GTI_SEQ_EVENT_DECLARATION(<event name>, <sub-system name>, <short description>) ****/

/* REMEMBER !!!Sort alphabetically after event name!!! */
GTI_SEQ_EVENT_DECLARATION(aen_afc_ind,                fspeed, "AFC_IND from Aeneas")
GTI_SEQ_EVENT_DECLARATION(aen_allow_gsm_acc_sim0,     fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_allow_gsm_acc_sim1,     fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_bchdata,                fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_cbs_act_sim0,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_cbs_act_sim1,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_cf_cmd,                 fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_csrch_ind,              fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_dbgloop,                fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_dlphy_buf0,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_dlphy_buf1,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_edch_stat,              fspeed, "EDCH_STAT set")
GTI_SEQ_EVENT_DECLARATION(aen_edch_stat2AB,           fspeed, "EDCH_STAT set")
GTI_SEQ_EVENT_DECLARATION(aen_flex_act_sim0,          fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_flex_act_sim1,          fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_freq_scan_ind,          fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_generate_agps_pulse,    fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_grf_act_stop_sim0,      fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_grf_act_stop_sim1,      fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_grp_dlbuf,              fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpa_buf0,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpa_buf0b,            fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpa_buf1,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpa_buf1b,            fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpa_buf2,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpa_buf2b,            fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpa_buf3,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpa_buf3b,            fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_hsdpastat_ind,          fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_ins_ind,                fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_internevent1_ind,       fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_internevent2_ind,       fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_internperiod1_ind,      fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_internperiod2_ind,      fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_interrat_prio_sim0_cnf, fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_interrat_prio_sim1_cnf, fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_iratrssi_ind,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_ncell1_ind,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_ncell2_ind,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_ncell3_ind,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_nextumts_ind,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_oos_ind,                fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_paging_act_sim0,        fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_paging_act_sim1,        fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_qualevent1_ind,         fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_qualevent2_ind,         fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_qualper1_ind,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_qualper2_ind,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_rach_ind,               fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_read_dsp_mem,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_readmem,                fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_readvers_ind,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_scellmeas1_ind,         fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_scellmeas_ind,          fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_sfndecode1_ind,         fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_sfndecode_ind,          fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_sysinfo_act_sim0,       fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_sysinfo_act_sim1,       fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_tracehci,               fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_transmode1,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_transmode2,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_transmode3,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_transmode4,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_transmode5,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_uldata_ind,             fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_ulest_ind,              fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_umtsinact_ind,          fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_umtssnpsht_ind,         fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_urf_act_stop_sim0,      fspeed, "")
GTI_SEQ_EVENT_DECLARATION(aen_urf_act_stop_sim1,      fspeed, "")
GTI_SEQ_EVENT_DECLARATION(ber_meas_ended_ch_0,        macsim, "Triggered when BER measurements ended in channel 0")
GTI_SEQ_EVENT_DECLARATION(ber_meas_ended_ch_1,        macsim, "Triggered when BER measurements ended in channel 1")
GTI_SEQ_EVENT_DECLARATION(ber_meas_ended_ch_2,        macsim, "Triggered when BER measurements ended in channel 2")
GTI_SEQ_EVENT_DECLARATION(ber_meas_ended_ch_3,        macsim, "Triggered when BER measurements ended in channel 3")
GTI_SEQ_EVENT_DECLARATION(ber_meas_ended_ch_4,        macsim, "Triggered when BER measurements ended in channel 4")
GTI_SEQ_EVENT_DECLARATION(ber_meas_ended_ch_5,        macsim, "Triggered when BER measurements ended in channel 5")
GTI_SEQ_EVENT_DECLARATION(ber_meas_ended_ch_6,        macsim, "Triggered when BER measurements ended in channel 6")
GTI_SEQ_EVENT_DECLARATION(ber_meas_ended_ch_7,        macsim, "Triggered when BER measurements ended in channel 7")
GTI_SEQ_EVENT_DECLARATION(cf_eol_ready_cleared,       fspeed, "")
GTI_SEQ_EVENT_DECLARATION(delay_reached,              fspeed, "")
GTI_SEQ_EVENT_DECLARATION(gcal_ready,                 gcal, "")
GTI_SEQ_EVENT_DECLARATION(gps_reliability_ind,        fspeed, "")
GTI_SEQ_EVENT_DECLARATION(gsm_deactivated,            gsm_l1, "")
GTI_SEQ_EVENT_DECLARATION(gsm_ep_int,                 gsm_l1, "")
GTI_SEQ_EVENT_DECLARATION(gsm_fr_int,                 gsm_l1, "")
GTI_SEQ_EVENT_DECLARATION(gsm_sync_rec,               getif, "")
GTI_SEQ_EVENT_DECLARATION(lpm_toggle,                 fspeed, "")
GTI_SEQ_EVENT_DECLARATION(new_frame_event,            halrf, "triggered at frame boundary")
GTI_SEQ_EVENT_DECLARATION(rft2g_ready,                gcal, "")
GTI_SEQ_EVENT_DECLARATION(rft3g_ready,                ucal, "")
GTI_SEQ_EVENT_DECLARATION(sfn_reached,                fspeed, "")
GTI_SEQ_EVENT_DECLARATION(temp_freqerror_ind,         fspeed, "")
GTI_SEQ_EVENT_DECLARATION(umts_dl_data_int,           fspeed, "")
GTI_SEQ_EVENT_DECLARATION(umts_fr_int,                fspeed, "")
GTI_SEQ_EVENT_DECLARATION(user_defined_event,         fspeed, "")



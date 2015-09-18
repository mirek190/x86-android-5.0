/*
 * Copyright (c) 2013, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __ACPI_H__
#define __ACPI_H__

#include "bootlogic.h"

/** Generic ACPI table header **/
struct ACPI_DESC_HEADER {
	CHAR8   signature[4];		/* ASCII Table identifier */
	UINT32  length;			/* Length of the table, including the header */
	CHAR8   revision;		/* Revision of the structure */
	CHAR8   checksum;		/* Sum of all fields must be 0 */
	CHAR8   oem_id[6];		/* ASCII OEM identifier */
	CHAR8   oem_table_id[8];	/* ASCII OEM table identifier */
	UINT32  oem_revision;		/* OEM supplied revision number */
	CHAR8   creator_id[4];		/* Vendor ID of utility creator of the table */
	UINT32  creator_revision;	/* Revision of utility creator of the table */
};

struct RSDP_TABLE {
	CHAR8	signature[8];		/* "RSD PTR " */
	CHAR8	checksum;		/* RSDP Checksum (bytes 0-19) */
	CHAR8	oem_id[6];		/* OEM ID String */
	CHAR8	revision;		/* ACPI Revision (0=1.0,2=2.0) */
	UINT32	rsdt_address;		/* 32-bit RSDT Pointer */
	UINT32	length;			/* RSDP Length */
	UINT64	xsdt_address;		/* 64-bit XSDT Pointer */
	CHAR8	extended_checksum;	/* RSDP Checksum (full) */
	CHAR8	reserved[3];		/* Reserved */
};

struct RSDT_TABLE {
	struct ACPI_DESC_HEADER header;	/* System Description Table Header */
	UINT32 entry[1];		/* Table Entries */
};

struct RSCI_TABLE {
	struct ACPI_DESC_HEADER header;	/* System Description Table Header */
	CHAR8 wake_source;		/* How system woken up from S4 or S5 */
	CHAR8 reset_source;		/* How system was reset */
	CHAR8 reset_type;		/* Identify type of reset */
	CHAR8 shutdown_source;		/* How system was last shutdown */
	UINT32 indicators;		/* Bitmap with additional info */
};

/** PIDV Definitions **/
struct ext_id_1 {
	UINT16 customer;
	UINT16 vendor;
	UINT16 device_manufacturer;
	UINT16 platform_family;
	UINT16 product_line;
	UINT16 hardware;
	CHAR8 fru[10];
	UINT8 reserved[8];
} __attribute__ ((packed));

struct ext_id_2 {
	UINT32 data1;
	UINT32 data2;
	UINT32 data3;
	UINT32 data4;
} __attribute__ ((packed));

struct PIDV_TABLE {
	struct ACPI_DESC_HEADER header;	/* System Description Table Header */
	CHAR8 part_number[32];		/* Serial number */
	struct ext_id_1 x_id1;		/* Extended ID 1 */
	struct ext_id_2 x_id2;		/* Extended ID 2 */
	UINT32 system_uuid;		/* Identify hardware platform */
};

enum {
	OEM1_USE_IA_APPS_CAP,
	OEM1_USE_IA_APPS_RUN
};
struct OEM1_TABLE {
	struct ACPI_DESC_HEADER header;	/* System Description Table Header */
	UINT8 fixedoptions0;		/* Fixed Platform Options 0 */
	UINT8 fixedoptions1;		/* Fixed Platform Options 1*/
	UINT8 dbiingpio;		/* DBIIN GPIO number */
	UINT8 dbioutgpio;		/* DBIOUT GPIO number */
	UINT8 batchptyp;       		/* Identification / Authentication chip
					 * inside the battery */
	UINT16 ia_apps_run;		/* Minimum battery voltage required to
					 * boot the platform if FG has been
					 * reset */
	UINT8 batiddbibase;		/* Resistance in KOhms for BSI used to
					 * indicate a digital battery */
	UINT8 batidanlgbase;		/* Resistance in KOhms for BSI beyond
					 * which the battery is an analog
					 * battery */
	UINT8 ia_apps_cap; 		/* Minimum capacity at which to boot to Main
					 * OS */
	UINT16 vbattfreqlmt;		/* Battery Voltage up to which the CPU
					 * frequency should be limited */
	UINT8 capfreqidx;   		/* Index into the Frequency table at which
					 * the CPU Frequency should be capped. */
	UINT8 rsvd1;			/* Reserved */
	UINT8 battidx; 			/* Battery Index: Charging profile to use in
					 * case of fixed battery */
	UINT8 ia_apps_to_use;		/* Whether to use the IA_APPS_RUN (value
					 * = 1) or IA_APPS_CAP (value = 0) to
					 * while booting */
	UINT8 turbochrg;		/* Maximum Turbo charge supported (in
					 * multiples of 100mA). Zero means no Turbo
					 * charge */
	UINT8 rsvd2[11];		/* Reserved */
} __attribute__ ((packed));

/* ACPI Generic Address Structure */
struct gas {
	UINT8 addr_space_id;	/* Address space where data structure or register exists */
	UINT8 reg_bit_width;	/* Size in bits of given register */
	UINT8 reg_bit_offset;	/* Bit offset of the given register */
	UINT8 access_size;	/* Specifies access size */
	UINT64 address;		/* 64-bit address of the data structure of register */
};

struct FACP_TABLE {
	struct ACPI_DESC_HEADER header;	/* System Description Table Header */
	UINT32 firmware_ctrl;		/* Address of the FACS */
	UINT32 dsdt;			/* Address of the DSDT */
	UINT8 model;			/* ACPI 1.0 INT_MODEL */
	UINT8 preferred_pm_profile;	/* Preferred power management profile */
	UINT16 sci_int;			/* System vector the SCI interrupt is wire to */
	UINT32 smi_cmd;			/* System port address of the SMI Command Port */
	UINT8 acpi_enable;		/* Value to write to smi_cmd to disable SMI ownership of the ACPI hardware registers */
	UINT8 acpi_disable;		/* Value to write to smi_cmd to re-enable SMI ownership of the ACPI hardware registers */
	UINT8 s4bios_req;		/* Value to write to smi_cmd to enter S4BIOS state */
	UINT8 pstate_cnt;		/* If not 0, value to write to smi_cmd to assume processor performance state control responsibility */
	UINT32 pm1a_evt_blk;		/* Port address of PM1a event register block */
	UINT32 pm1b_evt_blk;		/* Port address of PM1b event register block */
	UINT32 pm1a_cnt_blk;		/* Port address of PM1b control register block */
	UINT32 pm1b_cnt_blk;		/* Port address of PM1b control register block */
	UINT32 pm2_cnt_blk;		/* Port address of PM2 control register block */
	UINT32 pm_tmr_blk;		/* Port address of PM timer control register block */
	UINT32 gpe0_blk;		/* Port address of General Purpose Event 0 register block */
	UINT32 gpe1_blk;		/* Port address of General Purpose Event 1 register block */
	UINT8 pm1_evt_len;		/* Length in bytes at pm1a_evt_blk */
	UINT8 pm1_cnt_len;		/* Length in bytes at pm1a_cnt_blk */
	UINT8 pm2_cnt_len;		/* Length in bytes at pm2_cnt_blk */
	UINT8 pm_tmr_len;		/* Length in bytes at pm_tmr_blk */
	UINT8 gpe0_len;			/* Length in bytes at gpe0_blk */
	UINT8 gpe1_len;			/* Length in bytes at gpe1_blk */
	UINT8 gpe1_base;		/* Offset where gpe1 based events start */
	UINT8 cst_cnt;			/* If not 0, value to write to smi_cmd to indicate OS support for the _CST and C States Changed notification */
	UINT16 p_lvl2_lat;		/* Worst case hw latency to enter and exit a C2 state */
	UINT16 p_lvl3_lat;		/* Worst case hw latency to enter and exit a C3 state */
	UINT16 flush_size;		/* Number of flush strides to read to flush diry lines from CPU cache */
	UINT16 flush_stride;		/* Cache line width in bytes */
	UINT8 duty_offset;		/* Index of processor duty cycle in P_CNT register */
	UINT8 duty_width;		/* Width of processor duty cycle in P_CNT register */
	UINT8 day_alarm;		/* RTC CMOS RAM index to day-of-month alarm value */
	UINT8 mon_alarm;		/* RTC CMOS RAM index to month-of-year alarm value */
	UINT8 century;			/* RTC CMOS RAM index to century data value */
	UINT16 iapc_boot_arch;		/* IA-PC Boot Architecture Flags */
	UINT8 reserved;			/* Must be 0 */
	UINT32 flags;			/* Fixed feature flags */
	struct gas reset_reg; /* Address of reset register */
	UINT8 reset_value;			    /* Value to write to reset_reg to reset the system */
	UINT8 reserved2[3];			    /* Must be 0 */
	UINT64 Xfacs;				    /* 64-bit address of FACS */
	UINT64 Xdsdt;				    /* 64-bit address of DSDT */
	struct gas xpm1a_evt_blk;		    /* Extended address of PM1a event register block */
	struct gas xpm1b_evt_blk;		    /* Extended address of PM1b event register block */
	struct gas xpm1a_cnt_blk;		    /* Extended address of PM1b control register block */
	struct gas xpm1b_cnt_blk;		    /* Extended address of PM1b control register block */
	struct gas xpm2_cnt_blk;		    /* Extended address of PM2 control register block */
	struct gas xpm_tmr_blk;			    /* Extended address of PM timer control register block */
	struct gas xgpe0_blk;			    /* Extended address of General Purpose Event 0 register block */
	struct gas xgpe1_blk;			    /* Extended address of General Purpose Event 1 register block */
	struct gas sleep_control;		    /* 64-bit address of the sleep register */
	struct gas sleep_status;		    /* 64-bit address of the sleep status register */
};

EFI_STATUS list_acpi_tables(void);
EFI_STATUS get_acpi_table(CHAR8 *signature, VOID **table);
enum flow_types acpi_read_flow_type(void);

EFI_STATUS rsci_populate_indicators(void);
enum wake_sources rsci_get_wake_source(void);
enum reset_sources rsci_get_reset_source(void);
EFI_STATUS rsci_set_reset_source(enum reset_sources);
enum reset_types rsci_get_reset_type(void);
enum shutdown_sources rsci_get_shutdown_source(void);

UINT16 oem1_get_ia_apps_run(void);
UINT8 oem1_get_ia_apps_cap(void);
UINT8 oem1_get_capfreqidx(void);
UINT16 oem1_get_ia_vbattfreqlmt(void);
UINT8 oem1_get_ia_apps_to_use(void);

void print_pidv(void);
void print_rsci(void);
void dump_acpi_tables(void);
void load_dsdt(void);

#endif /* __ACPI_H__ */

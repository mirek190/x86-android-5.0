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
 */

#ifndef _MSGBUS_H_
#define _MSGBUS_H_

/* Extended Configuration Base Address.*/
#define EC_BASE				0xE0000000

/* VLV Message Bus Units Port ID*/
#define VLV_PUNIT		0x04		/*P Unit*/
#define VLV_DFXUNIT		0x05		/*Test Controller SSA-DFX Unit*/
#define VLV_DFXSOC		0x15		/*Test Controller DFX-SOC*/
#define VLV_DFXNC		0x16		/*Test Controller DFX-NC*/
#define VLV_DFXLAKEMORE		0x17		/*Test Controller DFX-Lakemore*/
#define VLV_DFXVISA		0x18		/*Test Controller DFX-VISA*/

// VLV Message Bus Register Definitions
//Common for A/B/C/D/T/SVID/CCK/I units
#define VLV_MBR_READ_CMD	0x10000000
#define VLV_MBR_WRITE_CMD	0x11000000

//Common for Gunit/DISPIO/DFXLAKSEMORE/DISPCONT units
#define VLV_MBR_GDISPIOREAD_CMD		0x00000000
#define VLV_MBR_GDISPIOWRITE_CMD	0x01000000

//Common for Smbus units
#define VLV_SMB_REGREAD_CMD	0x04000000
#define VLV_SMB_REGWRITE_CMD	0x05000000

//Common for Punit/DFX/GPIONC/DFXSOC/DFXNC/DFXVISA units
#define VLV_MBR_PDFXGPIODDRIOREAD_CMD	0x06000000
#define VLV_MBR_PDFXGPIODDRIOWRITE_CMD	0x07000000

//Msg Bus Registers
#define MC_MCR			0x000000D0		//Cunit Message Control Register
#define MC_MDR			0x000000D4		//Cunit Message Data Register
#define MC_MCRX			0x000000D8		//Cunit Message Control Register Extension

static inline UINT32 VlvMsgBusRead(UINT32 ReadOp, UINT8 PortId, UINT32 Register)
{
	*((UINT32*)(EC_BASE + MC_MCRX)) = (Register & 0xFFFFFF00);
	*((UINT32*)(EC_BASE + MC_MCR)) = (ReadOp |
					  (PortId << 16) |
					  ((Register & 0x000000FF) << 8) |
					  0xF0);
	return *((UINT32*)(EC_BASE + MC_MDR));
}

static inline UINT32 VlvMsgBusReadDfxLM(UINT32 Register)
{
	return VlvMsgBusRead(VLV_MBR_GDISPIOREAD_CMD, VLV_DFXLAKEMORE, Register);
}

static inline UINT32 VlvMsgBusReadPunit(UINT32 Register)
{
	return VlvMsgBusRead(VLV_MBR_PDFXGPIODDRIOREAD_CMD, VLV_PUNIT, Register);
}

#endif /* _MSGBUS_H_ */

/*
 * INTEL CONFIDENTIAL
 *
 * Copyright (C) 2010 - 2013 Intel Corporation.
 * All Rights Reserved.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or licensors. Title to the Material remains with Intel
 * Corporation or its licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No License under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.
 */

#ifndef __ISP_PUBLIC_H_INCLUDED__
#define __ISP_PUBLIC_H_INCLUDED__

#include <stddef.h>		/* size_t */
#include <stdbool.h>	/* bool */

/*! Write to the status and control register of ISP[ID]

 \param	ID[in]				ISP identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return none, ISP[ID].sc[reg] = value
 */
STORAGE_CLASS_ISP_H void isp_ctrl_store(
	const isp_ID_t		ID,
	const unsigned int	reg,
	const hrt_data		value);

/*! Read from the status and control register of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	reg[in]				register index
 \param value[in]			The data to be written

 \return ISP[ID].sc[reg]
 */
STORAGE_CLASS_ISP_H hrt_data isp_ctrl_load(
	const isp_ID_t		ID,
	const unsigned int	reg);

/*! Get the status of a bitfield in the control register of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	reg[in]				register index
 \param bit[in]				The bit index to be checked

 \return  (ISP[ID].sc[reg] & (1<<bit)) != 0
 */
STORAGE_CLASS_ISP_H bool isp_ctrl_getbit(
	const isp_ID_t		ID,
	const unsigned int	reg,
	const unsigned int	bit);

/*! Set a bitfield in the control register of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	reg[in]				register index
 \param bit[in]				The bit index to be set

 \return none, ISP[ID].sc[reg] |= (1<<bit)
 */
STORAGE_CLASS_ISP_H void isp_ctrl_setbit(
	const isp_ID_t		ID,
	const unsigned int	reg,
	const unsigned int	bit);

/*! Clear a bitfield in the control register of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	reg[in]				register index
 \param bit[in]				The bit index to be set

 \return none, ISP[ID].sc[reg] &= ~(1<<bit)
 */
STORAGE_CLASS_ISP_H void isp_ctrl_clearbit(
	const isp_ID_t		ID,
	const unsigned int	reg,
	const unsigned int	bit);

/*! Write to PMEM of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	addr[in]			the address in PMEM (written to)
 \param data[in]			The virtual DDR address of data to be written
 \param size[in]			The size (in bytes) of the data to be written

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An ISP device has only a single PMEM

 \return none, ISP[ID].pmem[addr...addr+size-1] = data
 */
STORAGE_CLASS_ISP_H void isp_pmem_store(
	const isp_ID_t		ID,
	unsigned int		addr,
	const hrt_vaddress	data,
	const size_t		size);

/*! Move data from or to DMEM of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	cmd[in]				Transfer type commant
 \param	addr[in]			the address in DMEM
 \param data[in]			The virtual DDR address of data
 \param size[in]			The size (in bytes) of the data to be moved

 \The transfer command "cmd" can be set to {read, write, clear}

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An ISP device has only a single DMEM
	- The transfer command may define further attributes, like
	  enabling or disabling acknowledgements

 \return none, ISP[ID].dmem[addr...addr+size-1] <-> data
 */
STORAGE_CLASS_ISP_H void isp_dmem_transfer(
	const isp_ID_t				ID,
	const dma_transfer_type_t	cmd,
	unsigned int				addr,
	hrt_vaddress				data,
	const size_t				size);

/*! Write to DMEM of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	addr[in]			the address in DMEM (written to)
 \param data[in]			The virtual DDR address of data to be written
 \param size[in]			The size (in bytes) of the data to be written

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An ISP device has only a single DMEM

 \return none, ISP[ID].dmem[addr...addr+size-1] = data
 */
STORAGE_CLASS_ISP_H void isp_dmem_store(
	const isp_ID_t		ID,
	unsigned int		addr,
	const hrt_vaddress	data,
	const size_t		size);

/*! Read from to DMEM of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	addr[in]			the address in DMEM (read from)
 \param data[in]			The virtual DDR address of data read to
 \param size[in]			The size (in bytes) of the data to be read

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An ISP device has only a single DMEM

 \return none, data = ISP[ID].dmem[addr...addr+size-1]
 */
STORAGE_CLASS_ISP_H void isp_dmem_load(
	const isp_ID_t		ID,
	const unsigned int	addr,
	hrt_vaddress		data,
	const size_t		size);

/*! Clear DMEM of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	addr[in]			the address in DMEM (to be cleared)
 \param size[in]			The size (in bytes) of the data to be cleared

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An ISP device has only a single DMEM

 \return none, ISP[ID].dmem[addr...addr+size-1] = 0
 */
STORAGE_CLASS_ISP_H void isp_dmem_clear(
	const isp_ID_t		ID,
	unsigned int		addr,
	const size_t		size);

/*! Write a 32-bit datum to the DMEM of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	addr[in]			the address in DMEM
 \param data[in]			The data to be written

 \return none, ISP[ID].dmem[addr] = data
 */
STORAGE_CLASS_ISP_H void isp_dmem_store_uint32(
	const isp_ID_t		ID,
	unsigned int		addr,
	const uint32_t  data);

/*! Load a 32-bit datum from the DMEM of ISP[ID]
 
 \param	ID[in]				ISP identifier
 \param	addr[in]			the address in DMEM
 \param data[in]			The data to be read
 \param size[in]			The size(in bytes) of the data to be read

 \return none, data = ISP[ID].dmem[addr]
 */
STORAGE_CLASS_ISP_H uint32_t isp_dmem_load_uint32(
	const isp_ID_t		ID,
	unsigned int		addr);

/*! Write to VMEM[ID]
 
 \param	ID[in]				VMEM identifier
 \param	addr[in]			the address in VMEM (written to)
 \param data[in]			The virtual DDR address of data to be written
 \param size[in]			The size (in bytes) of the data to be written

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- The VMEM's are enumerated separately from ISP devices

 \return none, VMEM[ID][addr...addr+size-1] = data
 */
STORAGE_CLASS_ISP_H void isp_vmem_store(
	const bamem_ID_t	ID,
	unsigned int		addr,
	const hrt_vaddress	data,
	const size_t		size);

/*! Read from VMEM[ID]
 
 \param	ID[in]				VMEM identifier
 \param	addr[in]			the address in VMEM (to be read from)
 \param data[in]			The virtual DDR address of data read to
 \param size[in]			The size (in bytes) of the data to be read

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- The VMEM's are enumerated separately from ISP devices

 \return none, data = VMEM[ID][addr...addr+size-1]
 */
STORAGE_CLASS_ISP_H void isp_vmem_load(
	const bamem_ID_t	ID,
	const unsigned int	addr,
	hrt_vaddress		data,
	const size_t		size);

/*! Clear VMEM[ID]
 
 \param	ID[in]				VMEM identifier
 \param	addr[in]			the address in VMEM (to be cleared)
 \param size[in]			The size (in bytes) of the data to be cleared

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- The VMEM's are enumerated separately from ISP devices

 \return none, VMEM[ID][addr...addr+size-1] = 0
 */
STORAGE_CLASS_ISP_H void isp_vmem_clear(
	const bamem_ID_t	ID,
	unsigned int		addr,
	const size_t		size);



/*! Copy from DMEM of SP[ME] to DMEM of ISP[ID]
 
 \param	ME[in]				SP identifier
 \param	ID[in]				ISP identifier
 \param	addr[in]			the address in ISP DMEM (written to)
 \param data[in]			Pointer to data in local SP DMEM
 \param size[in]			The size (in bytes) of the data to be written

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An SP device has only a single DMEM
	- An ISP device has only a single DMEM

 \return none, ISP[ID].dmem[addr...addr+size-1] = SP[ME].dmem->data
 */
STORAGE_CLASS_ISP_H void dmem_isp_dmem_store(
	const sp_ID_t		ME,
	const isp_ID_t		ID,
	unsigned int		addr,
	const void			*data,
	const size_t		size);

/*! Copy from DMEM of ISP[ID] to DMEM of SP[ME]
 
 \param	ME[in]				SP identifier
 \param	ID[in]				ISP identifier
 \param	addr[in]			the address in ISP DMEM (read from)
 \param data[in]			Pointer to data in local SP DMEM
 \param size[in]			The size (in bytes) of the data to be read

 \implementation dependent
	- This function depends on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An SP device has only a single DMEM
	- An ISP device has only a single DMEM

 \return none, SP[ME].dmem->data = ISP[ID].dmem[addr...addr+size-1]
 */
STORAGE_CLASS_ISP_H void dmem_isp_dmem_load(
	const sp_ID_t		ME,
	const isp_ID_t		ID,
	const unsigned int	addr,
	void				*data,
	const size_t		size);

/*! Copy from DMEM of SP[ME] to VMEM[ID]
 
 \param	ME[in]				SP identifier
 \param	ID[in]				VMEM identifier
 \param	addr[in]			the address in VMEM (written to)
 \param data[in]			Pointer to data in local DMEM
 \param size[in]			The size (in bytes) of the data to be written

 \implementation dependent
	- This function can depend on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An SP device has only a single DMEM
	- The VMEM's are enumerated separately from ISP devices

 \return none, VMEM[ID][addr...addr+size-1] = SP[ME].dmem->data
 */
STORAGE_CLASS_ISP_H void dmem_isp_vmem_store(
	const sp_ID_t		ME,
	const bamem_ID_t	ID,
	unsigned int		addr,
	const void			*data,
	const size_t		size);

/*! Copy from VMEM[ID] to DMEM of SP[ME]
 
 \param	ME[in]				SP identifier
 \param	ID[in]				VMEM identifier
 \param	addr[in]			the address in VMEM (read from)
 \param data[in]			Pointer to data in local DMEM
 \param size[in]			The size (in bytes) of the data to be read

 \implementation dependent
	- This function can depend on a DMA service
	- The DMA (channel) handle is acquired from a resource manager
	- An SP device has only a single DMEM
	- The VMEM's are enumerated separately from ISP devices

 \return none, SP[ME].dmem->data = VMEM[ID][addr...addr+size-1]
 */
STORAGE_CLASS_ISP_H void dmem_isp_vmem_load(
	const sp_ID_t		ME,
	const bamem_ID_t	ID,
	const unsigned int	addr,
	void				*data,
	const size_t		size);

#endif /* __ISP_PUBLIC_H_INCLUDED__ */

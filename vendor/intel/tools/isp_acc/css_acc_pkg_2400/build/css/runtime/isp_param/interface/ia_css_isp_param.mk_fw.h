/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef _IA_CSS_ISP_PARAM_MK_FW_H_
#define _IA_CSS_ISP_PARAM_MK_FW_H_

#include "ia_css_acc_types.h"

#if !defined(__ACC) && !defined(IS_ISP_2500_SYSTEM)
/* Include generated files */
#include "ia_css_isp_params.h"
#include "ia_css_isp_configs.h"
#include "ia_css_isp_states.h"
#endif

/* This module should be moved to a system specific directory */

/* Default parameter and configuration addresses and sizes per memory */

#ifndef ISP_DMEM_PARAMETERS_SIZE
#define ISP_DMEM_PARAMETERS_SIZE 0
#endif

#ifndef ISP_VMEM_PARAMETERS_SIZE
#define ISP_VMEM_PARAMETERS_SIZE 0
#endif

#ifndef ISP_VAMEM0_PARAMETERS_SIZE
#define ISP_VAMEM0_PARAMETERS_SIZE 0
#endif

#ifndef ISP_VAMEM1_PARAMETERS_SIZE
#define ISP_VAMEM1_PARAMETERS_SIZE 0
#endif

#ifndef ISP_VAMEM2_PARAMETERS_SIZE
#define ISP_VAMEM2_PARAMETERS_SIZE 0
#endif

#ifndef ISP_HMEM0_PARAMETERS_SIZE
#define ISP_HMEM0_PARAMETERS_SIZE 0
#endif

/***************/

#ifndef ISP_DMEM_CONFIGURATIONS_SIZE
#define ISP_DMEM_CONFIGURATIONS_SIZE 0
#endif

#ifndef ISP_VMEM_CONFIGURATIONS_SIZE
#define ISP_VMEM_CONFIGURATIONS_SIZE 0
#endif

#ifndef ISP_VAMEM0_CONFIGURATIONS_SIZE
#define ISP_VAMEM0_CONFIGURATIONS_SIZE 0
#endif

#ifndef ISP_VAMEM1_CONFIGURATIONS_SIZE
#define ISP_VAMEM1_CONFIGURATIONS_SIZE 0
#endif

#ifndef ISP_VAMEM2_CONFIGURATIONS_SIZE
#define ISP_VAMEM2_CONFIGURATIONS_SIZE 0
#endif

#ifndef ISP_HMEM0_CONFIGURATIONS_SIZE
#define ISP_HMEM0_CONFIGURATIONS_SIZE 0
#endif

/***************/

#ifndef ISP_DMEM_STATES_SIZE
#define ISP_DMEM_STATES_SIZE 0
#endif

#ifndef ISP_VMEM_STATES_SIZE
#define ISP_VMEM_STATES_SIZE 0
#endif

#ifndef ISP_VAMEM0_STATES_SIZE
#define ISP_VAMEM0_STATES_SIZE 0
#endif

#ifndef ISP_VAMEM1_STATES_SIZE
#define ISP_VAMEM1_STATES_SIZE 0
#endif

#ifndef ISP_VAMEM2_STATES_SIZE
#define ISP_VAMEM2_STATES_SIZE 0
#endif

#ifndef ISP_HMEM0_STATES_SIZE
#define ISP_HMEM0_STATES_SIZE 0
#endif

/***************/

#ifndef HIVE_ADDR_isp_dmem_parameters
#define HIVE_ADDR_isp_dmem_parameters	0x0
#endif

#ifndef HIVE_SIZE_isp_dmem_parameters
#define HIVE_SIZE_isp_dmem_parameters	ISP_DMEM_PARAMETERS_SIZE
#endif

#ifndef HIVE_ADDR_isp_vmem_parameters
#define HIVE_ADDR_isp_vmem_parameters	0x0
#endif

#ifndef HIVE_SIZE_isp_vmem_parameters
#define HIVE_SIZE_isp_vmem_parameters	ISP_VMEM_PARAMETERS_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem0_parameters
#define HIVE_ADDR_isp_vamem0_parameters	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem0_parameters
#define HIVE_SIZE_isp_vamem0_parameters	ISP_VAMEM0_PARAMETERS_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem1_parameters
#define HIVE_ADDR_isp_vamem1_parameters	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem1_parameters
#define HIVE_SIZE_isp_vamem1_parameters	ISP_VAMEM1_PARAMETERS_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem2_parameters
#define HIVE_ADDR_isp_vamem2_parameters	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem2_parameters
#define HIVE_SIZE_isp_vamem2_parameters	ISP_VAMEM2_PARAMETERS_SIZE
#endif

#ifndef HIVE_ADDR_isp_hmem0_parameters
#define HIVE_ADDR_isp_hmem0_parameters	0x0
#endif

#ifndef HIVE_SIZE_isp_hmem0_parameters
#define HIVE_SIZE_isp_hmem0_parameters	ISP_HMEM0_PARAMETERS_SIZE
#endif

/***************/

#ifndef HIVE_ADDR_isp_dmem_configurations
#define HIVE_ADDR_isp_dmem_configurations	0x0
#endif

#ifndef HIVE_SIZE_isp_dmem_configurations
#define HIVE_SIZE_isp_dmem_configurations	ISP_DMEM_CONFIGURATIONS_SIZE
#endif

#ifndef HIVE_ADDR_isp_vmem_configurations
#define HIVE_ADDR_isp_vmem_configurations	0x0
#endif

#ifndef HIVE_SIZE_isp_vmem_configurations
#define HIVE_SIZE_isp_vmem_configurations	ISP_VMEM_CONFIGURATIONS_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem0_configurations
#define HIVE_ADDR_isp_vamem0_configurations	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem0_configurations
#define HIVE_SIZE_isp_vamem0_configurations	ISP_VAMEM0_CONFIGURATIONS_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem1_configurations
#define HIVE_ADDR_isp_vamem1_configurations	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem1_configurations
#define HIVE_SIZE_isp_vamem1_configurations	ISP_VAMEM1_CONFIGURATIONS_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem2_configurations
#define HIVE_ADDR_isp_vamem2_configurations	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem2_configurations
#define HIVE_SIZE_isp_vamem2_configurations	ISP_VAMEM2_CONFIGURATIONS_SIZE
#endif

#ifndef HIVE_ADDR_isp_hmem0_configurations
#define HIVE_ADDR_isp_hmem0_configurations	0x0
#endif

#ifndef HIVE_SIZE_isp_hmem0_configurations
#define HIVE_SIZE_isp_hmem0_configurations	ISP_HMEM0_CONFIGURATIONS_SIZE
#endif

/***************/

#ifndef HIVE_ADDR_isp_dmem_states
#define HIVE_ADDR_isp_dmem_states	0x0
#endif

#ifndef HIVE_SIZE_isp_dmem_states
#define HIVE_SIZE_isp_dmem_states	ISP_DMEM_STATES_SIZE
#endif

#ifndef HIVE_ADDR_isp_vmem_states
#define HIVE_ADDR_isp_vmem_states	0x0
#endif

#ifndef HIVE_SIZE_isp_vmem_states
#define HIVE_SIZE_isp_vmem_states	ISP_VMEM_STATES_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem0_states
#define HIVE_ADDR_isp_vamem0_states	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem0_states
#define HIVE_SIZE_isp_vamem0_states	ISP_VAMEM0_STATES_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem1_states
#define HIVE_ADDR_isp_vamem1_states	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem1_states
#define HIVE_SIZE_isp_vamem1_states	ISP_VAMEM1_STATES_SIZE
#endif

#ifndef HIVE_ADDR_isp_vamem2_states
#define HIVE_ADDR_isp_vamem2_states	0x0
#endif

#ifndef HIVE_SIZE_isp_vamem2_states
#define HIVE_SIZE_isp_vamem2_states	ISP_VAMEM2_STATES_SIZE
#endif

#ifndef HIVE_ADDR_isp_hmem0_states
#define HIVE_ADDR_isp_hmem0_states	0x0
#endif

#ifndef HIVE_SIZE_isp_hmem0_states
#define HIVE_SIZE_isp_hmem0_states	ISP_HMEM0_STATES_SIZE
#endif

/**************/

#define PARAM_MEM_INITIALIZERS \
	{ { 0, 0 }, /* pmem */ \
	  { HIVE_ADDR_isp_dmem_parameters, \
	    HIVE_SIZE_isp_dmem_parameters, \
	  }, \
	  { HIVE_ADDR_isp_vmem_parameters, \
	    HIVE_SIZE_isp_vmem_parameters, \
	  }, \
	  { HIVE_ADDR_isp_vamem0_parameters, \
	    HIVE_SIZE_isp_vamem0_parameters, \
	  }, \
	  { HIVE_ADDR_isp_vamem1_parameters, \
	    HIVE_SIZE_isp_vamem1_parameters, \
	  }, \
	  { HIVE_ADDR_isp_vamem2_parameters, \
	    HIVE_SIZE_isp_vamem2_parameters, \
	  }, \
	  { HIVE_ADDR_isp_hmem0_parameters, \
	    HIVE_SIZE_isp_hmem0_parameters, \
	  }, \
	}

/**************/

#define CONFIG_MEM_INITIALIZERS \
	{ { 0, 0 }, /* pmem */ \
	  { HIVE_ADDR_isp_dmem_configurations, \
	    HIVE_SIZE_isp_dmem_configurations, \
	  }, \
	  { HIVE_ADDR_isp_vmem_configurations, \
	    HIVE_SIZE_isp_vmem_configurations, \
	  }, \
	  { HIVE_ADDR_isp_vamem0_configurations, \
	    HIVE_SIZE_isp_vamem0_configurations, \
	  }, \
	  { HIVE_ADDR_isp_vamem1_configurations, \
	    HIVE_SIZE_isp_vamem1_configurations, \
	  }, \
	  { HIVE_ADDR_isp_vamem2_configurations, \
	    HIVE_SIZE_isp_vamem2_configurations, \
	  }, \
	  { HIVE_ADDR_isp_hmem0_configurations, \
	    HIVE_SIZE_isp_hmem0_configurations, \
	  }, \
	}

/**************/

#define STATE_MEM_INITIALIZERS \
	{ { 0, 0 }, /* pmem */ \
	  { HIVE_ADDR_isp_dmem_states, \
	    HIVE_SIZE_isp_dmem_states, \
	  }, \
	  { HIVE_ADDR_isp_vmem_states, \
	    HIVE_SIZE_isp_vmem_states, \
	  }, \
	  { HIVE_ADDR_isp_vamem0_states, \
	    HIVE_SIZE_isp_vamem0_states, \
	  }, \
	  { HIVE_ADDR_isp_vamem1_states, \
	    HIVE_SIZE_isp_vamem1_states, \
	  }, \
	  { HIVE_ADDR_isp_vamem2_states, \
	    HIVE_SIZE_isp_vamem2_states, \
	  }, \
	  { HIVE_ADDR_isp_hmem0_states, \
	    HIVE_SIZE_isp_hmem0_states, \
	  }, \
	}

/**************/

#define MEM_INITIALIZERS \
	{ { PARAM_MEM_INITIALIZERS, \
	    CONFIG_MEM_INITIALIZERS, \
		STATE_MEM_INITIALIZERS, \
	} }

/**************/

void
ia_css_isp_param_print_mem_initializers(const struct ia_css_binary_info *info);

#endif /* _IA_CSS_ISP_PARAM_MK_FW_H_ */



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

#include "dma_settings.isp.h"
#include <sh_css_internal.h>

STORAGE_CLASS_INLINE void
dump_csc_parm (struct sh_css_isp_csc_params *csc)
{
	OP___dump(__LINE__, csc->m_shift);
	OP___dump(__LINE__, csc->m00);
	OP___dump(__LINE__, csc->m01);
	OP___dump(__LINE__, csc->m02);
	OP___dump(__LINE__, csc->m10);
	OP___dump(__LINE__, csc->m11);
	OP___dump(__LINE__, csc->m12);
	OP___dump(__LINE__, csc->m20);
	OP___dump(__LINE__, csc->m21);
	OP___dump(__LINE__, csc->m22);
}

STORAGE_CLASS_INLINE void
dump_ispparm (void)
{

	dump_csc_parm (&isp_dmem_parameters.csc);
#if 0
	/* Only for ISP 2 */
	dump_csc_parm (&isp_dmem_parameters.yuv2rgb);
	dump_csc_parm (&isp_dmem_parameters.rgb2yuv);
#endif

	OP___dump(__LINE__, isp_dmem_parameters.ob.blacklevel_gr);
	OP___dump(__LINE__, isp_dmem_parameters.ob.blacklevel_r);
	OP___dump(__LINE__, isp_dmem_parameters.ob.blacklevel_b);
	OP___dump(__LINE__, isp_dmem_parameters.ob.blacklevel_gb);
	OP___dump(__LINE__, isp_dmem_parameters.ob.area_start_bq);
	OP___dump(__LINE__, isp_dmem_parameters.ob.area_length_bq);
	OP___dump(__LINE__, isp_dmem_parameters.ob.area_length_bq_inverse);

	OP___dump(__LINE__, isp_dmem_parameters.sc.gain_shift);
	OP___dump(__LINE__, isp_dmem_parameters.wb.gain_shift);
	OP___dump(__LINE__, isp_dmem_parameters.wb.gain_gr);
	OP___dump(__LINE__, isp_dmem_parameters.wb.gain_r);
	OP___dump(__LINE__, isp_dmem_parameters.wb.gain_b);
	OP___dump(__LINE__, isp_dmem_parameters.wb.gain_gb);

	OP___dump(__LINE__, isp_dmem_parameters.s3a.ae.y_coef_r);
	OP___dump(__LINE__, isp_dmem_parameters.s3a.ae.y_coef_g);
	OP___dump(__LINE__, isp_dmem_parameters.s3a.ae.y_coef_b);

	OP___dump(__LINE__, isp_dmem_parameters.gc.gain_k1);
	OP___dump(__LINE__, isp_dmem_parameters.gc.gain_k2);

return;
}

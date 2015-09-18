#ifndef _ISP_ADD_DEFS_FOR_HIVE_H
#define _ISP_ADD_DEFS_FOR_HIVE_H

/*===========================================================
  The following definitions are used only in isp_vf_pp.
  The following definitions are NOT referred from host.
===========================================================*/
#ifdef CSS15
#define MY_OUTPUT_FORMATS \
	{ \

		SH_CSS_FRAME_FORMAT_NV12 \
	}
#else
#define MY_OUTPUT_FORMATS \
	{ \
		IA_CSS_FRAME_FORMAT_NV12 \
	}
#endif

#define BINARY_ID	      	     SH_CSS_BINARY_ID_VF_PP_FULL
#define BINARY  	      	     add
#define MODE		      	     IA_CSS_BINARY_MODE_VF_PP
#define ENABLE_DS	      	     0
#define ENABLE_FPNR	      	     0
#define ENABLE_SC	      	     0
#define ENABLE_S3A	      	     0
#define ENABLE_SDIS	      	     0
#define ENABLE_OUTPUT    	     0
#define ENABLE_VF_VECEVEN     	 0
#define ENABLE_UDS	      	     0
#define ENABLE_RGBA              0
#define ENABLE_CONTINUOUS        0
#define ENABLE_PARAMS	         0
#define ENABLE_OVERLAY		     0
#define ENABLE_ISP_ADDRESSES	 0
#define ENABLE_IN_FRAME		     0
#define ENABLE_OUT_FRAME	     0
#define ISP_DMEM_PARAMETERS_SIZE sizeof(isp_dmem_parameters)
#define ISP_PIPELINING  	     0
#define OUTPUT_NUM_CHUNKS     	 0
#define DUMMY_BUF_VECTORS     	 0
#define ISP_LEFT_CROPPING	     0
#define ISP_TOP_CROPPING	     0
#define ENABLE_DVS_ENVELOPE   	     0
#define ISP_MAX_DVS_ENVELOPE_WIDTH   0
#define ISP_MAX_DVS_ENVELOPE_HEIGHT  0
#define ISP_MAX_OUTPUT_WIDTH	 0
#define ISP_MAX_OUTPUT_HEIGHT	 0
#define ISP_MIN_OUTPUT_WIDTH	 0
#define ISP_MIN_OUTPUT_HEIGHT	 0
#define ISP_OUTPUT_FORMATS	     MY_OUTPUT_FORMATS
#define MAX_VF_LOG_DOWNSCALE     0
#define ISP_INPUT		         IA_CSS_BINARY_INPUT_MEMORY
#define ISP_C_SUBSAMPLING	     DEFAULT_C_SUBSAMPLING
#define ISP_FIXED_S3A_DECI_LOG   0

/* *************************
 * Cropping 
 * ************************* */
/* #define OUT_CROP_MAX_POS_X       ISP_VEC_NELEMS */
/* #define OUT_CROP_MAX_POS_Y       ISP_VEC_NELEMS */
/* #define VF_CROP_MAX_POS_X        ISP_VEC_NELEMS */
/* #define VF_CROP_MAX_POS_Y        ISP_VEC_NELEMS */

/* #define _ISP_MAX_VF_INPUT_WIDTH \ */
/* 	__ISP_MAX_VF_OUTPUT_WIDTH(2*ISP_MAX_OUTPUT_WIDTH,\ */
/* 				  ISP_LEFT_CROPPING) */
/* #define ISP_MAX_VF_INPUT_VECS CEIL_DIV(_ISP_MAX_VF_INPUT_WIDTH, ISP_VEC_NELEMS) */
/* #define MAX_VFPP_INPUT_VECTORS_PER_LINE     ISP_MAX_VF_INPUT_VECS */
/* #define MAX_VFPP_INPUT_C_VECTORS_PER_LINE   CEIL_DIV(ISP_MAX_VF_INPUT_VECS,2) */

/* #define _ISP_MAX_VFPP_OUTPUT_WIDTH \ */
/* 	__ISP_MAX_VF_OUTPUT_WIDTH(2*SH_CSS_MAX_VF_WIDTH, ISP_LEFT_CROPPING) */
/* #define ISP_MAX_VFPP_OUTPUT_VECS CEIL_DIV(_ISP_MAX_VFPP_OUTPUT_WIDTH, ISP_VEC_NELEMS) */

/* /\* output size = vga *\/ */
/* #define MAX_VFPP_OUTPUT_VECTORS_PER_LINE    ISP_MAX_VFPP_OUTPUT_VECS */
/* #define MAX_VFPP_OUTPUT_C_VECTORS_PER_LINE  CEIL_DIV(ISP_MAX_VFPP_OUTPUT_VECS,2) */

/* #define VF_PP_VMEM_IN_DIMY                  4 */
/* #define VF_PP_VMEM_IN_DIMU                  2 */

/* #define UDS_VECTORS_PER_LINE_IN           MAX_VFPP_INPUT_VECTORS_PER_LINE */
/* #define UDS_VECTORS_PER_LINE_OUT          MAX_VFPP_OUTPUT_VECTORS_PER_LINE */
/* #define UDS_VECTORS_C_PER_LINE_IN         MAX_VFPP_INPUT_C_VECTORS_PER_LINE */
/* #define UDS_VECTORS_C_PER_LINE_OUT        MAX_VFPP_OUTPUT_C_VECTORS_PER_LINE */
/* #define UDS_VMEM_GDC_IN_BLOCK_HEIGHT_Y    VF_PP_VMEM_IN_DIMY  */
/* #define UDS_VMEM_GDC_IN_BLOCK_HEIGHT_C    VF_PP_VMEM_IN_DIMU */

#include "../common/defs.h"

/* ************************
 * DMA 
 * ************************/
/* #define NUM_LUT_SUB_XFERS             1 */
/* #define NUM_SDIS_SUB_XFERS            1 */
/* #define NUM_ISPPARM_XFERS             1 */
/* #define NUM_FPNTBL_SUB_XFERS          1 */
/* #define NUM_SCTBL_SUB_XFERS           1 */
/* #define NUM_OUTPUT_SUB_XFERS          1 */
/* #define NUM_OUTPUT2L_SUB_XFERS        1 */
/* #define NUM_C_SUB_XFERS               1 */
/* #define NUM_PIXEL_SUB_XFERS           1 */
/* #define NUM_TNR_SUB_XFERS             1 */
/* #define NUM_VF_GDCOUT_SUB_XFERS       1 */
/* #define NUM_RAW_SUB_XFERS             1 */
/* #define NUM_VFOUT_SUB_XFERS           1 */
/* #define NUM_VFC_SUB_XFERS             1 */
/* #define NUM_QPLANE_SUB_XFERS          1 */
/* #define NUM_PLANE_SUB_XFERS           2 */

#include "add.hive.h"

#endif /* _ISP_ADD_DEFS_FOR_HIVE_H */

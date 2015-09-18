#include "add.hive.h"

#ifdef CSS_RUNTIME
#include <isp.h>
#include "hive_isp_css_isp_dma_api_modified.h"
#define IO_HEADER_ONLY
#include "isp_const.h"
#include "dma_proxy.isp.h"
#include "dma_proxy_commands.isp.h"
unsigned   NO_SYNC sh_dma_cmd_buffer_idx;
unsigned   NO_SYNC sh_dma_cmd_buffer_cnt;
unsigned   NO_SYNC sh_dma_cmd_buffer_need_ack;
unsigned   NO_SYNC sh_dma_cmd_buffer_enabled;
sh_dma_cmd SYNC_WITH(DMA_PROXY_BUF) * NO_SYNC sh_dma_cmd_ptr = &sh_dma_cmd_buffer[0];
sh_dma_cmd SYNC_WITH(DMA_PROXY_BUF) sh_dma_cmd_buffer[MAX_DMA_CMD_BUFFER];
#else
#include <hive_isp_css_isp_api.h>
#endif
#include <math_support.h>

static unsigned io_requests = 0;

#define DDR_BITS_PER_ELEMENT 16
#define ELEMS_PER_LINE       256
#define DMA_CH               0
#define DMA_ELEMS_ISP        ISP_VEC_NELEMS
#define DMA_ELEMS_DDR        (XMEM_WIDTH_BITS/DDR_BITS_PER_ELEMENT)
#define DMA_WIDTH_ISP        CEIL_DIV(ELEMS_PER_LINE, DMA_ELEMS_ISP)
#define DMA_WIDTH_DDR        CEIL_DIV(ELEMS_PER_LINE, DMA_ELEMS_DDR)

add_params isp_dmem_parameters;
static volatile tvector MEM(VMEM) in_buf_a[4];
static volatile tvector MEM(VMEM) in_buf_b[4];
static volatile tvector MEM(VMEM) out_buf[4];

static inline void
configure_dma(void)
{
    int height = 1;
#ifdef CSS_RUNTIME
    int stride_isp = 0, /* only used when height > 1 */
        crop_isp   = 0,
        width_isp  = DMA_WIDTH_ISP,
        stride_ddr = 0, /* only used when height > 1 */
        crop_ddr   = 0,
        width_ddr  = DMA_WIDTH_DDR;
    ISP_DMA_CONFIGURE_CHANNEL(DMA_CH, dma_isp_to_ddr_connection, dma_zero_extension, height,
                              stride_isp, DMA_ELEMS_ISP, crop_isp, width_isp,
                              stride_ddr, DMA_ELEMS_DDR, crop_ddr, width_ddr, 1);
#else
    isp_dma_configure_vmem_ddr_unsigned(DMA_CH, ELEMS_PER_LINE, height, DDR_BITS_PER_ELEMENT);
#endif
}

static inline void
read_inputs(void)
{
#ifdef CSS_RUNTIME
    dma_proxy_buffer_start();
    ISP_DMA_READ_DATA(1, DMA_CH, in_buf_a, isp_dmem_parameters.in_ptr_a, 0, 0, 0);
    ISP_DMA_READ_DATA(1, DMA_CH, in_buf_b, isp_dmem_parameters.in_ptr_b, 0, 0, 0);
    io_requests = dma_proxy_use_buffer_cmds();
    /* Acknowledge all outstanding transactions from DMA proxy */
    ISP_DMA_WAIT_FOR_ACK_N (DMA_CH, 1, io_requests, 1) SYNC(DMA_PROXY_BUF);
#else
    isp_dma_read_data(DMA_CH, in_buf_a, isp_dmem_parameters.in_ptr_a);
    isp_dma_read_data(DMA_CH, in_buf_b, isp_dmem_parameters.in_ptr_b);
    isp_dma_wait_for_ack();
    isp_dma_wait_for_ack();
#endif
}

static inline void
write_outputs(void)
{
#ifdef CSS_RUNTIME
    dma_proxy_buffer_start();
    ISP_DMA_WRITE_DATA(1, DMA_CH, out_buf, isp_dmem_parameters.out_ptr, 0, 0, 0);
    io_requests = dma_proxy_use_buffer_cmds();
    ISP_DMA_WAIT_FOR_ACK_N (DMA_CH, 1, io_requests, 1) SYNC(DMA_PROXY_BUF);
#else
    isp_dma_write_data(DMA_CH, out_buf, isp_dmem_parameters.out_ptr);
    isp_dma_wait_for_ack();
#endif
}

void isp_binary_main(void) ENTRY
{
    int i;

    configure_dma();
    read_inputs();

    for (i = 0; i < 4; i++) {
        out_buf[i] = in_buf_a[i] + in_buf_b[i];
    }

    write_outputs();


    isp_dmem_parameters.acc_ctrl = 1; /* done */
#ifdef CSS15
#ifdef CSS_RUNTIME
    event_send_token(SP0_EVENT_ID,
		     _irq_proxy_create_cmd_token(IRQ_PROXY_RAISE_CMD));
#endif
#endif
}

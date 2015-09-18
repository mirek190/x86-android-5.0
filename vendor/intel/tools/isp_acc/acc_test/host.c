#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HRT_CSIM
#include <string.h>
#endif

#ifndef CSS_RUNTIME
#include <hive_isp_css_hrt.h>
#include <hmm/hmm.h>
#ifdef C_RUN
#include "add.hive.h"
#else
#include "add.map.h"
#endif
#endif

#include "add_types.h"
#define NUM_VALUES 256

#ifdef CSS_RUNTIME
#include "ia_acc.h"
#define FW_PATH "/sdcard/isp/add.bin"
char *g_fw_path;

#define DMEM0_INDEX 0x01

#ifndef STR
#define _STR(x)   #x
#define STR(x)    _STR(x)
#endif

#ifdef C_RUN
static const char *csim_target = "crun";
#elif defined(HRT_SCHED)
static const char *csim_target = "sched";
#else
static const char *csim_target = "target";
#endif

static int
run_add(uint16_t *in_a, uint16_t *in_b, uint16_t *out, int num_values)
{
    void *fw;
    int result = 0;
    unsigned fw_size,
             acc_handle,
             plane_size = num_values*sizeof(uint16_t),
             prm_size = sizeof(add_params);
    ia_acc_buf *in_buf_a,
               *in_buf_b,
               *out_buf,
               *prm_buf;
    add_params *params;

    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    /* Initialize acceleration API */
    result = ia_acc_open(csim_target);
    if (result < 0) {
        printf("%s: Failed to initialize ACC, quitting with err = %d\n", __func__,
                result);
        return result;
    }

    fw = ia_acc_open_firmware(g_fw_path, &fw_size);
    if (fw == NULL) {
        printf("%s: Unable to open firmware: %s\n", __func__, g_fw_path);
        return -1;
    }

    result = ia_acc_load_firmware(fw, fw_size, &acc_handle);
    if (result < 0) {
        printf("%s: Failed to load firmware, quitting with err = %d\n", __func__,
                result);
        return result;
    }

    params = ia_acc_alloc(prm_size);
    in_buf_a = ia_acc_buf_create(in_a, plane_size);
    in_buf_b = ia_acc_buf_create(in_b, plane_size);
    out_buf  = ia_acc_buf_create(out,  plane_size); 
    prm_buf  = ia_acc_buf_create(params, prm_size);

    /* Initialize parameters */
    params->acc_ctrl   = 0;
    params->num_values = num_values;
    params->in_ptr_a   = in_buf_a->css_ptr;
    params->in_ptr_b   = in_buf_b->css_ptr;
    params->out_ptr    = out_buf->css_ptr;
    ia_acc_buf_sync_to_css(in_buf_a);
    ia_acc_buf_sync_to_css(in_buf_b);
    ia_acc_buf_sync_to_css(prm_buf);

    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    ia_acc_set_mapped_arg(acc_handle, DMEM0_INDEX, prm_buf->css_ptr, prm_size);

    /* Run binary and wait for it to finish */
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    result = ia_acc_start_firmware(acc_handle);
    printf ("At %s(%d) result %d\n", __FUNCTION__, __LINE__, result);
    if (result < 0) {
        printf("%s: Failed to start firmware, quitting with err = %d\n", __func__,
                result);
        return result;
    }

    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    result = ia_acc_wait_for_firmware(acc_handle);
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    if (result < 0) {
        printf("%s: Failed during wait for firmware, quitting with err = %d\n",
                __func__, result);
        return result;
    }

    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    ia_acc_buf_sync_to_cpu(out_buf);
    ia_acc_buf_free(in_buf_a);
    ia_acc_buf_free(in_buf_b);
    ia_acc_buf_free(out_buf);
    ia_acc_buf_free(prm_buf);

    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    ia_acc_free(params);

    /* Cleanup acceleration API */
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    ia_acc_unload_firmware(acc_handle);
    free(fw);
    /* ia_acc_close(); */
    return 0;
}

#else
static int
run_add(uint16_t *in_a, uint16_t *in_b, uint16_t *out, int num_values)
{
    /* On CSim we cannot share physical memory between host and ISP, so we allocate
     * duplicate buffers in ISP memory and copy the data back and forth. */
    int plane_size = num_values*sizeof(uint16_t);
    hmm_ptr in_ptr_a = hrt_isp_css_mm_calloc(plane_size),
            in_ptr_b = hrt_isp_css_mm_calloc(plane_size),
            out_ptr  = hrt_isp_css_mm_calloc(plane_size);
    add_params params;
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);

    /* Initialize parameters */
    params.acc_ctrl   = 0;
    params.in_ptr_a   = in_ptr_a;
    params.in_ptr_b   = in_ptr_b;
    params.out_ptr    = out_ptr;
    params.num_values = num_values;

    /* Copy input to ISP memory */
    hrt_isp_css_mm_store(in_ptr_a, in_a, plane_size);
    hrt_isp_css_mm_store(in_ptr_b, in_b, plane_size);
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);

    /* Load the binary */
    hrt_cell_load_program_id(ISP, add);
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);

    /* Write the parameters structure to ISP local memory */
#ifdef C_RUN
    *((add_params*)_hrt_cell_get_crun_symbol(ISP, isp_dmem_parameters)) = params;
#else
    hrt_mem_store(ISP, hrt_isp_dmem(ISP), HIVE_ADDR_isp_dmem_parameters, &params, sizeof(params));
#endif
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);

    /* Run the ISP function and wait for it to complete */
    hrt_cell_run_function(ISP, isp_binary_main);
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);

    /* Copy the output data back to host memory */
    hrt_isp_css_mm_load(out_ptr, out, plane_size);
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);

    hrt_isp_css_mm_free(in_ptr_a);
    hrt_isp_css_mm_free(in_ptr_b);
    hrt_isp_css_mm_free(out_ptr);
    return 0;
}
#endif

int hrt_main()
{
    int errors = 0, i;
    uint16_t *in_a,
             *in_b,
             *out;
    // Need aligned mallocs
#ifdef CSS_RUNTIME
    in_a = ia_acc_alloc(sizeof(uint16_t)*NUM_VALUES);
    in_b = ia_acc_alloc(sizeof(uint16_t)*NUM_VALUES);
    out  = ia_acc_alloc(sizeof(uint16_t)*NUM_VALUES);
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);

    if (!strncmp(csim_target, "target", strlen("target")))
        asprintf(&g_fw_path, "%s", FW_PATH);
    else
        asprintf(&g_fw_path, "../%s_%s/add.bin", STR(SYSTEM), csim_target);
#else
    in_a = malloc(sizeof(uint16_t)*NUM_VALUES);
    in_b = malloc(sizeof(uint16_t)*NUM_VALUES);
    out  = malloc(sizeof(uint16_t)*NUM_VALUES);
#endif

    for (i=0; i<NUM_VALUES; i++) {
        in_a[i] = i;
        in_b[i] = i;
        out[i] = -1;
    }

    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    errors = run_add(in_a, in_b, out, NUM_VALUES);
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);
    if (errors != 0)
      return errors != 0;

    for (i=0; i<NUM_VALUES; i++) {
        if (out[i] != 2*i) {
            printf("error in output at index %d (expected %d, got %d)\n",
		   i, 2*i, out[i]);
            errors++;
        }
    }
    printf ("At %s(%d)\n", __FUNCTION__, __LINE__);

#ifdef CSS_RUNTIME
    ia_acc_free(in_a);
    ia_acc_free(in_b);
    ia_acc_free(out);
#else
    free(in_a);
    free(in_b);
    free(out);
#endif
    if (errors == 0)
        printf ("Add Test successful!\n");
    else
        printf ("Add Test failed with %d errors!\n", errors);

    return (errors != 0);
}

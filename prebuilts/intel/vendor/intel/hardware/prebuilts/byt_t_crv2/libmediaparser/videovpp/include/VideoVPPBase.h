#ifndef __VIDEO_VPP_BASE_H__
#define __VIDEO_VPP_BASE_H__
#include <system/graphics.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <va/va.h>
#include <va/va_drmcommon.h>
#include <va/va_vpp.h>
#include <va/va_android.h>
#include <va/va_tpi.h>

#include <hardware/gralloc.h>

#include <utils/KeyedVector.h>

class VideoVPPBase;

struct FilterConfig {
    enum strength {
        LOW,
        MEDIUM,
        HIGH,
    };
    bool valid;
    int type;
    float min, max, step, def;
    float cur;
};

class VPParameters {
public:
    static VPParameters* create(VideoVPPBase *);
    ~VPParameters();
    VAStatus buildfilters();
    VAStatus reset(bool start);
    void getNR(FilterConfig& NR) { memcpy(&NR, &nr, sizeof(FilterConfig)); }
    void setNR(FilterConfig::strength str) { nr.cur = str; }
    void getDeblock(FilterConfig &DBK) { memcpy(&DBK, &deblock, sizeof(FilterConfig)); }
    void SetDeblock(FilterConfig::strength str) { deblock.cur = str; }
    void getSharpen(FilterConfig &SHP) { memcpy(&SHP, &sharpen, sizeof(FilterConfig)); }
    void setSharpen(FilterConfig SHP) { sharpen.cur = SHP.cur; }

private:
    bool mInitialized;
    VADisplay va_display;
    VAContextID va_context;
    VAStatus vret;

    VAProcFilterType supported_filters[VAProcFilterCount];
    unsigned int num_supported_filters;

    VAProcFilterCap denoise_caps, sharpen_caps, deblock_caps;
    VAProcFilterCapColorBalance color_balance_caps[VAProcColorBalanceCount];
    unsigned int num_denoise_caps, num_color_balance_caps, num_sharpen_caps, num_deblock_caps;

    VAProcFilterParameterBuffer denoise_buf, sharpen_buf, deblock_buf;
    VAProcFilterParameterBufferColorBalance balance_buf[VAProcColorBalanceCount];
    VABufferID sharpen_buf_id, denoise_buf_id, deblock_buf_id, balance_buf_id;

    VABufferID filter_bufs[VAProcFilterCount];
    unsigned int num_filter_bufs;

    FilterConfig nr;
    FilterConfig deblock;
    FilterConfig sharpen;
    FilterConfig colorbalance[VAProcColorBalanceCount];

    VPParameters(VideoVPPBase *);
    VPParameters(const VPParameters&);
    VPParameters &operator=(const VPParameters&);

    VAStatus init();

    friend class VideoVPPBase;
};

struct RenderTarget {
    enum bufType{
        KERNEL_DRM,
        ANDROID_GRALLOC,
    };

    int width;
    int height;
    int stride;
    bufType type;
    int format;
    int pixel_format;
    int handle;
    VARectangle rect;
};

class VideoVPPBase {
public:
    VideoVPPBase();
    ~VideoVPPBase();
    VAStatus start();
    VAStatus stop();
    VAStatus perform(RenderTarget Src, RenderTarget Dst, VPParameters *vpp, bool no_wait);

private:
    bool mInitialized;
    unsigned width, height;
    VAStatus vret;
    VADisplay va_display;
    VAConfigID va_config;
    VAContextID va_context;
    VABufferID vpp_pipeline_buf;
    VAProcPipelineParameterBuffer vpp_param;
    VASurfaceAttrib SrcSurfAttrib, DstSurfAttrib;
    VASurfaceAttribExternalBuffers  SrcSurfExtBuf, DstSurfExtBuf;
    VASurfaceID SrcSurf, DstSurf;
    VASurfaceAttributeTPI attribs;

	VABufferID filter_bufs[VAProcFilterCount];
	unsigned int num_filter_bufs;

    KeyedVector<int, VASurfaceID> SrcSurfHandleMap;
    KeyedVector<int, VASurfaceID> DstSurfHandleMap;

    VideoVPPBase(const VideoVPPBase &);
    VideoVPPBase &operator=(const VideoVPPBase &);

    VAStatus _perform(VASurfaceID SrcSurf, VARectangle SrcRect,
            VASurfaceID DstSurf, VARectangle DstRect, bool no_wait);

    VAStatus _CreateSurfaceFromGrallocHandle(RenderTarget rt, VASurfaceID *surf);

    friend class VPParameters;

};

#endif

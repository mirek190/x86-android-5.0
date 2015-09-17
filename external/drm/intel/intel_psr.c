#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xf86drm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>

#include "string.h"

#include "i915_drm.h"
#include "intel_psr.h"



/* LibDRM Function to enable/disable eDP PSR */
void drmeDPpsrCtl(int fd, int enable, int idle_frames)
{
	struct drm_i915_edp_psr_ctl data;
	data.state = enable;
	data.idle_frames = idle_frames;
	drmIoctl(fd, DRM_IOCTL_I915_EDP_PSR_CTL, &data);
}

/* LibDRM Function to exit eDP PSR */
void drmeDPpsrExit(int fd)
{
	drmIoctl(fd, DRM_IOCTL_I915_EDP_PSR_EXIT, NULL);
}

/* LibDRM Function to get Panel PSR Support */
bool drmeDPgetPsrSupport(int fd)
{
	bool res = false;
	drmIoctl(fd, DRM_IOCTL_I915_GET_PSR_SUPPORT, &res);
	return res;
}

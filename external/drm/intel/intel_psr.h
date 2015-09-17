#include <stdio.h>
#include <stdint.h>
#include <stdio.h>

void drmeDPpsrCtl(int fd, int enable, int idle_frames);
void drmeDPpsrExit(int fd);
bool drmeDPgetPsrSupport(int fd);

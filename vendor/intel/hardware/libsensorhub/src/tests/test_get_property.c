#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <cutils/log.h>

#include "../include/libsensorhub.h"
#include "../include/bist.h"
#include "../include/message.h"

int main(int argc, char **argv)
{
	handle_t handle;
	error_t ret;
	char buf[512];
	int len;

	handle = psh_open_session_with_name("ACCEL");

	if (handle == NULL) {
		printf("psh_open_session() returned NULL handle. \n");
		return -1;
	}

	ret = psh_get_property_with_size(handle, 4, buf, &len, buf);
	if (ret == ERROR_NONE)
		printf("get property %d,%s\n", len, buf);
	else
		printf("Can not get property\n");

	psh_close_session(handle);

	return 0;
}

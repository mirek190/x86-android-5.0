#include <unistd.h>
#include <ctime>
#include "utils.hpp"
#include <utils/Log.h>
#define MAX_LOG_SIZE 256
int64_t timevalToNano(timeval const& t)
{
        return static_cast<int64_t>(t.tv_sec)*1000000000LL + static_cast<int64_t>(t.tv_usec)*1000;
}

int64_t getTimestamp()
{
        struct timespec t = { 0, 0 };
        clock_gettime(CLOCK_MONOTONIC, &t);
        return static_cast<int64_t>(t.tv_sec)*1000000000LL + static_cast<int64_t>(t.tv_nsec);
}

static const char *log_file = "/data/sensorHalUmg.log";

static message_level current_level = DEBUG;

void set_log_level(message_level log_level)
{
	if (log_level >= MAX_LEVEL)
		return;

	current_level = log_level;
}

void log_message(const message_level level, const char *char_ptr, ...)
{
	char buffer[MAX_LOG_SIZE];
	va_list list_ptr;

	if (level > current_level)
		return;

	va_start(list_ptr, char_ptr);
	vsnprintf(buffer, MAX_LOG_SIZE, char_ptr, list_ptr);
	va_end(list_ptr);

	FILE *logf = fopen(log_file, "a");
	if (logf) {
		fprintf(logf, "%ld: %d %s", time(NULL), level, buffer);
		fclose(logf);
	} else {
		printf("Open log file failed, errno is %d \n", errno);
	}
}

#ifndef _UTILS_HPP_
#define _UTILS_HPP_

int64_t timevalToNano(timeval const& t);
int64_t getTimestamp();
typedef enum {
	CRITICAL,
	WARNING,
	DEBUG,
	MAX_LEVEL
} message_level;

void set_log_level(message_level log_level);

void log_message(const message_level level, const char *char_ptr, ...);

#endif

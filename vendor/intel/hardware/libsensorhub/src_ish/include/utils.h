#ifdef __cplusplus
extern "C" {
#endif

#include <utils/Log.h>

typedef enum {
	CRITICAL,
	WARNING,
	DEBUG,
	MAX_LEVEL
} message_level;

void set_log_level(message_level log_level);
void set_log_file(const char *fname);

void log_message(const message_level level, const char *char_ptr, ...);

int max_common_factor(int data1, int data2);

int least_common_multiple(int data1, int data2);

int is_first_instance();

#ifdef __cplusplus
}
#endif
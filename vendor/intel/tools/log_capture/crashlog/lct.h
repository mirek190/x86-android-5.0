#ifndef LCT_H_
# define LCT_H_

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/kct.h>

/* prefix of properties used to filter events */
#define	PROP_PREFIX	"dev.log"

#define PKT_SIZE(Ev) (((struct ct_event*)(Ev))->attchmt_size + sizeof(struct ct_event))
#define SK_NAME "\0crashtool_socket"

#define MKFN(fn,...) MKFN_N(fn,##__VA_ARGS__,8,7,6,5,4,3,2,1,0)(__VA_ARGS__)
#define MKFN_N(fn,n0,n1,n2,n3,n4,n5,n6,n7,n,...) fn##n
#define lct_log(...) MKFN(__lct_log_,##__VA_ARGS__)

#define __lct_log_4(Type, Submitter_name, Ev_name, flags) \
	do { \
		struct ct_event *__ev =	\
			lct_alloc_event(Submitter_name, Ev_name, Type, flags); \
		if (__ev) { \
			lct_log_event(__ev); \
		} \
	} while (0)

#define __lct_log_5(Type, Submitter_name, Ev_name, flags, Data0) \
	do { \
		struct ct_event *__ev =	\
			lct_alloc_event(Submitter_name, Ev_name, Type, flags); \
		if (__ev) { \
			lct_add_attchmt(&__ev, CT_ATTCHMT_DATA0, \
					strlen(Data0) + 1, Data0); \
			lct_log_event(__ev); \
		} \
	} while (0)

#define __lct_log_6(Type, Submitter_name, Ev_name, flags, Data0, Data1) \
	do { \
		struct ct_event *__ev =	\
			lct_alloc_event(Submitter_name, Ev_name, Type, flags); \
		if (__ev) { \
			lct_add_attchmt(&__ev, CT_ATTCHMT_DATA0, \
					strlen(Data0) + 1, Data0); \
			lct_add_attchmt(&__ev, CT_ATTCHMT_DATA1, \
					strlen(Data1) + 1, Data1); \
			lct_log_event(__ev); \
		} \
	} while (0)

#define __lct_log_7(Type, Submitter_name, Ev_name, flags, Data0, Data1, Data2) \
	do { \
		struct ct_event *__ev =	\
			lct_alloc_event(Submitter_name, Ev_name, Type, flags); \
		if (__ev) { \
			lct_add_attchmt(&__ev, CT_ATTCHMT_DATA0, \
					strlen(Data0) + 1, Data0); \
			lct_add_attchmt(&__ev, CT_ATTCHMT_DATA1, \
					strlen(Data1) + 1, Data1); \
			lct_add_attchmt(&__ev, CT_ATTCHMT_DATA2, \
					strlen(Data2) + 1, Data2); \
			lct_log_event(__ev); \
		} \
	} while (0)

/* keep ONE return to ensure inline */
extern inline struct ct_event *lct_alloc_event(const char *submitter_name,
					       const char *ev_name,
					       enum ct_ev_type ev_type,
					       unsigned int flags)
{
	struct ct_event *ev = NULL;

	if (submitter_name && ev_name) {
		ev = calloc(1, sizeof(*ev));
		if (ev)  {
			strncpy(ev->submitter_name, submitter_name, sizeof(ev->submitter_name)-1);
			strncpy(ev->ev_name, ev_name, sizeof(ev->ev_name)-1);
		
			ev->timestamp = time(NULL);
			ev->flags = flags;
			ev->type = ev_type;
		}
	}

	return ev;
}

extern inline int lct_add_attchmt(struct ct_event **ev,
				  enum ct_attchmt_type at_type,
				  unsigned int size, const char *data)
{
	__u32 new_size = sizeof(struct ct_event) + (*ev)->attchmt_size +
		KCT_ALIGN(size + sizeof(struct ct_attchmt), ATTCHMT_ALIGNMENT);
	struct ct_event *new_ev = new_ev = realloc(*ev, new_size);
	if (new_ev) {
		struct ct_attchmt *new_attchmt = (struct ct_attchmt *)
			(((char *) new_ev->attachments) + new_ev->attchmt_size);

		new_attchmt->size = size;
		new_attchmt->type = at_type;
		memcpy(new_attchmt->data, data, size);

		new_ev->attchmt_size = new_size - sizeof(*new_ev);

		*ev = new_ev;
	}
	return new_ev ? 0 : -ENOMEM;
}

extern inline int lct_log_event(struct ct_event *ev)
{
	int ret = -1;
	int sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock_fd > 0) {
		const struct sockaddr_un addr = {
			.sun_family = AF_UNIX,
			.sun_path = SK_NAME,
		};
		ret = sendto(sock_fd, ev, PKT_SIZE(ev),
			0x0, (const struct sockaddr*) &addr,
			sizeof(addr));
		close(sock_fd);
		free(ev);
	}
	return ret;
}

#endif /* !LCT_H_ */

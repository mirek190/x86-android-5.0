#ifndef __MMGR_CLI_H__
#define __MMGR_CLI_H__

#define MMGR_EVENTS \
    /* Events notification: MMGR -> Clients */ \
    X(EVENT_MODEM_DOWN),\
    X(EVENT_MODEM_UP),\
    X(EVENT_MODEM_OUT_OF_SERVICE),\
    /* Notifications: MMGR -> Clients */ \
    X(NOTIFY_MODEM_WARM_RESET),\
    X(NOTIFY_MODEM_COLD_RESET),\
    X(NOTIFY_MODEM_SHUTDOWN),\
    X(NOTIFY_PLATFORM_REBOOT),\
    X(NOTIFY_CORE_DUMP),\
    /* ACK: MMGR -> Clients */ \
    X(ACK),\
    X(NACK),\
    /* Notifications for crashtool */\
    X(NOTIFY_CORE_DUMP_COMPLETE),\
    X(NOTIFY_AP_RESET),\
    X(NOTIFY_SELF_RESET),\
    X(NOTIFY_ERROR),\
    /* flashing notifications */ \
    X(RESPONSE_MODEM_RND),\
    X(RESPONSE_MODEM_HW_ID),\
    X(RESPONSE_MODEM_NVM_ID),\
    X(RESPONSE_MODEM_FW_PROGRESS),\
    X(RESPONSE_MODEM_FW_RESULT),\
    X(RESPONSE_MODEM_NVM_PROGRESS),\
    X(RESPONSE_MODEM_NVM_RESULT),\
    X(RESPONSE_FUSE_INFO),\
    X(RESPONSE_GET_BACKUP_FILE_PATH),\
    X(RESPONSE_BACKUP_PRODUCTION_RESULT),\
    X(NUM_EVENTS)

typedef enum e_mmgr_events {
#undef X
#define X(a) E_MMGR_##a
    MMGR_EVENTS
} e_mmgr_events_t;

typedef void *mmgr_cli_handle_t;

typedef struct mmgr_cli_event {
    e_mmgr_events_t id;
    size_t len;
    void *data;
    void *context;
} mmgr_cli_event_t;

#define CORE_DUMP_STATE \
    X(SUCCEED),\
    X(SUCCEED_WITHOUT_PANIC_ID),\
    X(FAILED),\
    X(FAILED_WITH_PANIC_ID)

typedef enum e_core_dump_state {
#undef X
#define X(a) E_CD_##a
    CORE_DUMP_STATE
} e_core_dump_state_t;

typedef struct mmgr_cli_core_dump {
    e_core_dump_state_t state;
    int panic_id;
    size_t len;
    char *path;
} mmgr_cli_core_dump_t;

typedef struct mmgr_cli_ap_reset {
    size_t len;
    char *name;
} mmgr_cli_ap_reset_t;

typedef struct mmgr_cli_error {
    int id;
    size_t len;
    char *reason;
} mmgr_cli_error_t;

static inline void mmgr_cli_create_handle() {}
static inline void mmgr_cli_subscribe_event() {}
static inline void mmgr_cli_connect() {}
static inline void mmgr_cli_disconnect() {}
static inline void mmgr_cli_delete_handle() {}


#endif /* __MMGR_CLI_H__ */

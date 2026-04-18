#ifndef VELO_PLAYER_MINIMAL_MPV_CLIENT_H_
#define VELO_PLAYER_MINIMAL_MPV_CLIENT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mpv_handle mpv_handle;

typedef enum mpv_format {
    MPV_FORMAT_NONE = 0,
    MPV_FORMAT_STRING = 1,
    MPV_FORMAT_OSD_STRING = 2,
    MPV_FORMAT_FLAG = 3,
    MPV_FORMAT_INT64 = 4,
    MPV_FORMAT_DOUBLE = 5,
    MPV_FORMAT_NODE = 6,
    MPV_FORMAT_NODE_ARRAY = 7,
    MPV_FORMAT_NODE_MAP = 8,
    MPV_FORMAT_BYTE_ARRAY = 9
} mpv_format;

typedef enum mpv_event_id {
    MPV_EVENT_NONE = 0,
    MPV_EVENT_SHUTDOWN = 1,
    MPV_EVENT_LOG_MESSAGE = 2,
    MPV_EVENT_GET_PROPERTY_REPLY = 3,
    MPV_EVENT_SET_PROPERTY_REPLY = 4,
    MPV_EVENT_COMMAND_REPLY = 5,
    MPV_EVENT_START_FILE = 6,
    MPV_EVENT_END_FILE = 7,
    MPV_EVENT_FILE_LOADED = 8,
    MPV_EVENT_TRACKS_CHANGED = 9,
    MPV_EVENT_TRACK_SWITCHED = 10,
    MPV_EVENT_IDLE = 11,
    MPV_EVENT_PAUSE = 12,
    MPV_EVENT_UNPAUSE = 13,
    MPV_EVENT_TICK = 14,
    MPV_EVENT_SCRIPT_INPUT_DISPATCH = 15,
    MPV_EVENT_CLIENT_MESSAGE = 16,
    MPV_EVENT_VIDEO_RECONFIG = 17,
    MPV_EVENT_AUDIO_RECONFIG = 18,
    MPV_EVENT_METADATA_UPDATE = 19,
    MPV_EVENT_SEEK = 20,
    MPV_EVENT_PLAYBACK_RESTART = 21,
    MPV_EVENT_PROPERTY_CHANGE = 22,
    MPV_EVENT_CHAPTER_CHANGE = 23,
    MPV_EVENT_QUEUE_OVERFLOW = 24,
    MPV_EVENT_HOOK = 25
} mpv_event_id;

struct mpv_node;

typedef struct mpv_byte_array {
    void* data;
    size_t size;
} mpv_byte_array;

typedef struct mpv_node_list {
    int num;
    struct mpv_node* values;
    char** keys;
} mpv_node_list;

typedef struct mpv_node {
    mpv_format format;
    union {
        char* string;
        int flag;
        int64_t int64;
        double double_;
        mpv_node_list* list;
        mpv_byte_array* ba;
    } u;
} mpv_node;

typedef struct mpv_event_property {
    const char* name;
    mpv_format format;
    void* data;
} mpv_event_property;

typedef struct mpv_event_log_message {
    const char* prefix;
    const char* level;
    const char* text;
    const char* log_level;
} mpv_event_log_message;

typedef struct mpv_event_end_file {
    int reason;
    int error;
    int playlist_entry_id;
    int playlist_insert_id;
    int playlist_insert_num_entries;
} mpv_event_end_file;

typedef struct mpv_event {
    mpv_event_id event_id;
    int error;
    uint64_t reply_userdata;
    void* data;
} mpv_event;

typedef void (*mpv_wakeup_fn)(void* data);

#ifdef __cplusplus
}
#endif

#endif
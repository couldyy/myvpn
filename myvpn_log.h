#ifndef MYVPN_LOG_H
#define MYVPN_LOG_H
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// TODO let user specify a runtime flag that will enable messages up to a certain level
// -e   -> print only errors
// -v   -> errors + MYVPN_LOG
// -vv   -> errors + MYVPN_LOG + MYVPN_LOG_VERBOSE
// -vvv   -> errors + MYVPN_LOG + MYVPN_LOG_VERBOSE + MYVPN_LOG_VERBOSE_VERBOSE
#define LOG_LEVEL_LIST \
    X(MYVPN_LOG_ERROR) \
    X(MYVPN_LOG_ERROR_VERBOSE) \
    X(MYVPN_LOG_WARNING) \
    X(MYVPN_LOG) \
    X(MYVPN_LOG_VERBOSE) \
    X(MYVPN_LOG_VERBOSE_VERBOSE)

typedef enum {
#define X(name) name,
    LOG_LEVEL_LIST
#undef X
    MYVPN_LOG_MAX
} Log_level;

// TODO provide different levels for error and normal logging (or maybe use same level for both, 
//      _VERBOSE for log and error, _VERBOSE_VERBOSE and so on)
typedef struct {
    FILE* log_file;
    Log_level log_level;
} Log_context;

extern Log_context g_log_context;

#define MYVPN_ERROR_PREFIX "[ERROR]"
#define MYVPN_WARNING_PREFIX "[WARNING]"
#define MYVPN_LOG_PREFIX "[INFO]"

// TODO make function thread safe (mutexes)
#define MYVPN_LOG(loglevel, log_message, ...) \
    do { \
        assert(log_message != NULL); \
        /* dont do anything if user-defined log level is lower then in requested message */ \
        if(g_log_context.log_level < loglevel)   \
            break; \
     \
        char* prefix; \
        const char* at_func = __func__; \
        switch(loglevel) { \
            case MYVPN_LOG_ERROR: \
                prefix = MYVPN_ERROR_PREFIX; \
                break; \
 \
            case MYVPN_LOG_WARNING: \
                prefix = MYVPN_WARNING_PREFIX; \
                break; \
 \
            default: \
                prefix = MYVPN_LOG_PREFIX; \
                at_func = ""; \
                break; \
        } \
     \
        FILE* out_file; \
        if(g_log_context.log_file == NULL) { \
            if(loglevel == MYVPN_LOG_ERROR)  \
                out_file = stderr; \
            else  \
                out_file = stdout; \
        } \
        else { \
            out_file = g_log_context.log_file; \
        } \
    \
        time_t time_now = time(NULL); \
        struct tm time_broken_down; \
        if(gmtime_r(&time_now, &time_broken_down) == NULL) { \
            fprintf(out_file, "%s: failed to boke down time from integer representation\n", MYVPN_ERROR_PREFIX); \
            exit(1);    /* TODO dont exit on convertion failure */ \
        } \
        char gmtime_str[256]; \
        if(strftime(gmtime_str, sizeof(gmtime_str), "%a %d-%m-%Y %H:%M:%S %Z", &time_broken_down) == 0) { \
            fprintf(stderr, "%s: strftime(): %s\n", MYVPN_ERROR_PREFIX, strerror(errno)); \
            exit(1); \
        } \
        /* TODO: '##' in '##__VA_ARGS__' argument is only supported by Cland and GCC \
           find some compatible way to pass variadic argc and prevent trailing coma ( when \
           there are no args, e.g MYVPN_LOG(MYVPN_LOG, "Some log text"); will result in \
           fprintf(out_file, "%s %s:" "Some log text", prefix, gmtime_str, ); <<<------ trailing coma  */ \
        fprintf(out_file, "%s\n%s %s: "log_message"\n", gmtime_str, prefix, at_func,##__VA_ARGS__); \
    } while(0)

Log_context init_log_context(const char* log_file_path, Log_level max_loglevel);
#endif //MYVPN_LOG_H

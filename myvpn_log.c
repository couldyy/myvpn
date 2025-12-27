#include "myvpn_log.h"


Log_context init_log_context(const char* log_file_path, Log_level max_log_level)
{
    if(max_log_level >= MYVPN_LOG_MAX) {
        fprintf(stderr, "Invalid logging level: %d\n", max_log_level);
        exit(1);
    }
    if(log_file_path == NULL) {
        if(max_log_level >= MYVPN_LOG_VERBOSE_VERBOSE) {
            fprintf(stdout, "[MYVPN_LOG]: Using std inputs as loging\n");
        }
        return (Log_context) { .log_file = NULL, .log_level = max_log_level};
    }
    else {
        FILE* log_file = fopen(log_file_path, "a");
        if(log_file == NULL) {
            fprintf(stderr, "Could not open file '%s' : %s\n", log_file_path, strerror(errno));
            exit(1);
        }
        return (Log_context) { .log_file = log_file, .log_level = max_log_level};
    }
    
}


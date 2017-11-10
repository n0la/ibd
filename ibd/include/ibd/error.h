#ifndef IBD_ERROR_H
#define IBD_ERROR_H

typedef enum {
    error_success = 0,
    error_args,
    error_memory,
    error_internal,
    error_syntax,
    error_tls,
} error_t;

#endif

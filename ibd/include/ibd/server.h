#ifndef IBD_SERVER_H
#define IBD_SERVER_H

#include <stdbool.h>

typedef struct {
    char *name;
    char *host;
    char *port;
    bool ssl;
} ib_network_t;

#endif

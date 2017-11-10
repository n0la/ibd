#ifndef IBD_SERVER_H
#define IBD_SERVER_H

#include <ibd/error.h>
#include <irc/irc.h>

#include <event2/event.h>

#include <stdbool.h>
#include <tls.h>

typedef struct {
    char *name;
    char *host;
    char *port;
    char *nick;
    bool ssl;

    struct event *ev;

    int fd;
    struct tls *tls;
    struct tls_config *tls_config;

    irc_t irc;
} network_t;

network_t * network_new(void);
error_t network_set(network_t *n, char const *k, char const *v);

error_t network_connect(network_t *n, struct event_base *base);

#endif

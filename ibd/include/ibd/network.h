#ifndef IBD_SERVER_H
#define IBD_SERVER_H

#include <ibd/error.h>

#include <irc/irc.h>
#include <irc/strbuf.h>

#include <event2/event.h>

#include <stdbool.h>
#include <tls.h>

typedef struct {
    char *name;
    char *filename;

    int out;
    int in;
    int err;

    struct event *in_ev;
    struct event *err_ev;
    pid_t pid;

    char **argv;
    size_t argc;

    char **env;
    size_t envc;
} plugin_info_t;

typedef struct {
    char *name;
    char *host;
    char *port;
    char *nick;
    bool ssl;

    struct event *ev;
    struct event_base *base;

    strbuf_t plugin_buf;

    int fd;
    struct tls *tls;
    struct tls_config *tls_config;

    irc_t irc;

    plugin_info_t **plugin;
    size_t pluginlen;

} network_t;

network_t * network_new(void);
error_t network_set(network_t *n, char const *k, char const *v);

error_t network_disconnect(network_t *n);
error_t network_connect(network_t *n, struct event_base *base);

error_t network_add_plugin(network_t *n, plugin_info_t *p);
error_t network_add_arg(plugin_info_t *p, char *arg);
error_t network_add_env(plugin_info_t *p, char *env);

#endif

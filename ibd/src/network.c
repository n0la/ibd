#include <ibd/network.h>
#include <ibd/log.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

network_t * network_new(void)
{
    network_t *i = calloc(1, sizeof(network_t));

    if (!i) {
        return NULL;
    }

    i->fd = -1;

    i->irc = irc_new();
    if (i->irc == NULL) {
        free(i);
        return NULL;
    }

    irc_setopt(i->irc, ircopt_nick, "ibd");

    return i;
}

static error_t network_alloc_ssl(network_t *n)
{
    if (n->ssl && !n->tls_config) {
        n->tls_config = tls_config_new();
        if (n->tls_config == NULL) {
            log_error("malloc failed\n");
            return error_memory;
        }
    }

    return error_success;
}

error_t network_set(network_t *n, char const *k, char const *v)
{
    error_t r = 0;

    if (strcmp(k, "name") == 0) {
        free(n->name);
        n->name = strdup(v);
    } else if (strcmp(k, "host") == 0) {
        free(n->host);
        n->host = strdup(v);
        irc_setopt(n->irc, ircopt_server, n->host);
    } else if (strcmp(k, "port") == 0) {
        free(n->port);
        n->port = strdup(v);
    } else if (strcmp(k, "ssl") == 0) {
        n->ssl = (strcmp(v, "yes") == 0);
        r = network_alloc_ssl(n);
        if (r) {
            return r;
        }
    } else if (strcmp(k, "nick") == 0) {
        free(n->nick);
        n->nick = strdup(v);
        irc_setopt(n->irc, ircopt_nick, n->nick);
    } else {
        return error_args;
    }

    return error_success;
}

error_t network_disconnect(network_t *n)
{
    close(n->fd);
    n->fd = -1;

    if (n->ev) {
        event_del(n->ev);
        event_free(n->ev);
        n->ev = NULL;
    }

    if (n->tls) {
        tls_close(n->tls);
        tls_free(n->tls);
        n->tls = NULL;
    }

    return error_success;
}

error_t network_read(network_t *n)
{
    char buf[100];
    int ret = 0;

    if (n->ssl) {
        ret = tls_read(n->tls, buf, sizeof(buf));
        if (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT) {
            return error_success;
        }
    } else {
        ret = read(n->fd, buf, sizeof(buf));
    }

    if (ret > 0) {
        irc_feed(n->irc, buf, ret);
    } else if (ret < 0 || ret == 0) {
        log_error("reading: %s", strerror(errno));
        network_disconnect(n);
    }

    return error_success;
}

error_t network_write(network_t *n)
{
    char *line = NULL;
    size_t linelen = 0;
    irc_error_t r = irc_error_success;
    int ret = 0;

    r = irc_pop(n->irc, &line, &linelen);
    if (IRC_FAILED(r)) {
        return error_success;
    }

    if (!n->ssl) {
        ret = write(n->fd, line, linelen);
    } else {
        ret = tls_write(n->tls, line, linelen);
        if (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT) {
            return error_success;
        }
    }

    if (ret < 0) {
        log_error("writing: %s", strerror(errno));
        network_disconnect(n);
    }

    free(line);

    return error_success;
}

static void network_callback(evutil_socket_t s, short what, void *arg)
{
    network_t *n = (network_t*)arg;

    if ((what & EV_READ) == EV_READ) {
        network_read(n);
    }

    if ((what & EV_WRITE) == EV_WRITE) {
        network_write(n);
    }
}

error_t network_connect(network_t *n, struct event_base *base)
{
    struct addrinfo *info = NULL, *ai = NULL, hint = {0};
    int sock = -1;
    int ret = 0;

    if (n->fd > -1) {
        return error_success;
    }

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = AF_UNSPEC;
    hint.ai_flags = AI_PASSIVE;

    if ((ret = getaddrinfo(n->host, n->port, &hint, &info))) {
        log_error("failed to resolve host: %s",
                  gai_strerror(ret)
            );
        return error_internal;
    }

    for (ai = info; ai != NULL; ai = ai->ai_next) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            continue;
        }

        if (connect(sock, ai->ai_addr, ai->ai_addrlen) == 0) {
            break;
        }

        close(sock);
        sock = -1;
    }

    if (sock < 0) {
        log_error("could not connect");
        return error_internal;
    } else {
        n->fd = sock;
    }

    if (n->ssl) {
        n->tls = tls_client();
        if (n->tls == NULL) {
            network_disconnect(n);
            log_error("failed to alloc tls_client\n");
            return error_memory;
        }

        tls_configure(n->tls, n->tls_config);
        if (tls_connect_socket(n->tls, n->fd, n->host) < 0) {
            log_error("failed to connect via TLS\n");
            network_disconnect(n);
            return error_tls;
        }
    }

    n->ev = event_new(base, n->fd, EV_PERSIST|EV_READ|EV_WRITE,
                      network_callback, n
        );
    event_add(n->ev, NULL);

    irc_connected(n->irc);

    return error_success;
}

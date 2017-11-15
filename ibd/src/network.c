#include <ibd/network.h>
#include <ibd/log.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

static void network_handler(irc_t i, irc_message_t m, void *arg);

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

    irc_handler_add(i->irc, NULL, network_handler, i);

    i->plugin_buf = strbuf_new();
    if (i->plugin_buf == NULL) {
        irc_free(i->irc);
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
            log_error("malloc failed");
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
        n->ssl = (strcmp(v, "yes") == 0 || strcmp(v, "true") == 0);
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
    int i = 0, status = 0;

    if (n->ev) {
        event_del(n->ev);
        event_free(n->ev);
        n->ev = NULL;
    }

    close(n->fd);
    n->fd = -1;

    if (n->tls) {
        tls_close(n->tls);
        tls_free(n->tls);
        n->tls = NULL;
    }

    for (i = 0; i < n->pluginlen; i++) {
        plugin_info_t *p = n->plugin[i];
        if (p->pid >= 0) {
            kill(p->pid, SIGTERM);
            waitpid(p->pid, &status, 0);
            p->pid = -1;
        }
    }

    strbuf_reset(n->plugin_buf);

    return error_success;
}

static void network_handler(irc_t irc, irc_message_t m, void *arg)
{
    network_t *n = (network_t*)arg;
    char *line = NULL;
    size_t linelen = 0;
    size_t i = 0;
    int ret = 0;

    if (IRC_FAILED(irc_message_string(m, &line, &linelen))) {
        log_error("failed to parse incoming line");
        return;
    }

    for (; i < n->pluginlen; i++) {
        plugin_info_t *p = n->plugin[i];

        ret = write(p->in, line, linelen);
        if (ret <= 0) {
            log_error("failed to write to child: %s: %s",
                      p->name, strerror(errno));
        }
    }
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
    } else if (ret <= 0) {
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
        /* check plugins
         */
        ret = strbuf_getline(n->plugin_buf, &line, &linelen);
        if (ret <= 0) {
            return error_success;
        }
    }

    if (!n->ssl) {
        ret = write(n->fd, line, linelen);
    } else {
        ret = tls_write(n->tls, line, linelen);
        if (ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT) {
            return error_success;
        }
    }

    if (ret <= 0) {
        log_error("writing: %s", strerror(errno));
        network_disconnect(n);
    }

    free(line);

    return error_success;
}

static void network_callback(evutil_socket_t s, short what, void *arg)
{
    network_t *n = (network_t*)arg;

    if (n->fd == -1) {
        log_error("fd was -1");
        return;
    }

    if ((what & EV_READ) == EV_READ) {
        network_read(n);
    } else if ((what & EV_WRITE) == EV_WRITE) {
        network_write(n);
    }
}

static void plugin_err_callback(evutil_socket_t s, short what, void *arg)
{
    if ((what & EV_READ) == EV_READ) {
        char *line = NULL;
        size_t linelen;
        FILE *f = fdopen(s, "r");

        if (getline(&line, &linelen, f) > 0) {
            line[linelen-1] = '\0';
            log_error(line);
        }

        fclose(f);
        free(line);
        line = NULL;
    }
}

static void plugin_callback(evutil_socket_t s, short what, void *arg)
{
    network_t *n = (network_t*)arg;
    char buffer[100] = {0};

    if ((what & EV_READ) == EV_READ) {
        int ret = 0;

        ret = read(s, buffer, sizeof(buffer));
        if (ret > 0) {
            strbuf_append(n->plugin_buf, buffer, ret);
        }
    }
}

static int child_main(plugin_info_t *p, int in[2], int out[2], int err[2])
{
    size_t i = 0;

    for (i = 0; i < p->envc; i++) {
        putenv(p->env[i]);
    }

    p->in = in[0];
    p->out = out[1];
    p->err = err[1];

    close(in[1]);
    close(out[0]);
    close(err[0]);

    /* Preare stdin and stdout
     */
    dup2(p->in, STDIN_FILENO);
    dup2(p->out, STDOUT_FILENO);
    dup2(p->err, STDERR_FILENO);

    if (p->argv == NULL) {
        p->argv = calloc(2, sizeof(char*));
        if (p->argv == NULL) {
            log_error("memory exhaustion");
            return error_memory;
        }
        p->argc = 2;
    }

    p->argv[0] = p->filename;

    if (execv(p->filename, p->argv) < 0) {
        log_error("failed to execute: %s: %s", p->filename, strerror(errno));
        return 3;
    }

    return 0;
}

static error_t network_run(network_t *n)
{
    size_t i = 0;
    int in[2];
    int out[2];
    int err[2];

    for (; i < n->pluginlen; i++) {
        plugin_info_t *p = n->plugin[i];

        if (pipe2(in, O_CLOEXEC) < 0) {
            log_error("failed to create pipe for children: %s",
                      strerror(errno));
            continue;
        }

        if (pipe2(out, O_CLOEXEC) < 0) {
            close(in[0]);
            close(in[1]);
            log_error("failed to create pipe for children: %s",
                      strerror(errno));
            continue;
        }

        if (pipe2(err, O_CLOEXEC) < 0) {
            close(in[0]);
            close(in[1]);
            close(out[0]);
            close(out[1]);
            log_error("failed to create pipe for children: %s",
                      strerror(errno));
            continue;
        }

        log_info("forking plugin: %s: %s", p->name, p->filename);

        p->pid = fork();
        if (p->pid > 0) {
            p->in = in[1];
            p->out = out[0];
            p->err = err[0];

            close(in[0]);
            close(out[1]);
            close(err[1]);

            p->in_ev = event_new(n->base, p->out, EV_PERSIST|EV_READ,
                                 plugin_callback, n
                );

            p->err_ev = event_new(n->base, p->err, EV_PERSIST|EV_READ,
                                  plugin_err_callback, n
                );
        } else if (p->pid == 0) {
            exit(child_main(p, in, out, err));
        } else if (p->pid < 0) {
            log_error("failed to fork: %s", strerror(errno));
            return error_internal;
        }
    }

    return error_success;
}

error_t network_connect(network_t *n, struct event_base *base)
{
    struct addrinfo *info = NULL, *ai = NULL, hint = {0};
    int sock = -1;
    int ret = 0;

    if (n->fd > -1) {
        return error_success;
    }

    n->base = base;

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
        sock = socket(ai->ai_family, SOCK_STREAM, 0);
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
        log_error("could not connect to %s:%s", n->host, n->port);
        return error_internal;
    } else {
        char buf[100] = {0};
        inet_ntop(ai->ai_family, ai->ai_addr, buf, sizeof(buf)-1);
        log_debug("connected to %s:%s", buf, n->port);

        n->fd = sock;
    }

    if (n->ssl) {
        n->tls = tls_client();
        if (n->tls == NULL) {
            log_error("failed to alloc tls_client\n");
            network_disconnect(n);
            return error_memory;
        }

        tls_configure(n->tls, n->tls_config);

        if (tls_connect_socket(n->tls, n->fd, n->host) < 0) {
            log_error("failed to connect via TLS: %s", tls_error(n->tls));
            network_disconnect(n);
            return error_tls;
        }

        if (tls_handshake(n->tls) < 0) {
            log_error("failed to make TLS handshake: %s", tls_error(n->tls));
            network_disconnect(n);
            return error_tls;
        }
    }

    n->ev = event_new(base, n->fd, EV_PERSIST | EV_READ | EV_WRITE,
                      network_callback, n
        );
    event_add(n->ev, NULL);

    irc_connected(n->irc);

    /* Run child processes
     */
    network_run(n);

    return error_success;
}

error_t network_add_plugin(network_t *n, plugin_info_t *p)
{
    plugin_info_t **tmp = reallocarray(n->plugin, n->pluginlen+1,
                                       sizeof(plugin_info_t *)
        );

    if (tmp == NULL) {
        return error_memory;
    }

    n->plugin = tmp;
    n->plugin[n->pluginlen] = p;
    ++n->pluginlen;

    return error_success;
}

error_t network_add_env(plugin_info_t *p, char *env)
{
    char **tmp = reallocarray(p->env, p->envc+1, sizeof(char*));

    if (tmp == NULL) {
        return error_memory;
    }

    p->env = tmp;
    p->env[p->envc] = env;
    ++p->envc;

    return error_success;
}

error_t network_add_arg(plugin_info_t *p, char *arg)
{
    size_t add = (p->argc == 0 ? 3 : 1);
    char **tmp = reallocarray(p->argv, p->argc + add, sizeof(char*));

    if (tmp == NULL) {
        return error_memory;
    }

    p->argv = tmp;

    p->argc += add;
    p->argv[p->argc-2] = arg;
    p->argv[p->argc-1] = NULL;

    return error_success;
}

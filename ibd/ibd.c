#include <ibd/log.h>
#include <irc/irc.h>

#include <event2/event.h>

#include <tls.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <syslog.h>

static struct tls *ctx = NULL;
static struct tls_config *tconfig = NULL;

static int sock = -1;
static int done = 0;
static irc_t i = NULL;

static char *host = NULL;
static char *port = NULL;
static char *nick = NULL;
static bool ssl = false;

void on_command(irc_t irc, irc_message_t m, void *unused)
{
    int i = 0;

    printf(">> [prefix = %s] [command = %s] ",
           (m->prefix ? m->prefix : "N/A"),
           (m->command ? m->command : "N/A")
        );
    if (m->args) {
        for (i = 0; m->args[i] != NULL; i++) {
            printf("[arg(%d) = %s]", i, m->args[i]);
            if (m->args[i+1] != NULL) {
                printf(" ");
            }
        }
    }
    printf("\n");
}

static void ibd_disconnect(void)
{
    close(sock);
    sock = -1;

    tls_close(ctx);
    tls_free(ctx);
    ctx = NULL;
}

static void event_callback(evutil_socket_t s, short what, void *unused)
{
    int ret = 0;

    if ((what & EV_READ) == EV_READ) {
        char buf[100] = {0};

        if (!ssl) {
            ret = read(s, buf, sizeof(buf));
        } else {
            ret = tls_read(ctx, buf, sizeof(buf));
        }
        if (ret > 0) {
            irc_feed(i, buf, ret);
        } else if (ret < 0) {
            log_error("reading: %s", strerror(errno));
            ibd_disconnect();
        } else if (ret == 0) {
            log_error("disconnected: %s", strerror(errno));
            ibd_disconnect();
        }
    }

    if ((what & EV_WRITE) == EV_WRITE) {
        char *line = NULL;
        size_t linelen = 0;
        irc_error_t r = irc_error_success;

        r = irc_pop(i, &line, &linelen);
        if (IRC_FAILED(r)) {
            return;
        }

        printf("<< %s", line);

        if (!ssl) {
            ret = write(s, line, linelen);
        } else {
            ret = tls_write(ctx, line, linelen);
        }
        if (ret < 0) {
            log_error("writing: %s", strerror(errno));
            close(sock);
            sock = -1;
        }

        free(line);
    }
}

static int ibd_connect(char const *host, char const *port)
{
    struct addrinfo *info = NULL, *ai = NULL, hint = {0};
    int ret = 0;

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = AF_UNSPEC;
    hint.ai_flags = AI_PASSIVE;

    if ((ret = getaddrinfo(host, port, &hint, &info))) {
        log_error("failed to resolve host: %s",
                  gai_strerror(ret)
            );
        return -1;
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
        return -1;
    }

    return sock;
}

static void usage(void)
{
    puts("ibd -f -h host -p port -n nick -s");
}

int parse_args(int ac, char **av)
{
    static struct option opts[] = {
        { "nick", required_argument, NULL, 'n' },
        { "port", required_argument, NULL, 'p' },
        { "host", required_argument, NULL, 'h' },
        { "foreground", no_argument, NULL, 'f' },
        { "ssl", no_argument, NULL, 's' },
        { NULL, no_argument, NULL, 0 },
    };

    static char const *optstr = "fn:p:h:s";

    int c = 0;

    while ((c = getopt_long(ac, av, optstr, opts, NULL)) != -1) {
        switch (c) {
        case 'n': free(nick); nick = strdup(optarg); break;
        case 'p': free(port); port = strdup(optarg); break;
        case 'h': free(host); host = strdup(optarg); break;
        case 's': ssl = true; break;
        case 'f': log_foreground(true); break;
        case '?':
        default:
        {
            usage();
            return -1;
        } break;
        }
    }

    if (host == NULL) {
        fprintf(stderr, "error: no host specified\n");
        return -1;
    }

    return 0;
}

int main(int ac, char **av)
{
    struct event_base *base = NULL;
    struct event *ev = NULL;
    int ret = 0;

    tls_init();

    /* sane default values
     */
    nick = strdup("ibd");
    port = strdup("6667");

    if (parse_args(ac, av) < 0) {
        return 1;
    }

    openlog("ibd", 0, LOG_INFO);

    if (ssl) {
        tconfig = tls_config_new();
        if (tconfig == NULL) {
            log_error("tls_config_new failed");
            return 3;
        }
    }

    base = event_base_new();
    if (base == NULL) {
        return 3;
    }

    i = irc_new();
    if (i == NULL) {
        log_error("failed to allocate");
        return 3;
    }

    irc_setopt(i, ircopt_nick, nick);
    irc_setopt(i, ircopt_realname, nick);
    irc_setopt(i, ircopt_server, host);

    irc_handler_add(i, NULL, on_command, NULL);

    while (!done) {
        sock = ibd_connect(host, port);

        if (ssl) {
            if (ctx == NULL) {
                ctx = tls_client();
                if (ctx == NULL) {
                    log_error("tls_client failed");
                    return 3;
                }
                tls_configure(ctx, tconfig);
            }

            if (tls_connect_socket(ctx, sock, host) < 0) {
                log_error("tls_connect_socket");
                ibd_disconnect();
            }
        }

        if (sock < 0) {
            sleep(2);
            continue;
        }

        irc_connected(i);

        if (ev) {
            event_del(ev);
            event_free(ev);
            ev = NULL;
        }

        ev = event_new(base, sock, EV_PERSIST | EV_READ | EV_WRITE,
                       event_callback, NULL
            );
        if (ev == NULL) {
            return 3;
        }
        event_add(ev, NULL);

        while (true) {
            irc_think(i);

            ret = event_base_loop(base, EVLOOP_ONCE | EVLOOP_NONBLOCK);
            if (ret < 0) {
                break;
            }

            if (sock == -1) {
                break;
            }
        }
    }

    return 0;
}

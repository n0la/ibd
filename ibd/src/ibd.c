#include <ibd/log.h>
#include <ibd/config.h>

#include <irc/irc.h>

#include <pwd.h>
#include <grp.h>

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
#include <signal.h>

static char *configfile = NULL;
static bool foreground = false;

static void cleanup(void)
{
    int i = 0;

    for (i = 0; i < config.networklen; i++) {
        network_t *n = config.network[i];
        network_disconnect(n);
    }
}

static void sig(int signal)
{
    exit(1);
}

static void usage(void)
{
    puts("ibd -c config -f");
}

int parse_args(int ac, char **av)
{
    static struct option opts[] = {
        { "config", required_argument, NULL, 'c' },
        { "foreground", no_argument, NULL, 'f' },
        { NULL, no_argument, NULL, 0 },
    };

    static char const *optstr = "c:f";

    int c = 0;

    configfile = strdup("/etc/ibd.conf");

    while ((c = getopt_long(ac, av, optstr, opts, NULL)) != -1) {
        switch (c) {
        case 'c': free(configfile); configfile = strdup(optarg); break;
        case 'f': log_foreground(true); foreground = true; break;
        case '?':
        default:
        {
            usage();
            return -1;
        } break;
        }
    }

    return 0;
}

int drop_privileges(void)
{
    struct passwd *p = NULL;
    struct group *g = NULL;

    p = getpwnam(config.user);
    g = getgrnam(config.group);

    if (p == NULL) {
        log_error("no such username: %s", config.user);
        return 3;
    }

    if (g == NULL) {
        log_error("no such groupname: %s", config.group);
        return 3;
    }

    if (setuid(p->pw_uid) != 0) {
        log_error("failed to change user: %s", strerror(errno));
        return 3;
    }

    if (setgid(g->gr_gid) != 0) {
        log_error("failed to change group id: %s", strerror(errno));
        return 3;
    }

    if (setuid(0) == 0) {
        log_error("WHAT");
        return 3;
    }

    return 0;
}

int main(int ac, char **av)
{
    struct event_base *base = NULL;
    bool done = false;
    int i = 0;
    int ret = 0;

    openlog("ibd", LOG_NDELAY, LOG_USER);

    tls_init();

    if (parse_args(ac, av) < 0) {
        return 1;
    }

    if (configfile) {
        if (config_parse(configfile)) {
            return 3;
        }
    }

    if (getuid() == 0 && (config.user == NULL || config.group == NULL)) {
        fprintf(stderr, "starting ibd as root is a bad idea\n");
        return 3;
    }

    if (!foreground) {
        pid_t f = fork();
        if (f > 0) {
            return 0;
        } else if (f < 0) {
            fprintf(stderr, "failed to fork: %s", strerror(errno));
            return 3;
        } else if (f == 0) {
            openlog("ibd", LOG_NDELAY, LOG_USER);

            if (setsid() < 0) {
                log_error("failed to allocate session: %s", strerror(errno));
                return 3;
            }

            atexit(cleanup);

            signal(SIGTERM, sig);
            signal(SIGINT, sig);

            signal(SIGPIPE, SIG_IGN);
            signal(SIGCHLD, SIG_IGN);
            signal(SIGHUP, SIG_IGN);

            close(STDOUT_FILENO);
            close(STDIN_FILENO);
            close(STDERR_FILENO);

            chdir("/");

            log_info("ibd successfully started");
        }
    } else {
        openlog("ibd", 0, LOG_INFO);
        atexit(cleanup);
        signal(SIGTERM, sig);
        signal(SIGINT, sig);
        signal(SIGPIPE, SIG_IGN);
    }

    ret = drop_privileges();
    if (ret > 0) {
        return ret;
    }

    base = event_base_new();
    if (base == NULL) {
        return 3;
    }

    while (!done) {
        for (i = 0; i < config.networklen; i++) {
            network_t *n = config.network[i];

            if (network_connect(n, base)) {
                continue;
            }

            irc_think(n->irc);
        }

        ret = event_base_loop(base, EVLOOP_ONCE | EVLOOP_NONBLOCK);
        if (ret < 0) {
            log_error("failed to dispatch events");
            break;
        } else if (ret > 1) {
            usleep(10 * 1000);
        }
    }

    log_info("ibd exited, disconnecting...");

    return 0;
}

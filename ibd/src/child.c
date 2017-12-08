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

static void child_err_callback(evutil_socket_t s, short what, void *arg)
{
    network_t *n = (network_t*)arg;
    char buffer[100] = {0};
    char *line = NULL;
    size_t linelen = 0;

    if ((what & EV_READ) == EV_READ) {
        int ret = 0;

        ret = read(s, buffer, sizeof(buffer));
        if (ret > 0) {
            strbuf_append(n->plugin_err_buf, buffer, ret);

            if (strbuf_getline(n->plugin_err_buf, &line, &linelen) == 0) {
                log_error(line);
                free(line);
                line = NULL;
            }
        }
    }
}

static void child_callback(evutil_socket_t s, short what, void *arg)
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

    for (i = 0; i < p->env->vlen; i++) {
        putenv(p->env->v[i]);
    }

    p->arg->v[0] = strdup(p->name);
    strv_add(p->arg, NULL);

    p->in = in[1];
    p->out = out[1];
    p->err = err[1];

    close(in[0]);
    close(out[0]);
    close(err[0]);

    /* Preare stdin and stdout
     */
    dup2(p->in, STDIN_FILENO);
    dup2(p->out, STDOUT_FILENO);
    dup2(p->err, STDERR_FILENO);

    if (execv(p->filename, p->arg->v) < 0) {
        log_error("failed to execute: %s: %s", p->filename, strerror(errno));
        return 3;
    }

    return 0;
}

void child_close(network_t *n, plugin_info_t *p)
{
    int status = 0;

    if (p->pid >= 0) {
        kill(p->pid, SIGTERM);
        waitpid(p->pid, &status, 0);
        p->pid = -1;
    }

    close(p->in);
    p->in = -1;

    close(p->out);
    p->out = -1;

    close(p->err);
    p->err = -1;

    if (p->in_ev) {
        event_del(p->in_ev);
        event_free(p->in_ev);
        p->in_ev = NULL;
    }

    if (p->err_ev) {
        event_del(p->err_ev);
        event_free(p->err_ev);
        p->err_ev = NULL;
    }
}

error_t child_run(network_t *n, plugin_info_t *p)
{
    int in[2] = {-1, -1};
    int out[2] = {-1, -1};
    int err[2] = {-1, -1};

    if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, in)) {
        goto cleanup;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, PF_UNSPEC, out)) {
        goto cleanup;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, PF_UNSPEC, err)) {
        goto cleanup;
    }

    p->pid = fork();
    if (p->pid == -1) {
        log_error("could not fork: %s", strerror(errno));
        goto cleanup;
    } else if (p->pid == 0) {
        exit(child_main(p, in, out, err));
    }

    /* parent
     */
    p->in = in[0];
    p->out = out[0];
    p->err = err[0];

    close(in[1]);
    in[1] = -1;

    close(out[1]);
    out[1] = -1;

    close(err[1]);
    err[1] = -1;

    p->in_ev = event_new(n->base, p->out, EV_PERSIST|EV_READ,
                         child_callback, n
        );
    event_add(p->in_ev, NULL);

    p->err_ev = event_new(n->base, p->err, EV_PERSIST|EV_READ,
                          child_err_callback, n
        );
    event_add(p->err_ev, NULL);

    return error_success;

cleanup:

    p->in = p->out = p->err = -1;

    close(in[0]);
    close(in[1]);
    close(out[0]);
    close(out[1]);
    close(err[0]);
    close(err[1]);

    if (p->in_ev) {
        event_del(p->in_ev);
        event_free(p->in_ev);
        p->in_ev = NULL;
    }

    if (p->err_ev) {
        event_del(p->err_ev);
        event_free(p->err_ev);
        p->err_ev = NULL;
    }

    return error_internal;
}

void child_close_all(network_t *n)
{
    int i = 0;

    for (i = 0; i < n->pluginlen; i++) {
        plugin_info_t *p = n->plugin[i];
        child_close(n, p);
    }
}

error_t child_run_all(network_t *n)
{
    error_t r = 0;
    size_t i = 0;

    for (; i < n->pluginlen; i++) {
        plugin_info_t *p = n->plugin[i];

        log_info("forking plugin: %s: %s", p->name, p->filename);
        r = child_run(n, p);
    }

    return error_success;
}

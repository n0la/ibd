#include <irc/message.h>
#include <irc/util.h>
#include <irc/strbuf.h>

#include <string.h>

irc_message_t irc_message_new(void)
{
    return calloc(1, sizeof(struct irc_message_));
}

void irc_message_free(irc_message_t m)
{
    if (m == NULL) {
        return;
    }

    free(m->prefix);
    m->prefix = NULL;

    free(m->command);
    m->command = NULL;

    irc_strv_free(m->args);
    m->args = NULL;

    free(m);
}

static void irc_message_add(char ***ac, size_t *av, strbuf_t b)
{
    char *dup = NULL;

    if (strbuf_getc(b) == ':') {
        strbuf_delete(b, 1);
    }

    dup = strbuf_strdup(b);
    irc_strv_add(ac, av, dup);
}

irc_error_t irc_message_parse(irc_message_t c, char const *l, size_t len)
{
    strbuf_t argbuf = NULL;
    char *prefix = NULL, *command = NULL;
    char **args = NULL;
    size_t argslen = 0;

    char *line = strdup(l);
    char *ptr = line;
    char *part = NULL;
    irc_error_t r = irc_error_memory;
    int i = 0;

    while ((part = strsep(&line, " ")) != NULL) {
        if (*part == '\0') {
            ++part;
            continue;
        }

        switch (i)
        {
        case 0:
        {
            /* check if we actually have a prefix. They start with ':'
             * and if we don't have one, we assume the first part is
             * the command and move past the command thing.
             */
            if (*part == ':') {
                prefix = strdup(part);
            } else {
                command = strdup(part);
                ++i;
            }
        } break;

        case 1: command = strdup(part); break;

        default:
        {
            if (*part == ':') {
                if (argbuf) {
                    irc_message_add(&args, &argslen, argbuf);
                    strbuf_free(argbuf);
                    argbuf = NULL;
                }

                argbuf = strbuf_new();
                if (argbuf == NULL) {
                    goto cleanup;
                }
                strbuf_append(argbuf, part, strlen(part));
            } else {
                if (argbuf == NULL) {
                    irc_message_add(&args, &argslen, argbuf);
                } else {
                    strbuf_append(argbuf, " ", 1);
                    strbuf_append(argbuf, part, strlen(part));
                }
            }

        } break;
        }

        ++i;
    }

    /* Check if their is remaining argument
     */
    if (argbuf) {
        irc_message_add(&args, &argslen, argbuf);
    }

    c->prefix = prefix;
    c->command = command;
    c->args = args;
    c->argslen = argslen;

    r = irc_error_success;

cleanup:

    strbuf_free(argbuf);
    argbuf = NULL;

    if (r != irc_error_success) {
        free(prefix);
        free(command);
        irc_strv_free(args);
        args = NULL;
    }

    free(ptr);

    return r;
}

irc_message_t irc_message_make(char const *prefix,
                               char const *cmd, ...)
{
    va_list lst;
    irc_message_t m = NULL;

    va_start(lst, cmd);
    m = irc_message_makev(prefix, cmd, lst);
    va_end(lst);

    return m;
}

irc_message_t irc_message_makev(char const *prefix,
                                char const *cmd,
                                va_list lst)
{
    irc_message_t m = irc_message_new();
    char *arg = NULL;

    if (!m) {
        return NULL;
    }

    if (prefix) {
        m->prefix = strdup(prefix);
    }
    m->command = strdup(cmd);

    while ((arg = va_arg(lst, char*)) != NULL) {
        char *dup = strdup(arg);
        irc_strv_add(&m->args, &m->argslen, dup);
    }

    return m;
}

irc_error_t irc_message_string(irc_message_t m, char **s, size_t *slen)
{
    strbuf_t buf = NULL;
    char **tmp = NULL;

    if (m == NULL || s == NULL || slen == NULL) {
        return irc_error_argument;
    }

    if (m->command == NULL) {
        return irc_error_protocol;
    }

    buf = strbuf_new();
    if (buf == NULL) {
        return irc_error_memory;
    }

    if (m->prefix) {
        strbuf_append(buf, ":", 1);
        strbuf_append(buf, m->prefix, strlen(m->prefix));
        strbuf_append(buf, " ", 1);
    }

    strbuf_append(buf, m->command, strlen(m->command));
    strbuf_append(buf, " ", 1);

    if (m->args) {
        for (tmp = m->args; *tmp != NULL; ++tmp) {
            size_t tmplen = strlen(*tmp);
            /* if we find a space we add a `:`
             */
            if (strchr(*tmp, ' ') != NULL) {
                strbuf_append(buf, ":", 1);
            }
            strbuf_append(buf, *tmp, tmplen);

            if (tmp[1] != NULL) {
                strbuf_append(buf, " ", 1);
            }
        }
    }

    strbuf_append(buf, "\r\n", 2);

    *s = strbuf_strdup(buf);
    *slen = strlen(*s);

    strbuf_free(buf);
    buf = NULL;

    return irc_error_success;
}

#include <ibd/log.h>

#include <stdio.h>
#include <stdbool.h>

#include <syslog.h>

static bool foreground = false;

void log_foreground(bool what)
{
    foreground = what;
}

void log_error(char const *fmt, ...)
{
    va_list lst;

    va_start(lst, fmt);
    if (foreground) {
        fprintf(stderr, "error: ");
        vfprintf(stderr, fmt, lst);
        fprintf(stderr, "\n");
    } else {
        vsyslog(LOG_ERR, fmt, lst);
    }
    va_end(lst);
}

void log_debug(char const *fmt, ...)
{
    va_list lst;

    va_start(lst, fmt);
    if (foreground) {
        fprintf(stdout, "debug: ");
        vfprintf(stdout, fmt, lst);
        fprintf(stdout, "\n");
    } else {
        vsyslog(LOG_DEBUG, fmt, lst);
    }
    va_end(lst);

}

void log_info(char const *fmt, ...)
{
    va_list lst;

    va_start(lst, fmt);
    if (foreground) {
        fprintf(stdout, "info: ");
        vfprintf(stdout, fmt, lst);
        fprintf(stdout, "\n");
    } else {
        vsyslog(LOG_NOTICE, fmt, lst);
    }
    va_end(lst);
}

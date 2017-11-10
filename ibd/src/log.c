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
        fputs("", stderr);
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
        fprintf(stderr, "debug: ");
        vfprintf(stderr, fmt, lst);
        fputs("", stderr);
    } else {
        vsyslog(LOG_DEBUG, fmt, lst);
    }
    va_end(lst);

}

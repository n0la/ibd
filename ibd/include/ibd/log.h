#ifndef IBD_LOG_H
#define IBD_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

void log_error(char const *fmt, ...);
void log_debug(char const *fmt, ...);
void log_info(char const *fmt, ...);

void log_foreground(bool what);

#endif

#ifndef LIBIRC_MESSAGE_H
#define LIBIRC_MESSAGE_H

#include <irc/error.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

struct irc_message_
{
    char *prefix;
    char *command;
    char **args;
    size_t argslen;
};

typedef struct irc_message_ *irc_message_t;

extern irc_message_t irc_message_new(void);
extern void irc_message_free(irc_message_t m);

extern irc_error_t irc_message_parse(irc_message_t m,
                                     char const *line,
                                     size_t lensize);

extern irc_message_t irc_message_make(char const *prefix,
                                      char const *command, ...);

extern irc_message_t irc_message_makev(char const *prefix,
                                       char const *command,
                                       va_list lst);

extern irc_error_t irc_message_string(irc_message_t m, char **s, size_t *slen);

#endif

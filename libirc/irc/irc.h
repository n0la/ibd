#ifndef LIB_IRC_H
#define LIB_IRC_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <irc/error.h>
#include <irc/message.h>

struct irc_;

typedef struct irc_ * irc_t;

typedef void (*irc_command_handler_t)(irc_t, irc_message_t m, void *);

irc_t irc_new(void);
void irc_free(irc_t i);

irc_error_t irc_feed(irc_t i, char const *buffer, size_t len);
irc_error_t irc_think(irc_t i);

irc_error_t irc_connected(irc_t i);

irc_error_t irc_queue_command(irc_t i, char const *type, ...);

irc_error_t irc_nick(irc_t i, char const *nick);
irc_error_t irc_realname(irc_t i, char const *name);
irc_error_t irc_server(irc_t i, char const *server);

irc_error_t irc_handler_add(irc_t i, char const *cmd,
                            irc_command_handler_t handler,
                            void *arg);

irc_error_t irc_pop(irc_t i, char **message, size_t *len);

#endif

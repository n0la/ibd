#include <ibd/plugin.h>

static int echo_onmessage(irc_t *i, irc_message_t *m, void *);

static id_plugin_t echo_plugin = {
    "echo",
    ID_MAKE_VERSION(1, 0),
    NULL,
    echo_onmessage
};

id_plugin_t *id_echo_plugin(void)
{
    return &echo_plugin;
}

static int echo_onmessage(irc_t *i, irc_message_t *m, void *arg)
{
    return 0;
}

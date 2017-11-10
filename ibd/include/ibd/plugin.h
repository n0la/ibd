#ifndef IBD_PLUGIN_H
#define IBD_PLUGIN_H

#include <irc/irc.h>
#include <stdint.h>

#define ID_MAKE_VERSION(v, m) ((uint32_t)(v << 16) | m)
#define ID_MAJOR_VERSION(v) ((uint32_t)(v >> 16))
#define ID_MINOR_VERSION(v) ((uint32_t)(v & 0xFFFF0000));

typedef int (*id_plugin_message)(irc_t *i, irc_message_t *m, void *);
typedef int (*id_plugin_load)(uint32_t version);

typedef struct {
    char *name;
    uint32_t version;

    id_plugin_load on_load;
    id_plugin_message on_message;
} id_plugin_t;

#endif

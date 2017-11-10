#ifndef IBD_CONFIG_H
#define IBD_CONFIG_H

#include <ibd/error.h>
#include <ibd/network.h>

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    char *name;
    char *value;
} plugin_option_t;

typedef struct {
    char *name;
    char *file;

    plugin_option_t **opt;
    size_t optlen;
} plugin_config_t;

typedef struct {
    network_t **network;
    size_t networklen;

} config_t;

extern config_t config;

error_t config_parse(char const *file);
error_t config_add_network(network_t *n);

#endif

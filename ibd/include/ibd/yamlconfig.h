#ifndef IBD_YAML_CONFIG_H
#define IBD_YAML_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include <yaml.h>

struct yaml_config;

typedef struct yaml_config *yaml_config_t;

yaml_config_t yaml_config_new(void);
void yaml_config_free(yaml_config_t c);

bool yaml_config_load(yaml_config_t c, char const *file);

bool yaml_config_exists(yaml_config_t c, char const *url);
bool yaml_config_string(yaml_config_t c, char const *url,
                        char **value, char const *def);
bool yaml_config_bool(yaml_config_t c, char const *url,
                      bool *value, bool def);

#endif

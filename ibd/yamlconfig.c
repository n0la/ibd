#include <ibd/yamlconfig.h>

#include <yaml.h>

#include <stdbool.h>
#include <stdio.h>

struct yaml_config
{
    yaml_document_t doc;
    yaml_node_t *root;
    bool init;
};

static void yaml_config_close(yaml_config_t c);

yaml_config_t yaml_config_new(void)
{
    return calloc(1, sizeof(struct yaml_config));
}

void yaml_config_free(yaml_config_t c)
{
    if (c == NULL) {
        return;
    }

    yaml_config_close(c);
    free(c);
}

static void yaml_config_close(yaml_config_t c)
{
    if (c->init) {
        yaml_document_delete(&c->doc);
        c->root = NULL;
        c->init = false;
    }
}

bool yaml_config_load(yaml_config_t c, char const *file)
{
    FILE *f = NULL;
    yaml_parser_t parser;
    bool ret = false;

    yaml_config_close(c);

    f = fopen(file, "r");
    if (f == NULL) {
        return false;
    }

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, f);

    if (!yaml_parser_load(&parser, &c->doc)) {
        goto cleanup;
    }

    c->root = yaml_document_get_root_node(&c->doc);
    if (c->root == NULL) {
        yaml_document_delete(&c->doc);
        goto cleanup;
    }

    c->init = true;
    ret = true;

cleanup:

    fclose(f);
    yaml_parser_delete(&parser);

    return ret;
}

static yaml_node_t *
yaml_config_find_(yaml_config_t c, yaml_node_t *k, char const *name)
{
    if (k == NULL) {
        return NULL;
    }

    if (k->type == YAML_MAPPING_NODE) {
        yaml_node_pair_t *p = NULL;

        for (p = k->data.mapping.pairs.start;
             p < k->data.mapping.pairs.end; ++p) {
            yaml_node_t *key = yaml_document_get_node(&c->doc, p->key);
            yaml_node_t *value = yaml_document_get_node(&c->doc, p->value);

            if (strcmp(key->data.scalar.value, name) == 0) {
                return value;
            }
        }
    } else if (k->type == YAML_SCALAR_NODE) {
        if (strcmp(k->data.scalar.value, name) == 0) {
            return k;
        }
    }

    return NULL;
}

static yaml_node_t *
yaml_config_find(yaml_config_t c, char const *name)
{
    char *url = strdup(name);
    char *part = NULL;
    yaml_node_t *next = c->root;

    while ((part = strsep(&url, ".")) != NULL) {
        if (*part == '\0') {
            ++part;
            continue;
        }

        next = yaml_config_find_(c, next, part);
        if (next == NULL) {
            break;
        }
    }

    free(url);

    return next;
}

bool yaml_config_exists(yaml_config_t c, char const *url)
{
    yaml_node_t *fnd = NULL;

    fnd = yaml_config_find(c, url);

    return (fnd != NULL);
}

bool yaml_config_string(yaml_config_t c, char const *url,
                        char **value, char const *def)
{
    yaml_node_t *fnd = NULL;

    fnd = yaml_config_find(c, url);

    if (fnd == NULL) {
        if (def) {
            *value = strdup(def);
        } else {
            *value = NULL;
        }
        return false;
    }

    if (fnd->type != YAML_SCALAR_NODE) {
        if (def) {
            *value = strdup(def);
        } else {
            *value = NULL;
        }
        return false;
    }

    *value = strdup(fnd->data.scalar.value);

    return true;
}

bool yaml_config_bool(yaml_config_t c, char const *url,
                      bool *value, bool def)
{
    yaml_node_t *fnd = NULL;

    fnd = yaml_config_find(c, url);

    if (fnd == NULL) {
        *value = def;
        return false;
    }

    if (fnd->type != YAML_SCALAR_NODE) {
        *value = def;
        return false;
    }

    if (strcmp(fnd->data.scalar.value, "true") == 0 ||
        strcmp(fnd->data.scalar.value, "yes") == 0 ||
        strcmp(fnd->data.scalar.value, "on") == 0) {
        *value = true;
    } else {
        *value = false;
    }

    return true;
}

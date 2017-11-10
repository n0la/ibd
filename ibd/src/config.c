#include <ibd/config.h>
#include <ibd/log.h>

#include <string.h>
#include <errno.h>

config_t config;

int yyparse(void);
extern FILE *yyin;

error_t config_parse(char const *filename)
{
    int ret = 0;
    FILE *f = fopen(filename, "r");

    if (f == NULL) {
        log_error("failed to open file: %s: %s\n", filename,
                  strerror(errno)
            );
        return error_internal;
    }

    yyin = f;
    ret = yyparse();
    fclose(f);

    if (ret) {
        return error_syntax;
    }

    return error_success;
}

error_t config_add_network(network_t *n)
{
    network_t **tmp = reallocarray(config.network,
                                   config.networklen+1,
                                   sizeof(network_t *)
        );

    if (tmp == NULL) {
        return error_memory;
    }

    config.network = tmp;
    config.network[config.networklen] = n;
    ++config.networklen;

    return error_success;
}

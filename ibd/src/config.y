%{
#include <stdio.h>
#include <string.h>

#include <ibd/config.h>
#include <ibd/network.h>

extern int yylex(void);

static plugin_info_t *p = NULL;
static network_t *n = NULL;

void yyerror(char const *str)
{
    fprintf(stderr, "error reading configuration file: %s\n", str);
}

int yywrap(void)
{
    return 1;
}

char *strip_quote(char *s)
{
    size_t len = strlen(s);

    if (len < 1) {
        return s;
    }

    if (*s == '"') {
        memmove(s, s+1, len-1);
        --len;
    }

    if (len > 0 && s[len-1] == '"') {
        s[len-1] = '\0';
    }

    return s;
}

%}

%union {
    char *string;
    int integer;
}

%token <integer>       TOK_NETWORK TOK_PAR_OPEN TOK_PAR_CLOSE TOK_EQUAL TOK_SEMI_COLON TOK_PLUGIN TOK_FILENAME TOK_OPTION TOK_ARGS
%token <string>        TOK_STRING
%token <string>        TOK_QUOTED_STRING

%%

grammar: /* empty */
        |       grammar network
        |       grammar plugin
                ;

network:        TOK_NETWORK TOK_QUOTED_STRING TOK_PAR_OPEN network_vars TOK_PAR_CLOSE TOK_SEMI_COLON
                {
                    n->name = strip_quote($2);
                    config_add_network(n);
                    n = NULL;
                }
                ;

network_var:    TOK_STRING TOK_EQUAL TOK_QUOTED_STRING TOK_SEMI_COLON
                {
                    char *val = strip_quote($3);

                    if (n == NULL) {
                        n = network_new();
                        if (n == NULL) {
                            yyerror("memory allocation");
                        }
                    }

                    if (network_set(n, $1, val)) {
                        yyerror("invalid attribute for network");
                    }

                    free($1);
                    free($3);
                }
        ;

plugin:         TOK_PLUGIN TOK_QUOTED_STRING TOK_PAR_OPEN plugin_vars TOK_PAR_CLOSE TOK_SEMI_COLON
                {
                    p->name = strip_quote($2);
                    network_add_plugin(n, p);
                    p = NULL;
                }
                ;

plugin_vars:
        |       plugin_vars plugin_filename
        |       plugin_vars plugin_args
        ;

plugin_filename:TOK_FILENAME TOK_QUOTED_STRING TOK_SEMI_COLON
                {
                    if (p == NULL) {
                        p = calloc(1, sizeof(plugin_info_t));
                        if (p == NULL) {
                            yyerror("memory allocation");
                        }
                    }

                    free(p->filename);
                    p->filename = strip_quote($2);
                }
        ;

plugin_args:    TOK_ARGS strings TOK_SEMI_COLON
                {
                }
        ;

strings:
        |       strings TOK_QUOTED_STRING
                {
                    if (p == NULL) {
                        p = calloc(1, sizeof(plugin_info_t));
                        if (p == NULL) {
                            yyerror("memory allocation");
                        }
                    }

                    network_add_arg(p, strip_quote($2));
                }
        ;

network_vars:
        |       network_vars network_var
        |       network_vars plugin
        ;

%%
